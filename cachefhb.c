/*
 * EECS 370, University of Michigan
 * Project 4: LC-2K Cache Simulator
 * Instructions are found in the project spec.
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_CACHE_SIZE 256
#define MAX_BLOCK_SIZE 256

// **Note** this is a preprocessor macro. This is not the same as a function.
// Powers of 2 have exactly one 1 and the rest 0's, and 0 isn't a power of 2.
#define is_power_of_2(val) (val && !(val & (val - 1)))


/*
 * Accesses 1 word of memory.
 * addr is a 16-bit LC2K word address.
 * write_flag is 0 for reads and 1 for writes.
 * write_data is a word, and is only valid if write_flag is 1.
 * If write flag is 1, mem_access does: state.mem[addr] = write_data.
 * The return of mem_access is state.mem[addr].
 */
extern int mem_access(int addr, int write_flag, int write_data);

/*
 * Returns the number of times mem_access has been called.
 */
extern int get_num_mem_accesses(void);

//Use this when calling printAction. Do not modify the enumerated type below.
enum actionType
{
    cacheToProcessor,
    processorToCache,
    memoryToCache,
    cacheToMemory,
    cacheToNowhere
};

/* You may add or remove variables from these structs */
typedef struct blockStruct
{
    int data[MAX_BLOCK_SIZE];
    int dirty;
    int lruLabel;
    int tag;
    int valid;
} blockStruct;

typedef struct cacheStruct
{
    blockStruct blocks[MAX_CACHE_SIZE];
    int blockSize;
    int numSets;
    int blocksPerSet;
    // add any variables for end-of-run stats
} cacheStruct;

/* Global Cache variable */
cacheStruct cache;

void printAction(int, int, enum actionType);
void printCache(void);

/*
 * Set up the cache with given command line parameters. This is
 * called once in main(). You must implement this function.
 */
void cache_init(int blockSize, int numSets, int blocksPerSet)
{
    if (blockSize <= 0 || numSets <= 0 || blocksPerSet <= 0) {
        printf("error: input parameters must be positive numbers\n");
        exit(1);
    }
    if (blocksPerSet * numSets > MAX_CACHE_SIZE) {
        printf("error: cache must be no larger than %d blocks\n", MAX_CACHE_SIZE);
        exit(1);
    }
    if (blockSize > MAX_BLOCK_SIZE) {
        printf("error: blocks must be no larger than %d words\n", MAX_BLOCK_SIZE);
        exit(1);
    }
    if (!is_power_of_2(blockSize)) {
        printf("warning: blockSize %d is not a power of 2\n", blockSize);
    }
    if (!is_power_of_2(numSets)) {
        printf("warning: numSets %d is not a power of 2\n", numSets);
    }

    printf("Simulating a cache with %d total lines; each line has %d words\n",
        numSets * blocksPerSet, blockSize);

    printf("Each set in the cache contains %d lines; there are %d sets\n",
        blocksPerSet, numSets);

    /********************* Initialize Cache *********************/

    cache.blockSize = blockSize;
    cache.numSets = numSets;
    cache.blocksPerSet = blocksPerSet;
    for(int i = 0; i < numSets * blocksPerSet; i++) {
        cache.blocks[i].dirty = 0;
        cache.blocks[i].lruLabel = 0;
        cache.blocks[i].valid = 0;
        cache.blocks[i].tag = 0;
        for(int j = 0; j < blockSize; j++) {cache.blocks[i].data[j] = 0;}
    }
    return;
}

/*
 * Access the cache. This is the main part of the project,
 * and should call printAction as is appropriate.
 * It should only call mem_access when absolutely necessary.
 * addr is a 16-bit LC2K word address.
 * write_flag is 0 for reads (fetch/lw) and 1 for writes (sw).
 * write_data is a word, and is only valid if write_flag is 1.
 * The return of mem_access is undefined if write_flag is 1.
 * Thus the return of cache_access is undefined if write_flag is 1.
 */
