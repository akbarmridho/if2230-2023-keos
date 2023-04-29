#ifndef _MEMORY_H
#define _MEMORY_H

#include "paging.h"

#define HEAP_START_ADDRESS (0 + PAGE_FRAME_SIZE)

struct allocator
{
    uint32_t size;
    uint32_t prevsize;
    uint8_t is_allocated;
};

void initialize_memory();

uint32_t malloc(uint32_t size);

bool free(void *ptr);

uint32_t realloc(void *ptr, uint32_t size);

#endif