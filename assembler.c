/**
 * Project 1
 * Assembler code fragment for LC-2K
 */

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

//Every LC2K file will contain less than 1000 lines of assembly.
#define MAXLINELENGTH 1000

int readAndParse(FILE *, char *, char *, char *, char *, char *);
static void checkForBlankLinesInCode(FILE *inFilePtr);
static inline int isNumber(char *);
static inline void printHexToFile(FILE *, int);
static int findLabel(FILE *, char *, char *, char *, char *, char *, const char *, int);
static int checkDuplicatedLabel(FILE *, const char *);

int
main(int argc, char **argv)
{
    char *inFileString, *outFileString;
    FILE *inFilePtr, *outFilePtr;
    char label[MAXLINELENGTH], opcode[MAXLINELENGTH], arg0[MAXLINELENGTH],
            arg1[MAXLINELENGTH], arg2[MAXLINELENGTH];

    if (argc != 3) {
        printf("error: usage: %s <assembly-code-file> <machine-code-file>\n",
            argv[0]);
        exit(1);
    }

    inFileString = argv[1];
    outFileString = argv[2];

    inFilePtr = fopen(inFileString, "r");
    if (inFilePtr == NULL) {
        printf("error in opening %s\n", inFileString);
        exit(1);
    }

    // Check for blank lines in the middle of the code.
    checkForBlankLinesInCode(inFilePtr);

    outFilePtr = fopen(outFileString, "w");
    if (outFilePtr == NULL) {
        printf("error in opening %s\n", outFileString);
        exit(1);
    }
    

    int currLineNum = 0;
    while(readAndParse(inFilePtr, label, opcode, arg0, arg1, arg2)) {
        int result = 0b000;

        if (strcmp(label, "")) { // Label defined
            checkDuplicatedLabel(inFilePtr, label);
        }

        if (strlen(label) > 6 || (!isNumber(arg2) && strlen(arg2) > 6
            && strcmp(opcode, "noop") && strcmp(opcode, "halt") && 
            strcmp(opcode, ".fill"))) { //label longer than 6 char
            printf("Error: label too long");
            exit(1);
        }

        if (strcmp(opcode, "noop") && strcmp(opcode, "halt") && 
            strcmp(opcode, ".fill")) { // Opcodes with Reg arguments
            if (!isNumber(arg0) || !isNumber(arg1)) {
                printf("non-integer reg types");
                exit(1);
            }

            if ((atoi(arg0) > 7) || (atoi(arg0) < 0) || 
                (atoi(arg1) > 7)||(atoi(arg1) < 0)) {
                printf("regs outside the range of [0,7]");
                exit(1);
            }
        }

        

        // R-type instructions
        if (!strcmp(opcode, "add") || !strcmp(opcode, "nor")) {
            if (!isNumber(arg2)) {
                printf("Error: reg2 not a number");
                exit(1);
            }
            if ((atoi(arg2) > 7) || (atoi(arg2) < 0)) {
                printf("Error: regs outside the range of [0,7]");
                exit(1);
            }
            if (!strcmp(opcode, "add")) result = result | 0b000;
            if (!strcmp(opcode, "nor")) result = result | 0b001;
            result = result << 3;
            result = result | (*arg0 - '0');
            result = result << 3;
            result = result | (*arg1 - '0');
            result = result << 16;
            result = result | (*arg2 - '0');
        }

        // I-type instructions
        else if (!strcmp(opcode, "lw") || !strcmp(opcode, "beq") || 
                !strcmp(opcode, "sw")) {
            if ((atoi(arg2) > 32767)||(atoi(arg2) < -32768)) { //offsetfield that doesnt fit in 16 bits;
                printf("Error: offsetfield doesn't fit in 16 bits");
                exit(1);
            }
            if (!strcmp(opcode, "lw")) result = result | 0b010;
            if (!strcmp(opcode, "sw")) result = result | 0b011;
            if (!strcmp(opcode, "beq")) result = result | 0b100;
            result = result << 3;
            result = result | (*arg0 - '0');
            result = result << 3;
            result = result | (*arg1 - '0');
            result = result << 16;
            if (!isNumber(arg2)) {
                char target[MAXLINELENGTH];
                strcpy(target, arg2);
                int location = findLabel(inFilePtr, label, opcode, arg0, 
                                        arg1, arg2, target, currLineNum);
                result = result | (location & 0xffff);
            } else {
                result = result | (atoi(arg2) & 0xffff);
            }
        }
        
        // J-type instructions
        else if (!strcmp(opcode, "jalr")) {
            result = result | 0b101;
            result = result << 3;
            result = result | (*arg0 - '0');
            result = result << 3;
            result = result | (*arg1 - '0');
            result = result << 16;
        }

        // O-type instructions
        else if (!strcmp(opcode, "halt") || !strcmp(opcode, "noop")) {
            if (!strcmp(opcode, "halt")) result = result | 0b110;
            if (!strcmp(opcode, "noop")) result = result | 0b111;
            result = result << 22;
        }

        // LC2K directive
        else if (!strcmp(opcode, ".fill")) {
            if (!isNumber(arg0)) {
                char target[MAXLINELENGTH];
                strcpy(target, arg0);
                int location = findLabel(inFilePtr, label, opcode, arg0, 
                                        arg1, arg2, target, currLineNum);
                result = result | location;
            } else {
                int num = atoi(arg0);
                char *endptr;
                errno = 0;
                long val = strtol(arg0, &endptr, 10);
                if ((errno == ERANGE && (val == LONG_MAX || val == LONG_MIN))
                    || (val > INT_MAX || val < INT_MIN)) {
                    printf("Error: .fill number overflow");
                    exit(1);
                }
                result = result | num;
            }
        }


        else { // unrecognized opcodes
            printf("error: unrecognized opcodes\n");
            exit(1);
        }

        currLineNum++;
        printHexToFile(outFilePtr, result);
    }

    return(0);
}

