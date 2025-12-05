#include "benchmark_cache.h"
#include <algorithm>
#include <iostream>
#include <memory>

BenchmarkCache::BenchmarkCache(size_t capacity, EvictionPolicy eviction_policy) 
    : max_capacity(capacity), policy(eviction_policy), clock_hand(0) {
    
    if (policy == EvictionPolicy::CLOCK) {
        clock_buffer.reserve(capacity);
    }
}

bool BenchmarkCache::get(const std::string& key, std::string& value) {
    std::lock_guard<std::mutex> lock(cache_mutex);
    
    total_requests++;
    
    auto it = cache_map.find(key);
    if (it != cache_map.end()) {
        // Cache hit
        hit_count++;
        value = it->second->value;
        
        // Update access time and policy-specific data structures
        it->second->access_time = std::chrono::steady_clock::now();
        
        switch (policy) {
            case EvictionPolicy::LRU:
                update_lru(key);
                break;
            case EvictionPolicy::FIFO:
                // FIFO doesn't update on access
                break;
            case EvictionPolicy::SIEVE:
                update_sieve(key);
                break;
            case EvictionPolicy::CLOCK:
                update_clock(key);
                break;
        }
        
        return true;
    } else {
        // Cache miss
        miss_count++;
        return false;
    }
}

bool BenchmarkCache::put(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(cache_mutex);
    
    // Check if key already exists
    if (cache_map.find(key) != cache_map.end()) {
        // Update existing entry
        cache_map[key]->value = value;
        cache_map[key]->access_time = std::chrono::steady_clock::now();
        
        switch (policy) {
            case EvictionPolicy::LRU:
                update_lru(key);
                break;
            case EvictionPolicy::FIFO:
                // FIFO doesn't update on access
                break;
            case EvictionPolicy::SIEVE:
                update_sieve(key);
                break;
            case EvictionPolicy::CLOCK:
                update_clock(key);
                break;
        }
        return true;
    }
    
    // Check if we need to evict
    if (cache_map.size() >= max_capacity) {
        switch (policy) {
            case EvictionPolicy::LRU:
                evict_lru();
                break;
            case EvictionPolicy::FIFO:
                evict_fifo();
                break;
            case EvictionPolicy::SIEVE:
                evict_sieve();
                break;
            case EvictionPolicy::CLOCK:
                evict_clock();
                break;
        }
    }
    
    // Add new entry
    auto entry = std::make_shared<BenchmarkCacheEntry>(key, value);
    cache_map[key] = entry;
    
    switch (policy) {
        case EvictionPolicy::LRU:
            add_lru(key);
            break;
        case EvictionPolicy::FIFO:
            add_fifo(key);
            break;
        case EvictionPolicy::SIEVE:
            add_sieve(key);
            break;
        case EvictionPolicy::CLOCK:
            add_clock(key);
            break;
    }
    
    return true;
}

// LRU Implementation
void BenchmarkCache::evict_lru() {
    if (!lru_order.empty()) {
        std::string lru_key = lru_order.back();
        lru_order.pop_back();
        cache_map.erase(lru_key);
    }
}

void BenchmarkCache::update_lru(const std::string& key) {
    // Remove from current position
    auto it = std::find(lru_order.begin(), lru_order.end(), key);
    if (it != lru_order.end()) {
        lru_order.erase(it);
    }
    // Add to front (most recently used)
    lru_order.push_front(key);
}

void BenchmarkCache::add_lru(const std::string& key) {
    lru_order.push_front(key);
}

// FIFO Implementation
void BenchmarkCache::evict_fifo() {
    if (!fifo_queue.empty()) {
        std::string fifo_key = fifo_queue.front();
        fifo_queue.pop();
        cache_map.erase(fifo_key);
    }
}

void BenchmarkCache::update_fifo(const std::string& key) {
    // FIFO doesn't change order on access
    (void)key; // Suppress unused parameter warning
}

void BenchmarkCache::add_fifo(const std::string& key) {
    fifo_queue.push(key);
}

