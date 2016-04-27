/* ----------------------------------------------------------------------------
 * umm_malloc.c - a memory allocator for embedded systems (microcontrollers)
 *
 * See copyright notice in LICENSE.TXT
 * ----------------------------------------------------------------------------
 *
 * R.Hempel 2007-09-22 - Original
 * R.Hempel 2008-12-11 - Added MIT License biolerplate
 *                     - realloc() now looks to see if previous block is free
 *                     - made common operations functions
 * R.Hempel 2009-03-02 - Added macros to disable tasking
 *                     - Added function to dump heap and check for valid free
 *                        pointer
 * R.Hempel 2009-03-09 - Changed name to umm_malloc to avoid conflicts with
 *                        the mm_malloc() library functions
 *                     - Added some test code to assimilate a free block
 *                        with the very block if possible. Complicated and
 *                        not worth the grief.
 * D.Frank 2014-04-02  - Fixed heap configuration when UMM_TEST_MAIN is NOT set,
 *                        added user-dependent configuration file umm_malloc_cfg.h
 * ----------------------------------------------------------------------------
 *
 *  Note: when upgrading this file with upstream code, replace all %i with %d in
 *        printf format strings. ets_printf doesn't handle %i.
 *
 * ----------------------------------------------------------------------------
 *
 * This is a memory management library specifically designed to work with the
 * ARM7 embedded processor, but it should work on many other 32 bit processors,
 * as well as 16 and 8 bit devices.
 *
 * ACKNOWLEDGEMENTS
 *
 * Joerg Wunsch and the avr-libc provided the first malloc() implementation
 * that I examined in detail.
 *
 * http: *www.nongnu.org/avr-libc
 *
 * Doug Lea's paper on malloc() was another excellent reference and provides
 * a lot of detail on advanced memory management techniques such as binning.
 *
 * http: *g.oswego.edu/dl/html/malloc.html
 *
 * Bill Dittman provided excellent suggestions, including macros to support
 * using these functions in critical sections, and for optimizing realloc()
 * further by checking to see if the previous block was free and could be
 * used for the new block size. This can help to reduce heap fragmentation
 * significantly.
 *
 * Yaniv Ankin suggested that a way to dump the current heap condition
 * might be useful. I combined this with an idea from plarroy to also
 * allow checking a free pointer to make sure it's valid.
 *
 * ----------------------------------------------------------------------------
 *
 * The memory manager assumes the following things:
 *
 * 1. The standard POSIX compliant malloc/realloc/free semantics are used
 * 2. All memory used by the manager is allocated at link time, it is aligned
 *    on a 32 bit boundary, it is contiguous, and its extent (start and end
 *    address) is filled in by the linker.
 * 3. All memory used by the manager is initialized to 0 as part of the
 *    runtime startup routine. No other initialization is required.
 *
 * The fastest linked list implementations use doubly linked lists so that
 * its possible to insert and delete blocks in constant time. This memory
 * manager keeps track of both free and used blocks in a doubly linked list.
 *
 * Most memory managers use some kind of list structure made up of pointers
 * to keep track of used - and sometimes free - blocks of memory. In an
 * embedded system, this can get pretty expensive as each pointer can use
 * up to 32 bits.
 *
 * In most embedded systems there is no need for managing large blocks
 * of memory dynamically, so a full 32 bit pointer based data structure
 * for the free and used block lists is wasteful. A block of memory on
 * the free list would use 16 bytes just for the pointers!
 *
 * This memory management library sees the malloc heap as an array of blocks,
 * and uses block numbers to keep track of locations. The block numbers are
 * 15 bits - which allows for up to 32767 blocks of memory. The high order
 * bit marks a block as being either free or in use, which will be explained
 * later.
 *
 * The result is that a block of memory on the free list uses just 8 bytes
 * instead of 16.
 *
 * In fact, we go even one step futher when we realize that the free block
 * index values are available to store data when the block is allocated.
 *
 * The overhead of an allocated block is therefore just 4 bytes.
 *
 * Each memory block holds 8 bytes, and there are up to 32767 blocks
 * available, for about 256K of heap space. If that's not enough, you
 * can always add more data bytes to the body of the memory block
 * at the expense of free block size overhead.
 *
 * There are a lot of little features and optimizations in this memory
 * management system that makes it especially suited to small embedded, but
 * the best way to appreciate them is to review the data structures and
 * algorithms used, you can find description in file ./algorithm.txt.
 *
 */

#include <stdio.h>
#include <string.h>

#include "umm_malloc.h"

#include "umm_malloc_cfg.h"   /* user-dependent */

extern uint16_t gTotalHeapOp; //total heap operations
extern void recordHeapOp(char op, uint32_t size, uint32_t addr, uint32_t addrOld);

#ifndef UMM_FIRST_FIT
#  ifndef UMM_BEST_FIT
#    define UMM_BEST_FIT
#  endif
#endif

#ifndef DBG_LOG_LEVEL
#  undef  DBG_LOG_LEVEL
#  define DBG_LOG_LEVEL 0
#else
#  undef  DBG_LOG_LEVEL
#  define DBG_LOG_LEVEL DBG_LOG_LEVEL
#endif

/* -- dbglog {{{ */

/* ----------------------------------------------------------------------------
 *            A set of macros that cleans up code that needs to produce debug
 *            or log information.
 *
 * See copyright notice in LICENSE.TXT
 * ----------------------------------------------------------------------------
 *
 * There are macros to handle the following decreasing levels of detail:
 *
 * 6 = TRACE
 * 5 = DEBUG
 * 4 = CRITICAL
 * 3 = ERROR
 * 2 = WARNING
 * 1 = INFO
 * 0 = FORCE - The printf is always compiled in and is called only when
 *              the first parameter to the macro is non-0
 *
 * ----------------------------------------------------------------------------
 *
 * The following #define should be set up before this file is included so
 * that we can be sure that the correct macros are defined.
 *
 * #define DBG_LOG_LEVEL x
 * ----------------------------------------------------------------------------
 */

