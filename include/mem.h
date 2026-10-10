#ifndef MEM_H
#define MEM_H

#include "types.h"

/* 32-bit physical-address view retained for the existing shell. */
extern uint32_t mem_heap_start;
extern uint32_t mem_heap_end;
extern uint32_t mem_heap_ptr;

void mem_init(uint32_t heap_start, uint32_t heap_end);
void mem_set_total_ram(uint32_t bytes);
uint32_t mem_total_ram(void);

void *kmalloc(uint32_t size);
void *mem_alloc(uint32_t size);
void mem_free(void *ptr);

uint32_t mem_used(void);
uint32_t mem_free_bytes(void);
uint32_t mem_capacity(void);

#endif
