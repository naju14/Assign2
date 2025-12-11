# Buffer Manager Assignment 2

## Overview

Implementation of a buffer pool manager for database systems with multiple page replacement strategies.

## Features

### Core Functions
- `initBufferPool()` - Initialize buffer pool with replacement strategy
- `shutdownBufferPool()` - Cleanup and flush all dirty pages
- `pinPage()` / `unpinPage()` - Page pinning/unpinning
- `markDirty()` / `forcePage()` - Dirty page management
- `forceFlushPool()` - Flush all dirty pages to disk

### Replacement Strategies
- **FIFO** - First-In-First-Out (tracks original load time)
- **LRU** - Least Recently Used
- **LRU-K** - K-th most recent access (K=2)
- **CLOCK** - Clock replacement algorithm
- **LFU** - Least Frequently Used

### Statistics
- `getFrameContents()`, `getDirtyFlags()`, `getFixCounts()`
- `getNumReadIO()`, `getNumWriteIO()`

## Building

```bash
make all      # Build test executables
make test     # Build and run tests
make clean    # Remove build artifacts
```

## Implementation

- **Frame structure**: Tracks page data, dirty flags, fix counts, and access history
- **FIFO**: Load time never updates on re-access (true FIFO)
- **LRU**: Access time updated on every pin
- **LRU-K**: Maintains history of last K accesses
- Proper error handling and memory management

## Submission

Main file: `buffer_mgr.c`