#undef DBG_LOG_TRACE
#undef DBG_LOG_DEBUG
#undef DBG_LOG_CRITICAL
#undef DBG_LOG_ERROR
#undef DBG_LOG_WARNING
#undef DBG_LOG_INFO
#undef DBG_LOG_FORCE

#define MAX_LOG_OP_LIMIT 40

/* ------------------------------------------------------------------------- */

#define DBG_MEM_OP( format, ... )

/*
uint32_t sizeBufLog = 0, printSz;
#define SZ_LOG_BUF 2048
char bufLog[SZ_LOG_BUF];
char printed=0;

#define DBG_MEM_OP( format, ... ) { \
										if(sizeBufLog+30 < SZ_LOG_BUF) \
											sizeBufLog += m_snprintf(bufLog+sizeBufLog, SZ_LOG_BUF - sizeBufLog,  format, ## __VA_ARGS__ ); \
										 else if(gTotalHeapOp > 80 && !printed) \
										 { \
											 printed=1; \
											 for(printSz=0; printSz<sizeBufLog; printSz++) \
											 m_putc(bufLog[printSz]); \
										 } \
									}
*/

#if DBG_LOG_LEVEL >= 6
#  define DBG_LOG_TRACE( format, ... ) { if(gTotalHeapOp < MAX_LOG_OP_LIMIT) LOG_II( format, ## __VA_ARGS__ ); }
#else
#  define DBG_LOG_TRACE( format, ... )
#endif

#if DBG_LOG_LEVEL >= 5
#  define DBG_LOG_DEBUG( format, ... ) { if(gTotalHeapOp < MAX_LOG_OP_LIMIT) LOG_II( format, ## __VA_ARGS__ ); }
#else
#  define DBG_LOG_DEBUG( format, ... )
#endif

#if DBG_LOG_LEVEL >= 4
#  define DBG_LOG_CRITICAL( format, ... ) { if(gTotalHeapOp < MAX_LOG_OP_LIMIT) LOG_II( format, ## __VA_ARGS__ ); }
#else
#  define DBG_LOG_CRITICAL( format, ... )
#endif

#if DBG_LOG_LEVEL >= 3
#  define DBG_LOG_ERROR( format, ... ) { if(gTotalHeapOp < MAX_LOG_OP_LIMIT) LOG_II( format, ## __VA_ARGS__ ); }
#else
#  define DBG_LOG_ERROR( format, ... )
#endif

#if DBG_LOG_LEVEL >= 2
#  define DBG_LOG_WARNING( format, ... ) { if(gTotalHeapOp < MAX_LOG_OP_LIMIT) LOG_II( format, ## __VA_ARGS__ ); }
#else
#  define DBG_LOG_WARNING( format, ... )
#endif

#if DBG_LOG_LEVEL >= 1
#  define DBG_LOG_INFO( format, ... ) { if(gTotalHeapOp < MAX_LOG_OP_LIMIT) LOG_II( format, ## __VA_ARGS__ ); }
#else
#  define DBG_LOG_INFO( format, ... )
#endif

#define DBG_LOG_FORCE( force, format, ... ) {if(force) {LOG_II( format, ## __VA_ARGS__  );}}

/* }}} */

/* ------------------------------------------------------------------------- */

UMM_H_ATTPACKPRE typedef struct umm_ptr_t {
  unsigned short int next;
  unsigned short int prev;
} UMM_H_ATTPACKSUF umm_ptr;


UMM_H_ATTPACKPRE typedef struct umm_block_t {
  union {
    umm_ptr used;
  } header;
  union {
    umm_ptr free;
    unsigned char data[4];
  } body;
} UMM_H_ATTPACKSUF umm_block;

#define UMM_FREELIST_MASK (0x8000)
#define UMM_BLOCKNO_MASK  (0x7FFF)

/* ------------------------------------------------------------------------- */

#ifdef UMM_REDEFINE_MEM_FUNCTIONS
#  define umm_free    free
#  define umm_malloc  malloc
#  define umm_calloc  calloc
#  define umm_realloc realloc
#endif

umm_block *umm_heap = NULL;
unsigned short int umm_numblocks = 0;

#define UMM_NUMBLOCKS (umm_numblocks)

/* ------------------------------------------------------------------------ */

#define UMM_BLOCK(b)  (umm_heap[b])

#define UMM_NBLOCK(b) (UMM_BLOCK(b).header.used.next)
#define UMM_PBLOCK(b) (UMM_BLOCK(b).header.used.prev)
#define UMM_NFREE(b)  (UMM_BLOCK(b).body.free.next)
#define UMM_PFREE(b)  (UMM_BLOCK(b).body.free.prev)
#define UMM_DATA(b)   (UMM_BLOCK(b).body.data)

/* integrity check (UMM_INTEGRITY_CHECK) {{{ */
#if defined(UMM_INTEGRITY_CHECK)
/*
 * Perform integrity check of the whole heap data. Returns 1 in case of
 * success, 0 otherwise.
 *
 * First of all, iterate through all free blocks, and check that all backlinks
 * match (i.e. if block X has next free block Y, then the block Y should have
 * previous free block set to X).
 *
 * Additionally, we check that each free block is correctly marked with
 * `UMM_FREELIST_MASK` on the `next` pointer: during iteration through free
 * list, we mark each free block by the same flag `UMM_FREELIST_MASK`, but
 * on `prev` pointer. We'll check and unmark it later.
 *
 * Then, we iterate through all blocks in the heap, and similarly check that
 * all backlinks match (i.e. if block X has next block Y, then the block Y
 * should have previous block set to X).
 *
 * But before checking each backlink, we check that the `next` and `prev`
 * pointers are both marked with `UMM_FREELIST_MASK`, or both unmarked.
 * This way, we ensure that the free flag is in sync with the free pointers
 * chain.
 */
