#pragma once

#include <algorithm>
#include "MemoryConstants.h"
#include "PhysicalMemory.h"

//----------------------------------------------------Declarations----------------------------------------------------//

word_t getFrame(uint64_t virtualAddress);
uint64_t findAvailableFrame(word_t pageSwappedIn, uint64_t sourceFrame);
uint64_t DFS(uint64_t currentFrame, unsigned int currentDepth, uint64_t &maxFrameIndex, uint64_t &maximalCyclicalNode,
             unsigned int &maximalCyclicalDistance, word_t pageSwappedIn, word_t pagePath, uint64_t &parentFrame, uint64_t sourceFrame);
unsigned int minCyclicDistance(word_t pageSwappedIn, word_t pagePath);
void deleteLinkToFrame(uint64_t  parentFrame, word_t pagePath);
void initializeFrame(uint64_t frameAddress);

//---------------------------------------------------Implementation---------------------------------------------------//

/*
 * Initialize the virtual memory.
 */
void VMinitialize(){
    initializeFrame(0);
}

/* Reads a word from the given virtual address
 * and puts its content in *value.
 *
 * returns 1 on success.
 * returns 0 on failure (if the address cannot be mapped to a physical
 * address for any reason)
 */
int VMread(uint64_t virtualAddress, word_t* value) {
    uint64_t offset = virtualAddress & ((1LL << OFFSET_WIDTH) -1);
    word_t finalFrame = getFrame(virtualAddress);
    uint64_t finalAddress = finalFrame * PAGE_SIZE + offset;
    PMread(finalAddress, value);
    return 1;
}

/*
 *
 */
word_t getFrame(uint64_t virtualAddress) {
    unsigned int currentDepth = 0;
    uint64_t tempAddress = virtualAddress;
    uint64_t parentFrame = 0;
    word_t pageTableAddress;
    while (currentDepth < TABLES_DEPTH) {
        unsigned int leftoversSize = (TABLES_DEPTH - currentDepth) * OFFSET_WIDTH;
        uint64_t currentOffset = tempAddress >> leftoversSize;
        tempAddress -= currentOffset << leftoversSize;
        PMread(parentFrame * PAGE_SIZE + currentOffset, &pageTableAddress);
        if (pageTableAddress == 0) {
            word_t newFrame = findAvailableFrame((word_t) virtualAddress >> OFFSET_WIDTH, parentFrame);
            if (currentDepth == TABLES_DEPTH - 1) {
                PMrestore(newFrame, virtualAddress >> OFFSET_WIDTH);
            }
            else {
                initializeFrame(newFrame);
            }
            PMwrite(parentFrame * PAGE_SIZE + currentOffset, newFrame);
            pageTableAddress = newFrame;
        }
        parentFrame = pageTableAddress;
        currentDepth ++;
    }
    return pageTableAddress;
}

/*
 * traverse the tree with DFS and check:
 * 1. if there is a table where all rows are 0
 * 2. if there is a frame which is not used at all (max_frame_index + 1 < NUM_FRAMES)
 * 3. when reaching a leaf, calculate the cyclical distance, and if (1,2) is false return the leaf with the maximal cyclical distance . evict the chosen frame
 */
uint64_t findAvailableFrame(word_t pageSwappedIn, uint64_t sourceFrame) {
    uint64_t root = 0, maximalCyclicalNode = 0, maxFrameIndex = 0, parentFrame= 0;
    unsigned int currentDepth = 0, maximalCyclicalDistance = 0;
    word_t pagePath = 0;
    uint64_t availableFrame = DFS(root, currentDepth, maxFrameIndex, maximalCyclicalNode, maximalCyclicalDistance, pageSwappedIn, pagePath, parentFrame, sourceFrame);
    // check if an empty table was found
    if (availableFrame > 0 && availableFrame != sourceFrame) {
        deleteLinkToFrame(parentFrame, pagePath);
        return availableFrame;
    }
    if (maxFrameIndex + 1 < NUM_FRAMES)
    {
        return maxFrameIndex + 1;
    }
    PMevict(maximalCyclicalNode, pagePath);
    deleteLinkToFrame(parentFrame, pagePath);
    return maximalCyclicalNode;
}

/*
 *
 */
void deleteLinkToFrame(uint64_t  parentFrame, word_t pagePath)
{
    uint64_t linkToAvailableFrame = parentFrame * PAGE_SIZE + (pagePath & ((1LL << OFFSET_WIDTH) -1));
    PMwrite(linkToAvailableFrame, 0);
}

/*
 *
 */
uint64_t DFS(uint64_t currentFrame, unsigned int currentDepth, uint64_t &maxFrameIndex, uint64_t &maximalCyclicalNode,
             unsigned int &maximalCyclicalDistance, word_t pageSwappedIn, word_t pagePath, uint64_t &parentFrame, uint64_t sourceFrame){
    maxFrameIndex = maxFrameIndex < currentFrame ? currentFrame : maxFrameIndex;
    if (currentDepth == TABLES_DEPTH - 1) {
        unsigned int cyclicDistance = minCyclicDistance(pageSwappedIn, pagePath);
        if ( maximalCyclicalDistance < cyclicDistance ){
            maximalCyclicalDistance = cyclicDistance;
            maximalCyclicalNode = currentFrame;
        }
        return 0;
    }
    word_t currentRead;
    uint64_t tempFrame;
    bool isEmptyTable = true;
    for (unsigned int offset = 0; offset < PAGE_SIZE; offset++)
    {
        PMread(currentFrame * PAGE_SIZE + offset, &currentRead);
        if (currentRead != 0){
            isEmptyTable = false;
            unsigned int newPagePath = (offset << ((TABLES_DEPTH - currentDepth - 1) * OFFSET_WIDTH)) + pagePath;
            tempFrame = DFS(currentRead, currentDepth + 1, maxFrameIndex,
                                     maximalCyclicalNode, maximalCyclicalDistance, pageSwappedIn, newPagePath, currentFrame, sourceFrame);
            if (tempFrame != 0)
            {
                return tempFrame;
            }
        }
    }
    return !isEmptyTable || currentFrame == sourceFrame ? 0: currentFrame;
}

/*
 *
 */
unsigned int minCyclicDistance(word_t pageSwappedIn, word_t pagePath)
{
    unsigned int lhs = NUM_PAGES - std::abs(pageSwappedIn - pagePath), rhs = std::abs(pageSwappedIn - pagePath);
    return (lhs < rhs)? lhs: rhs;
}

/* Writes a word to the given virtual address.
 *
 * returns 1 on success.
 * returns 0 on failure (if the address cannot be mapped to a physical
 * address for any reason)
 */
int VMwrite(uint64_t virtualAddress, word_t value) {
    uint64_t offset = virtualAddress & ((1LL << OFFSET_WIDTH) -1);
    word_t finalFrame = getFrame(virtualAddress);
    uint64_t finalAddress = finalFrame * PAGE_SIZE + offset;
    PMwrite(finalAddress, value);
    return 1;
}

/*
 *
 */
void initializeFrame(uint64_t frameAddress)
{
    for (unsigned int offset = 0; offset < PAGE_SIZE; offset++)
    {
        PMwrite(frameAddress * PAGE_SIZE + offset, 0);
    }
}