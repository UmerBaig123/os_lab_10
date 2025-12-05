#!/bin/bash

echo "Wikimedia Cache Trace Benchmark"
echo "==============================="
echo ""

# Configuration
TRACE_URL="https://analytics.wikimedia.org/published/datasets/caching/2019/text/cache-t-00.gz"
CACHE_CAPACITY=10000
SAMPLE_SIZE=50000  # Number of requests to sample for quick testing

# Check if cache_benchmark executable exists
if [ ! -f "./cache_benchmark" ]; then
    echo "Error: cache_benchmark executable not found. Please run 'make cache_benchmark' first."
    exit 1
fi

echo "This script will:"
echo "1. Download the Wikimedia cache trace file (cache-t-00.gz)"
echo "2. Run benchmarks for LRU, FIFO, SIEVE, and CLOCK algorithms"
echo "3. Compare hit ratios with cache capacity of $CACHE_CAPACITY entries"
echo ""

# Check if user wants to proceed
read -p "Do you want to continue? This will download ~few hundred MB of data (y/n): " -n 1 -r
echo ""
if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    echo "Benchmark cancelled."
    exit 0
fi

echo "Starting Wikimedia cache trace benchmark..."
echo ""

# Run benchmark with smaller capacity first for quick results
echo "=========================================="
echo "Quick Test: Cache capacity = 1000 entries"
echo "=========================================="
./cache_benchmark --capacity 1000 --url "$TRACE_URL"

echo ""
echo ""
echo "=========================================="
echo "Main Test: Cache capacity = $CACHE_CAPACITY entries"
echo "=========================================="

# Run individual algorithm tests
echo ""
echo "Testing each algorithm individually:"
echo "-----------------------------------"

echo ""
echo "1/4 - Testing LRU Algorithm:"
./cache_benchmark --capacity $CACHE_CAPACITY --algorithm LRU --url "$TRACE_URL"

echo ""
echo "2/4 - Testing FIFO Algorithm:"
./cache_benchmark --capacity $CACHE_CAPACITY --algorithm FIFO --url "$TRACE_URL"

echo ""
echo "3/4 - Testing SIEVE Algorithm:"
./cache_benchmark --capacity $CACHE_CAPACITY --algorithm SIEVE --url "$TRACE_URL"

echo ""
echo "4/4 - Testing CLOCK Algorithm:"
./cache_benchmark --capacity $CACHE_CAPACITY --algorithm CLOCK --url "$TRACE_URL"

echo ""
echo "=========================================="
echo "Final Comparison - All Algorithms"
echo "=========================================="
./cache_benchmark --capacity $CACHE_CAPACITY --url "$TRACE_URL"

echo ""
echo "Benchmark completed!"
echo ""
echo "Key Findings:"
echo "============"
echo "- LRU (Least Recently Used): Classic algorithm, tracks access order"
echo "- FIFO (First In, First Out): Simple queue-based eviction"
echo "- SIEVE: State-of-the-art algorithm designed for modern workloads"
echo "- CLOCK: Approximation of LRU with lower overhead"
echo ""
echo "Note: The results show hit ratios for different cache eviction policies"
echo "      when processing real-world Wikipedia cache access patterns."