static int integrity_check(void) {
  int ok = 1;
  unsigned short int prev;
  unsigned short int cur;

  if (umm_heap == NULL) {
    umm_init();
  }

  /* Iterate through all free blocks */
  prev = 0;
  while(1) {
    cur = UMM_NFREE(prev);

    /* Check that next free block number is valid */
    if (cur >= UMM_NUMBLOCKS) {
      printf("heap integrity broken: too large next free num: %d "
          "(in block %d, addr 0x%lx)\n", cur, prev,
          (unsigned long)&UMM_NBLOCK(prev));
      ok = 0;
      goto clean;
    }
    if (cur == 0) {
      /* No more free blocks */
      break;
    }

    /* Check if prev free block number matches */
    if (UMM_PFREE(cur) != prev) {
      printf("heap integrity broken: free links don't match: "
          "%d -> %d, but %d -> %d\n",
          prev, cur, cur, UMM_PFREE(cur));
      ok = 0;
      goto clean;
    }

    UMM_PBLOCK(cur) |= UMM_FREELIST_MASK;

    prev = cur;
  }

  /* Iterate through all blocks */
  prev = 0;
  while(1) {
    cur = UMM_NBLOCK(prev) & UMM_BLOCKNO_MASK;

    /* Check that next block number is valid */
    if (cur >= UMM_NUMBLOCKS) {
      printf("heap integrity broken: too large next block num: %d "
          "(in block %d, addr 0x%lx)\n", cur, prev,
          (unsigned long)&UMM_NBLOCK(prev));
      ok = 0;
      goto clean;
    }
    if (cur == 0) {
      /* No more blocks */
      break;
    }

    /* make sure the free mark is appropriate, and unmark it */
    if ((UMM_NBLOCK(cur) & UMM_FREELIST_MASK)
        != (UMM_PBLOCK(cur) & UMM_FREELIST_MASK))
    {
      printf("heap integrity broken: mask wrong at addr 0x%lx: n=0x%x, p=0x%x\n",
          (unsigned long)&UMM_NBLOCK(cur),
          (UMM_NBLOCK(cur) & UMM_FREELIST_MASK),
          (UMM_PBLOCK(cur) & UMM_FREELIST_MASK)
          );
      ok = 0;
      goto clean;
    }

    /* unmark */
    UMM_PBLOCK(cur) &= UMM_BLOCKNO_MASK;

    /* Check if prev block number matches */
    if (UMM_PBLOCK(cur) != prev) {
      printf("heap integrity broken: block links don't match: "
          "%d -> %d, but %d -> %d\n",
          prev, cur, cur, UMM_PBLOCK(cur));
      ok = 0;
      goto clean;
    }

    prev = cur;
  }

clean:
  if (!ok){
    UMM_HEAP_CORRUPTION_CB();
  }
  return ok;
}

#define INTEGRITY_CHECK() integrity_check()
#else
/*
 * Integrity check is disabled, so just define stub macro
 */
#define INTEGRITY_CHECK() 1
#endif
/* }}} */

/* poisoning (UMM_POISON) {{{ */
#if defined(UMM_POISON)
#define POISON_BYTE (0xa5)

/*
 * Yields a size of the poison for the block of size `s`.
 * If `s` is 0, returns 0.
 */
#define POISON_SIZE(s) (                                \
    (s) ?                                             \
    (UMM_POISON_SIZE_BEFORE + UMM_POISON_SIZE_AFTER + \
     sizeof(UMM_POISONED_BLOCK_LEN_TYPE)              \
    ) : 0                                             \
    )

/*
 * Print memory contents starting from given `ptr`
 */
static void dump_mem ( const unsigned char *ptr, size_t len ) {
  while (len--) {
    printf(" 0x%.2x", (unsigned int)(*ptr++));
  }
}

/*
 * Put poison data at given `ptr` and `poison_size`
 */
static void put_poison( unsigned char *ptr, size_t poison_size ) {
  memset(ptr, POISON_BYTE, poison_size);
}

/*
 * Check poison data at given `ptr` and `poison_size`. `where` is a pointer to
 * a string, either "before" or "after", meaning, before or after the block.
 *
 * If poison is there, returns 1.
 * Otherwise, prints the appropriate message, and returns 0.
 */
static int check_poison( const unsigned char *ptr, size_t poison_size,
    const char *where) {
  size_t i;
  int ok = 1;

  for (i = 0; i < poison_size; i++) {
    if (ptr[i] != POISON_BYTE) {
      ok = 0;
      break;
    }
  }

  if (!ok) {
    printf("there is no poison %s the block. "
        "Expected poison address: 0x%lx, actual data:",
        where, (unsigned long)ptr);
    dump_mem(ptr, poison_size);
    printf("\n");
  }

  return ok;
}

/*
 * Check if a block is properly poisoned. Must be called only for non-free
 * blocks.
 */
static int check_poison_block( umm_block *pblock ) {
  int ok = 1;

  if (pblock->header.used.next & UMM_FREELIST_MASK) {
    printf("check_poison_block is called for free block 0x%lx\n",
        (unsigned long)pblock);
  } else {
    /* the block is used; let's check poison */
    unsigned char *pc = (unsigned char *)pblock->body.data;
    unsigned char *pc_cur;

    pc_cur = pc + sizeof(UMM_POISONED_BLOCK_LEN_TYPE);
    if (!check_poison(pc_cur, UMM_POISON_SIZE_BEFORE, "before")) {
      UMM_HEAP_CORRUPTION_CB();
      ok = 0;
      goto clean;
    }

    pc_cur = pc + *((UMM_POISONED_BLOCK_LEN_TYPE *)pc) - UMM_POISON_SIZE_AFTER;
    if (!check_poison(pc_cur, UMM_POISON_SIZE_AFTER, "after")) {
      UMM_HEAP_CORRUPTION_CB();
      ok = 0;
      goto clean;
    }
  }

clean:
  return ok;
}

/*
 * Iterates through all blocks in the heap, and checks poison for all used
 * blocks.
 */