// Returns non-zero if the line contains only whitespace.
static int lineIsBlank(char *line) {
    char whitespace[4] = {'\t', '\n', '\r', ' '};
    int nonempty_line = 0;
    for(int line_idx=0; line_idx < strlen(line); ++line_idx) {
        int line_char_is_whitespace = 0;
        for(int whitespace_idx = 0; whitespace_idx < 4; ++ whitespace_idx) {
            if(line[line_idx] == whitespace[whitespace_idx]) {
                line_char_is_whitespace = 1;
                break;
            }
        }
        if(!line_char_is_whitespace) {
            nonempty_line = 1;
            break;
        }
    }
    return !nonempty_line;
}

// Exits 2 if file contains an empty line anywhere other than at the end of the file.
// Note calling this function rewinds inFilePtr.
static void checkForBlankLinesInCode(FILE *inFilePtr) {
    char line[MAXLINELENGTH];
    int blank_line_encountered = 0;
    int address_of_blank_line = 0;
    rewind(inFilePtr);

    for(int address = 0; fgets(line, MAXLINELENGTH, inFilePtr) != NULL; ++address) {
        // Check for line too long
        if (strlen(line) >= MAXLINELENGTH-1) {
            printf("error: line too long\n");
            exit(1);
        }

        // Check for blank line.
        if(lineIsBlank(line)) {
            if(!blank_line_encountered) {
                blank_line_encountered = 1;
                address_of_blank_line = address;
            }
        } else {
            if(blank_line_encountered) {
                printf("Invalid Assembly: Empty line at address %d\n", address_of_blank_line);
                exit(2);
            }
        }
    }
    rewind(inFilePtr);
}

// find the locatiion of the target label, changes all the pointers, exit 1 if label undefined
static int findLabel(FILE *inFilePtr, char *label, char *opcode, char *arg0,
    char *arg1, char *arg2, const char *target, int currLineNum) {
    int mark = 0;
    int location = 0;

    //for "lw" and "sw" opcode, offsetField is the target position, for ".fill", position
    if (!strcmp(opcode, "lw") || !strcmp(opcode, "sw") || !strcmp(opcode, ".fill")) {
        mark = 1;
    }

    long originalPos = ftell(inFilePtr);
    rewind(inFilePtr);
    for (int i = 0; i < MAXLINELENGTH; i++) {
        if (!readAndParse(inFilePtr, label, opcode, arg0, arg1, arg2)) break;
        if (!strcmp(label, target)) {
            fseek(inFilePtr, originalPos, SEEK_SET);
            location = i;
            return (mark) ? location : location - currLineNum - 1;
        }
    }

    printf("Error: Label not found\n");
    exit(1); //nothing found
}

// Check for duplicated label
static int checkDuplicatedLabel(FILE *inFilePtr, const char *target) {
    char FuncLabel[MAXLINELENGTH], FuncOpcode[MAXLINELENGTH], FuncArg0[MAXLINELENGTH],
            FuncArg1[MAXLINELENGTH], FuncArg2[MAXLINELENGTH]; // local variables used for readandparse function
    long originalPos = ftell(inFilePtr);
    while (readAndParse(inFilePtr, FuncLabel, FuncOpcode, FuncArg0, FuncArg1, FuncArg2)) {
        if (!strcmp(FuncLabel, target)) {
            printf("Error: Found duplicated label\n");
            exit(1);
        }
    }
    fseek(inFilePtr, originalPos, SEEK_SET);
    return 0;
}



/*
* NOTE: The code defined below is not to be modifed as it is implimented correctly.
*/

/*
 * Read and parse a line of the assembly-language file.  Fields are returned
 * in label, opcode, arg0, arg1, arg2 (these strings must have memory already
 * allocated to them).
 *
 * Return values:
 *     0 if reached end of file
 *     1 if all went well
 *
 * exit(1) if line is too long.
 */
int
readAndParse(FILE *inFilePtr, char *label, char *opcode, char *arg0,
    char *arg1, char *arg2)
{
    char line[MAXLINELENGTH];
    char *ptr = line;

    /* delete prior values */
    label[0] = opcode[0] = arg0[0] = arg1[0] = arg2[0] = '\0';

    /* read the line from the assembly-language file */
    if (fgets(line, MAXLINELENGTH, inFilePtr) == NULL) {
	/* reached end of file */
        return(0);
    }

    /* check for line too long */
    if (strlen(line) == MAXLINELENGTH-1) {
	printf("error: line too long\n");
	exit(1);
    }

    // Ignore blank lines at the end of the file.
    if(lineIsBlank(line)) {
        return 0;
    }

    /* is there a label? */
    ptr = line;
    if (sscanf(ptr, "%[^\t\n ]", label)) {
	/* successfully read label; advance pointer over the label */
        ptr += strlen(label);
    }

    /*
     * Parse the rest of the line.  Would be nice to have real regular
     * expressions, but scanf will suffice.
     */
    sscanf(ptr, "%*[\t\n\r ]%[^\t\n\r ]%*[\t\n\r ]%[^\t\n\r ]%*[\t\n\r ]%[^\t\n\r ]%*[\t\n\r ]%[^\t\n\r ]",
        opcode, arg0, arg1, arg2);

    return(1);
}

static inline int
isNumber(char *string)
{
    int num;
    char c;
    return((sscanf(string, "%d%c",&num, &c)) == 1);
}


// Prints a machine code word in the proper hex format to the file
static inline void 
printHexToFile(FILE *outFilePtr, int word) {
    fprintf(outFilePtr, "0x%08X\n", word);
}
