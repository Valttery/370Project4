/*
 * Project 4
 * EECS 370 LC-2K Instruction-level simulator
 *
 * Make sure to NOT modify printState or any of the associated functions
 */

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

//DO NOT CHANGE THE FOLLOWING DEFINITIONS 

// Machine Definitions
#define MEMORYSIZE 65536 /* maximum number of words in memory (maximum number of lines in a given file)*/
#define NUMREGS 8 /*total number of machine registers [0,7]*/

// File Definitions
#define MAXLINELENGTH 1000 /* MAXLINELENGTH is the max number of characters we read */

typedef struct 
stateStruct {
    int pc;
    int mem[MEMORYSIZE];
    int reg[NUMREGS];
    int numMemory;
} stateType;

extern void cache_init(int blockSize, int numSets, int blocksPerSet);
extern int cache_access(int addr, int write_flag, int write_data);
extern void printStats();
static stateType state;
static int num_mem_accesses = 0;

int mem_access(int addr, int write_flag, int write_data) {
    ++num_mem_accesses;
    if (write_flag) {
        state.mem[addr] = write_data;
        if(state.numMemory <= addr) {
            state.numMemory = addr + 1;
        }
    }
    return state.mem[addr];
}

int get_num_mem_accesses(){
    return num_mem_accesses;
}

void printState(stateType *);

static inline int convertNum(int32_t);

int main(int argc, char **argv)
{
    char line[MAXLINELENGTH];
    FILE *filePtr;
    int temp;

    if (argc != 5) {
        printf("error: usage: %s <machine-code file>\n", argv[0]);
        exit(1);
    }

    filePtr = fopen(argv[1], "r");
    if (filePtr == NULL) {
        printf("error: can't open file %s , please ensure you are providing the correct path", argv[1]);
        perror("fopen");
        exit(2);
    }

    cache_init(atoi(argv[2]), atoi(argv[3]), atoi(argv[4]));

    /* read the entire machine-code file into memory */
    for (state.numMemory=0; fgets(line, MAXLINELENGTH, filePtr) != NULL; state.numMemory++) {
		    if (state.numMemory >= MEMORYSIZE) {
			      fprintf(stderr, "exceeded memory size\n");
			      exit(2);
		    }
		    if (sscanf(line, "%x", state.mem+state.numMemory) != 1) {
			      fprintf(stderr, "error in reading address %d\n", state.numMemory);
			      exit(2);
		    }
		    //printf("memory[%d]=0x%x\n", state.numMemory, state.mem[state.numMemory]);
    }

    state.pc = 0;
    temp = state.numMemory;
    for(int i = 0;i<8;i++){state.reg[i] = 0;}
    state.numMemory = temp;

    while(true){
        //printState(&state);
        int instr =  cache_access(state.pc,0,0);
        if(((instr>>22)&0b111)==0b000){state.reg[instr&0b111]=state.reg[(instr>>19)&0b111]+state.reg[(instr>>16)&0b111];}
        else if(((instr>>22)&0b111)==0b001){state.reg[instr&0b111]=~(state.reg[(instr>>19)&0b111]|state.reg[(instr>>16)&0b111]);}
        else if(((instr>>22)&0b111)==0b010){state.reg[(instr>>16)&0b111]=cache_access(convertNum(instr&0xFFFF)+state.reg[(instr>>19)&0b111],0,0);}
        else if(((instr>>22)&0b111)==0b011){cache_access(convertNum(instr&0xFFFF)+state.reg[(instr>>19)&0b111],1,state.reg[(instr>>16)&0b111]);}
        else if(((instr>>22)&0b111)==0b100){if(state.reg[(instr>>19)&0b111]==state.reg[(instr>>16)&0b111]){state.pc+=convertNum(instr&0xFFFF);}}
        else if(((instr>>22)&0b111)==0b101){
            state.reg[(instr>>16)&0b111] = state.pc+1;
            state.pc = convertNum(state.reg[(instr>>19)&0b111]);
            continue;
        }
        else if(((instr>>22)&0b111)==0b110){
            state.pc++;
            printState(&state);
            printStats();
            break;
        }
        else if(((instr>>22)&0b111)==0b111){}
        state.pc++;
    }
    return(0);
}

/*
* DO NOT MODIFY ANY OF THE CODE BELOW. 
*/

void printState(stateType *statePtr) {
    int i;
    printf("\n@@@\nstate:\n");
    printf("\tpc %d\n", statePtr->pc);
    printf("\tmemory:\n");
    for (i=0; i<statePtr->numMemory; i++) {
        printf("\t\tmem[ %d ] 0x%08X\n", i, statePtr->mem[i]);
    }
    printf("\tregisters:\n");
	  for (i=0; i<NUMREGS; i++) {
	      printf("\t\treg[ %d ] %d\n", i, statePtr->reg[i]);
	  }
    printf("end state\n");
}

// convert a 16-bit number into a 32-bit Linux integer
static inline int convertNum(int num) 
{
    return num - ( (num & (1<<15)) ? 1<<16 : 0 );
}

/*
* Write any helper functions that you wish down here. 
*/
