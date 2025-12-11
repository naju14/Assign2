/************************************************************
 * Najira Getachew
 * CS525 - Advanced Database Organization
 * Storage Manager Implementation - Assignment 1
 ************************************************************/
#include "storage_mgr.h"
#include "dberror.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/************************************************************
 * INITIALIZATION
 ************************************************************/

/* Initialize the storage manager */
void initStorageManager(void) {
    // Currently no initialization needed
    // Could be used for future enhancements
}

/************************************************************
 * FILE MANIPULATION FUNCTIONS
 ************************************************************/

/* Create a new page file with one page filled with '\0' bytes */
RC createPageFile(char *fileName) {
    FILE *fp;
    char *emptyPage;
    int totalPages = 1;
    
    if (fileName == NULL) {
        THROW(RC_FILE_NOT_FOUND, "File name is NULL");
    }
    
    // Open file in write-binary mode
    fp = fopen(fileName, "wb");
    if (fp == NULL) {
        THROW(RC_WRITE_FAILED, "Could not create page file");
    }
    
    // Write metadata: total number of pages at the beginning
    if (fwrite(&totalPages, sizeof(int), 1, fp) != 1) {
        fclose(fp);
        remove(fileName);
        THROW(RC_WRITE_FAILED, "Could not write metadata");
    }
    
    // Allocate memory for one empty page
    emptyPage = (char *)calloc(PAGE_SIZE, sizeof(char));
    if (emptyPage == NULL) {
        fclose(fp);
        remove(fileName);
        THROW(RC_WRITE_FAILED, "Memory allocation failed");
    }
    
    // Write empty page to file
    size_t written = fwrite(emptyPage, sizeof(char), PAGE_SIZE, fp);
    
    // Clean up
    free(emptyPage);
    fclose(fp);
    
    if (written < PAGE_SIZE) {
        remove(fileName);
        THROW(RC_WRITE_FAILED, "Could not write initial page");
    }
    
    return RC_OK;
}

/* Open an existing page file */
RC openPageFile(char *fileName, SM_FileHandle *fHandle) {
    FILE *fp;
    long fileSize;
    int totalPages;
    
    if (fileName == NULL) {
        THROW(RC_FILE_NOT_FOUND, "File name is NULL");
    }
    
    if (fHandle == NULL) {
        THROW(RC_FILE_HANDLE_NOT_INIT, "File handle is NULL");
    }
    
    // Try to open the file in read-binary+ mode
    fp = fopen(fileName, "rb+");
    if (fp == NULL) {
        THROW(RC_FILE_NOT_FOUND, "Page file not found");
    }
    
    // Read metadata: total number of pages
    if (fread(&totalPages, sizeof(int), 1, fp) != 1) {
        fclose(fp);
        THROW(RC_READ_NON_EXISTING_PAGE, "Could not read metadata");
    }
    
    // Validate metadata and verify file size
    fseek(fp, 0, SEEK_END);
    fileSize = ftell(fp);
    
    // Recalculate if file size doesn't match (handles corrupted metadata)
    long expectedSize = sizeof(int) + (long)totalPages * PAGE_SIZE;
    if (fileSize < expectedSize) {
        fclose(fp);
        THROW(RC_READ_NON_EXISTING_PAGE, "File size is smaller than expected");
    }
    if (fileSize > expectedSize) {
        // Recalculate from actual file size
        totalPages = (int)((fileSize - sizeof(int)) / PAGE_SIZE);
        // Update metadata
        fseek(fp, 0, SEEK_SET);
        fwrite(&totalPages, sizeof(int), 1, fp);
    }
    
    // Allocate and copy file name
    fHandle->fileName = (char *)malloc(strlen(fileName) + 1);
    if (fHandle->fileName == NULL) {
        fclose(fp);
        THROW(RC_FILE_HANDLE_NOT_INIT, "Memory allocation failed");
    }
    strcpy(fHandle->fileName, fileName);
    
    // Initialize file handle
    fHandle->totalNumPages = totalPages;
    fHandle->curPagePos = 0;
    fHandle->mgmtInfo = fp;  // Store FILE pointer for later use
    
    return RC_OK;
}