static int check_poison_all_blocks(void) {
  int ok = 1;
  unsigned short int blockNo = 0;

  if (umm_heap == NULL) {
    umm_init();
  }

  /* Now iterate through the blocks list */
  blockNo = UMM_NBLOCK(blockNo) & UMM_BLOCKNO_MASK;

  while( UMM_NBLOCK(blockNo) & UMM_BLOCKNO_MASK ) {
    if ( !(UMM_NBLOCK(blockNo) & UMM_FREELIST_MASK) ) {
      /* This is a used block (not free), so, check its poison */
      ok = check_poison_block(&UMM_BLOCK(blockNo));
      if (!ok){
        break;
      }
    }

    blockNo = UMM_NBLOCK(blockNo) & UMM_BLOCKNO_MASK;
  }

  return ok;
}

/*
 * Takes a pointer returned by actual allocator function (`_umm_malloc` or
 * `_umm_realloc`), puts appropriate poison, and returns adjusted pointer that
 * should be returned to the user.
 *
 * `size_w_poison` is a size of the whole block, including a poison.
 */
static void *get_poisoned( unsigned char *ptr, size_t size_w_poison ) {
  if (size_w_poison != 0 && ptr != NULL) {

    /* Put exact length of the user's chunk of memory */
    memcpy(ptr, &size_w_poison, sizeof(UMM_POISONED_BLOCK_LEN_TYPE));

    /* Poison beginning and the end of the allocated chunk */
    put_poison(ptr + sizeof(UMM_POISONED_BLOCK_LEN_TYPE),
        UMM_POISON_SIZE_BEFORE);
    put_poison(ptr + size_w_poison - UMM_POISON_SIZE_AFTER,
        UMM_POISON_SIZE_AFTER);

    /* Return pointer at the first non-poisoned byte */
    return ptr + sizeof(UMM_POISONED_BLOCK_LEN_TYPE) + UMM_POISON_SIZE_BEFORE;
  } else {
    return ptr;
  }
}

/*
 * Takes "poisoned" pointer (i.e. pointer returned from `get_poisoned()`),
 * and checks that the poison of this particular block is still there.
 *
 * Returns unpoisoned pointer, i.e. actual pointer to the allocated memory.
 */
static void *get_unpoisoned( unsigned char *ptr ) {
  if (ptr != NULL) {
    unsigned short int c;

    ptr -= (sizeof(UMM_POISONED_BLOCK_LEN_TYPE) + UMM_POISON_SIZE_BEFORE);

    /* Figure out which block we're in. Note the use of truncated division... */
    c = (((char *)ptr)-(char *)(&(umm_heap[0])))/sizeof(umm_block);

    check_poison_block(&UMM_BLOCK(c));
  }

  return ptr;
}

#define CHECK_POISON_ALL_BLOCKS() check_poison_all_blocks()
#define GET_POISONED(ptr, size)   get_poisoned(ptr, size)
#define GET_UNPOISONED(ptr)       get_unpoisoned(ptr)

#else
/*
 * Integrity check is disabled, so just define stub macros
 */
#define POISON_SIZE(s)            0
#define CHECK_POISON_ALL_BLOCKS() 1
#define GET_POISONED(ptr, size)   (ptr)
#define GET_UNPOISONED(ptr)       (ptr)
#endif
/* }}} */

/* ----------------------------------------------------------------------------
 * One of the coolest things about this little library is that it's VERY
 * easy to get debug information about the memory heap by simply iterating
 * through all of the memory blocks.
 *
 * As you go through all the blocks, you can check to see if it's a free
 * block by looking at the high order bit of the next block index. You can
 * also see how big the block is by subtracting the next block index from
 * the current block number.
 *
 * The umm_info function does all of that and makes the results available
 * in the ummHeapInfo structure.
 * ----------------------------------------------------------------------------
 */

UMM_HEAP_INFO ummHeapInfo;

