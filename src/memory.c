#include "lib-header/memory.h"
#include "lib-header/stdmem.h"

void initialize_memory()
{
    int code_retval = allocate_single_user_page_frame((uint8_t *)0); // user code region
    (void)code_retval;
    int heap_retval = allocate_single_user_page_frame((uint8_t *)HEAP_START_ADDRESS); // heap region
    (void)heap_retval;
}

uint32_t malloc(uint32_t size)
{
    struct allocator *entry = (struct allocator *)HEAP_START_ADDRESS;
    struct allocator *prev = (struct allocator *)HEAP_START_ADDRESS;

    bool found = FALSE;
    bool has_prev = FALSE;

    while (!found && ((uint32_t)entry < HEAP_START_ADDRESS + PAGE_FRAME_SIZE))
    {
        if ((*entry).is_allocated)
        {
            prev = entry;
            has_prev = TRUE;
            entry += sizeof(struct allocator) + (*entry).size;
        }
        else
        {
            found = TRUE;
            (*entry).is_allocated = TRUE;

            if (has_prev)
            {
                entry->prevsize = prev->size;
            }
            else
            {
                entry->prevsize = 0;
            }

            if ((*entry).size != 0)
            {
                if (entry->size < size)
                {
                    entry += sizeof(struct allocator) + (*entry).size;
                }
                else if ((*entry).size - size < sizeof(struct allocator))
                {
                    // pass, just allocate more than needed (internal fragmentation)
                    // keep default size
                }
                else
                {
                    struct allocator *next = (struct allocator *)entry + size + sizeof(struct allocator);
                    next->is_allocated = FALSE;
                    next->size = entry->size - size - sizeof(struct allocator);
                    next->prevsize = size;
                    (*entry).size = size;
                }
            }
            else
            {
                (*entry).size = size;
            }
        }
    }

    if (found)
    {
        return (uint32_t)entry + sizeof(struct allocator);
    }

    return 0;
}

bool free(void *ptr)
{
    struct allocator *entry = (struct allocator *)(ptr - sizeof(struct allocator));

    if (!entry->is_allocated)
    {
        return FALSE;
    }

    (*entry).is_allocated = FALSE;

    // cek berikutnya dan gabung bila ada yang kosong
    struct allocator *next = entry + sizeof(struct allocator) + entry->size;
    while (!next->is_allocated && next->size != 0 && ((uint32_t)next < HEAP_START_ADDRESS + PAGE_FRAME_SIZE))
    {
        entry->size += next->size + sizeof(struct allocator);
        next = next + sizeof(struct allocator) + next->size;
    }

    // kasus ketika terdapat alokasi memory A - B - C dan kita melakukan free pada C sedangkan B merupakan
    // blok memori kosong. B dan C digabung
    struct allocator *current = entry;
    struct allocator *prev = (struct allocator *)(current - current->prevsize - sizeof(struct allocator));

    while ((uint32_t)prev >= HEAP_START_ADDRESS && !prev->is_allocated && prev->size != 0)
    {
        prev->size += current->size + sizeof(struct allocator);
        current = prev;
        prev = (struct allocator *)(current - current->prevsize - sizeof(struct allocator));
    }

    return TRUE;
}

uint32_t realloc(void *ptr, uint32_t size)
{
    uint32_t newptr = malloc(size);

    if (newptr == 0) // failed to malloc
    {
        return 0;
    }

    memcpy((void *)newptr, (void *)ptr, size);

    if (free((void *)ptr) == FALSE) // failed to free
    {
        free((void *)newptr);
        return 0;
    }

    return newptr;
}