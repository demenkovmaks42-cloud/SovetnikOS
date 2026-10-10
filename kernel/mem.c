#include "mem.h"

#define MEM_ALIGNMENT 4u
#define UINT32_MAX_VALUE 0xFFFFFFFFu

/* Эти переменные используются оболочкой */
uint32_t mem_heap_start = 0;
uint32_t mem_heap_end   = 0;
uint32_t mem_heap_ptr   = 0;

static uint32_t total_ram = 0;

void mem_init(uint32_t start, uint32_t end)
{
    /* Не допускаем переполнения при выравнивании */
    if (start > UINT32_MAX_VALUE - (MEM_ALIGNMENT - 1u)) {
        mem_heap_start = 0;
        mem_heap_end = 0;
        mem_heap_ptr = 0;
        return;
    }

    start = (start + MEM_ALIGNMENT - 1u)
    & ~(MEM_ALIGNMENT - 1u);

    if (end <= start) {
        mem_heap_start = 0;
        mem_heap_end = 0;
        mem_heap_ptr = 0;
        return;
    }

    mem_heap_start = start;
    mem_heap_end = end;
    mem_heap_ptr = start;
}

void *kmalloc(uint32_t size)
{
    uint32_t aligned_size;
    uint32_t result;

    if (size == 0 || mem_heap_start == 0) {
        return NULL;
    }

    /* Выравниваем размер до 4 байт */
    if (size > UINT32_MAX_VALUE - (MEM_ALIGNMENT - 1u)) {
        return NULL;
    }

    aligned_size = (size + MEM_ALIGNMENT - 1u)
    & ~(MEM_ALIGNMENT - 1u);

    /* Проверяем границы без сложения, которое может переполниться */
    if (mem_heap_ptr > mem_heap_end) {
        return NULL;
    }

    if (aligned_size > mem_heap_end - mem_heap_ptr) {
        return NULL;
    }

    result = mem_heap_ptr;
    mem_heap_ptr += aligned_size;

    return (void *)result;
}

/* Альтернативное имя для оболочки */
void *mem_alloc(uint32_t size)
{
    return kmalloc(size);
}

void mem_set_total_ram(uint32_t bytes)
{
    total_ram = bytes;
}

uint32_t mem_total_ram(void)
{
    return total_ram;
}

uint32_t mem_get_heap_start(void)
{
    return mem_heap_start;
}

uint32_t mem_get_heap_end(void)
{
    return mem_heap_end;
}

uint32_t mem_get_heap_ptr(void)
{
    return mem_heap_ptr;
}