void ICACHE_FLASH_ATTR *umm_info( void *ptr, int force ) {

  unsigned short int blockNo = 0;

  if (umm_heap == NULL) {
      umm_init();
  }

  /* Protect the critical section... */
  UMM_CRITICAL_ENTRY();

  /*
   * Clear out all of the entries in the ummHeapInfo structure before doing
   * any calculations..
   */
  memset( &ummHeapInfo, 0, sizeof( ummHeapInfo ) );

  DBG_LOG_FORCE( force, "\n\nDumping the umm_heap...\n" );

  DBG_LOG_FORCE( force, "|0x%08lx|B %5d|NB %5d|PB %5d|Z %5d|NF %5d|PF %5d|\n",
      (unsigned long)(&UMM_BLOCK(blockNo)),
      blockNo,
      UMM_NBLOCK(blockNo) & UMM_BLOCKNO_MASK,
      UMM_PBLOCK(blockNo),
      (UMM_NBLOCK(blockNo) & UMM_BLOCKNO_MASK )-blockNo,
      UMM_NFREE(blockNo),
      UMM_PFREE(blockNo) );

  /*
   * Now loop through the block lists, and keep track of the number and size
   * of used and free blocks. The terminating condition is an nb pointer with
   * a value of zero...
   */

  blockNo = UMM_NBLOCK(blockNo) & UMM_BLOCKNO_MASK;

  while( UMM_NBLOCK(blockNo) & UMM_BLOCKNO_MASK ) {
    size_t curBlocks = (UMM_NBLOCK(blockNo) & UMM_BLOCKNO_MASK )-blockNo;

    ++ummHeapInfo.totalEntries;
    ummHeapInfo.totalBlocks += curBlocks;

    /* Is this a free block? */

    if( UMM_NBLOCK(blockNo) & UMM_FREELIST_MASK ) {
      ++ummHeapInfo.freeEntries;
      ummHeapInfo.freeBlocks += curBlocks;

      if (ummHeapInfo.maxFreeContiguousBlocks < curBlocks) {
        ummHeapInfo.maxFreeContiguousBlocks = curBlocks;
      }

      DBG_LOG_FORCE( force, "|0x%08lx|B %5d|NB %5d|PB %5d|Z %5u|NF %5d|PF %5d|\n",
          (unsigned long)(&UMM_BLOCK(blockNo)),
          blockNo,
          UMM_NBLOCK(blockNo) & UMM_BLOCKNO_MASK,
          UMM_PBLOCK(blockNo),
          (unsigned int)curBlocks,
          UMM_NFREE(blockNo),
          UMM_PFREE(blockNo) );

      /* Does this block address match the ptr we may be trying to free? */

      if( ptr == &UMM_BLOCK(blockNo) ) {

        /* Release the critical section... */
        UMM_CRITICAL_EXIT();

        return( ptr );
      }
    } else {
      ++ummHeapInfo.usedEntries;
      ummHeapInfo.usedBlocks += curBlocks;

      DBG_LOG_FORCE( force, "|0x%08lx|B %5d|NB %5d|PB %5d|Z %5u|\n",
          (unsigned long)(&UMM_BLOCK(blockNo)),
          blockNo,
          UMM_NBLOCK(blockNo) & UMM_BLOCKNO_MASK,
          UMM_PBLOCK(blockNo),
          (unsigned int)curBlocks );
    }

    blockNo = UMM_NBLOCK(blockNo) & UMM_BLOCKNO_MASK;
  }

  /*
   * Update the accounting totals with information from the last block, the
   * rest must be free!
   */

  {
    size_t curBlocks = UMM_NUMBLOCKS-blockNo;
    ummHeapInfo.freeBlocks  += curBlocks;
    ummHeapInfo.totalBlocks += curBlocks;

    if (ummHeapInfo.maxFreeContiguousBlocks < curBlocks) {
      ummHeapInfo.maxFreeContiguousBlocks = curBlocks;
    }
  }

  DBG_LOG_FORCE( force, "|0x%08lx|B %5d|NB %5d|PB %5d|Z %5d|NF %5d|PF %5d|\n",
      (unsigned long)(&UMM_BLOCK(blockNo)),
      blockNo,
      UMM_NBLOCK(blockNo) & UMM_BLOCKNO_MASK,
      UMM_PBLOCK(blockNo),
      UMM_NUMBLOCKS-blockNo,
      UMM_NFREE(blockNo),
      UMM_PFREE(blockNo) );

  DBG_LOG_FORCE( force, "Total Entries %5d    Used Entries %5d    Free Entries %5d\n",
      ummHeapInfo.totalEntries,
      ummHeapInfo.usedEntries,
      ummHeapInfo.freeEntries );

  DBG_LOG_FORCE( force, "Total Blocks  %5d    Used Blocks  %5d    Free Blocks  %5d\n",
      ummHeapInfo.totalBlocks,
      ummHeapInfo.usedBlocks,
      ummHeapInfo.freeBlocks  );

  /* Release the critical section... */
  UMM_CRITICAL_EXIT();

  return( NULL );
}

/* ------------------------------------------------------------------------ */

static unsigned short int umm_blocks( size_t size ) {

  /*
   * The calculation of the block size is not too difficult, but there are
   * a few little things that we need to be mindful of.
   *
   * When a block removed from the free list, the space used by the free
   * pointers is available for data. That's what the first calculation
   * of size is doing.
   */

  if( size <= (sizeof(((umm_block *)0)->body)) )
    return( 1 );

  /*
   * If it's for more than that, then we need to figure out the number of
   * additional whole blocks the size of an umm_block are required.
   */

  size -= ( 1 + (sizeof(((umm_block *)0)->body)) );

  return( 2 + size/(sizeof(umm_block)) );
}

/* ------------------------------------------------------------------------ */

/*
 * Split the block `c` into two blocks: `c` and `c + blocks`.
 *
 * - `cur_freemask` should be `0` if `c` used, or `UMM_FREELIST_MASK`
 *   otherwise.
 * - `new_freemask` should be `0` if `c + blocks` used, or `UMM_FREELIST_MASK`
 *   otherwise.
 *
 * Note that free pointers are NOT modified by this function.
 */
static void umm_make_new_block( unsigned short int c,
    unsigned short int blocks,
    unsigned short int cur_freemask, unsigned short int new_freemask ) {

  UMM_NBLOCK(c+blocks) = (UMM_NBLOCK(c) & UMM_BLOCKNO_MASK) | new_freemask;
  UMM_PBLOCK(c+blocks) = c;

  UMM_PBLOCK(UMM_NBLOCK(c) & UMM_BLOCKNO_MASK) = (c+blocks);
  UMM_NBLOCK(c)                                = (c+blocks) | cur_freemask;
}

/* ------------------------------------------------------------------------ */

static void umm_disconnect_from_free_list( unsigned short int c ) {
  /* Disconnect this block from the FREE list */

  UMM_NFREE(UMM_PFREE(c)) = UMM_NFREE(c);
  UMM_PFREE(UMM_NFREE(c)) = UMM_PFREE(c);

  /* And clear the free block indicator */

  UMM_NBLOCK(c) &= (~UMM_FREELIST_MASK);
}

/* ------------------------------------------------------------------------ */

static void umm_assimilate_up( unsigned short int c ) {

  if( UMM_NBLOCK(UMM_NBLOCK(c)) & UMM_FREELIST_MASK ) {
    /*
     * The next block is a free block, so assimilate up and remove it from
     * the free list
     */

   // DBG_LOG_DEBUG( "Assimilate up to next block, which is FREE\n" );

    /* Disconnect the next block from the FREE list */

    umm_disconnect_from_free_list( UMM_NBLOCK(c) );

    /* Assimilate the next block with this one */

    UMM_PBLOCK(UMM_NBLOCK(UMM_NBLOCK(c)) & UMM_BLOCKNO_MASK) = c;
    UMM_NBLOCK(c) = UMM_NBLOCK(UMM_NBLOCK(c)) & UMM_BLOCKNO_MASK;
  }
}

/* ------------------------------------------------------------------------ */

static unsigned short int umm_assimilate_down( unsigned short int c, unsigned short int freemask ) {

  UMM_NBLOCK(UMM_PBLOCK(c)) = UMM_NBLOCK(c) | freemask;
  UMM_PBLOCK(UMM_NBLOCK(c)) = UMM_PBLOCK(c);

  return( UMM_PBLOCK(c) );
}

/* ------------------------------------------------------------------------- */

