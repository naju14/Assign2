#include "buffer_mgr.h"
#include "storage_mgr.h"
#include "dberror.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

// Internal data structures
typedef struct Frame {
    PageNumber pageNum;
    char *data;
    bool dirty;
    int fixCount;
    int lastAccessTime;
    int loadTime;
    int accessCount;
    int *accessHistory;
    int historySize;
} Frame;

typedef struct BM_MgmtData {
    Frame *frames;
    int numFrames;
    SM_FileHandle *fileHandle;
    int numReadIO;
    int numWriteIO;
    int clockHand;
    int timeCounter;
} BM_MgmtData;

// Helper: find frame with page
static int findFrame(BM_MgmtData *mgmtData, PageNumber pageNum) {
    for (int i = 0; i < mgmtData->numFrames; i++)
        if (mgmtData->frames[i].pageNum == pageNum) return i;
    return -1;
}

// Helper: find empty frame
static int findEmptyFrame(BM_MgmtData *mgmtData) {
    for (int i = 0; i < mgmtData->numFrames; i++)
        if (mgmtData->frames[i].pageNum == NO_PAGE) return i;
    return -1;
}

// Helper: update LRU-K history
static void updateLRUKHistory(Frame *frame, int time, int k) {
    if (frame->accessHistory == NULL) {
        frame->accessHistory = (int *)malloc(sizeof(int) * k);
        frame->historySize = 0;
    }
    if (frame->historySize < k) {
        frame->accessHistory[frame->historySize++] = time;
    } else {
        for (int i = 0; i < k - 1; i++)
            frame->accessHistory[i] = frame->accessHistory[i + 1];
        frame->accessHistory[k - 1] = time;
    }
}

// Helper: select victim frame
static int selectVictimFrame(BM_BufferPool *const bm) {
    BM_MgmtData *mgmtData = (BM_MgmtData *)bm->mgmtData;
    int victim = -1, minVal = INT_MAX, emptyFrame = -1;
    
    for (int i = 0; i < mgmtData->numFrames; i++) {
        if (mgmtData->frames[i].fixCount > 0) continue;
        if (mgmtData->frames[i].pageNum == NO_PAGE) {
            emptyFrame = i;
            break;
        }
        
        int val = INT_MAX;
        switch (bm->strategy) {
            case RS_FIFO: val = mgmtData->frames[i].loadTime; break;
            case RS_LRU: val = mgmtData->frames[i].lastAccessTime; break;
            case RS_LFU: val = mgmtData->frames[i].accessCount; break;
            case RS_LRU_K:
                val = (mgmtData->frames[i].historySize >= 2) ? 
                      mgmtData->frames[i].accessHistory[0] : 0;
                break;
            case RS_CLOCK:
                if (mgmtData->clockHand == i) {
                    victim = i;
                    mgmtData->clockHand = (mgmtData->clockHand + 1) % mgmtData->numFrames;
                    return victim;
                }
                continue;
        }
        if (val < minVal) { minVal = val; victim = i; }
    }
    return (emptyFrame >= 0) ? emptyFrame : victim;
}

// Helper: write frame to disk
static RC writeFrameToDisk(BM_BufferPool *const bm, int frameIndex) {
    BM_MgmtData *mgmtData = (BM_MgmtData *)bm->mgmtData;
    Frame *frame = &mgmtData->frames[frameIndex];
    
    if (frame->dirty && frame->pageNum != NO_PAGE) {
        RC rc = writeBlock(frame->pageNum, mgmtData->fileHandle, frame->data);
        if (rc != RC_OK) return rc;
        frame->dirty = false;
        mgmtData->numWriteIO++;
    }
    return RC_OK;
}

// Helper: cleanup frames
static void cleanupFrames(BM_MgmtData *mgmtData, int numFrames) {
    for (int i = 0; i < numFrames; i++) {
        free(mgmtData->frames[i].data);
        free(mgmtData->frames[i].accessHistory);
    }
}

