# Paged Key-Value Cache Server

A high-performance TCP server implementing a paged memory management system for key-value storage in C++. This project implements Lab 9 requirements with a complete cache system that divides 2 GB of memory into 40 KB pages, supporting multiple concurrent clients with client isolation and LRU eviction.

## Project Description

This cache server addresses the challenge of efficient memory management in large-scale key-value storage systems. The server implements:

- **Paged Memory Architecture**: Divides the 2 GB cache into fixed 40 KB pages for efficient allocation
- **Contiguous Page Allocation**: Ensures each client's data is stored in sequential pages
- **Client Isolation**: Automatic client identification and access control
- **High-Performance Networking**: Uses epoll() for scalable I/O multiplexing
- **LRU Eviction Policy**: Intelligent memory management when cache capacity is exceeded
- **Custom Communication Protocol**: Structured text-based protocol for client-server communication

## Features

- **2 GB Cache**: Divided into 40 KB pages (~52,428 total pages)
- **Paged Memory Management**: Contiguous page allocation for each client's data
- **TCP Server**: Uses `epoll()` for efficient I/O multiplexing
- **Custom Protocol**: Text-based communication protocol
- **LRU Eviction**: Least Recently Used eviction when cache is full
- **Client Isolation**: Each client can only access their own data
- **Multi-client Support**: Handles multiple concurrent connections

## Protocol

All commands follow this format:

```
Method:<operation>\r\n
Key:<key>\r\n
[Value:<value>\r\n]
\r\n
```

### Supported Operations

| Operation | Format                                                 | Response                        |
| --------- | ------------------------------------------------------ | ------------------------------- |
| ADD       | `Method:ADD\r\nKey:mykey\r\nValue:myvalue\r\n\r\n`     | `OK: Key added successfully.`   |
| UPDATE    | `Method:UPDATE\r\nKey:mykey\r\nValue:newvalue\r\n\r\n` | `OK: Key updated successfully.` |
| GET       | `Method:GET\r\nKey:mykey\r\n\r\n`                      | `OK: Value=myvalue`             |
| DELETE    | `Method:DELETE\r\nKey:mykey\r\n\r\n`                   | `OK: Key deleted successfully.` |

### Error Responses

- `ERROR: Key already exists.` - Duplicate key on ADD
- `ERROR: Key not found.` - Key doesn't exist on UPDATE/GET/DELETE
- `ERROR: Not enough contiguous space.` - Cannot find contiguous pages
- `ERROR: Access denied to key.` - Client trying to access another client's key
- `ERROR: Invalid command format.` - Malformed request

## Building

### Prerequisites

- C++17 compatible compiler (GCC 7+ or Clang 5+)
- Linux system with epoll support
- Make utility

### Compile

```bash
make
```

### Debug Build

```bash
make debug
```

### Test Build (with sanitizers)

```bash
make test
```

## Running

### Start the Server

```bash
./cache_server [port]
```

Default port is 8080.

### Test with Python Client

```bash
# Run automated tests
python3 test_client.py

# Interactive mode
python3 test_client.py interactive
```

### Manual Testing with telnet

```bash
telnet localhost 8080
```

Then send commands:

```
Method:ADD
Key:test
Value:hello world

Method:GET
Key:test

```

## Architecture

### Components

1. **PagedCache** (`cache.cpp`): Core memory management and storage

   - Bitmap-based page allocation
   - LRU eviction policy
   - Thread-safe operations

2. **TCPServer** (`server.cpp`): Network communication

   - Epoll-based event handling
   - Non-blocking I/O
   - Connection management

3. **ProtocolParser** (`protocol.cpp`): Message parsing
   - Text protocol parsing
   - Response formatting
   - Command validation

### Key Design Decisions

- **Contiguous Allocation**: Each key's data must fit in contiguous pages
- **Client Isolation**: Automatic client ID assignment based on socket descriptor
- **LRU Eviction**: Only evicts data from other clients, never from requesting client
- **Non-blocking I/O**: Uses edge-triggered epoll for scalability

## Performance Characteristics

- **Memory**: 2 GB total cache capacity
- **Page Size**: 40 KB per page (configurable)
- **Concurrency**: Supports many concurrent clients
- **Allocation**: O(n) worst case for contiguous page search
- **LRU Operations**: O(1) for access time updates

## Configuration

Constants in `cache.h`:

```cpp
const size_t TOTAL_CACHE_SIZE = 2ULL * 1024 * 1024 * 1024;  // 2 GB
const size_t PAGE_SIZE = 40 * 1024;  // 40 KB
```

## Testing

### Test Suite Overview

The project includes a comprehensive test suite (`test_client.py`) that validates all server functionality through automated and interactive testing modes.

### Automated Test Cases

#### 1. Basic CRUD Operations Test (`run_basic_tests()`)

