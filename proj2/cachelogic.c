#include "tips.h"
#include "util.h"

/* The following two functions are defined in util.c */

/* finds the highest 1 bit, and returns its position, else 0xFFFFFFFF */
unsigned int uint_log2(word w); 

/* return random int from 0..x-1 */
int randomint( int x );

/*
  This function allows the lfu information to be displayed

    assoc_index - the cache unit that contains the block to be modified
    block_index - the index of the block to be modified

  returns a string representation of the lfu information
 */
char* lfu_to_string(int assoc_index, int block_index)
{
  /* Buffer to print lfu information -- increase size as needed. */
  static char buffer[9];
  sprintf(buffer, "%u", cache[assoc_index].block[block_index].accessCount);

  return buffer;
}

/*
  This function allows the lru information to be displayed

    assoc_index - the cache unit that contains the block to be modified
    block_index - the index of the block to be modified

  returns a string representation of the lru information
 */
char* lru_to_string(int assoc_index, int block_index)
{
  /* Buffer to print lru information -- increase size as needed. */
  static char buffer[9];
  sprintf(buffer, "%u", cache[assoc_index].block[block_index].lru.value);

  return buffer;
}

/*
  This function initializes the lfu information

    assoc_index - the cache unit that contains the block to be modified
    block_number - the index of the block to be modified

*/
void init_lfu(int assoc_index, int block_index)
{
  cache[assoc_index].block[block_index].accessCount = 0;
}

/*
  This function initializes the lru information

    assoc_index - the cache unit that contains the block to be modified
    block_number - the index of the block to be modified

*/
void init_lru(int assoc_index, int block_index)
{
  cache[assoc_index].block[block_index].lru.value = 0;
}

/*
  This is the primary function you are filling out,
  You are free to add helper functions if you need them

  @param addr 32-bit byte address
  @param data a pointer to a SINGLE word (32-bits of data)
  @param we   if we == READ, then data used to return
              information back to CPU

              if we == WRITE, then data used to
              update Cache/DRAM
*/
int findReplacementBlock(unsigned int index){
  unsigned int foundBlock = 0;
  if(policy == LRU){
    unsigned int highLRU = 0; //will be used to track highest LRU value
    /*highest LRU will be the block that has gone the longest w/o being chosen*/
    for(int i = 0; i < assoc; i++){ //check all blocks within a set
        if(/*cache[index].block[i].lru.value > 0 &&*/ 
          cache[index].block[highLRU].lru.value < cache[index].block[i].lru.value){
          highLRU = i;
        }
    }
    foundBlock = highLRU; 
    /* set the chosen block LRU value to zero and increment the rest*/
    /*
    for(int i = 0; i < assoc; i++){
        if(i == foundBlock)
          cache[index].block[i].lru.value = 0;
        else
          cache[index].block[i].lru.value++; 
    }*///every other block is selected incorrectly this way, move this somewhere
  }

  if(policy == LFU){
    unsigned int lowLFU = 0;
    for(int i = 0; i < assoc; i++){
      if(cache[index].block[i].accessCount > 0 && 
        cache[index].block[i].accessCount < cache[index].block[lowLFU].accessCount)
        lowLFU = i;
    }
    foundBlock = lowLFU; 
    cache[index].block[foundBlock].accessCount++; //increment block after its chosen
  }
  
  if(policy == RANDOM){
    foundBlock = randomint(assoc);
  }

  return foundBlock;
}

void resetBlock(int index, int block, int valid, int dirty, unsigned int tag){
      cache[index].block[block].valid = valid;
      cache[index].block[block].dirty = dirty;
      cache[index].block[block].tag = tag;
      cache[index].block[block].lru.value = 0;
}

void resolveDirtyBit(int index, int block, address addr, int indexSize, int offsetSize, TransferUnit mode){
      addr = cache[index].block[block].tag << ((indexSize + offsetSize) + (index << offsetSize));
      accessDRAM(addr, cache[index].block[block].data, mode, WRITE);
}