// Initialize buffer pool
RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName,
                  const int numPages, ReplacementStrategy strategy, void *stratData) {
    if (!bm || !pageFileName) return RC_FILE_NOT_FOUND;
    
    BM_MgmtData *mgmtData = (BM_MgmtData *)malloc(sizeof(BM_MgmtData));
    if (!mgmtData) return RC_WRITE_FAILED;
    
    bm->pageFile = (char *)pageFileName;
    bm->numPages = numPages;
    bm->strategy = strategy;
    bm->mgmtData = mgmtData;
    
    mgmtData->frames = (Frame *)malloc(sizeof(Frame) * numPages);
    if (!mgmtData->frames) { free(mgmtData); return RC_WRITE_FAILED; }
    
    // Initialize frames
    for (int i = 0; i < numPages; i++) {
        mgmtData->frames[i].pageNum = NO_PAGE;
        mgmtData->frames[i].data = (char *)malloc(PAGE_SIZE);
        if (!mgmtData->frames[i].data) {
            cleanupFrames(mgmtData, i);
            free(mgmtData->frames);
            free(mgmtData);
            return RC_WRITE_FAILED;
        }
        mgmtData->frames[i].dirty = false;
        mgmtData->frames[i].fixCount = 0;
        mgmtData->frames[i].lastAccessTime = 0;
        mgmtData->frames[i].loadTime = 0;
        mgmtData->frames[i].accessCount = 0;
        mgmtData->frames[i].accessHistory = NULL;
        mgmtData->frames[i].historySize = 0;
    }
    
    mgmtData->numFrames = numPages;
    mgmtData->numReadIO = 0;
    mgmtData->numWriteIO = 0;
    mgmtData->clockHand = 0;
    mgmtData->timeCounter = 0;
    
    mgmtData->fileHandle = (SM_FileHandle *)malloc(sizeof(SM_FileHandle));
    if (!mgmtData->fileHandle) {
        cleanupFrames(mgmtData, numPages);
        free(mgmtData->frames);
        free(mgmtData);
        return RC_WRITE_FAILED;
    }
    
    RC rc = openPageFile(pageFileName, mgmtData->fileHandle);
    if (rc != RC_OK) {
        cleanupFrames(mgmtData, numPages);
        free(mgmtData->frames);
        free(mgmtData->fileHandle);
        free(mgmtData);
        return rc;
    }
    return RC_OK;
}

// Shutdown buffer pool
RC shutdownBufferPool(BM_BufferPool *const bm) {
    if (!bm || !bm->mgmtData) return RC_FILE_HANDLE_NOT_INIT;
    
    BM_MgmtData *mgmtData = (BM_MgmtData *)bm->mgmtData;
    
    for (int i = 0; i < mgmtData->numFrames; i++)
        if (mgmtData->frames[i].pageNum != NO_PAGE && mgmtData->frames[i].dirty)
            writeFrameToDisk(bm, i);
    
    if (mgmtData->fileHandle) {
        closePageFile(mgmtData->fileHandle);
        free(mgmtData->fileHandle);
    }
    
    cleanupFrames(mgmtData, mgmtData->numFrames);
    free(mgmtData->frames);
    free(mgmtData);
    bm->mgmtData = NULL;
    return RC_OK;
}

// Force flush all dirty pages
RC forceFlushPool(BM_BufferPool *const bm) {
    if (!bm || !bm->mgmtData) return RC_FILE_HANDLE_NOT_INIT;
    
    BM_MgmtData *mgmtData = (BM_MgmtData *)bm->mgmtData;
    for (int i = 0; i < mgmtData->numFrames; i++)
        if (mgmtData->frames[i].pageNum != NO_PAGE && mgmtData->frames[i].dirty) {
            RC rc = writeFrameToDisk(bm, i);
            if (rc != RC_OK) return rc;
        }
    return RC_OK;
}

// Pin a page
RC pinPage(BM_BufferPool *const bm, BM_PageHandle *const page, const PageNumber pageNum) {
    if (!bm || !bm->mgmtData) return RC_FILE_HANDLE_NOT_INIT;
    if (pageNum < 0) return RC_READ_NON_EXISTING_PAGE;
    
    BM_MgmtData *mgmtData = (BM_MgmtData *)bm->mgmtData;
    int frameIndex = findFrame(mgmtData, pageNum);
    
    if (frameIndex >= 0) {
        // Page already in buffer
        Frame *frame = &mgmtData->frames[frameIndex];
        frame->fixCount++;
        mgmtData->timeCounter++;
        
        if (bm->strategy == RS_LRU) frame->lastAccessTime = mgmtData->timeCounter;
        if (bm->strategy == RS_LFU) frame->accessCount++;
        if (bm->strategy == RS_LRU_K) updateLRUKHistory(frame, mgmtData->timeCounter, 2);
        
        page->pageNum = pageNum;
        page->data = frame->data;
        return RC_OK;
    }
    
    // Page not in buffer - load it
    frameIndex = findEmptyFrame(mgmtData);
    if (frameIndex < 0) {
        frameIndex = selectVictimFrame(bm);
        if (frameIndex < 0) return RC_WRITE_FAILED;
        RC rc = writeFrameToDisk(bm, frameIndex);
        if (rc != RC_OK) return rc;
    }
    
    Frame *frame = &mgmtData->frames[frameIndex];
    RC rc = readBlock(pageNum, mgmtData->fileHandle, frame->data);
    if (rc != RC_OK) return rc;
    
    mgmtData->numReadIO++;
    mgmtData->timeCounter++;
    
    frame->pageNum = pageNum;
    frame->dirty = false;
    frame->fixCount = 1;
    frame->lastAccessTime = mgmtData->timeCounter;
    frame->loadTime = mgmtData->timeCounter;
    frame->accessCount = 1;
    
    if (bm->strategy == RS_LRU_K) {
        frame->accessHistory = (int *)malloc(sizeof(int) * 2);
        frame->historySize = 1;
        frame->accessHistory[0] = mgmtData->timeCounter;
    }
    
    page->pageNum = pageNum;
    page->data = frame->data;
    return RC_OK;
}

