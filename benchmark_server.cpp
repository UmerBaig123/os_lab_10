#include "benchmark_server.h"
#include <iostream>
#include <chrono>
#include <sstream>
#include <thread>

BenchmarkServer::BenchmarkServer(size_t cache_capacity, EvictionPolicy policy) {
    cache = std::make_unique<BenchmarkCache>(cache_capacity, policy);
}

BenchmarkServer::~BenchmarkServer() {
    stop();
}

bool BenchmarkServer::start() {
    if (running) {
        std::cerr << "Server already running" << std::endl;
        return false;
    }
    
    running = true;
    shutdown = false;
    
    // Start worker threads
    for (int i = 0; i < NUM_WORKER_THREADS; i++) {
        worker_threads.emplace_back(&BenchmarkServer::worker_thread_function, this);
    }
    
    // Start listener thread
    listener_thread = std::thread(&BenchmarkServer::listener_thread_function, this);
    
    std::cout << "Benchmark server started with " << NUM_WORKER_THREADS << " worker threads" << std::endl;
    std::cout << "Cache policy: " << cache->get_policy_name() << std::endl;
    
    return true;
}

void BenchmarkServer::stop() {
    if (!running) {
        return;
    }
    
    std::cout << "Stopping benchmark server..." << std::endl;
    
    shutdown = true;
    running = false;
    
    // Wake up all worker threads
    queue_condition.notify_all();
    
    // Wait for worker threads to finish
    for (auto& thread : worker_threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    worker_threads.clear();
    
    // Wait for listener thread
    if (listener_thread.joinable()) {
        listener_thread.join();
    }
    
    std::cout << "Benchmark server stopped" << std::endl;
}

void BenchmarkServer::worker_thread_function() {
    while (running) {
        std::unique_lock<std::mutex> lock(queue_mutex);
        
        // Wait for request or shutdown
        queue_condition.wait(lock, [this] {
            return !request_queue.empty() || shutdown;
        });
        
        if (shutdown && request_queue.empty()) {
            break;
        }
        
        if (!request_queue.empty()) {
            BenchmarkRequest request = request_queue.front();
            request_queue.pop();
            lock.unlock();
            
            // Process the request
            process_request(request);
        }
    }
}

void BenchmarkServer::listener_thread_function() {
    // This thread would normally handle network connections
    // For benchmark purposes, it's mainly a placeholder
    // The actual requests come from the trace processor
    
    while (running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // In a real server, this would accept connections and
        // parse incoming requests, then add them to the queue
    }
}

void BenchmarkServer::process_request(const BenchmarkRequest& request) {
    if (request.operation == "GET") {
        std::string value;
        bool hit = cache->get(request.key, value);
        
        if (!hit) {
            // Cache miss - add the key with a dummy value
            // In real scenario, this would fetch from storage
            std::string dummy_value = "cached_content_for_" + request.key;
            cache->put(request.key, dummy_value);
        }
    } else if (request.operation == "PUT") {
        std::string dummy_value = "cached_content_for_" + request.key;
        cache->put(request.key, dummy_value);
    }
}

void BenchmarkServer::add_request(const BenchmarkRequest& request) {
    if (!running) {
        return;
    }
    
    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        request_queue.push(request);
    }
    queue_condition.notify_one();
}

void BenchmarkServer::run_trace_benchmark(const std::string& trace_url) {
    std::cout << "Starting benchmark with trace: " << trace_url << std::endl;
    
    TraceProcessor processor(trace_url);
    if (!processor.initialize()) {
        std::cerr << "Failed to initialize trace processor" << std::endl;
        return;
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    cache->reset_stats();
    
    int processed_requests = 0;
    while (processor.has_next()) {
        TraceEntry entry = processor.get_next();
        
        if (entry.key.empty() || entry.operation == "EOF") {
            break;
        }
        
        // Add GET request to simulate the benchmark algorithm:
        // 1. GET (key)
        // 2. If not hit, ADD
        BenchmarkRequest request(entry.key, "GET");
        add_request(request);
        
        processed_requests++;
        
        // Print progress every 10000 requests
        if (processed_requests % 10000 == 0) {
            std::cout << "Processed " << processed_requests << " requests..." << std::endl;
        }
    }
    
    // Wait for all requests to be processed
    std::cout << "Waiting for all requests to be processed..." << std::endl;
    
    while (true) {
        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            if (request_queue.empty()) {
                break;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // Give workers a bit more time to finish current work
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "\n=== Benchmark Results ===" << std::endl;
    std::cout << "Total processing time: " << duration.count() << " ms" << std::endl;
    std::cout << "Total requests processed: " << processed_requests << std::endl;
    
    if (duration.count() > 0) {
        double requests_per_second = (processed_requests * 1000.0) / duration.count();
        std::cout << "Requests per second: " << requests_per_second << std::endl;
    }
    
    print_statistics();
}

void BenchmarkServer::run_trace_benchmark_from_file(const std::string& trace_file) {
    std::cout << "Starting benchmark with local trace file: " << trace_file << std::endl;
    
    // Pass the file path directly to the TraceProcessor
    run_trace_benchmark(trace_file);
}

void BenchmarkServer::print_statistics() const {
    std::cout << "\n=== Cache Statistics ===" << std::endl;
    cache->print_stats();
}