void accessMemory(address addr, word* data, WriteEnable we)
{
  /* Declare variables here */
  unsigned int indexBits, offsetBits; //refer to physical number of bits
  indexBits = uint_log2(set_count); //index bits: log2(set_count) = indexBits 
  offsetBits = uint_log2(block_size); //offset bits: log2(block_size) = offsetBits

  TransferUnit mode = uint_log2(block_size); //size of data being moved
  unsigned int index, offset, tag; //refer to actual values
  index = (addr >> offsetBits) & ((1 << indexBits) - 1);
  offset = addr & ((1 << offsetBits) - 1);
  tag = addr >> (offsetBits + indexBits);

  unsigned int foundBlock; //block that we are currently working with in cache
  CacheAction cacheAction = MISS;

/*int accessDRAM(address addr, byte* data, TransferUnit mode, WriteEnable flag);
  void highlight_offset(unsigned int set_num, unsigned int assoc_num, unsigned int offset, CacheAction action);
  void highlight_block(unsigned int set_num, unsigned int assoc_num); */
  /* handle the case of no cache at all - leave this in */
  if(assoc == 0) {
    accessDRAM(addr, (byte*)data, WORD_SIZE, we);
    return;
  }

  /*
  You need to read/write between memory (via the accessDRAM() function) and
  the cache (via the cache[] global structure defined in tips.h)

  Remember to read tips.h for all the global variables that tell you the
  cache parameters

  The same code should handle random, LFU, and LRU policies. Test the policy
  variable (see tips.h) to decide which policy to execute. The LRU policy
  should be written such that no two blocks (when their valid bit is VALID)
  will ever be a candidate for replacement. In the case of a tie in the
  least number of accesses for LFU, you use the LRU information to determine
  which block to replace.

  Your cache should be able to support write-through mode (any writes to
  the cache get immediately copied to main memory also) and write-back mode
  (and writes to the cache only gets copied to main memory when the block
  is kicked out of the cache.

  Also, cache should do allocate-on-write. This means, a write operation
  will bring in an entire block if the block is not already in the cache.

  To properly work with the GUI, the code needs to tell the GUI code
  when to redraw and when to flash things. Descriptions of the animation
  functions can be found in tips.h
  */

  /* Start adding code here */
for(int i = 0; i < assoc; i++){
  /*if tag = cacheTag and is valid*/
  if(tag == cache[index].block[i].tag && cache[index].block[i].valid == VALID){
    cacheAction = HIT;
    // highlight_offset(index, foundBlock, offset, cacheAction);
    memcpy(data, cache[index].block[i].data + offset, mode);
    break;
  }
}

foundBlock = findReplacementBlock(index);
unsigned int blockBefore = foundBlock;
if(we == READ){
  if(cacheAction == MISS){
    if(cache[index].block[foundBlock].dirty == DIRTY){
      resolveDirtyBit(index, foundBlock, addr, indexBits, offsetBits, mode);
    }
      accessDRAM(addr, cache[index].block[foundBlock].data, mode, READ);
      resetBlock(index, foundBlock, 1, 0, tag);// valid = VALID, dirty = VIRGIN, cacheTag = tag
      memcpy(data, cache[index].block[foundBlock].data + offset, block_size);
  }
}

if(we == WRITE){
  if(memory_sync_policy == WRITE_BACK){
      if(cache[index].block[foundBlock].dirty == DIRTY){
        resolveDirtyBit(index, foundBlock, addr, indexBits, offsetBits, mode);
      }
      resetBlock(index, foundBlock, 1, 0, tag);// valid = VALID, dirty = VIRGIN, cacheTag = tag
      accessDRAM(addr, cache[index].block[foundBlock].data, mode, READ);
      memcpy(cache[index].block[foundBlock].data + offset, data, block_size);
    }
  }
  else if (memory_sync_policy == WRITE_THROUGH){
    if(cacheAction == HIT){
      memcpy(cache[index].block[foundBlock].data + offset, data, block_size);
      resetBlock(index, foundBlock, 1, 0, cache[index].block[foundBlock].tag); //dont change tag here
      accessDRAM(addr, cache[index].block[foundBlock].data, mode, WRITE);
    }
    else{ 
    memcpy(cache[index].block[foundBlock].data + offset, data, block_size);
    resetBlock(index, foundBlock, 1, 0, tag);
    accessDRAM(addr, cache[index].block[foundBlock].data, mode, READ);
    }
  }
    /*unsigned int blockAfter = foundBlock;
    if(blockBefore == blockAfter){
      printf("works Right \n");
    }else printf("no work right\n");*/
    for(int i = 0; i < assoc; i++){
      cache[index].block[i].lru.value++; 
    }
    /*highlight offset where miss occurred and draw a box around the block*/
    if(cacheAction == MISS){
     highlight_block(index, foundBlock);
    }
    highlight_offset(index, foundBlock, offset, cacheAction);
  /* This call to accessDRAM occurs when you modify any of the
     cache parameters. It is provided as a stop gap solution.
     At some point, ONCE YOU HAVE MORE OF YOUR CACHELOGIC IN PLACE,
     THIS LINE SHOULD BE REMOVED.
  */
 // accessDRAM(addr, (byte*)data, WORD_SIZE, we);
}