int cache_access(int addr, int write_flag, int write_data)
{
    int block_offset_bits = floor(log2(cache.blockSize));
    int set_index_bits = floor(log2(cache.numSets));
    int bo = addr&((1 << block_offset_bits) - 1);
    int si = (addr>>block_offset_bits)&((1<<set_index_bits)-1);
    int tag = addr>>(block_offset_bits+set_index_bits);

    int i = 0,temp = 0,iffind = 0;

    for(i = cache.blocksPerSet*si; i < cache.blocksPerSet*(si+1); i++) {
        if(cache.blocks[i].tag == tag&&cache.blocks[i].valid == 1) {
            temp = cache.blocks[i].lruLabel;
            if(write_flag==0){
                printAction(addr,1,cacheToProcessor);
                iffind = 1;
                break;
            }
            else{
                printAction(addr,1,processorToCache);
                cache.blocks[i].data[bo] = write_data;
                cache.blocks[i].dirty = 1;
                iffind = 1;
                break;
            }
        }
        else if(cache.blocks[i].valid == 0) {
            printAction(addr-(addr%cache.blockSize), cache.blockSize, memoryToCache);
            cache.blocks[i].valid = 1;
            cache.blocks[i].tag = tag;
            for(int j = 0; j<cache.blockSize;j++){cache.blocks[i].data[j] = mem_access(addr-(addr%cache.blockSize)+j,0,0);}
            temp = cache.blocks[i].lruLabel;
            if(write_flag==0){
                printAction(addr,1,cacheToProcessor);
                iffind = 2;
                break;
            }
            else{
                printAction(addr,1,processorToCache);
                cache.blocks[i].data[bo] = write_data;
                cache.blocks[i].dirty = 1;
                iffind = 2;
                break;
            }        
        }
    }
    if(iffind==1){
        for(int ii = cache.blocksPerSet*si; ii < cache.blocksPerSet*(si+1); ii++) {
            if(ii == i) {cache.blocks[ii].lruLabel = 0;}
            else if((ii!=i)&&(cache.blocks[ii].valid == 1)&&(cache.blocks[ii].lruLabel <= temp)) {cache.blocks[ii].lruLabel++;}
        }
        return cache.blocks[i].data[bo];
    }
    else if(iffind==2){
        for(int ii = cache.blocksPerSet*si; ii < cache.blocksPerSet*(si+1); ii++) {
            if(ii == i) {cache.blocks[ii].lruLabel = 0;}
            else if((ii!=i)&&(cache.blocks[ii].valid == 1)) {cache.blocks[ii].lruLabel++;}
        }
        return cache.blocks[i].data[bo];
    }

    for(int j = cache.blocksPerSet*si; j < cache.blocksPerSet*(si+1); j++) {
        if(cache.blocks[j].lruLabel == cache.blocksPerSet-1) {
            cache.blocks[j].lruLabel = 0;
            if(cache.blocks[j].dirty == 1) {
                cache.blocks[j].dirty = 0;
                printAction((cache.blocks[j].tag<<(block_offset_bits+set_index_bits))+(si<<block_offset_bits), cache.blockSize, cacheToMemory);
                for(int jj = 0; jj<cache.blockSize;jj++){mem_access((cache.blocks[j].tag<<(block_offset_bits+set_index_bits))+(si<<block_offset_bits)+jj,1,cache.blocks[j].data[jj]);}
            }
            else{printAction((cache.blocks[j].tag<<(block_offset_bits+set_index_bits))+(si<<block_offset_bits), cache.blockSize, cacheToNowhere);}
            printAction(addr-(addr%cache.blockSize), cache.blockSize, memoryToCache);
            cache.blocks[j].tag = tag;
            for(int jj = 0; jj<cache.blockSize;jj++){cache.blocks[j].data[jj] = mem_access(addr-(addr%cache.blockSize)+jj,0,0);}
            if(write_flag==0){
                printAction(addr,1,cacheToProcessor);
                temp = cache.blocks[j].data[bo];
            }
            else{
                printAction(addr,1,processorToCache);
                cache.blocks[j].data[bo] = write_data;
                cache.blocks[j].dirty = 1;
                temp = cache.blocks[j].data[bo];
            } 
        }
        else{cache.blocks[j].lruLabel++;}
    }
    return temp;
}


/*
 * print end of run statistics like in the spec. **This is not required**,
 * but is very helpful in debugging.
 * This should be called once a halt is reached.
 * DO NOT delete this function, or else it won't compile.
 * DO NOT print $$$ in this function
 */
void printStats(void)
{
    printf("End of run statistics:\n");
    return;
}

/*
 * Log the specifics of each cache action.
 *
 *DO NOT modify the content below.
 * address is the starting word address of the range of data being transferred.
 * size is the size of the range of data being transferred.
 * type specifies the source and destination of the data being transferred.
 *  -    cacheToProcessor: reading data from the cache to the processor
 *  -    processorToCache: writing data from the processor to the cache
 *  -    memoryToCache: reading data from the memory to the cache
 *  -    cacheToMemory: evicting cache data and writing it to the memory
 *  -    cacheToNowhere: evicting cache data and throwing it away
 */
void printAction(int address, int size, enum actionType type)
{
    printf("$$$ transferring word [%d-%d] ", address, address + size - 1);

    if (type == cacheToProcessor) {
        printf("from the cache to the processor\n");
    }
    else if (type == processorToCache) {
        printf("from the processor to the cache\n");
    }
    else if (type == memoryToCache) {
        printf("from the memory to the cache\n");
    }
    else if (type == cacheToMemory) {
        printf("from the cache to the memory\n");
    }
    else if (type == cacheToNowhere) {
        printf("from the cache to nowhere\n");
    }
    else {
        printf("Error: unrecognized action\n");
        exit(1);
    }

}

/*
 * Prints the cache based on the configurations of the struct
 * This is for debugging only and is not graded, so you may
 * modify it, but that is not recommended.
 */
void printCache(void)
{
    int blockIdx;
    int decimalDigitsForWaysInSet = (cache.blocksPerSet == 1) ? 1 : (int)ceil(log10((double)cache.blocksPerSet));
    printf("\ncache:\n");
    for (int set = 0; set < cache.numSets; ++set) {
        printf("\tset %i:\n", set);
        for (int block = 0; block < cache.blocksPerSet; ++block) {
            blockIdx = set * cache.blocksPerSet + block;
            if(cache.blocks[set * cache.blocksPerSet + block].valid) {
                printf("\t\t[ %0*i ] : ( V:T | D:%c | LRU:%-*i | T:%i )\n\t\t%*s{",
                    decimalDigitsForWaysInSet, block,
                    (cache.blocks[blockIdx].dirty) ? 'T' : 'F',
                    decimalDigitsForWaysInSet, cache.blocks[blockIdx].lruLabel,
                    cache.blocks[blockIdx].tag,
                    7+decimalDigitsForWaysInSet, "");
                for (int index = 0; index < cache.blockSize; ++index) {
                    printf(" 0x%08X", cache.blocks[blockIdx].data[index]);
                }
                printf(" }\n");
            }
            else {
                printf("\t\t[ %0*i ] : (V:F)\n\t\t%*s{  }\n", decimalDigitsForWaysInSet, block, 7+decimalDigitsForWaysInSet, "");
            }
        }
    }
    printf("end cache\n");
}
