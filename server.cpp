#include "server.h"
#include <iostream>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <fcntl.h>
#include <signal.h>

const int MAX_EVENTS = 64;
const int BUFFER_SIZE = 4096;

TCPServer::TCPServer() : server_socket(-1), epoll_fd(-1) {
    // Ignore SIGPIPE to handle broken connections gracefully
    signal(SIGPIPE, SIG_IGN);
}

TCPServer::~TCPServer() {
    stop();
}

bool TCPServer::setup_server_socket(int port) {
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("socket creation failed");
        return false;
    }
    
    // Set socket options
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        close(server_socket);
        return false;
    }
    
    // Make socket non-blocking
    int flags = fcntl(server_socket, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl F_GETFL failed");
        close(server_socket);
        return false;
    }
    
    if (fcntl(server_socket, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl F_SETFL failed");
        close(server_socket);
        return false;
    }
    
    // Bind socket
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        close(server_socket);
        return false;
    }
    
    // Listen
    if (listen(server_socket, SOMAXCONN) < 0) {
        perror("listen failed");
        close(server_socket);
        return false;
    }
    
    std::cout << "Server listening on port " << port << std::endl;
    return true;
}

void TCPServer::setup_epoll() {
    epoll_fd = epoll_create1(EPOLL_CLOEXEC);
    if (epoll_fd == -1) {
        perror("epoll_create1 failed");
        return;
    }
    
    // Add server socket to epoll
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = server_socket;
    
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_socket, &ev) == -1) {
        perror("epoll_ctl failed for server socket");
    }
}

bool TCPServer::start(int port) {
    if (!setup_server_socket(port)) {
        return false;
    }
    
    setup_epoll();
    if (epoll_fd == -1) {
        close(server_socket);
        return false;
    }
    
    return true;
}

void TCPServer::handle_new_connection() {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    while (true) {
        int client_fd = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break; // No more pending connections
            } else {
                perror("accept failed");
                break;
            }
        }
        
        // Make client socket non-blocking
        int flags = fcntl(client_fd, F_GETFL, 0);
        if (flags != -1) {
            fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);
        }
        
        // Add to epoll
        struct epoll_event ev;
        ev.events = EPOLLIN | EPOLLET; // Edge-triggered
        ev.data.fd = client_fd;
        
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev) == -1) {
            perror("epoll_ctl failed for client");
            close(client_fd);
            continue;
        }
        
        // Create client connection object
        clients[client_fd] = std::make_unique<ClientConnection>(client_fd);
        
        std::cout << "New client connected: fd=" << client_fd 
                  << " (" << inet_ntoa(client_addr.sin_addr) << ":" 
                  << ntohs(client_addr.sin_port) << ")" << std::endl;
    }
}

void TCPServer::handle_client_data(int client_fd) {
    auto client_it = clients.find(client_fd);
    if (client_it == clients.end()) {
        return;
    }
    
    auto& client = client_it->second;
    char buffer[BUFFER_SIZE];
    
    while (true) {
        ssize_t bytes_read = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
        
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            client->buffer += std::string(buffer);
            
            // Process complete messages
            size_t msg_end;
            while ((msg_end = client->buffer.find("\r\n\r\n")) != std::string::npos) {
                std::string message = client->buffer.substr(0, msg_end + 4);
                client->buffer.erase(0, msg_end + 4);
                
                // Parse and process command
                Command cmd = ProtocolParser::parse_message(message);
                std::string response = process_command(cmd, client->client_id);
                
                if (!send_response(client_fd, response)) {
                    handle_client_disconnect(client_fd);
                    return;
                }
            }
        } else if (bytes_read == 0) {
            // Client disconnected
            handle_client_disconnect(client_fd);
            break;
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break; // No more data to read
            } else {
                perror("recv failed");
                handle_client_disconnect(client_fd);
                break;
            }
        }
    }
}

void TCPServer::handle_client_disconnect(int client_fd) {
    std::cout << "Client disconnected: fd=" << client_fd << std::endl;
    
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, nullptr);
    close(client_fd);
    clients.erase(client_fd);
}