void umm_init( void ) {
  /* init heap pointer and size, and memset it to 0 */
  umm_heap = (umm_block *)UMM_MALLOC_CFG__HEAP_ADDR;
  umm_numblocks = (UMM_MALLOC_CFG__HEAP_SIZE / sizeof(umm_block));
  memset(umm_heap, 0x00, UMM_MALLOC_CFG__HEAP_SIZE);

  /* setup initial blank heap structure */
  {
    /* index of the 0th `umm_block` */
    const unsigned short int block_0th = 0;
    /* index of the 1st `umm_block` */
    const unsigned short int block_1th = 1;
    /* index of the latest `umm_block` */
    const unsigned short int block_last = UMM_NUMBLOCKS - 1;

    /* setup the 0th `umm_block`, which just points to the 1st */
    UMM_NBLOCK(block_0th) = block_1th;
    UMM_NFREE(block_0th)  = block_1th;

    /*
     * Now, we need to set the whole heap space as a huge free block. We should
     * not touch the 0th `umm_block`, since it's special: the 0th `umm_block`
     * is the head of the free block list. It's a part of the heap invariant.
     *
     * See the detailed explanation at the beginning of the file.
     */

    /*
     * 1th `umm_block` has pointers:
     *
     * - next `umm_block`: the latest one
     * - prev `umm_block`: the 0th
     *
     * Plus, it's a free `umm_block`, so we need to apply `UMM_FREELIST_MASK`
     *
     * And it's the last free block, so the next free block is 0.
     */
    UMM_NBLOCK(block_1th) = block_last | UMM_FREELIST_MASK;
    UMM_NFREE(block_1th)  = 0;
    UMM_PBLOCK(block_1th) = block_0th;
    UMM_PFREE(block_1th)  = block_0th;

    /*
     * latest `umm_block` has pointers:
     *
     * - next `umm_block`: 0 (meaning, there are no more `umm_blocks`)
     * - prev `umm_block`: the 1st
     *
     * It's not a free block, so we don't touch NFREE / PFREE at all.
     */
    UMM_NBLOCK(block_last) = 0;
    UMM_PBLOCK(block_last) = block_1th;
  }
}

/* ------------------------------------------------------------------------ */

static void _umm_free( void *ptr ) {

  unsigned short int c;

  /* If we're being asked to free a NULL pointer, well that's just silly! */

  if( (void *)0 == ptr ) {
    DBG_LOG_DEBUG( "MEM: free NULL -> nop\n" );

    return;
  }

  /*
   * FIXME: At some point it might be a good idea to add a check to make sure
   *        that the pointer we're being asked to free up is actually within
   *        the umm_heap!
   *
   * NOTE:  See the new umm_info() function that you can use to see if a ptr is
   *        on the free list!
   */

  /* Protect the critical section... */
  UMM_CRITICAL_ENTRY();

  /* Figure out which block we're in. Note the use of truncated division... */

  c = (((char *)ptr)-(char *)(&(umm_heap[0])))/sizeof(umm_block);

  DBG_MEM_OP( "#F %d %d %x", gTotalHeapOp, c, (uint32_t)ptr );
  DBG_LOG_DEBUG( "MEM[%d]: Free block %d ptr %x", gTotalHeapOp, c, (uint32_t)ptr );

  /* Now let's assimilate this block with the next one if possible. */

  umm_assimilate_up( c );

  /* Then assimilate with the previous block if possible */

  if( UMM_NBLOCK(UMM_PBLOCK(c)) & UMM_FREELIST_MASK ) {

	DBG_MEM_OP( " down\n");
    DBG_LOG_DEBUG( "- Assim down next block\n" );

    c = umm_assimilate_down(c, UMM_FREELIST_MASK);
  } else {
    /*
     * The previous block is not a free block, so add this one to the head
     * of the free list
     */

	  DBG_MEM_OP( " up\n");
    DBG_LOG_DEBUG( "- Add head free list\n" );

    UMM_PFREE(UMM_NFREE(0)) = c;
    UMM_NFREE(c)            = UMM_NFREE(0);
    UMM_PFREE(c)            = 0;
    UMM_NFREE(0)            = c;

    UMM_NBLOCK(c)          |= UMM_FREELIST_MASK;
  }

#if 0
  /*
   * The following is experimental code that checks to see if the block we just
   * freed can be assimilated with the very last block - it's pretty convoluted in
   * terms of block index manipulation, and has absolutely no effect on heap
   * fragmentation. I'm not sure that it's worth including but I've left it
   * here for posterity.
   */

  if( 0 == UMM_NBLOCK(UMM_NBLOCK(c) & UMM_BLOCKNO_MASK ) ) {

    if( UMM_PBLOCK(UMM_NBLOCK(c) & UMM_BLOCKNO_MASK) != UMM_PFREE(UMM_NBLOCK(c) & UMM_BLOCKNO_MASK) ) {
      UMM_NFREE(UMM_PFREE(UMM_NBLOCK(c) & UMM_BLOCKNO_MASK)) = c;
      UMM_NFREE(UMM_PFREE(c))                                = UMM_NFREE(c);
      UMM_PFREE(UMM_NFREE(c))                                = UMM_PFREE(c);
      UMM_PFREE(c)                                           = UMM_PFREE(UMM_NBLOCK(c) & UMM_BLOCKNO_MASK);
    }

    UMM_NFREE(c)  = 0;
    UMM_NBLOCK(c) = 0;
  }
#endif

  /* Release the critical section... */
  UMM_CRITICAL_EXIT();

  recordHeapOp('f', 0, (uint32_t)ptr, 0);
}

/* ------------------------------------------------------------------------ */