// SIEVE Implementation
void BenchmarkCache::evict_sieve() {
    if (sieve_order.empty()) return;
    
    // SIEVE: Look for first non-visited item, mark visited items as non-visited
    auto it = sieve_order.begin();
    while (it != sieve_order.end()) {
        auto cache_it = cache_map.find(*it);
        if (cache_it != cache_map.end()) {
            if (!cache_it->second->visited) {
                // Found item to evict
                std::string key_to_evict = *it;
                sieve_order.erase(it);
                cache_map.erase(key_to_evict);
                return;
            } else {
                // Reset visited flag and continue
                cache_it->second->visited = false;
                ++it;
            }
        } else {
            // Inconsistent state, remove from sieve_order
            it = sieve_order.erase(it);
        }
    }
    
    // If all items were visited, evict the first one
    if (!sieve_order.empty()) {
        std::string key_to_evict = sieve_order.front();
        sieve_order.pop_front();
        cache_map.erase(key_to_evict);
    }
}

void BenchmarkCache::update_sieve(const std::string& key) {
    auto it = cache_map.find(key);
    if (it != cache_map.end()) {
        it->second->visited = true;
    }
}

void BenchmarkCache::add_sieve(const std::string& key) {
    sieve_order.push_back(key);
    auto it = cache_map.find(key);
    if (it != cache_map.end()) {
        it->second->visited = false;
    }
}

// CLOCK Implementation
void BenchmarkCache::evict_clock() {
    if (clock_buffer.empty()) return;
    
    // CLOCK algorithm: sweep until we find a page with reference bit = 0
    while (true) {
        std::string current_key = clock_buffer[clock_hand];
        auto cache_it = cache_map.find(current_key);
        
        if (cache_it != cache_map.end()) {
            if (!cache_it->second->reference_bit) {
                // Found item to evict
                cache_map.erase(current_key);
                clock_buffer[clock_hand] = ""; // Mark as empty
                return;
            } else {
                // Reset reference bit and continue
                cache_it->second->reference_bit = false;
            }
        }
        
        clock_hand = (clock_hand + 1) % clock_buffer.size();
    }
}

void BenchmarkCache::update_clock(const std::string& key) {
    auto it = cache_map.find(key);
    if (it != cache_map.end()) {
        it->second->reference_bit = true;
    }
}

void BenchmarkCache::add_clock(const std::string& key) {
    // Find empty slot or expand if needed
    bool found_slot = false;
    for (size_t i = 0; i < clock_buffer.size(); i++) {
        if (clock_buffer[i].empty()) {
            clock_buffer[i] = key;
            found_slot = true;
            break;
        }
    }
    
    if (!found_slot) {
        clock_buffer.push_back(key);
    }
    
    auto it = cache_map.find(key);
    if (it != cache_map.end()) {
        it->second->reference_bit = true;
    }
}

double BenchmarkCache::get_hit_ratio() const {
    uint64_t total = total_requests.load();
    if (total == 0) return 0.0;
    return static_cast<double>(hit_count.load()) / total;
}

size_t BenchmarkCache::get_current_size() const {
    return cache_map.size();
}

void BenchmarkCache::reset_stats() {
    hit_count = 0;
    miss_count = 0;
    total_requests = 0;
}

void BenchmarkCache::print_stats() const {
    std::cout << "Cache Stats for " << get_policy_name() << ":" << std::endl;
    std::cout << "  Total Requests: " << total_requests.load() << std::endl;
    std::cout << "  Hits: " << hit_count.load() << std::endl;
    std::cout << "  Misses: " << miss_count.load() << std::endl;
    std::cout << "  Hit Ratio: " << (get_hit_ratio() * 100) << "%" << std::endl;
    std::cout << "  Current Size: " << get_current_size() << "/" << max_capacity << std::endl;
}

std::string BenchmarkCache::get_policy_name() const {
    switch (policy) {
        case EvictionPolicy::LRU: return "LRU";
        case EvictionPolicy::FIFO: return "FIFO";
        case EvictionPolicy::SIEVE: return "SIEVE";
        case EvictionPolicy::CLOCK: return "CLOCK";
        default: return "UNKNOWN";
    }
}