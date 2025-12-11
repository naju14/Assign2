# Buffer Manager Assignment

## Overview

This project implements a buffer pool manager for a database system. The buffer manager handles page caching, replacement strategies, and I/O operations to efficiently manage memory and disk access.

## Files

### Implementation Files
- **buffer_mgr.c** - Main buffer manager implementation (YOUR SUBMISSION)
- **buffer_mgr.h** - Buffer manager interface definitions
- **buffer_mgr_stat.c** - Statistics and debugging functions
- **buffer_mgr_stat.h** - Statistics interface
- **dberror.c** - Error handling implementation
- **dberror.h** - Error codes and macros
- **dt.h** - Data type definitions (bool)

### Test Files
- **test_assign2_1.c** - Tests for FIFO and LRU replacement strategies
- **test_assign2_2.c** - Tests for LRU-K replacement strategy and error cases
- **test_helper.h** - Test helper macros and utilities

### Header Files (Provided)
- **storage_mgr.h** - Storage manager interface (implementation should be provided separately)

## Features Implemented

### Core Functions
- `initBufferPool()` - Initialize buffer pool with specified number of pages and replacement strategy
- `shutdownBufferPool()` - Cleanup buffer pool and flush all dirty pages
- `forceFlushPool()` - Write all dirty pages to disk
- `pinPage()` - Load a page into the buffer pool
- `unpinPage()` - Release a page (decrement fix count)
- `markDirty()` - Mark a page as modified
- `forcePage()` - Force write a specific page to disk

### Replacement Strategies
- **FIFO (First-In-First-Out)** - Evicts pages based on original load time
- **LRU (Least Recently Used)** - Evicts least recently accessed pages
- **LRU-K** - Evicts based on K-th most recent access (K=2)
- **CLOCK** - Basic clock replacement algorithm
- **LFU (Least Frequently Used)** - Evicts least frequently accessed pages

### Statistics Functions
- `getFrameContents()` - Get page numbers stored in each frame
- `getDirtyFlags()` - Get dirty flags for each frame
- `getFixCounts()` - Get fix counts (pin counts) for each frame
- `getNumReadIO()` - Get total number of read I/O operations
- `getNumWriteIO()` - Get total number of write I/O operations

## Building

### Prerequisites
- GCC compiler (or compatible C compiler)
- Storage manager implementation (storage_mgr.c or library)

### Compilation

Using Makefile:
```bash
make all          # Build all test executables
make test         # Build and run all tests
make clean        # Remove build artifacts
```

Manual compilation:
```bash
# Compile object files
gcc -c buffer_mgr.c -o buffer_mgr.o
gcc -c buffer_mgr_stat.c -o buffer_mgr_stat.o
gcc -c dberror.c -o dberror.o

# Link test executables (requires storage_mgr implementation)
gcc test_assign2_1.c buffer_mgr.o buffer_mgr_stat.o dberror.o -o test_assign2_1
gcc test_assign2_2.c buffer_mgr.o buffer_mgr_stat.o dberror.o -o test_assign2_2
```

## Running Tests

```bash
# Run test 1 (FIFO and LRU)
./test_assign2_1

# Run test 2 (LRU-K and error cases)
./test_assign2_2
```

## Implementation Details

### Data Structures
- **Frame**: Internal structure tracking page data, dirty flags, fix counts, and access history
- **BM_MgmtData**: Management data structure containing frame array, statistics, and strategy-specific data

### Key Design Decisions
1. **FIFO Strategy**: Tracks original load time and does not update on re-access, ensuring true FIFO behavior
2. **LRU Strategy**: Updates access time on every pin operation
3. **LRU-K Strategy**: Maintains access history for K accesses (K=2) and evicts based on K-th most recent access
4. **Error Handling**: Proper validation of parameters and buffer pool state
5. **Memory Management**: Proper allocation and cleanup of all resources

### Error Handling
The implementation handles various error cases:
- Invalid buffer pool or page handles
- Negative page numbers
- Attempting to pin pages when buffer is full of pinned pages
- Operations on uninitialized buffer pools
- Operations on pages not in the buffer pool

## Submission

**Submit only: `buffer_mgr.c`**

This is the main implementation file containing all the buffer manager functionality.

## Author

[Your Name]

## Date

Fall 2025

