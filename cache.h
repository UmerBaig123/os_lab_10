#ifndef CACHE_H
#define CACHE_H

#include <unordered_map>
#include <vector>
#include <string>
#include <memory>
#include <list>
#include <mutex>

// Cache configuration constants
const size_t TOTAL_CACHE_SIZE = 2ULL * 1024 * 1024 * 1024;  // 2 GB
const size_t PAGE_SIZE = 40 * 1024;  // 40 KB
const size_t TOTAL_PAGES = TOTAL_CACHE_SIZE / PAGE_SIZE;  // ~52,428 pages

struct CacheEntry {
    std::string key;
    std::string value;
    size_t start_page;
    size_t num_pages;
    std::chrono::steady_clock::time_point last_access;
    std::string client_id;
    
    CacheEntry(const std::string& k, const std::string& v, size_t start, size_t count, const std::string& client)
        : key(k), value(v), start_page(start), num_pages(count), client_id(client) {
        last_access = std::chrono::steady_clock::now();
    }
};

class PagedCache {
private:
    std::vector<bool> page_bitmap;  // Track allocated pages
    std::unordered_map<std::string, std::unique_ptr<CacheEntry>> cache_map;
    std::list<std::string> lru_order;  // For LRU eviction
    std::mutex cache_mutex;
    
    size_t calculate_pages_needed(const std::string& value);
    bool find_contiguous_pages(size_t num_pages, size_t& start_page);
    void allocate_pages(size_t start_page, size_t num_pages);
    void deallocate_pages(size_t start_page, size_t num_pages);
    void update_lru(const std::string& key);
    bool evict_lru_if_needed(size_t pages_needed, const std::string& requesting_client);
    
public:
    PagedCache();
    ~PagedCache() = default;
    
    enum class OperationResult {
        SUCCESS,
        KEY_EXISTS,
        KEY_NOT_FOUND,
        NO_SPACE,
        CLIENT_MISMATCH
    };
    
    OperationResult add_key(const std::string& key, const std::string& value, const std::string& client_id);
    OperationResult update_key(const std::string& key, const std::string& value, const std::string& client_id);
    OperationResult get_key(const std::string& key, std::string& value, const std::string& client_id);
    OperationResult delete_key(const std::string& key, const std::string& client_id);
    
    size_t get_free_pages() const;
    size_t get_total_pages() const { return TOTAL_PAGES; }
};

#endif // CACHE_H