#ifndef BENCHMARK_CACHE_H
#define BENCHMARK_CACHE_H

#include <unordered_map>
#include <vector>
#include <string>
#include <list>
#include <queue>
#include <mutex>
#include <chrono>
#include <atomic>
#include <memory>

// Cache eviction policies
enum class EvictionPolicy {
    LRU,    // Least Recently Used
    FIFO,   // First In, First Out  
    SIEVE,  // SIEVE algorithm
    CLOCK   // Clock algorithm
};

struct BenchmarkCacheEntry {
    std::string key;
    std::string value;
    std::chrono::steady_clock::time_point access_time;
    std::chrono::steady_clock::time_point insert_time;
    
    // For SIEVE algorithm
    bool visited;
    
    // For CLOCK algorithm
    bool reference_bit;
    
    BenchmarkCacheEntry(const std::string& k, const std::string& v) 
        : key(k), value(v), visited(false), reference_bit(true) {
        auto now = std::chrono::steady_clock::now();
        access_time = now;
        insert_time = now;
    }
};

class BenchmarkCache {
private:
    size_t max_capacity;
    EvictionPolicy policy;
    std::unordered_map<std::string, std::shared_ptr<BenchmarkCacheEntry>> cache_map;
    std::mutex cache_mutex;
    
    // Statistics
    std::atomic<uint64_t> hit_count{0};
    std::atomic<uint64_t> miss_count{0};
    std::atomic<uint64_t> total_requests{0};
    
    // LRU specific
    std::list<std::string> lru_order;
    
    // FIFO specific  
    std::queue<std::string> fifo_queue;
    
    // SIEVE specific
    std::list<std::string> sieve_order;
    
    // CLOCK specific
    std::vector<std::string> clock_buffer;
    size_t clock_hand;
    
    // Eviction methods
    void evict_lru();
    void evict_fifo();
    void evict_sieve();
    void evict_clock();
    
    // Update methods for each policy
    void update_lru(const std::string& key);
    void update_fifo(const std::string& key);
    void update_sieve(const std::string& key);
    void update_clock(const std::string& key);
    
    // Add methods for each policy
    void add_lru(const std::string& key);
    void add_fifo(const std::string& key);
    void add_sieve(const std::string& key);
    void add_clock(const std::string& key);

public:
    BenchmarkCache(size_t capacity, EvictionPolicy eviction_policy);
    ~BenchmarkCache() = default;
    
    bool get(const std::string& key, std::string& value);
    bool put(const std::string& key, const std::string& value);
    
    // Statistics
    double get_hit_ratio() const;
    uint64_t get_hit_count() const { return hit_count.load(); }
    uint64_t get_miss_count() const { return miss_count.load(); }
    uint64_t get_total_requests() const { return total_requests.load(); }
    size_t get_current_size() const;
    void reset_stats();
    
    // For debugging
    void print_stats() const;
    std::string get_policy_name() const;
};

#endif // BENCHMARK_CACHE_H