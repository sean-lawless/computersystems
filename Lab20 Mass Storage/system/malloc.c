/*...................................................................*/
/*                                                                   */
/*   Module:  malloc.c                                               */
/*   Version: 2018.0                                                 */
/*   Purpose: Heap based memory allocation                           */
/*                                                                   */
/*...................................................................*/
/*                                                                   */
/*                   Copyright 2018, Sean Lawless                    */
/*                                                                   */
/*                      ALL RIGHTS RESERVED                          */
/*                                                                   */
/* Redistribution and use in source, binary or derived forms, with   */
/* or without modification, are permitted provided that the          */
/* following conditions are met:                                     */
/*                                                                   */
/*  1. Redistributions in any form, including but not limited to     */
/*     source code, binary, or derived works, must include the above */
/*     copyright notice, this list of conditions and the following   */
/*     disclaimer.                                                   */
/*                                                                   */
/*  2. Any change or addition to this copyright notice requires the  */
/*     prior written permission of the above copyright holder.       */
/*                                                                   */
/* THIS SOFTWARE IS PROVIDED ''AS IS''. ANY EXPRESS OR IMPLIED       */
/* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES */
/* OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE       */
/* DISCLAIMED. IN NO EVENT SHALL ANY AUTHOR AND/OR COPYRIGHT HOLDER  */
/* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,          */
/* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED   */
/* TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,     */
/* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON */
/* ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,   */
/* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY    */
/* OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE           */
/* POSSIBILITY OF SUCH DAMAGE.                                       */
/*...................................................................*/
#include <malloc.h>
#include <board.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

#if ENABLE_MALLOC

/*...................................................................*/
/* Symbol Definitions                                                */
/*...................................................................*/
#define BLOCK_ALIGN 4
#define BLOCK_TAG   0x900DF00D
#define NUM_BLOCKS  8

/*...................................................................*/
/* Type Definitions                                                  */
/*...................................................................*/
typedef struct BlockHeader
{
  u32  tag;
  u32  size;
  struct BlockHeader *next;
  u8 data[0];
} __attribute__((packed)) BlockHeader;

typedef struct Block
{
  u32  size;
  BlockHeader  *freeList;
} Block;

/*...................................................................*/
/* Global Variables                                                  */
/*...................................................................*/
static u8 *NextBlock;
static u8 *BlockLimit;
static Block Blocks[NUM_BLOCKS];

/*...................................................................*/
/* Global Functions                                                  */
/*...................................................................*/

/*...................................................................*/
/* MallocInit: Initialize the heap memory allocation (malloc)        */
/*                                                                   */
/*      Input: base is the pointer to the start of the heap          */
/*             size is the size of the heap                          */
/*...................................................................*/
void MallocInit(uintptr_t base, u32 size)
{
  // Initialize the blocks with sizes
  bzero(Blocks, sizeof(Block) * NUM_BLOCKS);
  Blocks[0].size = 0x0040;
  Blocks[1].size = 0x0080;
  Blocks[2].size = 0x0100;
  Blocks[3].size = 0x0400;
  Blocks[4].size = 0x0800;
  Blocks[5].size = 0x1000;
  Blocks[6].size = 0x4000;
  Blocks[7].size = 0x8000;

  // Initialize the next block and limit
  if (base < MEM_HEAP_START)
    base = MEM_HEAP_START;
  NextBlock = (u8 *)base;
  BlockLimit = (u8 *)(base + size);
}

/*...................................................................*/
/* MallocInit: Initialize the heap memory allocation (malloc)        */
/*                                                                   */
/*    Returns: size of the remaining heap                            */
/*...................................................................*/
u32 MallocRemaining(void)
{
  return (u32)(BlockLimit - NextBlock);
}

/*...................................................................*/
/*     malloc: Allocate a portion of memory from the heap            */
/*                                                                   */
/*      Input: size is the size of memory to allocate                */
/*                                                                   */
/*    Returns: a pointer to the allocated memory, or NULL if error   */
/*...................................................................*/
void *malloc(u32 size)
{
  BlockHeader *blockHeader = NULL;
  Block *block;
  int i;

  assert(NextBlock != 0);

  // Find a blob big enough for this size allocation
  for (i = 0; i < NUM_BLOCKS; ++i)
  {
    block = &Blocks[i];
    if (size <= block->size)
    {
      size = block->size;
      blockHeader = block->freeList;
      break;
    }
  }

  // If a valid block large enough found
  if (blockHeader && (block->size > 0))
  {
    // Check if the block header tag is valid
    if (blockHeader->tag != BLOCK_TAG)
    {
      puts("Malloc corruption detected.");
      assert(0);
      return NULL;
    }

    // Update the block free list to the next block
    block->freeList = blockHeader->next;
  }
  else
  {
    blockHeader = (BlockHeader *)NextBlock;
    NextBlock += (sizeof(BlockHeader) + size + BLOCK_ALIGN - 1) &
                                                    ~(BLOCK_ALIGN - 1);
    if (NextBlock > BlockLimit)
    {
      assert(0);
      return NULL;
    }

    blockHeader->tag = BLOCK_TAG;
    blockHeader->size = size;
  }

  // Clear next to remove from linked list
  blockHeader->next = NULL;

  // Return the blocks data pointer
  assert(((u32)blockHeader->data & (BLOCK_ALIGN - 1)) == 0);
  return blockHeader->data;
}

/*...................................................................*/
/*       free: Free, or deallocate, a previously allocated pointer   */
/*                                                                   */
/*      Input: ptr is a pointer previously allocated by malloc       */
/*...................................................................*/
void free(void *ptr)
{
  BlockHeader *blockHeader;
  Block *block;
  int i;

  assert (ptr != 0);

  // Calculate the start of the block header from the data pointer
  blockHeader = (BlockHeader *)((uintptr_t)ptr - sizeof(BlockHeader));

  // Use the block tag to verify that this is a valid block
  if (blockHeader->tag != BLOCK_TAG)
  {
    puts("Free with invalid pointer, tag mismatch");
    assert(0);
    return;
  }

  // Loop through all the blocks until we find a matching size
  for (i = 0; i < NUM_BLOCKS; ++i)
  {
    block = &Blocks[i];
    if (blockHeader->size == block->size)
    {
      // Free the block by putting it back on the free list
      blockHeader->next = block->freeList;
      block->freeList = blockHeader;
      break;
    }
  }
}

#endif /* ENABLE_MALLOC */