static void *_umm_malloc( size_t size ) {
  unsigned short int blocks;
  unsigned short int blockSize = 0;

  unsigned short int bestSize;
  unsigned short int bestBlock;

  unsigned short int cf;

  if (umm_heap == NULL) {
    umm_init();
  }

  /*
   * the very first thing we do is figure out if we're being asked to allocate
   * a size of 0 - and if we are we'll simply return a null pointer. if not
   * then reduce the size by 1 byte so that the subsequent calculations on
   * the number of blocks to allocate are easier...
   */

  if( 0 == size ) {
    DBG_LOG_DEBUG( "MEM: Alo sz 0\n" );

    return( (void *)NULL );
  }

  /* Protect the critical section... */
  UMM_CRITICAL_ENTRY();

  blocks = umm_blocks( size );

  /*
   * Now we can scan through the free list until we find a space that's big
   * enough to hold the number of blocks we need.
   *
   * This part may be customized to be a best-fit, worst-fit, or first-fit
   * algorithm
   */

  cf = UMM_NFREE(0);

  bestBlock = UMM_NFREE(0);
  bestSize  = 0x7FFF;
  DBG_MEM_OP( "#M %d>", gTotalHeapOp);
  DBG_LOG_DEBUG( "MEM[%d]: malloc SRCH: ",gTotalHeapOp );
  while( cf ) {
    blockSize = (UMM_NBLOCK(cf) & UMM_BLOCKNO_MASK) - cf;

    DBG_MEM_OP( "%d/%d ", cf, blockSize);
    DBG_LOG_TRACE( "[%d]=%d ", cf, blockSize );

#if defined UMM_FIRST_FIT
    /* This is the first block that fits! */
    if( (blockSize >= blocks) )
      break;
#elif defined UMM_BEST_FIT
    if( (blockSize >= blocks) && (blockSize < bestSize) ) {
      bestBlock = cf;
      bestSize  = blockSize;
    }
#endif

    cf = UMM_NFREE(cf);
  }

  if( 0x7FFF != bestSize ) {
    cf        = bestBlock;
    blockSize = bestSize;
  }

  if( UMM_NBLOCK(cf) & UMM_BLOCKNO_MASK && blockSize >= blocks ) {
    /*
     * This is an existing block in the memory heap, we just need to split off
     * what we need, unlink it from the free list and mark it as in use, and
     * link the rest of the block back into the freelist as if it was a new
     * block on the free list...
     */

    if( blockSize == blocks ) {
      /* It's an exact fit and we don't neet to split off a block. */
      DBG_MEM_OP( "!X %d %d %x\n", cf, blocks, (uint32_t)&UMM_DATA(cf));
      DBG_LOG_DEBUG( "- XACT %d(+%d)=> %x\n", cf, blocks, (uint32_t)&UMM_DATA(cf));

      /* Disconnect this block from the FREE list */

      umm_disconnect_from_free_list( cf );

    } else {
    	DBG_MEM_OP( "!E %d %d %x\n", cf, blocks, (uint32_t)&UMM_DATA(cf));
      /* It's not an exact fit and we need to split off a block. */
    	DBG_LOG_DEBUG( "- xist %d(+%d)=> %x \n", cf, blocks, (uint32_t)&UMM_DATA(cf));

      /*
       * split current free block `cf` into two blocks. The first one will be
       * returned to user, so it's not free, and the second one will be free.
       */
      umm_make_new_block( cf, blocks,
          0/*`cf` is not free*/,
          UMM_FREELIST_MASK/*new block is free*/);

      /*
       * `umm_make_new_block()` does not update the free pointers (it affects
       * only free flags), but effectively we've just moved beginning of the
       * free block from `cf` to `cf + blocks`. So we have to adjust pointers
       * to and from adjacent free blocks.
       */

      /* previous free block */
      UMM_NFREE( UMM_PFREE(cf) ) = cf + blocks;
      UMM_PFREE( cf + blocks ) = UMM_PFREE(cf);

      /* next free block */
      UMM_PFREE( UMM_NFREE(cf) ) = cf + blocks;
      UMM_NFREE( cf + blocks ) = UMM_NFREE(cf);
    }
  } else {
    /* Out of memory */
	DBG_MEM_OP( "NO %d\n", blocks);
    DBG_LOG_DEBUG(  "- NO alloc sz %d\n", blocks );

    /* Release the critical section... */
    UMM_CRITICAL_EXIT();

    return( (void *)NULL );
  }

  /* Release the critical section... */
  UMM_CRITICAL_EXIT();

  recordHeapOp('m', size, (uint32_t)UMM_DATA(cf), 0);
  return( (void *)&UMM_DATA(cf) );
}

/* ------------------------------------------------------------------------ */

