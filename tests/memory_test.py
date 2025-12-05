#!/usr/bin/env python3
"""
Memory pressure testing script for the Paged Key-Value Cache Server
Tests cache limits, page allocation, and LRU eviction
"""

import sys
import time
from test_client import CacheClient

def test_progressive_sizes():
    """Test with progressively larger values"""
    print("=== Progressive Size Testing ===")
    
    client = CacheClient()
    if not client.connect():
        print("Failed to connect to server")
        return False
    
    # Test sizes from 100 bytes to 1MB
    test_sizes = [
        100,        # Small value
        1024,       # 1KB
        10240,      # 10KB  
        40960,      # 40KB (1 page)
        81920,      # 80KB (2 pages)
        163840,     # 160KB (4 pages)
        327680,     # 320KB (8 pages)
        1048576,    # 1MB (25+ pages)
    ]
    
    success_count = 0
    
    for size in test_sizes:
        print(f"\nTesting value size: {size:,} bytes ({size//1024}KB)")
        large_value = "x" * size
        key = f"size_test_{size}"
        
        # Test ADD
        add_result = client.add(key, large_value)
        print(f"  ADD: {add_result}")
        
        if "OK" in add_result:
            # Test GET
            get_result = client.get(key)
            if f"Value={'x' * size}" in get_result:
                print(f"  GET: ✓ Retrieved {size:,} bytes correctly")
                success_count += 1
                
                # Test UPDATE with larger value
                larger_value = "y" * (size + 1000)
                update_result = client.update(key, larger_value)
                print(f"  UPDATE (+1000 bytes): {update_result}")
                
            else:
                print(f"  GET: ✗ Retrieval failed")
                
            # Cleanup
            client.delete(key)
        else:
            print(f"  Failed to add {size:,} byte value")
            if "not enough" in add_result.lower():
                print(f"  Cache exhausted at {size:,} bytes")
                break
    
    client.disconnect()
    print(f"\nSuccessfully handled {success_count}/{len(test_sizes)} size categories")
    return success_count > 0

def test_cache_exhaustion():
    """Test cache exhaustion and LRU eviction"""
    print("\n=== Cache Exhaustion Testing ===")
    
    client = CacheClient()
    if not client.connect():
        return False
    
    page_size = 40 * 1024  # 40KB per page
    value_size = page_size  # Use full page per value
    
    added_keys = []
    key_counter = 0
    
    print(f"Adding {value_size:,} byte values until cache exhaustion...")
    
    # Keep adding until we get "not enough space" error
    while len(added_keys) < 1000:  # Safety limit
        key = f"exhaust_test_{key_counter}"
        value = "z" * value_size
        
        result = client.add(key, value)
        
        if "OK" in result:
            added_keys.append(key)
            if len(added_keys) % 100 == 0:
                print(f"  Added {len(added_keys)} keys ({len(added_keys) * value_size // (1024*1024):,} MB)")
        else:
            print(f"  Cache exhausted after {len(added_keys)} keys")
            print(f"  Total data stored: ~{len(added_keys) * value_size // (1024*1024):,} MB")
            break
        
        key_counter += 1
    
    # Test LRU eviction by trying to add one more item
    if added_keys:
        print("\nTesting LRU eviction...")
        eviction_key = f"eviction_test_{key_counter}"
        eviction_value = "w" * value_size
        
        result = client.add(eviction_key, eviction_value)
        print(f"  Add after exhaustion: {result}")
        
        if "OK" in result:
            print("  ✓ LRU eviction successful")
            # Check if oldest key was evicted
            oldest_key = added_keys[0]
            get_result = client.get(oldest_key)
            if "not found" in get_result.lower():
                print(f"  ✓ Oldest key '{oldest_key}' was evicted")
            else:
                print(f"  ? Oldest key '{oldest_key}' still exists")
        
        # Cleanup some keys
        for key in added_keys[-10:]:  # Clean up last 10 keys
            client.delete(key)
    
    client.disconnect()
    return len(added_keys) > 0

def test_fragmentation():
    """Test memory fragmentation scenarios"""
    print("\n=== Fragmentation Testing ===")
    
    client = CacheClient()
    if not client.connect():
        return False
    
    # Add many small values
    small_keys = []
    for i in range(100):
        key = f"small_{i}"
        value = "s" * 1000  # 1KB values
        if "OK" in client.add(key, value):
            small_keys.append(key)
    
    print(f"Added {len(small_keys)} small values (1KB each)")
    
    # Delete every other key to create fragmentation
    deleted_count = 0
    for i in range(0, len(small_keys), 2):
        if "OK" in client.delete(small_keys[i]):
            deleted_count += 1
    
    print(f"Deleted {deleted_count} keys to create fragmentation")
    
    # Try to add a large value that requires contiguous pages
    large_key = "fragmentation_test"
    large_value = "L" * (40 * 1024)  # 40KB value requiring 1 page
    
    result = client.add(large_key, large_value)
    print(f"Add large value after fragmentation: {result}")
    
    # Cleanup
    client.delete(large_key)
    for key in small_keys[1::2]:  # Delete remaining keys
        client.delete(key)
    
    client.disconnect()
    return True

def main():
    print("Memory Pressure Testing for Paged Key-Value Cache")
    print("Make sure the server is running before starting tests...")
    time.sleep(1)
    
    tests = [
        ("Progressive Sizes", test_progressive_sizes),
        ("Cache Exhaustion", test_cache_exhaustion), 
        ("Fragmentation", test_fragmentation)
    ]
    
    if len(sys.argv) > 1:
        test_name = sys.argv[1].lower()
        if test_name == "sizes":
            test_progressive_sizes()
        elif test_name == "exhaustion":
            test_cache_exhaustion()
        elif test_name == "fragmentation":
            test_fragmentation()
        else:
            print("Usage: python3 memory_test.py [sizes|exhaustion|fragmentation]")
    else:
        # Run all tests
        passed = 0
        for name, test_func in tests:
            print(f"\n{'='*50}")
            print(f"Running: {name}")
            print('='*50)
            
            if test_func():
                print(f"✓ {name} completed")
                passed += 1
            else:
                print(f"✗ {name} failed")
            
            time.sleep(2)  # Brief pause between tests
        
        print(f"\n{'='*50}")
        print(f"Memory Testing Summary: {passed}/{len(tests)} tests completed")
        print('='*50)

if __name__ == "__main__":
    main()