/* Close an open page file */
RC closePageFile(SM_FileHandle *fHandle) {
    if (fHandle == NULL || fHandle->mgmtInfo == NULL) {
        THROW(RC_FILE_HANDLE_NOT_INIT, "File handle not initialized");
    }
    
    FILE *fp = (FILE *)fHandle->mgmtInfo;
    
    fflush(fp);
    if (fclose(fp) != 0) {
        THROW(RC_WRITE_FAILED, "Could not close file");
    }
    
    // Free allocated memory
    if (fHandle->fileName != NULL) {
        free(fHandle->fileName);
        fHandle->fileName = NULL;
    }
    
    fHandle->mgmtInfo = NULL;
    fHandle->totalNumPages = 0;
    fHandle->curPagePos = 0;
    
    return RC_OK;
}

/* Destroy (delete) a page file */
RC destroyPageFile(char *fileName) {
    if (fileName == NULL) {
        THROW(RC_FILE_NOT_FOUND, "File name is NULL");
    }
    
    if (remove(fileName) != 0) {
        THROW(RC_FILE_NOT_FOUND, "Could not destroy page file");
    }
    
    return RC_OK;
}

/************************************************************
 * READING BLOCKS FROM DISK
 ************************************************************/

/* Read a specific block from disk */
RC readBlock(int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage) {
    FILE *fp;
    size_t bytesRead;
    
    // Validate file handle
    if (fHandle == NULL || fHandle->mgmtInfo == NULL) {
        THROW(RC_FILE_HANDLE_NOT_INIT, "File handle not initialized");
    }
    
    // Validate memory page buffer
    if (memPage == NULL) {
        THROW(RC_READ_NON_EXISTING_PAGE, "Memory page buffer is NULL");
    }
    
    // Check if page number is valid
    if (pageNum < 0 || pageNum >= fHandle->totalNumPages) {
        THROW(RC_READ_NON_EXISTING_PAGE, "Page number out of bounds");
    }
    
    fp = (FILE *)fHandle->mgmtInfo;
    
    // Seek to the page position (account for metadata at the beginning)
    long offset = sizeof(int) + (long)pageNum * PAGE_SIZE;
    if (fseek(fp, offset, SEEK_SET) != 0) {
        THROW(RC_READ_NON_EXISTING_PAGE, "Could not seek to page");
    }
    
    // Read the page into memory
    bytesRead = fread(memPage, sizeof(char), PAGE_SIZE, fp);
    
    if (bytesRead < PAGE_SIZE) {
        THROW(RC_READ_NON_EXISTING_PAGE, "Could not read complete page");
    }
    
    // Update current page position
    fHandle->curPagePos = pageNum;
    
    return RC_OK;
}

/* Get the current block position */
int getBlockPos(SM_FileHandle *fHandle) {
    if (fHandle == NULL) {
        return -1;
    }
    return fHandle->curPagePos;
}

/* Read the first block */
RC readFirstBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    return readBlock(0, fHandle, memPage);
}

/* Read the previous block */
RC readPreviousBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    if (fHandle == NULL) {
        THROW(RC_FILE_HANDLE_NOT_INIT, "File handle not initialized");
    }
    
    int prevPage = fHandle->curPagePos - 1;
    
    if (prevPage < 0) {
        THROW(RC_READ_NON_EXISTING_PAGE, "No previous page exists");
    }
    
    return readBlock(prevPage, fHandle, memPage);
}

/* Read the current block */
RC readCurrentBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    if (fHandle == NULL) {
        THROW(RC_FILE_HANDLE_NOT_INIT, "File handle not initialized");
    }
    
    return readBlock(fHandle->curPagePos, fHandle, memPage);
}

/* Read the next block */
RC readNextBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    if (fHandle == NULL) {
        THROW(RC_FILE_HANDLE_NOT_INIT, "File handle not initialized");
    }
    
    int nextPage = fHandle->curPagePos + 1;
    
    if (nextPage >= fHandle->totalNumPages) {
        THROW(RC_READ_NON_EXISTING_PAGE, "No next page exists");
    }
    
    return readBlock(nextPage, fHandle, memPage);
}

/* Read the last block */
RC readLastBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    if (fHandle == NULL) {
        THROW(RC_FILE_HANDLE_NOT_INIT, "File handle not initialized");
    }
    
    int lastPage = fHandle->totalNumPages - 1;
    
    if (lastPage < 0) {
        THROW(RC_READ_NON_EXISTING_PAGE, "No pages in file");
    }
    
    return readBlock(lastPage, fHandle, memPage);
}

