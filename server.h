#ifndef SERVER_H
#define SERVER_H

#include "cache.h"
#include "protocol.h"
#include <vector>
#include <unordered_map>
#include <string>
#include <sys/socket.h>
#include <sys/epoll.h>

struct ClientConnection {
    int socket_fd;
    std::string client_id;
    std::string buffer;
    bool authenticated;
    
    ClientConnection(int fd) : socket_fd(fd), authenticated(false) {
        // Generate simple client ID based on socket descriptor
        client_id = "client_" + std::to_string(fd);
    }
};

class TCPServer {
private:
    int server_socket;
    int epoll_fd;
    PagedCache cache;
    std::unordered_map<int, std::unique_ptr<ClientConnection>> clients;
    
    bool setup_server_socket(int port);
    void setup_epoll();
    void handle_new_connection();
    void handle_client_data(int client_fd);
    void handle_client_disconnect(int client_fd);
    std::string process_command(const Command& cmd, const std::string& client_id);
    bool send_response(int client_fd, const std::string& response);
    
public:
    TCPServer();
    ~TCPServer();
    
    bool start(int port = 8080);
    void run();
    void stop();
    
    // Statistics
    void print_stats() const;
};

#endif // SERVER_H