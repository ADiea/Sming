/* heap.c - overrides of SDK heap handling functions
 * Copyright (c) 2016 Ivan Grokhotkov. All rights reserved.
 * This file is distributed under MIT license.
 */

#include <stdlib.h>
#include "espinc/c_types_compatible.h"
#include "umm_malloc/umm_malloc.h"

#define IRAM_ATTR __attribute__((section(".iram.text")))

extern void *umm_malloc( size_t size );
extern void *umm_calloc( size_t num, size_t size );
extern void *umm_realloc( void *ptr, size_t size );
extern void umm_free( void *ptr );

void* IRAM_ATTR pvPortMalloc(size_t size, const char* file, int line)
{
    void* ret =  malloc(size);
   	ets_printf("\nhl{m,%d,0,%x}", size, ret);
    return ret;
}

void IRAM_ATTR vPortFree(void *ptr, const char* file, int line)
{
	ets_printf("\nhl{f,0,%x}", ptr);
	free(ptr);
}

void* IRAM_ATTR pvPortCalloc(size_t count, size_t size, const char* file, int line)
{
	void* ret = calloc(count, size);
	ets_printf("\nhl{m,%d,0,%x}", size*count, ret);
    return ret;
}

void* IRAM_ATTR pvPortRealloc(void *ptr, size_t size, const char* file, int line)
{
	void* ret = realloc(ptr, size);
	ets_printf("\nhl{m,%d,0,%x}", size, ret);
    return ret;
}

void* IRAM_ATTR pvPortZalloc(size_t size, const char* file, int line)
{
	void* ret = calloc(1, size);
	ets_printf("\nhl{m,%d,0,%x}", size, ret);
    return ret;
}

size_t xPortGetFreeHeapSize(void)
{
    return umm_free_heap_size();
}

size_t IRAM_ATTR xPortWantedSizeAlign(size_t size)
{
    return (size + 3) & ~((size_t) 3);
}

void system_show_malloc(void)
{
    umm_info(NULL, 1);
}