/************************************************************
 * WRITING BLOCKS TO DISK
 ************************************************************/

/* Write a block at a specific position */
RC writeBlock(int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage) {
    FILE *fp;
    size_t bytesWritten;
    
    // Validate file handle
    if (fHandle == NULL || fHandle->mgmtInfo == NULL) {
        THROW(RC_FILE_HANDLE_NOT_INIT, "File handle not initialized");
    }
    
    // Validate memory page buffer
    if (memPage == NULL) {
        THROW(RC_WRITE_FAILED, "Memory page buffer is NULL");
    }
    
    // Check if page number is valid
    if (pageNum < 0 || pageNum >= fHandle->totalNumPages) {
        THROW(RC_WRITE_FAILED, "Page number out of bounds");
    }
    
    fp = (FILE *)fHandle->mgmtInfo;
    
    // Seek to the page position (account for metadata at the beginning)
    long offset = sizeof(int) + (long)pageNum * PAGE_SIZE;
    if (fseek(fp, offset, SEEK_SET) != 0) {
        THROW(RC_WRITE_FAILED, "Could not seek to page");
    }
    
    // Write the page to disk
    bytesWritten = fwrite(memPage, sizeof(char), PAGE_SIZE, fp);
    
    if (bytesWritten < PAGE_SIZE) {
        THROW(RC_WRITE_FAILED, "Could not write complete page");
    }
    
    // Flush to ensure data is written
    fflush(fp);
    
    // Update current page position
    fHandle->curPagePos = pageNum;
    
    return RC_OK;
}

/* Write the current block */
RC writeCurrentBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    if (fHandle == NULL) {
        THROW(RC_FILE_HANDLE_NOT_INIT, "File handle not initialized");
    }
    
    return writeBlock(fHandle->curPagePos, fHandle, memPage);
}

/* Append an empty block to the file */
RC appendEmptyBlock(SM_FileHandle *fHandle) {
    FILE *fp;
    char *emptyPage;
    size_t bytesWritten;
    int newTotalPages;
    long currentPos;
    
    // Validate file handle
    if (fHandle == NULL || fHandle->mgmtInfo == NULL) {
        THROW(RC_FILE_HANDLE_NOT_INIT, "File handle not initialized");
    }
    
    fp = (FILE *)fHandle->mgmtInfo;
    
    // Allocate memory for empty page
    emptyPage = (char *)calloc(PAGE_SIZE, sizeof(char));
    if (emptyPage == NULL) {
        THROW(RC_WRITE_FAILED, "Memory allocation failed");
    }
    
    // Seek to end of file
    if (fseek(fp, 0, SEEK_END) != 0) {
        free(emptyPage);
        THROW(RC_WRITE_FAILED, "Could not seek to end of file");
    }
    
    // Write empty page
    bytesWritten = fwrite(emptyPage, sizeof(char), PAGE_SIZE, fp);
    
    free(emptyPage);
    
    if (bytesWritten < PAGE_SIZE) {
        THROW(RC_WRITE_FAILED, "Could not append empty block");
    }
    
    // Update total number of pages
    newTotalPages = fHandle->totalNumPages + 1;
    fHandle->totalNumPages = newTotalPages;
    
    // Update metadata at the beginning of the file
    currentPos = ftell(fp);
    if (fseek(fp, 0, SEEK_SET) != 0) {
        THROW(RC_WRITE_FAILED, "Could not seek to metadata");
    }
    
    if (fwrite(&newTotalPages, sizeof(int), 1, fp) != 1) {
        THROW(RC_WRITE_FAILED, "Could not update metadata");
    }
    
    // Restore file position
    fseek(fp, currentPos, SEEK_SET);
    
    // Flush to ensure data is written
    fflush(fp);
    
    return RC_OK;
}

/* Ensure the file has at least numberOfPages pages */
RC ensureCapacity(int numberOfPages, SM_FileHandle *fHandle) {
    if (fHandle == NULL) {
        THROW(RC_FILE_HANDLE_NOT_INIT, "File handle not initialized");
    }
    
    // Append empty blocks until we reach the desired capacity
    while (fHandle->totalNumPages < numberOfPages) {
        RC rc = appendEmptyBlock(fHandle);
        if (rc != RC_OK) {
            return rc;
        }
    }
    
    return RC_OK;
}