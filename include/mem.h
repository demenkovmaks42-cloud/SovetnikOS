#ifndef MEM_H
#define MEM_H

#include "types.h"

void     mem_init(uint32_t heap_start, uint32_t heap_end);
void*    kmalloc(uint32_t size);
uint32_t mem_total_ram(void);
uint32_t mem_heap_ptr(void);
uint32_t mem_heap_start(void);
uint32_t mem_heap_end(void);

#endif