**Purpose**: Validates core functionality of all operations
**Test Cases**:

- **TC-001**: ADD operation with new key
  - Input: `ADD test_key "Hello World"`
  - Expected: `OK: Key added successfully.`
- **TC-002**: GET operation for existing key
  - Input: `GET test_key`
  - Expected: `OK: Value=Hello World`
- **TC-003**: UPDATE operation for existing key
  - Input: `UPDATE test_key "Updated Value"`
  - Expected: `OK: Key updated successfully.`
- **TC-004**: GET operation after UPDATE
  - Input: `GET test_key`
  - Expected: `OK: Value=Updated Value`
- **TC-005**: DELETE operation
  - Input: `DELETE test_key`
  - Expected: `OK: Key deleted successfully.`
- **TC-006**: GET operation after DELETE
  - Input: `GET test_key`
  - Expected: `ERROR: Key not found.`
- **TC-007**: Duplicate ADD operation
  - Setup: ADD dup_key with value1
  - Input: `ADD dup_key value2`
  - Expected: `ERROR: Key already exists.`

#### 2. Large Value Handling Test (`test_large_values()`)

**Purpose**: Tests page allocation with varying data sizes
**Test Cases**:

- **TC-008**: Small value (100 bytes)
  - Tests single page allocation efficiency
- **TC-009**: Medium value (1 KB)
  - Tests intra-page storage optimization
- **TC-010**: Large value (10 KB)
  - Tests partial page utilization
- **TC-011**: Page-sized value (40 KB)
  - Tests exact page boundary handling
- **TC-012**: Multi-page value (100 KB)
  - Tests contiguous page allocation for 3+ pages
  - Validates page boundary calculations

**Validation**: Each test verifies:

- Successful storage (`OK: Key added successfully.`)
- Correct retrieval of exact value
- Proper cleanup after deletion

#### 3. Concurrent Client Test (`test_concurrent_clients()`)

**Purpose**: Validates multi-client support and client isolation
**Test Configuration**:

- 5 concurrent clients
- 10 operations per client
- Operations: ADD → GET → UPDATE → DELETE sequence

**Test Cases**:

- **TC-013**: Concurrent ADD operations
  - Validates no key conflicts between clients
- **TC-014**: Client isolation verification
  - Client A cannot access Client B's keys
- **TC-015**: Concurrent memory allocation
  - Tests page allocation under concurrent load
- **TC-016**: LRU eviction under load
  - Validates eviction policy with multiple clients

### Interactive Testing

#### Manual Test Mode

```bash
python3 test_client.py interactive
```

**Available Commands**:

- `add <key> <value>` - Test ADD operation
- `get <key>` - Test GET operation
- `update <key> <value>` - Test UPDATE operation
- `delete <key>` - Test DELETE operation
- `quit` - Exit interactive mode

### Error Condition Testing

#### Protocol Error Tests

- **TC-017**: Invalid method name
  - Input: `Method:INVALID\r\nKey:test\r\n\r\n`
  - Expected: `ERROR: Invalid command format.`
- **TC-018**: Missing key field
  - Input: `Method:ADD\r\nValue:test\r\n\r\n`
  - Expected: `ERROR: Invalid command format.`
- **TC-019**: Missing value for ADD/UPDATE
  - Input: `Method:ADD\r\nKey:test\r\n\r\n`
  - Expected: `ERROR: Invalid command format.`

#### Memory Management Tests

- **TC-020**: Cache exhaustion
  - Add large values until memory full
  - Expected: `ERROR: Not enough contiguous space.`
- **TC-021**: LRU eviction trigger
  - Fill cache, add new item
  - Verify oldest item evicted
- **TC-022**: Client isolation during eviction
  - Verify client's own data never evicted

### Network and Connection Testing

#### Connection Handling Tests

- **TC-023**: Multiple simultaneous connections
  - Connect 10+ clients simultaneously
  - Verify all connections accepted
- **TC-024**: Client disconnection handling
  - Abrupt client disconnection
  - Verify server remains stable
- **TC-025**: Large message handling
  - Send messages up to buffer limits
  - Verify proper message parsing

### How to Run Tests

#### 1. Automated Test Suite

```bash
# Compile the server
make

# Start server in background
./cache_server 8080 &
SERVER_PID=$!

# Run all automated tests
python3 test_client.py

# Stop server
kill $SERVER_PID
```

#### 2. Quick Demo

```bash
./demo.sh
```

#### 3. Comprehensive Test Suite

```bash
# Run all tests
./run_tests.sh

# Run specific test categories
./run_tests.sh basic      # Basic functionality only
./run_tests.sh load       # Load testing only
./run_tests.sh memory     # Memory testing only
./run_tests.sh protocol   # Protocol testing only
```

#### 4. Individual Test Scripts

