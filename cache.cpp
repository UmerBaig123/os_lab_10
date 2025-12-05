#include "cache.h"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <cstring>

PagedCache::PagedCache() : page_bitmap(TOTAL_PAGES, false) {
    std::cout << "Initialized cache with " << TOTAL_PAGES << " pages (" 
              << (TOTAL_CACHE_SIZE / (1024 * 1024)) << " MB)" << std::endl;
}

size_t PagedCache::calculate_pages_needed(const std::string& value) {
    size_t data_size = value.size();
    return (data_size + PAGE_SIZE - 1) / PAGE_SIZE;  // Ceiling division
}

bool PagedCache::find_contiguous_pages(size_t num_pages, size_t& start_page) {
    if (num_pages > TOTAL_PAGES) return false;
    
    for (size_t i = 0; i <= TOTAL_PAGES - num_pages; ++i) {
        bool found = true;
        for (size_t j = 0; j < num_pages; ++j) {
            if (page_bitmap[i + j]) {
                found = false;
                i = i + j;  // Skip ahead past this allocated page
                break;
            }
        }
        if (found) {
            start_page = i;
            return true;
        }
    }
    return false;
}

void PagedCache::allocate_pages(size_t start_page, size_t num_pages) {
    for (size_t i = start_page; i < start_page + num_pages; ++i) {
        page_bitmap[i] = true;
    }
}

void PagedCache::deallocate_pages(size_t start_page, size_t num_pages) {
    for (size_t i = start_page; i < start_page + num_pages; ++i) {
        page_bitmap[i] = false;
    }
}

void PagedCache::update_lru(const std::string& key) {
    // Remove key from current position in LRU list
    auto it = std::find(lru_order.begin(), lru_order.end(), key);
    if (it != lru_order.end()) {
        lru_order.erase(it);
    }
    // Add to front (most recently used)
    lru_order.push_front(key);
}

bool PagedCache::evict_lru_if_needed(size_t pages_needed, const std::string& requesting_client) {
    size_t free_pages = get_free_pages();
    
    if (free_pages >= pages_needed) {
        return true;  // No eviction needed
    }
    
    size_t pages_to_free = pages_needed - free_pages;
    size_t pages_freed = 0;
    
    // Try to evict from least recently used, but only from other clients
    auto it = lru_order.rbegin();
    while (it != lru_order.rend() && pages_freed < pages_to_free) {
        const std::string& lru_key = *it;
        auto entry_it = cache_map.find(lru_key);
        
        if (entry_it != cache_map.end() && entry_it->second->client_id != requesting_client) {
            size_t entry_pages = entry_it->second->num_pages;
            deallocate_pages(entry_it->second->start_page, entry_pages);
            cache_map.erase(entry_it);
            
            it = std::reverse_iterator(lru_order.erase(std::next(it).base()));
            pages_freed += entry_pages;
        } else {
            ++it;
        }
    }
    
    return pages_freed >= pages_to_free;
}

PagedCache::OperationResult PagedCache::add_key(const std::string& key, const std::string& value, const std::string& client_id) {
    std::lock_guard<std::mutex> lock(cache_mutex);
    
    // Check if key already exists
    if (cache_map.find(key) != cache_map.end()) {
        return OperationResult::KEY_EXISTS;
    }
    
    size_t pages_needed = calculate_pages_needed(value);
    size_t start_page;
    
    // Try to find contiguous space
    if (!find_contiguous_pages(pages_needed, start_page)) {
        // Try LRU eviction
        if (!evict_lru_if_needed(pages_needed, client_id)) {
            return OperationResult::NO_SPACE;
        }
        // Try again after eviction
        if (!find_contiguous_pages(pages_needed, start_page)) {
            return OperationResult::NO_SPACE;
        }
    }
    
    // Allocate pages and store data
    allocate_pages(start_page, pages_needed);
    cache_map[key] = std::make_unique<CacheEntry>(key, value, start_page, pages_needed, client_id);
    update_lru(key);
    
    return OperationResult::SUCCESS;
}

PagedCache::OperationResult PagedCache::update_key(const std::string& key, const std::string& value, const std::string& client_id) {
    std::lock_guard<std::mutex> lock(cache_mutex);
    
    auto it = cache_map.find(key);
    if (it == cache_map.end()) {
        return OperationResult::KEY_NOT_FOUND;
    }
    
    // Check client ownership
    if (it->second->client_id != client_id) {
        return OperationResult::CLIENT_MISMATCH;
    }
    
    size_t new_pages_needed = calculate_pages_needed(value);
    size_t old_pages = it->second->num_pages;
    
    if (new_pages_needed <= old_pages) {
        // Can fit in existing allocation
        it->second->value = value;
        it->second->num_pages = new_pages_needed;
        it->second->last_access = std::chrono::steady_clock::now();
        
        // Deallocate unused pages
        if (new_pages_needed < old_pages) {
            deallocate_pages(it->second->start_page + new_pages_needed, old_pages - new_pages_needed);
        }
        
        update_lru(key);
        return OperationResult::SUCCESS;
    } else {
        // Need more space - deallocate old and try to allocate new
        size_t old_start = it->second->start_page;
        deallocate_pages(old_start, old_pages);
        
        size_t new_start_page;
        if (!find_contiguous_pages(new_pages_needed, new_start_page)) {
            // Try LRU eviction
            if (!evict_lru_if_needed(new_pages_needed, client_id)) {
                // Restore old allocation
                allocate_pages(old_start, old_pages);
                return OperationResult::NO_SPACE;
            }
            if (!find_contiguous_pages(new_pages_needed, new_start_page)) {
                // Restore old allocation
                allocate_pages(old_start, old_pages);
                return OperationResult::NO_SPACE;
            }
        }
        
        // Update with new allocation
        allocate_pages(new_start_page, new_pages_needed);
        it->second->value = value;
        it->second->start_page = new_start_page;
        it->second->num_pages = new_pages_needed;
        it->second->last_access = std::chrono::steady_clock::now();
        
        update_lru(key);
        return OperationResult::SUCCESS;
    }
}

PagedCache::OperationResult PagedCache::get_key(const std::string& key, std::string& value, const std::string& client_id) {
    std::lock_guard<std::mutex> lock(cache_mutex);
    
    auto it = cache_map.find(key);
    if (it == cache_map.end()) {
        return OperationResult::KEY_NOT_FOUND;
    }
    
    // Check client ownership
    if (it->second->client_id != client_id) {
        return OperationResult::CLIENT_MISMATCH;
    }
    
    value = it->second->value;
    it->second->last_access = std::chrono::steady_clock::now();
    update_lru(key);
    
    return OperationResult::SUCCESS;
}

PagedCache::OperationResult PagedCache::delete_key(const std::string& key, const std::string& client_id) {
    std::lock_guard<std::mutex> lock(cache_mutex);
    
    auto it = cache_map.find(key);
    if (it == cache_map.end()) {
        return OperationResult::KEY_NOT_FOUND;
    }
    
    // Check client ownership
    if (it->second->client_id != client_id) {
        return OperationResult::CLIENT_MISMATCH;
    }
    
    // Deallocate pages
    deallocate_pages(it->second->start_page, it->second->num_pages);
    
    // Remove from LRU list
    auto lru_it = std::find(lru_order.begin(), lru_order.end(), key);
    if (lru_it != lru_order.end()) {
        lru_order.erase(lru_it);
    }
    
    // Remove from cache
    cache_map.erase(it);
    
    return OperationResult::SUCCESS;
}

size_t PagedCache::get_free_pages() const {
    return std::count(page_bitmap.begin(), page_bitmap.end(), false);
}