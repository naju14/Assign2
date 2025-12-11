=============================================
Najira Getachew  CS525
Buffer Manager Implementation - Assignment 2
=============================================

Solution Overview:
------------------
This implementation provides a buffer pool manager for a database system. The buffer 
manager handles page caching, replacement strategies, and I/O operations to efficiently 
manage memory and disk access.

Files in this directory:
------------------------
Required files (provided):
- buffer_mgr.h              - Buffer manager interface definitions
- buffer_mgr_stat.c        - Statistics and debugging functions
- buffer_mgr_stat.h         - Statistics interface
- dberror.c                - Error handling implementation
- dberror.h                - Error codes and macros
- dt.h                     - Data type definitions (bool)
- storage_mgr.h            - Storage manager interface
- storage_mgr.c            - Storage manager implementation (from Assignment 1)
- test_assign2_1.c         - Tests for FIFO and LRU replacement strategies
- test_assign2_2.c         - Tests for LRU-K replacement strategy and error cases
- test_helper.h            - Test helper macros and utilities

Implementation files (submitted):
- buffer_mgr.c             - Main buffer manager implementation

Build files:
- Makefile                 - Build configuration for compiling and testing

Documentation:
- README.md                - Detailed documentation (markdown format)
- README.txt               - This file (text format)

Implementation Details:
-----------------------
1. Buffer Pool Management:
   - The buffer pool uses an array of Frame structures to manage pages
   - Each frame tracks: page number, data, dirty flag, fix count, and access history
   - Management data (BM_MgmtData) tracks statistics and strategy-specific information

2. Core Functions:
   - initBufferPool: Initializes buffer pool with specified number of pages and strategy
   - shutdownBufferPool: Cleans up and flushes all dirty pages
   - pinPage: Loads pages into buffer, handles replacement when pool is full
   - unpinPage: Decrements fix count to allow page eviction
   - markDirty: Marks pages as modified
   - forcePage: Forces write of specific page to disk
   - forceFlushPool: Writes all dirty pages to disk

3. Replacement Strategies:
   - FIFO (First-In-First-Out): Evicts based on original load time
   - LRU (Least Recently Used): Evicts least recently accessed pages
   - LRU-K: Evicts based on K-th most recent access (K=2)
   - CLOCK: Basic clock replacement algorithm
   - LFU (Least Frequently Used): Evicts least frequently accessed pages

4. Statistics Functions:
   - getFrameContents: Returns page numbers in each frame
   - getDirtyFlags: Returns dirty flags for each frame
   - getFixCounts: Returns fix counts (pin counts) for each frame
   - getNumReadIO: Returns total number of read I/O operations
   - getNumWriteIO: Returns total number of write I/O operations

Key Design Decisions:
---------------------
- FIFO tracks original load time and does NOT update on re-access (true FIFO behavior)
- LRU updates access time on every pin operation
- LRU-K maintains access history for K accesses and evicts based on K-th most recent
- Proper error handling for edge cases (full buffer, invalid pages, etc.)
- Statistics tracking for I/O operations

Build Instructions:
------------------
Using Makefile:
  make all          - Build all test executables
  make test         - Build and run all tests
  make clean        - Remove build artifacts

Manual compilation:
  gcc -c buffer_mgr.c -o buffer_mgr.o
  gcc -c buffer_mgr_stat.c -o buffer_mgr_stat.o
  gcc -c dberror.c -o dberror.o
  gcc -c storage_mgr.c -o storage_mgr.o
  gcc test_assign2_1.c buffer_mgr.o buffer_mgr_stat.o dberror.o storage_mgr.o -o test_assign2_1
  gcc test_assign2_2.c buffer_mgr.o buffer_mgr_stat.o dberror.o storage_mgr.o -o test_assign2_2

Testing:
--------
Run the test executables:
  ./test_assign2_1    - Tests FIFO and LRU strategies
  ./test_assign2_2    - Tests LRU-K strategy and error cases

Submission:
-----------
Submit only: buffer_mgr.c
This file contains the complete buffer manager implementation.
