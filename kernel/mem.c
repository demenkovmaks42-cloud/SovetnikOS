#include "mem.h"

static uint32_t heap_ptr = 0;
static uint32_t heap_start = 0;
static uint32_t heap_end = 0;
static uint32_t total_ram = 0;

void mem_init(uint32_t start, uint32_t end) {
    heap_start = (start + 3) & ~3u;
    heap_end = end;
    heap_ptr = heap_start;
}

void* kmalloc(uint32_t size) {
    if (size == 0) return NULL;
    size = (size + 3) & ~3u;
    if (heap_ptr + size > heap_end) return NULL;
    void *p = (void*)heap_ptr;
    heap_ptr += size;
    return p;
}

uint32_t mem_total_ram(void) { return total_ram; }
uint32_t mem_heap_ptr(void)  { return heap_ptr; }
uint32_t mem_heap_start(void){ return heap_start; }
uint32_t mem_heap_end(void)  { return heap_end; }

// вызывается из kmain после парсинга mmap
void mem_set_total_ram(uint32_t bytes) { total_ram = bytes; }
