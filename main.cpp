#include "server.h"
#include <iostream>
#include <signal.h>
#include <csignal>

TCPServer* g_server = nullptr;

void signal_handler(int signal) {
    if (g_server) {
        std::cout << "\nReceived signal " << signal << ", shutting down..." << std::endl;
        g_server->stop();
        exit(0);
    }
}

int main(int argc, char* argv[]) {
    int port = 8080;
    
    // Parse command line arguments
    if (argc == 2) {
        port = std::atoi(argv[1]);
        if (port <= 0 || port > 65535) {
            std::cerr << "Invalid port number: " << argv[1] << std::endl;
            return 1;
        }
    } else if (argc > 2) {
        std::cerr << "Usage: " << argv[0] << " [port]" << std::endl;
        return 1;
    }
    
    // Setup signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    try {
        TCPServer server;
        g_server = &server;
        
        std::cout << "Starting Paged Key-Value Cache Server..." << std::endl;
        std::cout << "Cache Configuration:" << std::endl;
        std::cout << "  Total Size: 2 GB" << std::endl;
        std::cout << "  Page Size: 40 KB" << std::endl;
        std::cout << "  Total Pages: ~52,428" << std::endl;
        std::cout << "  Features: LRU eviction, Client isolation" << std::endl;
        std::cout << std::endl;
        
        if (!server.start(port)) {
            std::cerr << "Failed to start server" << std::endl;
            return 1;
        }
        
        // Print initial statistics
        server.print_stats();
        std::cout << std::endl;
        
        // Run server
        server.run();
        
    } catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}