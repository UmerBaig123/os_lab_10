# Cache Benchmark Suite

A comprehensive benchmarking system for comparing cache eviction algorithms using real-world traces from Wikimedia's content delivery network.

## Overview

This project implements a multi-threaded cache server with support for four different eviction algorithms:

- **LRU** (Least Recently Used)
- **FIFO** (First In, First Out)
- **SIEVE** (State-of-the-art eviction algorithm)
- **CLOCK** (Approximation of LRU)

The benchmark follows the algorithm:

1. GET (key)
2. If not hit, ADD (key)
3. Repeat until trace ends

## Architecture

- **Server**: 1 connection listener thread + 4 worker threads
- **Cache**: Thread-safe implementation with configurable eviction policies
- **Trace Processor**: Handles Wikimedia trace files (supports compressed .gz files)
- **Benchmark Engine**: Measures hit ratios and performance metrics

## Build Instructions

```bash
# Build the benchmark system
make cache_benchmark

# Build both original server and benchmark
make all

# Clean build files
make clean
```

## Usage

### Basic Usage

```bash
# Run all algorithms with default settings
./cache_benchmark

# Test specific algorithm with custom capacity
./cache_benchmark --capacity 5000 --algorithm LRU

# Use local trace file
./cache_benchmark --file my_trace.txt

# Use custom URL
./cache_benchmark --url "https://example.com/trace.gz"
```

### Command Line Options

- `-c, --capacity <size>`: Cache capacity in entries (default: 10000)
- `-f, --file <path>`: Local trace file path
- `-u, --url <url>`: Trace file URL (default: Wikimedia cache-t-00.gz)
- `-a, --algorithm <alg>`: Specific algorithm (LRU, FIFO, SIEVE, CLOCK)
- `-h, --help`: Show help message

### Quick Test Scripts

```bash
# Run quick test with simple trace
./run_benchmark_test.sh

# Run full Wikimedia benchmark (downloads real trace)
./run_wikimedia_benchmark.sh
```

## Cache Algorithms

### LRU (Least Recently Used)

- **Strategy**: Evicts the item that was accessed least recently
- **Use Case**: Good for temporal locality patterns
- **Complexity**: O(1) for get/put operations
- **Memory**: Maintains doubly-linked list of access order

### FIFO (First In, First Out)

- **Strategy**: Evicts the oldest inserted item regardless of access
- **Use Case**: Simple workloads, predictable behavior
- **Complexity**: O(1) for get/put operations
- **Memory**: Maintains simple queue structure

### SIEVE

- **Strategy**: Modern algorithm designed for web workloads
- **Use Case**: Optimized for one-hit-wonder patterns common in web caching
- **Complexity**: O(1) amortized operations
- **Memory**: Uses visited bits to track item popularity

### CLOCK

- **Strategy**: Approximates LRU using circular buffer with reference bits
- **Use Case**: Lower memory overhead than LRU
- **Complexity**: O(1) amortized operations
- **Memory**: Fixed-size circular buffer with reference bits

## Trace File Format

The benchmark supports traces in the format:

```
timestamp key size
timestamp key size
...
```

Or simplified format with just keys:

```
key1
key2
...
```

## Performance Metrics

The benchmark reports:

- **Hit Ratio**: Percentage of cache hits vs total requests
- **Request Rate**: Requests processed per second
- **Cache Size**: Current entries vs maximum capacity
- **Processing Time**: Total time to process trace

## Example Output

```
=== BENCHMARK SUMMARY ===
      Algorithm       Hit Ratio (%)
-----------------------------------
            LRU               23.45%
           FIFO               18.92%
          SIEVE               26.78%
          CLOCK               21.33%

Best performing algorithm: SIEVE (26.78%)
```

## Technical Details

### Thread Safety

- All cache implementations are thread-safe using mutex locks
- Lock-free atomic counters for statistics
- Worker thread pool processes requests concurrently

### Memory Management

- Smart pointers for automatic memory management
- Efficient data structures for each algorithm
- Configurable cache capacity to control memory usage

### Trace Processing

- Automatic download and extraction of compressed traces
- Support for local files and remote URLs
- Robust parsing of different trace formats

## Files Structure

```
cache_benchmark.cpp      # Main benchmark application
benchmark_server.h/cpp   # Multi-threaded benchmark server
benchmark_cache.h/cpp    # Cache implementations with different policies
trace_processor.h/cpp    # Trace file handling and parsing
run_benchmark_test.sh    # Quick testing script
run_wikimedia_benchmark.sh # Full Wikimedia trace benchmark
```

## Dependencies

- C++17 compiler (g++)
- pthread library
- wget or curl (for downloading traces)
- gzip (for extracting compressed traces)

## Research Background

This benchmark is based on real-world caching research and uses traces from:

- **Wikimedia CDN**: Real content delivery network traffic patterns
- **Academic Research**: Implementations of state-of-the-art algorithms like SIEVE

The SIEVE algorithm, in particular, was designed to address limitations of traditional LRU in modern web workloads where many objects are accessed only once ("one-hit wonders").

## Contributing

To add new eviction algorithms:

1. Add new `EvictionPolicy` enum value
2. Implement eviction logic in `BenchmarkCache`
3. Add policy name mapping in `get_policy_name()`
4. Update documentation

## License

This project is for educational and research purposes in operating systems and caching algorithms.
