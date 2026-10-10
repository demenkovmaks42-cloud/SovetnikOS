#include "mem.h"

#define MEM_ALIGN 8u
#define MEM_MAX_U32 0xFFFFFFFFu

typedef struct mem_block {
    uint32_t size;
    uint32_t free;
    struct mem_block *next;
    uint32_t reserved;
} mem_block_t;

uint32_t mem_heap_start = 0;
uint32_t mem_heap_end = 0;
uint32_t mem_heap_ptr = 0;

static uint32_t total_ram = 0;
static mem_block_t *first_block = NULL;
static uint32_t used_payload = 0;
static uint32_t peak_ptr = 0;

static uint32_t align_up(uint32_t value)
{
    if (value > MEM_MAX_U32 - (MEM_ALIGN - 1u)) return 0;
    return (value + MEM_ALIGN - 1u) & ~(MEM_ALIGN - 1u);
}

void mem_init(uint32_t start, uint32_t end)
{
    uint32_t aligned_start;

    mem_heap_start = 0;
    mem_heap_end = 0;
    mem_heap_ptr = 0;
    first_block = NULL;
    used_payload = 0;
    peak_ptr = 0;

    aligned_start = align_up(start);
    if (aligned_start == 0 || end <= aligned_start) return;
    if ((end - aligned_start) <= sizeof(mem_block_t) + MEM_ALIGN) return;

    mem_heap_start = aligned_start;
    mem_heap_end = end & ~(MEM_ALIGN - 1u);
    if (mem_heap_end <= mem_heap_start + sizeof(mem_block_t) + MEM_ALIGN) {
        mem_heap_start = 0;
        mem_heap_end = 0;
        return;
    }

    first_block = (mem_block_t *)mem_heap_start;
    first_block->size = mem_heap_end - mem_heap_start - (uint32_t)sizeof(mem_block_t);
    first_block->free = 1;
    first_block->next = NULL;
    first_block->reserved = 0;
    mem_heap_ptr = mem_heap_start;
    peak_ptr = mem_heap_start;
}

void mem_set_total_ram(uint32_t bytes) { total_ram = bytes; }
uint32_t mem_total_ram(void) { return total_ram; }

void *kmalloc(uint32_t size)
{
    mem_block_t *block;
    uint32_t wanted;

    if (!size || !first_block) return NULL;
    wanted = align_up(size);
    if (!wanted) return NULL;

    for (block = first_block; block; block = block->next) {
        if (!block->free || block->size < wanted) continue;

        if (block->size >= wanted && block->size - wanted >= (uint32_t)sizeof(mem_block_t) + MEM_ALIGN) {
            mem_block_t *split = (mem_block_t *)((uint8_t *)(block + 1) + wanted);
            split->size = block->size - wanted - (uint32_t)sizeof(mem_block_t);
            split->free = 1;
            split->next = block->next;
            split->reserved = 0;
            block->next = split;
            block->size = wanted;
        }

        block->free = 0;
        used_payload += block->size;
        {
            uint32_t block_end = (uint32_t)(block + 1) + block->size;
            if (block_end > peak_ptr) peak_ptr = block_end;
            mem_heap_ptr = peak_ptr;
        }
        return (void *)(block + 1);
    }

    return NULL;
}

void *mem_alloc(uint32_t size) { return kmalloc(size); }

void mem_free(void *ptr)
{
    mem_block_t *block;
    mem_block_t *prev = NULL;
    mem_block_t *cur;
    uint32_t p = (uint32_t)ptr;

    if (!ptr || !first_block || p < mem_heap_start + sizeof(mem_block_t) || p >= mem_heap_end)
        return;

    /* Validate the pointer against the allocator's block list. */
    for (cur = first_block; cur; prev = cur, cur = cur->next) {
        if ((uint32_t)(cur + 1) == p) break;
    }
    if (!cur || cur->free) return;

    block = cur;
    block->free = 1;
    if (used_payload >= block->size) used_payload -= block->size;
    else used_payload = 0;

    /* Merge with the following free block. */
    if (block->next && block->next->free) {
        block->size += (uint32_t)sizeof(mem_block_t) + block->next->size;
        block->next = block->next->next;
    }

    /* Merge into the previous free block. */
    if (prev && prev->free) {
        prev->size += (uint32_t)sizeof(mem_block_t) + block->size;
        prev->next = block->next;
    }
}

uint32_t mem_used(void) { return used_payload; }

uint32_t mem_free_bytes(void)
{
    uint32_t total = 0;
    mem_block_t *block;
    for (block = first_block; block; block = block->next) {
        if (block->free && total <= MEM_MAX_U32 - block->size) total += block->size;
    }
    return total;
}

uint32_t mem_capacity(void)
{
    if (!mem_heap_end || mem_heap_end <= mem_heap_start) return 0;
    return mem_heap_end - mem_heap_start;
}