// Unpin a page
RC unpinPage(BM_BufferPool *const bm, BM_PageHandle *const page) {
    if (!bm || !bm->mgmtData || !page) return RC_FILE_HANDLE_NOT_INIT;
    
    BM_MgmtData *mgmtData = (BM_MgmtData *)bm->mgmtData;
    int frameIndex = findFrame(mgmtData, page->pageNum);
    if (frameIndex < 0) return RC_READ_NON_EXISTING_PAGE;
    
    if (mgmtData->frames[frameIndex].fixCount > 0)
        mgmtData->frames[frameIndex].fixCount--;
    return RC_OK;
}

// Mark page as dirty
RC markDirty(BM_BufferPool *const bm, BM_PageHandle *const page) {
    if (!bm || !bm->mgmtData || !page) return RC_FILE_HANDLE_NOT_INIT;
    
    BM_MgmtData *mgmtData = (BM_MgmtData *)bm->mgmtData;
    int frameIndex = findFrame(mgmtData, page->pageNum);
    if (frameIndex < 0) return RC_READ_NON_EXISTING_PAGE;
    
    mgmtData->frames[frameIndex].dirty = true;
    return RC_OK;
}

// Force write page to disk
RC forcePage(BM_BufferPool *const bm, BM_PageHandle *const page) {
    if (!bm || !bm->mgmtData || !page) return RC_FILE_HANDLE_NOT_INIT;
    
    BM_MgmtData *mgmtData = (BM_MgmtData *)bm->mgmtData;
    int frameIndex = findFrame(mgmtData, page->pageNum);
    if (frameIndex < 0) return RC_READ_NON_EXISTING_PAGE;
    
    return writeFrameToDisk(bm, frameIndex);
}

// Statistics functions
PageNumber *getFrameContents(BM_BufferPool *const bm) {
    if (!bm || !bm->mgmtData) return NULL;
    BM_MgmtData *mgmtData = (BM_MgmtData *)bm->mgmtData;
    PageNumber *contents = (PageNumber *)malloc(sizeof(PageNumber) * mgmtData->numFrames);
    for (int i = 0; i < mgmtData->numFrames; i++)
        contents[i] = mgmtData->frames[i].pageNum;
    return contents;
}

bool *getDirtyFlags(BM_BufferPool *const bm) {
    if (!bm || !bm->mgmtData) return NULL;
    BM_MgmtData *mgmtData = (BM_MgmtData *)bm->mgmtData;
    bool *dirty = (bool *)malloc(sizeof(bool) * mgmtData->numFrames);
    for (int i = 0; i < mgmtData->numFrames; i++)
        dirty[i] = mgmtData->frames[i].dirty;
    return dirty;
}

int *getFixCounts(BM_BufferPool *const bm) {
    if (!bm || !bm->mgmtData) return NULL;
    BM_MgmtData *mgmtData = (BM_MgmtData *)bm->mgmtData;
    int *fixCounts = (int *)malloc(sizeof(int) * mgmtData->numFrames);
    for (int i = 0; i < mgmtData->numFrames; i++)
        fixCounts[i] = mgmtData->frames[i].fixCount;
    return fixCounts;
}

int getNumReadIO(BM_BufferPool *const bm) {
    return (bm && bm->mgmtData) ? ((BM_MgmtData *)bm->mgmtData)->numReadIO : 0;
}

int getNumWriteIO(BM_BufferPool *const bm) {
    return (bm && bm->mgmtData) ? ((BM_MgmtData *)bm->mgmtData)->numWriteIO : 0;
}
