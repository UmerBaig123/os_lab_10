#!/usr/bin/env python3
"""
Load testing script for the Paged Key-Value Cache Server
Tests throughput and concurrent performance
"""

import threading
import time
import sys
from test_client import CacheClient

def load_worker(worker_id, operations=1000, server_host='localhost', server_port=8080):
    """Worker function for load testing"""
    client = CacheClient(server_host, server_port)
    if not client.connect():
        print(f"Worker {worker_id}: Failed to connect")
        return
    
    try:
        start = time.time()
        success_count = 0
        
        for i in range(operations):
            key = f"load_key_{worker_id}_{i}"
            value = f"load_value_{worker_id}_{i}_{'x' * 100}"  # ~120 byte values
            
            # Perform ADD-GET-UPDATE-DELETE cycle
            if "OK" in client.add(key, value):
                if "Value=" in client.get(key):
                    if "OK" in client.update(key, value + "_updated"):
                        if "OK" in client.delete(key):
                            success_count += 1
        
        elapsed = time.time() - start
        ops_per_sec = (success_count * 4) / elapsed  # 4 operations per successful cycle
        
        print(f"Worker {worker_id}: {success_count}/{operations} cycles completed in {elapsed:.2f}s = {ops_per_sec:.1f} ops/sec")
        
    finally:
        client.disconnect()

def run_load_test(num_workers=5, operations_per_worker=1000):
    """Run load test with specified parameters"""
    print(f"Starting load test with {num_workers} workers, {operations_per_worker} operations each")
    print("Each operation cycle: ADD → GET → UPDATE → DELETE")
    
    start_time = time.time()
    
    # Start worker threads
    workers = []
    for i in range(num_workers):
        worker = threading.Thread(target=load_worker, args=(i, operations_per_worker))
        workers.append(worker)
        worker.start()
    
    # Wait for all workers to complete
    for worker in workers:
        worker.join()
    
    total_time = time.time() - start_time
    total_operations = num_workers * operations_per_worker * 4
    overall_throughput = total_operations / total_time
    
    print(f"\n=== Load Test Summary ===")
    print(f"Total time: {total_time:.2f} seconds")
    print(f"Total operations: {total_operations}")
    print(f"Overall throughput: {overall_throughput:.1f} ops/sec")

def run_stress_test():
    """Run stress test with increasing load"""
    print("=== Stress Test ===")
    
    worker_counts = [1, 2, 5, 10, 20]
    operations = 500
    
    for workers in worker_counts:
        print(f"\nTesting with {workers} concurrent workers...")
        run_load_test(workers, operations)
        time.sleep(2)  # Brief pause between tests

def main():
    if len(sys.argv) > 1:
        if sys.argv[1] == "stress":
            run_stress_test()
        elif sys.argv[1] == "help":
            print("Usage:")
            print("  python3 load_test.py           # Run default load test (5 workers, 1000 ops)")
            print("  python3 load_test.py stress    # Run stress test with increasing load")
            print("  python3 load_test.py help      # Show this help")
        else:
            try:
                workers = int(sys.argv[1])
                ops = int(sys.argv[2]) if len(sys.argv) > 2 else 1000
                run_load_test(workers, ops)
            except ValueError:
                print("Invalid arguments. Use 'python3 load_test.py help' for usage.")
    else:
        run_load_test()

if __name__ == "__main__":
    main()