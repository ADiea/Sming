/* heap.c - overrides of SDK heap handling functions
 * Copyright (c) 2016 Ivan Grokhotkov. All rights reserved.
 * This file is distributed under MIT license.
 */

#include <stdlib.h>
#include "espinc/c_types_compatible.h"
#include "umm_malloc/umm_malloc.h"

#define IRAM_ATTR __attribute__((section(".iram.text")))

#define HEAP_OP_SIZE 64

extern void *umm_malloc( size_t size );
extern void *umm_calloc( size_t num, size_t size );
extern void *umm_realloc( void *ptr, size_t size );
extern void umm_free( void *ptr );

typedef struct _heapOp
{
	char op;
	uint32_t addr;
	uint32_t addrOld;
	uint32_t size;
	uint16_t opCounter;
}heapOp;

heapOp gLastHeapOp[HEAP_OP_SIZE];
uint16_t gTotalHeapOp = 0;
uint8_t gHeapOpFlushAfter = 63;

void recordHeapOp(char op, uint32_t size, uint32_t addr, uint32_t addrOld)
{
	static int8_t heapOpIndex = -1;
	uint8_t i;
	if(!addr)
		return;

	if(heapOpIndex == HEAP_OP_SIZE - 1 ||
	   heapOpIndex >= gHeapOpFlushAfter - 1)
	{
		//Need to flush
		m_printf("\n");
		for(i=0; i <= heapOpIndex; i++)
		{
			if(gLastHeapOp[i].op == 'f')
				m_printf("hl{f,%x,0} %d\n", gLastHeapOp[i].addr, gLastHeapOp[i].opCounter);
			/*
			else if(gLastHeapOp[i].op == 'r')
			{
				if(gLastHeapOp[i].addrOld == 0)
					LOG_I("hl{m,%d,0,%x} (r)%d", gLastHeapOp[i].size, gLastHeapOp[i].addr, gLastHeapOp[i].opCounter);
				else
				{
					LOG_I("hl{f,%x,0} (r)%d", gLastHeapOp[i].addrOld, gLastHeapOp[i].opCounter);
					LOG_I("hl{m,%d,0,%x} (r)%d", gLastHeapOp[i].size, gLastHeapOp[i].addr, gLastHeapOp[i].opCounter);
				}
			}
			*/
			else
				m_printf("hl{%c,%d,0,%x} %d\n", gLastHeapOp[i].op, gLastHeapOp[i].size, gLastHeapOp[i].addr, gLastHeapOp[i].opCounter);
		}
		LOG_I("hl flush %d - remains %d", gTotalHeapOp, umm_free_heap_size());
		heapOpIndex = -1;
	}
	++heapOpIndex;
	++gTotalHeapOp;
	gLastHeapOp[heapOpIndex].op = op;
	gLastHeapOp[heapOpIndex].addr = addr;
	gLastHeapOp[heapOpIndex].size = size;
	gLastHeapOp[heapOpIndex].addrOld = addrOld;
	gLastHeapOp[heapOpIndex].opCounter = gTotalHeapOp;
}



void* IRAM_ATTR pvPortMalloc(size_t size, const char* file, int line)
{
    void* ret =  malloc(size);
    return ret;
}

void IRAM_ATTR vPortFree(void *ptr, const char* file, int line)
{
	free(ptr);
}

void* IRAM_ATTR pvPortCalloc(size_t count, size_t size, const char* file, int line)
{
	void* ret = calloc(count, size);
    //recordHeapOp('c', size*count, (uint32_t)ret, 0);
    return ret;
}

void* IRAM_ATTR pvPortRealloc(void *ptr, size_t size, const char* file, int line)
{
	void* ret = realloc(ptr, size);
    //recordHeapOp('r', size, (uint32_t)ret, (uint32_t)ptr);
    return ret;
}

void* IRAM_ATTR pvPortZalloc(size_t size, const char* file, int line)
{
	void* ret = calloc(1, size);
	//recordHeapOp('z', size, (uint32_t)ret, 0);
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
