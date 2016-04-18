/* heap.c - overrides of SDK heap handling functions
 * Copyright (c) 2016 Ivan Grokhotkov. All rights reserved.
 * This file is distributed under MIT license.
 */

#include <stdlib.h>
#include "espinc/c_types_compatible.h"
#include "umm_malloc/umm_malloc.h"

#define IRAM_ATTR __attribute__((section(".iram.text")))

#define HEAP_OP_SIZE 250

extern void *umm_malloc( size_t size );
extern void *umm_calloc( size_t num, size_t size );
extern void *umm_realloc( void *ptr, size_t size );
extern void umm_free( void *ptr );

typedef struct _heapOp
{
	char op;
	uint32_t addr;
	uint32_t size;
	uint16_t opCounter;
}heapOp;

heapOp gLastHeapOp[HEAP_OP_SIZE];
uint16_t gTotalHeapOp = 0;
uint8_t gHeapOpFlushAfter = 240;

void recordHeapOp(char op, uint32_t size, uint32_t addr)
{
	static int8_t heapOpIndex = -1;
	uint8_t i;
	if(!addr)
		return;

	if(heapOpIndex == HEAP_OP_SIZE - 1 ||
	   heapOpIndex >= gHeapOpFlushAfter - 1)
	{
		//Need to flush
		ets_printf("\n");
		for(i=0; i <= heapOpIndex; i++)
		{
			if(gLastHeapOp[i].op == 'f')
				ets_printf("hl{f,%x,0} %d\n", gLastHeapOp[i].addr, gLastHeapOp[i].opCounter);
			else
				ets_printf("hl{%c,%d,0,%x} %d\n", gLastHeapOp[i].op, gLastHeapOp[i].size, gLastHeapOp[i].addr, gLastHeapOp[i].opCounter);
		}
		heapOpIndex = -1;
	}
	++heapOpIndex;
	++gTotalHeapOp;
	gLastHeapOp[heapOpIndex].op = op;
	gLastHeapOp[heapOpIndex].addr = addr;
	gLastHeapOp[heapOpIndex].size = size;
	gLastHeapOp[heapOpIndex].opCounter = gTotalHeapOp;
}



void* IRAM_ATTR pvPortMalloc(size_t size, const char* file, int line)
{
    void* ret =  malloc(size);
    recordHeapOp('m', size, (uint32_t)ret);
    return ret;
}

void IRAM_ATTR vPortFree(void *ptr, const char* file, int line)
{
	recordHeapOp('f', 0, (uint32_t)ptr);

	free(ptr);
}

void* IRAM_ATTR pvPortCalloc(size_t count, size_t size, const char* file, int line)
{
	void* ret = calloc(count, size);
    recordHeapOp('m', size*count, (uint32_t)ret);
    return ret;
}

void* IRAM_ATTR pvPortRealloc(void *ptr, size_t size, const char* file, int line)
{
	void* ret = realloc(ptr, size);
    recordHeapOp('m', size, (uint32_t)ret);
    return ret;
}

void* IRAM_ATTR pvPortZalloc(size_t size, const char* file, int line)
{
	void* ret = calloc(1, size);
	recordHeapOp('m', size, (uint32_t)ret);
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
