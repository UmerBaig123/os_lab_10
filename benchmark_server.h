#ifndef BENCHMARK_SERVER_H
#define BENCHMARK_SERVER_H

#include "benchmark_cache.h"
#include "trace_processor.h"
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <memory>
#include <functional>

struct BenchmarkRequest {
    std::string key;
    std::string operation;
    std::string client_id;
    
    BenchmarkRequest(const std::string& k, const std::string& op, const std::string& client = "benchmark")
        : key(k), operation(op), client_id(client) {}
};

class BenchmarkServer {
private:
    std::unique_ptr<BenchmarkCache> cache;
    std::vector<std::thread> worker_threads;
    std::thread listener_thread;
    
    // Thread pool for handling requests
    std::queue<BenchmarkRequest> request_queue;
    std::mutex queue_mutex;
    std::condition_variable queue_condition;
    std::atomic<bool> running{false};
    std::atomic<bool> shutdown{false};
    
    // Worker configuration
    static const int NUM_WORKER_THREADS = 4;
    
    // Worker thread function
    void worker_thread_function();
    
    // Listener thread function (simulates network listener)
    void listener_thread_function();
    
    // Process individual request
    void process_request(const BenchmarkRequest& request);

public:
    BenchmarkServer(size_t cache_capacity, EvictionPolicy policy);
    ~BenchmarkServer();
    
    bool start();
    void stop();
    
    // Add request to queue (simulates network request)
    void add_request(const BenchmarkRequest& request);
    
    // Benchmark specific methods
    void run_trace_benchmark(const std::string& trace_url);
    void run_trace_benchmark_from_file(const std::string& trace_file);
    
    // Statistics
    void print_statistics() const;
    BenchmarkCache* get_cache() { return cache.get(); }
};

#endif // BENCHMARK_SERVER_H