#include "benchmark_server.h"
#include "benchmark_cache.h"
#include "trace_processor.h"
#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <iomanip>
#include <algorithm>
#include <stdexcept>

void print_usage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [options]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -c, --capacity <size>    Cache capacity (default: 10000)" << std::endl;
    std::cout << "  -f, --file <path>        Local trace file path" << std::endl;
    std::cout << "  -u, --url <url>          Trace file URL (default: cache-t-00.gz from Wikimedia)" << std::endl;
    std::cout << "  -a, --algorithm <alg>    Cache algorithm: LRU, FIFO, SIEVE, CLOCK (default: all)" << std::endl;
    std::cout << "  -h, --help              Show this help message" << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  " << program_name << " --capacity 5000 --algorithm LRU" << std::endl;
    std::cout << "  " << program_name << " --file ./my_trace.txt" << std::endl;
    std::cout << "  " << program_name << " --url https://analytics.wikimedia.org/published/datasets/caching/2019/text/cache-t-00.gz" << std::endl;
}

EvictionPolicy string_to_policy(const std::string& policy_str) {
    std::string upper_str = policy_str;
    std::transform(upper_str.begin(), upper_str.end(), upper_str.begin(), ::toupper);
    
    if (upper_str == "LRU") return EvictionPolicy::LRU;
    if (upper_str == "FIFO") return EvictionPolicy::FIFO;
    if (upper_str == "SIEVE") return EvictionPolicy::SIEVE;
    if (upper_str == "CLOCK") return EvictionPolicy::CLOCK;
    
    throw std::invalid_argument("Unknown policy: " + policy_str);
}

void run_single_benchmark(size_t capacity, EvictionPolicy policy, 
                         const std::string& trace_source, bool is_url) {
    
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "Running benchmark for: " << 
        (policy == EvictionPolicy::LRU ? "LRU" :
         policy == EvictionPolicy::FIFO ? "FIFO" :
         policy == EvictionPolicy::SIEVE ? "SIEVE" : "CLOCK") << std::endl;
    std::cout << "Cache capacity: " << capacity << " entries" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    BenchmarkServer server(capacity, policy);
    
    if (!server.start()) {
        std::cerr << "Failed to start benchmark server" << std::endl;
        return;
    }
    
    if (is_url) {
        server.run_trace_benchmark(trace_source);
    } else {
        server.run_trace_benchmark_from_file(trace_source);
    }
    
    server.stop();
}

int main(int argc, char* argv[]) {
    size_t capacity = 10000;
    std::string trace_source = "https://analytics.wikimedia.org/published/datasets/caching/2019/text/cache-t-00.gz";
    std::string algorithm = "ALL";
    bool is_url = true;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if ((arg == "-h") || (arg == "--help")) {
            print_usage(argv[0]);
            return 0;
        } else if ((arg == "-c") || (arg == "--capacity")) {
            if (i + 1 < argc) {
                capacity = std::stoull(argv[++i]);
            } else {
                std::cerr << "Error: --capacity requires a value" << std::endl;
                return 1;
            }
        } else if ((arg == "-f") || (arg == "--file")) {
            if (i + 1 < argc) {
                trace_source = argv[++i];
                is_url = false;
            } else {
                std::cerr << "Error: --file requires a path" << std::endl;
                return 1;
            }
        } else if ((arg == "-u") || (arg == "--url")) {
            if (i + 1 < argc) {
                trace_source = argv[++i];
                is_url = true;
            } else {
                std::cerr << "Error: --url requires a URL" << std::endl;
                return 1;
            }
        } else if ((arg == "-a") || (arg == "--algorithm")) {
            if (i + 1 < argc) {
                algorithm = argv[++i];
            } else {
                std::cerr << "Error: --algorithm requires a value" << std::endl;
                return 1;
            }
        } else {
            std::cerr << "Unknown argument: " << arg << std::endl;
            print_usage(argv[0]);
            return 1;
        }
    }
    
    std::cout << "Cache Benchmarking Tool" << std::endl;
    std::cout << "======================" << std::endl;
    std::cout << "Trace source: " << trace_source << std::endl;
    std::cout << "Cache capacity: " << capacity << " entries" << std::endl;
    
    std::vector<EvictionPolicy> policies_to_test;
    
    if (algorithm == "ALL") {
        policies_to_test = {
            EvictionPolicy::LRU,
            EvictionPolicy::FIFO,
            EvictionPolicy::SIEVE,
            EvictionPolicy::CLOCK
        };
    } else {
        try {
            policies_to_test.push_back(string_to_policy(algorithm));
        } catch (const std::invalid_argument& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            std::cerr << "Valid algorithms: LRU, FIFO, SIEVE, CLOCK" << std::endl;
            return 1;
        }
    }
    
    std::vector<std::pair<std::string, double>> results;
    
    // Run benchmarks
    for (auto policy : policies_to_test) {
        run_single_benchmark(capacity, policy, trace_source, is_url);
        
        // Quick test to get hit ratio (create a temporary cache)
        BenchmarkCache temp_cache(capacity, policy);
        
        // Store policy name for results summary
        std::string policy_name = temp_cache.get_policy_name();
        
        // We'll need to run a quick benchmark to get actual results
        TraceProcessor processor(trace_source);
        if (processor.initialize()) {
            int count = 0;
            while (processor.has_next() && count < 10000) { // Sample first 10k for quick results
                TraceEntry entry = processor.get_next();
                if (entry.key.empty()) break;
                
                std::string value;
                if (!temp_cache.get(entry.key, value)) {
                    temp_cache.put(entry.key, "dummy_value");
                }
                count++;
            }
            results.emplace_back(policy_name, temp_cache.get_hit_ratio());
        }
    }
    
    // Print summary
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "BENCHMARK SUMMARY" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    std::cout << std::setw(15) << "Algorithm" << std::setw(20) << "Hit Ratio (%)" << std::endl;
    std::cout << std::string(35, '-') << std::endl;
    
    for (const auto& result : results) {
        std::cout << std::setw(15) << result.first 
                  << std::setw(20) << std::fixed << std::setprecision(2) 
                  << (result.second * 100) << "%" << std::endl;
    }
    
    // Find best performing algorithm
    if (!results.empty()) {
        auto best = std::max_element(results.begin(), results.end(),
            [](const auto& a, const auto& b) { return a.second < b.second; });
        
        std::cout << "\nBest performing algorithm: " << best->first 
                  << " (" << std::fixed << std::setprecision(2) 
                  << (best->second * 100) << "%)" << std::endl;
    }
    
    return 0;
}