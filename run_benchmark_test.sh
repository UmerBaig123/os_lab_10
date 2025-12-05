#!/bin/bash

echo "Cache Benchmark Test Script"
echo "=========================="

# Check if cache_benchmark executable exists
if [ ! -f "./cache_benchmark" ]; then
    echo "Error: cache_benchmark executable not found. Please run 'make cache_benchmark' first."
    exit 1
fi

# Create a simple test trace file for quick testing
echo "Creating simple test trace..."
cat > test_trace.txt << EOF
timestamp1 key1 1024
timestamp2 key2 1024
timestamp3 key3 1024
timestamp4 key1 1024
timestamp5 key4 1024
timestamp6 key2 1024
timestamp7 key5 1024
timestamp8 key1 1024
timestamp9 key6 1024
timestamp10 key7 1024
EOF

echo "Test trace created with 10 entries (3 unique keys repeated)"
echo ""

# Test with small cache capacity to see evictions
echo "Running benchmark with small cache (capacity=2) to test eviction algorithms..."
echo ""

echo "Testing LRU algorithm:"
./cache_benchmark --capacity 2 --algorithm LRU --file test_trace.txt
echo ""

echo "Testing FIFO algorithm:"
./cache_benchmark --capacity 2 --algorithm FIFO --file test_trace.txt
echo ""

echo "Testing SIEVE algorithm:"
./cache_benchmark --capacity 2 --algorithm SIEVE --file test_trace.txt
echo ""

echo "Testing CLOCK algorithm:"
./cache_benchmark --capacity 2 --algorithm CLOCK --file test_trace.txt
echo ""

echo "Running comparison with all algorithms:"
./cache_benchmark --capacity 2 --file test_trace.txt

# Check if user wants to download and test with real Wikimedia trace
echo ""
read -p "Do you want to download and test with the real Wikimedia trace file? (y/n): " -n 1 -r
echo ""
if [[ $REPLY =~ ^[Yy]$ ]]; then
    echo "This will download cache-t-00.gz (~few hundred MB) and run the full benchmark..."
    echo "Running with default settings (capacity=10000, all algorithms):"
    ./cache_benchmark
fi

echo ""
echo "Cleaning up test files..."
rm -f test_trace.txt

echo "Benchmark testing completed!"