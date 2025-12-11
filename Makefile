CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g

# Assignment 1 files (from previous assignment)
STORAGE_MGR_SRC = storage_mgr.c
DBERROR_SRC = dberror.c

# Assignment 2 files
BUFFER_MGR_SRC = buffer_mgr.c
BUFFER_MGR_STAT_SRC = buffer_mgr_stat.c

# Test files
TEST1_SRC = test_assign2_1.c
TEST2_SRC = test_assign2_2.c

# Object files
STORAGE_MGR_OBJ = $(STORAGE_MGR_SRC:.c=.o)
DBERROR_OBJ = $(DBERROR_SRC:.c=.o)
BUFFER_MGR_OBJ = $(BUFFER_MGR_SRC:.c=.o)
BUFFER_MGR_STAT_OBJ = $(BUFFER_MGR_STAT_SRC:.c=.o)

# Executables
TEST1_TARGET = test_assign2_1
TEST2_TARGET = test_assign2_2

# Common object files needed by both tests
COMMON_OBJS = $(STORAGE_MGR_OBJ) $(DBERROR_OBJ) $(BUFFER_MGR_OBJ) $(BUFFER_MGR_STAT_OBJ)

# Default target - build both test executables
all: $(TEST1_TARGET) $(TEST2_TARGET)

# Build test 1
$(TEST1_TARGET): $(TEST1_SRC) $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $(TEST1_TARGET) $(TEST1_SRC) $(COMMON_OBJS)

# Build test 2
$(TEST2_TARGET): $(TEST2_SRC) $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $(TEST2_TARGET) $(TEST2_SRC) $(COMMON_OBJS)

# Compile object files with proper dependencies
$(STORAGE_MGR_OBJ): $(STORAGE_MGR_SRC) storage_mgr.h dberror.h
	$(CC) $(CFLAGS) -c $(STORAGE_MGR_SRC) -o $(STORAGE_MGR_OBJ)

$(DBERROR_OBJ): $(DBERROR_SRC) dberror.h
	$(CC) $(CFLAGS) -c $(DBERROR_SRC) -o $(DBERROR_OBJ)

$(BUFFER_MGR_OBJ): $(BUFFER_MGR_SRC) buffer_mgr.h storage_mgr.h dberror.h dt.h
	$(CC) $(CFLAGS) -c $(BUFFER_MGR_SRC) -o $(BUFFER_MGR_OBJ)

$(BUFFER_MGR_STAT_OBJ): $(BUFFER_MGR_STAT_SRC) buffer_mgr_stat.h buffer_mgr.h
	$(CC) $(CFLAGS) -c $(BUFFER_MGR_STAT_SRC) -o $(BUFFER_MGR_STAT_OBJ)

# Run tests
test: $(TEST1_TARGET) $(TEST2_TARGET)
	@echo "Running test_assign2_1..."
	./$(TEST1_TARGET)
	@echo ""
	@echo "Running test_assign2_2..."
	./$(TEST2_TARGET)

# Clean build artifacts
clean:
	rm -f $(COMMON_OBJS) $(TEST1_TARGET) $(TEST2_TARGET)
	rm -f *.o
	rm -f testbuffer.bin test_pagefile.bin

# Clean everything including test files
distclean: clean
	rm -f *.bin

.PHONY: all test clean distclean