std::string TCPServer::process_command(const Command& cmd, const std::string& client_id) {
    if (cmd.type == CommandType::INVALID) {
        return ProtocolParser::format_response(Response::ERROR_INVALID_COMMAND);
    }
    
    PagedCache::OperationResult result;
    std::string value;
    
    switch (cmd.type) {
        case CommandType::ADD:
            result = cache.add_key(cmd.key, cmd.value, client_id);
            switch (result) {
                case PagedCache::OperationResult::SUCCESS:
                    return ProtocolParser::format_response(Response::OK_ADDED);
                case PagedCache::OperationResult::KEY_EXISTS:
                    return ProtocolParser::format_response(Response::ERROR_EXISTS);
                case PagedCache::OperationResult::NO_SPACE:
                    return ProtocolParser::format_response(Response::ERROR_NO_SPACE);
                default:
                    return ProtocolParser::format_response(Response::ERROR_INVALID_COMMAND);
            }
            
        case CommandType::UPDATE:
            result = cache.update_key(cmd.key, cmd.value, client_id);
            switch (result) {
                case PagedCache::OperationResult::SUCCESS:
                    return ProtocolParser::format_response(Response::OK_UPDATED);
                case PagedCache::OperationResult::KEY_NOT_FOUND:
                    return ProtocolParser::format_response(Response::ERROR_NOT_FOUND);
                case PagedCache::OperationResult::CLIENT_MISMATCH:
                    return ProtocolParser::format_response(Response::ERROR_ACCESS_DENIED);
                case PagedCache::OperationResult::NO_SPACE:
                    return ProtocolParser::format_response(Response::ERROR_NO_SPACE);
                default:
                    return ProtocolParser::format_response(Response::ERROR_INVALID_COMMAND);
            }
            
        case CommandType::GET:
            result = cache.get_key(cmd.key, value, client_id);
            switch (result) {
                case PagedCache::OperationResult::SUCCESS:
                    return ProtocolParser::format_response(Response::ok_value(value));
                case PagedCache::OperationResult::KEY_NOT_FOUND:
                    return ProtocolParser::format_response(Response::ERROR_NOT_FOUND);
                case PagedCache::OperationResult::CLIENT_MISMATCH:
                    return ProtocolParser::format_response(Response::ERROR_ACCESS_DENIED);
                default:
                    return ProtocolParser::format_response(Response::ERROR_INVALID_COMMAND);
            }
            
        case CommandType::DELETE:
            result = cache.delete_key(cmd.key, client_id);
            switch (result) {
                case PagedCache::OperationResult::SUCCESS:
                    return ProtocolParser::format_response(Response::OK_DELETED);
                case PagedCache::OperationResult::KEY_NOT_FOUND:
                    return ProtocolParser::format_response(Response::ERROR_NOT_FOUND);
                case PagedCache::OperationResult::CLIENT_MISMATCH:
                    return ProtocolParser::format_response(Response::ERROR_ACCESS_DENIED);
                default:
                    return ProtocolParser::format_response(Response::ERROR_INVALID_COMMAND);
            }
            
        default:
            return ProtocolParser::format_response(Response::ERROR_INVALID_COMMAND);
    }
}

bool TCPServer::send_response(int client_fd, const std::string& response) {
    size_t total_sent = 0;
    const char* data = response.c_str();
    size_t length = response.length();
    
    while (total_sent < length) {
        ssize_t sent = send(client_fd, data + total_sent, length - total_sent, MSG_NOSIGNAL);
        if (sent == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                continue; // Try again
            } else {
                perror("send failed");
                return false;
            }
        }
        total_sent += sent;
    }
    
    return true;
}

void TCPServer::run() {
    std::cout << "Server running... Press Ctrl+C to stop" << std::endl;
    
    struct epoll_event events[MAX_EVENTS];
    
    while (true) {
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        
        if (nfds == -1) {
            if (errno == EINTR) {
                continue; // Interrupted by signal, continue
            } else {
                perror("epoll_wait failed");
                break;
            }
        }
        
        for (int i = 0; i < nfds; i++) {
            if (events[i].data.fd == server_socket) {
                // New connection
                handle_new_connection();
            } else {
                // Client data
                handle_client_data(events[i].data.fd);
            }
        }
    }
}

void TCPServer::stop() {
    // Close all client connections
    for (auto& pair : clients) {
        close(pair.second->socket_fd);
    }
    clients.clear();
    
    // Close server socket
    if (server_socket != -1) {
        close(server_socket);
        server_socket = -1;
    }
    
    // Close epoll
    if (epoll_fd != -1) {
        close(epoll_fd);
        epoll_fd = -1;
    }
    
    std::cout << "Server stopped" << std::endl;
}

void TCPServer::print_stats() const {
    std::cout << "\n=== Server Statistics ===" << std::endl;
    std::cout << "Connected clients: " << clients.size() << std::endl;
    std::cout << "Free pages: " << cache.get_free_pages() << "/" << cache.get_total_pages() << std::endl;
    std::cout << "Memory usage: " << 
        (float)(cache.get_total_pages() - cache.get_free_pages()) / cache.get_total_pages() * 100.0f 
        << "%" << std::endl;
}