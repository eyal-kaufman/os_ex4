#pragma once

#include "MemoryConstants.h"
#include "PhysicalMemory.h"

//-----------------------------------------------------Declarations-----------------------------------------------------
word_t getFrame(uint64_t virtualAddress);
word_t findUnusedFrame();

//----------------------------------------------------------END---------------------------------------------------------

/*
 * Initialize the virtual memory.
 */
void VMinitialize();

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

word_t getFrame(uint64_t virtualAddress) {
    unsigned int currentDepth = 0;
    uint64_t tempAddress = virtualAddress;
    uint64_t parentFrame = 0;
    word_t pageTableAddress;
    while (currentDepth < TABLES_DEPTH) {
        unsigned int leftoversSize = (TABLES_DEPTH - currentDepth) * PAGE_SIZE;
        uint64_t currentOffset = tempAddress >> leftoversSize;
        tempAddress -= currentOffset << leftoversSize;
        PMread(parentFrame * PAGE_SIZE + currentOffset, &pageTableAddress);
        if (pageTableAddress == 0) {
            word_t unusedFrame = findUnusedFrame();
            PMwrite(parentFrame * PAGE_SIZE + currentOffset,unusedFrame);
            pageTableAddress = unusedFrame;
        }
        parentFrame = pageTableAddress;
        currentDepth ++;
    }
    return pageTableAddress;
}

word_t findUnusedFrame() {

}
/* Writes a word to the given virtual address.
 *
 * returns 1 on success.
 * returns 0 on failure (if the address cannot be mapped to a physical
 * address for any reason)
 */
int VMwrite(uint64_t virtualAddress, word_t value);