```bash
# Load testing
python3 load_test.py                    # Default: 5 workers, 1000 ops each
python3 load_test.py 10 500             # Custom: 10 workers, 500 ops each
python3 load_test.py stress             # Stress test with increasing load

# Memory testing
python3 memory_test.py                  # Run all memory tests
python3 memory_test.py sizes            # Test different value sizes
python3 memory_test.py exhaustion       # Test cache exhaustion
python3 memory_test.py fragmentation    # Test memory fragmentation
```

#### 5. Individual Test Categories

```bash
# Start server
./cache_server &

# Test basic operations only
python3 -c "
from test_client import run_basic_tests
run_basic_tests()
"

# Test large values only
python3 -c "
from test_client import test_large_values
test_large_values()
"

# Test concurrency only
python3 -c "
from test_client import test_concurrent_clients
test_concurrent_clients()
"
```

#### 4. Manual Protocol Testing with telnet

```bash
# Connect to server
telnet localhost 8080

# Test ADD
Method:ADD
Key:manual_test
Value:Hello from telnet

# Test GET
Method:GET
Key:manual_test

# Test UPDATE
Method:UPDATE
Key:manual_test
Value:Updated from telnet

# Test DELETE
Method:DELETE
Key:manual_test
```

### Performance Testing

#### Throughput Test

```bash
# Generate load test script
cat > load_test.py << 'EOF'
import threading
import time
from test_client import CacheClient

def load_worker(worker_id, operations=1000):
    client = CacheClient()
    client.connect()

    start = time.time()
    for i in range(operations):
        key = f"load_key_{worker_id}_{i}"
        value = f"load_value_{i}"
        client.add(key, value)
        client.get(key)
        client.delete(key)

    elapsed = time.time() - start
    print(f"Worker {worker_id}: {operations} ops in {elapsed:.2f}s = {operations/elapsed:.1f} ops/sec")
    client.disconnect()

# Run 5 workers with 1000 operations each
workers = []
for i in range(5):
    worker = threading.Thread(target=load_worker, args=(i,))
    workers.append(worker)
    worker.start()

for worker in workers:
    worker.join()
EOF

# Run load test
python3 load_test.py
```

#### Memory Pressure Test

```bash
# Test cache limits
cat > memory_test.py << 'EOF'
from test_client import CacheClient

client = CacheClient()
client.connect()

# Add progressively larger values
for size in [1024, 10240, 40960, 100000]:
    large_value = "x" * size
    result = client.add(f"size_test_{size}", large_value)
    print(f"Size {size}: {result}")

client.disconnect()
EOF

python3 memory_test.py
```

### Test Result Validation

#### Success Criteria

- All automated tests pass without errors
- Server handles concurrent clients without crashes
- Memory allocation/deallocation works correctly
- Client isolation maintained under all conditions
- LRU eviction preserves requesting client's data
- Protocol parsing handles malformed messages gracefully

#### Expected Test Output

```
=== Running Basic Tests ===
✓ ADD operation: OK: Key added successfully.
✓ GET operation: OK: Value=Hello World
✓ UPDATE operation: OK: Key updated successfully.
✓ DELETE operation: OK: Key deleted successfully.
✓ Error handling: ERROR: Key not found.

=== Testing Large Values ===
✓ 100 bytes: OK: Key added successfully.
✓ 1024 bytes: OK: Key added successfully.
✓ 40960 bytes: OK: Key added successfully.

=== Testing Concurrent Clients ===
✓ Client 0 completed 10 operations
✓ Client 1 completed 10 operations
✓ All clients completed successfully

🎉 All tests completed successfully!
```

## Error Handling

- Network errors are handled gracefully
- Memory allocation failures return appropriate error codes
- Client disconnections are detected and cleaned up
- Invalid protocol messages are rejected

## Limitations

- Maximum value size limited by available contiguous pages
- No persistence - all data lost on server restart
- Text-based protocol (not binary optimized)
- Single-threaded request processing per client

## Future Enhancements

- Binary protocol for better performance
- Compression for large values
- Persistence layer
- Configurable eviction policies
- Memory-mapped file backing

## Files Overview

### Core Implementation

- `cache.h/cpp` - Paged memory management and LRU eviction
- `server.h/cpp` - TCP server with epoll-based I/O multiplexing
- `protocol.h/cpp` - Text protocol parsing and response formatting
- `main.cpp` - Server entry point with signal handling
- `Makefile` - Build system with debug/test targets

### Testing Suite

- `test_client.py` - Basic CRUD and concurrent testing
- `load_test.py` - Performance and throughput testing
- `memory_test.py` - Cache exhaustion and fragmentation testing
- `run_tests.sh` - Comprehensive automated test runner
- `demo.sh` - Quick demonstration script

### Documentation

- `README.md` - Complete project documentation and test cases