static void *_umm_realloc( void *ptr, size_t size ) {

  unsigned short int blocks;
  unsigned short int blockSize;

  unsigned short int c;

  size_t curSize;

  if (umm_heap == NULL) {
    umm_init();
  }

  /*
   * This code looks after the case of a NULL value for ptr. The ANSI C
   * standard says that if ptr is NULL and size is non-zero, then we've
   * got to work the same a malloc(). If size is also 0, then our version
   * of malloc() returns a NULL pointer, which is OK as far as the ANSI C
   * standard is concerned.
   */

  if( ((void *)NULL == ptr) ) {
    DBG_LOG_DEBUG( "MEM: realloc NULL => malloc\n" );
    return( _umm_malloc(size) );
  }

  /*
   * Now we're sure that we have a non_NULL ptr, but we're not sure what
   * we should do with it. If the size is 0, then the ANSI C standard says that
   * we should operate the same as free.
   */

  if( 0 == size ) {
    DBG_LOG_DEBUG( "MEM: realloc sz 0, => free\n" );
    _umm_free( ptr );
    return( (void *)NULL );
  }

  /* Protect the critical section... */
  UMM_CRITICAL_ENTRY();

  /*
   * Otherwise we need to actually do a reallocation. A naiive approach
   * would be to malloc() a new block of the correct size, copy the old data
   * to the new block, and then free the old block.
   *
   * While this will work, we end up doing a lot of possibly unnecessary
   * copying. So first, let's figure out how many blocks we'll need.
   */

  blocks = umm_blocks( size );

  /* Figure out which block we're in. Note the use of truncated division... */

  c = (((char *)ptr)-(char *)(&(umm_heap[0])))/sizeof(umm_block);

  /* Figure out how big this block is... */

  blockSize = (UMM_NBLOCK(c) - c);

  /* Figure out how many bytes are in this block */

  curSize   = (blockSize*sizeof(umm_block))-(sizeof(((umm_block *)0)->header));

  /*
   * Ok, now that we're here, we know the block number of the original chunk
   * of memory, and we know how much new memory we want, and we know the original
   * block size...
   */

  if( blockSize == blocks ) {
    /* This space intentionally left blank - return the original pointer! */

    DBG_LOG_DEBUG( "MEM[%d]: realloc same sz %d =>nop\n", gTotalHeapOp, blocks );

    /* Release the critical section... */
    UMM_CRITICAL_EXIT();

    return( ptr );
  }

  /*
   * Now we have a block size that could be bigger or smaller. Either
   * way, try to assimilate up to the next block before doing anything...
   *
   * If it's still too small, we have to free it anyways and it will save the
   * assimilation step later in free :-)
   */

  umm_assimilate_up( c );

  /*
   * Now check if it might help to assimilate down, but don't actually
   * do the downward assimilation unless the resulting block will hold the
   * new request! If this block of code runs, then the new block will
   * either fit the request exactly, or be larger than the request.
   */

  if( (UMM_NBLOCK(UMM_PBLOCK(c)) & UMM_FREELIST_MASK) &&
      (blocks <= (UMM_NBLOCK(c)-UMM_PBLOCK(c)))    ) {

    /* Check if the resulting block would be big enough... */

    DBG_LOG_DEBUG( "MEM[%d]: realloc() assim down sz %d - fits!\n", gTotalHeapOp, c-UMM_PBLOCK(c) );

    /* Disconnect the previous block from the FREE list */

    umm_disconnect_from_free_list( UMM_PBLOCK(c) );

    /*
     * Connect the previous block to the next block ... and then
     * realign the current block pointer
     */

    c = umm_assimilate_down(c, 0);

    /*
     * Move the bytes down to the new block we just created, but be sure to move
     * only the original bytes.
     */

    memmove( (void *)&UMM_DATA(c), ptr, curSize );

    /* And don't forget to adjust the pointer to the new block location! */

    ptr    = (void *)&UMM_DATA(c);
  }

  /* Now calculate the block size again...and we'll have three cases */

  blockSize = (UMM_NBLOCK(c) - c);

  if( blockSize == blocks ) {
    /* This space intentionally left blank - return the original pointer! */

    DBG_LOG_DEBUG( "MEM[%d]: realloc the same size block - %d, do nothing\n", gTotalHeapOp, blocks );

  } else if (blockSize > blocks ) {
    /*
     * New block is smaller than the old block, so just make a new block
     * at the end of this one and put it up on the free list...
     */

    DBG_LOG_DEBUG( "MEM[%d]: realloc %d -> %d, shrink + free\n", gTotalHeapOp, blockSize, blocks );

    umm_make_new_block( c, blocks, 0, 0 );
    _umm_free( (void *)&UMM_DATA(c+blocks) );
  } else {
    /* New block is bigger than the old block... */

    void *oldptr = ptr;

    DBG_LOG_DEBUG( "MEM[%d]: realloc %d -> %d, new, copy, free\n", gTotalHeapOp, blockSize, blocks );

    /*
     * Now _umm_malloc() a new/ one, copy the old data to the new block, and
     * free up the old block, but only if the malloc was sucessful!
     */

    if( (ptr = _umm_malloc( size )) ) {
      memcpy( ptr, oldptr, curSize );
      _umm_free( oldptr );
    }

  }

  /* Release the critical section... */
  UMM_CRITICAL_EXIT();

  return( ptr );
}

/* ------------------------------------------------------------------------ */

void *umm_malloc( size_t size ) {
  void *ret;

  /* check poison of each blocks, if poisoning is enabled */
  if (!CHECK_POISON_ALL_BLOCKS()) {
    return NULL;
  }

  /* check full integrity of the heap, if this check is enabled */
  if (!INTEGRITY_CHECK()) {
    return NULL;
  }

  size += POISON_SIZE(size);

  ret = _umm_malloc( size );

  ret = GET_POISONED(ret, size);

  return ret;
}

/* ------------------------------------------------------------------------ */

void *umm_calloc( size_t num, size_t item_size ) {
  void *ret;
  size_t size = item_size * num;

  /* check poison of each blocks, if poisoning is enabled */
  if (!CHECK_POISON_ALL_BLOCKS()) {
    return NULL;
  }

  /* check full integrity of the heap, if this check is enabled */
  if (!INTEGRITY_CHECK()) {
    return NULL;
  }

  size += POISON_SIZE(size);
  ret = _umm_malloc(size);
  memset(ret, 0x00, size);

  ret = GET_POISONED(ret, size);

  return ret;
}

/* ------------------------------------------------------------------------ */

void *umm_realloc( void *ptr, size_t size ) {
  void *ret;

  ptr = GET_UNPOISONED(ptr);

  /* check poison of each blocks, if poisoning is enabled */
  if (!CHECK_POISON_ALL_BLOCKS()) {
    return NULL;
  }

  /* check full integrity of the heap, if this check is enabled */
  if (!INTEGRITY_CHECK()) {
    return NULL;
  }

  size += POISON_SIZE(size);
  ret = _umm_realloc( ptr, size );

  ret = GET_POISONED(ret, size);

  return ret;
}

/* ------------------------------------------------------------------------ */

void umm_free( void *ptr ) {

  ptr = GET_UNPOISONED(ptr);

  /* check poison of each blocks, if poisoning is enabled */
  if (!CHECK_POISON_ALL_BLOCKS()) {
    return;
  }

  /* check full integrity of the heap, if this check is enabled */
  if (!INTEGRITY_CHECK()) {
    return;
  }

  _umm_free( ptr );
}

/* ------------------------------------------------------------------------ */

size_t ICACHE_FLASH_ATTR umm_free_heap_size( void ) {
  umm_info(NULL, 0);
  return (size_t)ummHeapInfo.freeBlocks * sizeof(umm_block);
}

/* ------------------------------------------------------------------------ */
