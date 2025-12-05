CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -pthread
LDFLAGS = -pthread

# Source files for original server
SOURCES = main.cpp server.cpp cache.cpp protocol.cpp
OBJECTS = $(SOURCES:.cpp=.o)
TARGET = cache_server

# Source files for benchmark
BENCHMARK_SOURCES = cache_benchmark.cpp benchmark_server.cpp benchmark_cache.cpp trace_processor.cpp
BENCHMARK_OBJECTS = $(BENCHMARK_SOURCES:.cpp=.o)
BENCHMARK_TARGET = cache_benchmark

# Default target - build both
all: $(TARGET) $(BENCHMARK_TARGET)

# Build the main executable
$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

# Build the benchmark executable
$(BENCHMARK_TARGET): $(BENCHMARK_OBJECTS)
	$(CXX) $(BENCHMARK_OBJECTS) -o $(BENCHMARK_TARGET) $(LDFLAGS)

# Build object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean build files
clean:
	rm -f $(OBJECTS) $(BENCHMARK_OBJECTS) $(TARGET) $(BENCHMARK_TARGET)

# Install (optional)
install: $(TARGET) $(BENCHMARK_TARGET)
	cp $(TARGET) /usr/local/bin/
	cp $(BENCHMARK_TARGET) /usr/local/bin/

# Download test trace (for quick testing)
download-trace:
	@echo "Downloading Wikimedia cache trace (this may take a while)..."
	wget -O cache-t-00.gz "https://analytics.wikimedia.org/published/datasets/caching/2019/text/cache-t-00.gz"
	@echo "Trace downloaded successfully!"

# Run quick benchmark test
test-benchmark: $(BENCHMARK_TARGET)
	@echo "Running quick benchmark test..."
	./run_benchmark_test.sh

# Run full Wikimedia benchmark
benchmark-wikimedia: $(BENCHMARK_TARGET)
	@echo "Running full Wikimedia benchmark..."
	./run_wikimedia_benchmark.sh

# Debug build
debug: CXXFLAGS += -g -DDEBUG
debug: $(TARGET) $(BENCHMARK_TARGET)

# Test build with address sanitizer
test: CXXFLAGS += -g -fsanitize=address -fsanitize=undefined
test: LDFLAGS += -fsanitize=address -fsanitize=undefined
test: $(TARGET) $(BENCHMARK_TARGET)

# Dependencies
main.o: main.cpp server.h
server.o: server.cpp server.h cache.h protocol.h
cache.o: cache.cpp cache.h
protocol.o: protocol.cpp protocol.h

# Benchmark dependencies
cache_benchmark.o: cache_benchmark.cpp benchmark_server.h benchmark_cache.h trace_processor.h
benchmark_server.o: benchmark_server.cpp benchmark_server.h benchmark_cache.h trace_processor.h
benchmark_cache.o: benchmark_cache.cpp benchmark_cache.h
trace_processor.o: trace_processor.cpp trace_processor.h

.PHONY: all clean install debug test download-trace test-benchmark benchmark-wikimedia