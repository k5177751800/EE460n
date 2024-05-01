/*
    Name 1: Konghwan Shin
    UTEID 1: KS54897
*/

/***************************************************************/
/*                                                             */
/*   LC-3b Simulator                                           */
/*                                                             */
/*   EE 460N                                                   */
/*   The University of Texas at Austin                         */
/*                                                             */
/***************************************************************/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>


/***************************************************************/
/*                                                             */
/* Files:  ucode        Microprogram file                      */
/*         isaprogram   LC-3b machine language program file    */
/*                                                             */
/***************************************************************/

/***************************************************************/
/* These are the functions you'll have to write.               */
/***************************************************************/

void eval_micro_sequencer();
void cycle_memory();
void eval_bus_drivers();
void drive_bus();
void latch_datapath_values();

/***************************************************************/
/* A couple of useful definitions.                             */
/***************************************************************/
#define FALSE 0
#define TRUE  1

/***************************************************************/
/* Use this to avoid overflowing 16 bits on the bus.           */
/***************************************************************/
#define Low16bits(x) ((x) & 0xFFFF)

/***************************************************************/
/* Definition of the control store layout.                     */
/***************************************************************/
#define CONTROL_STORE_ROWS 64
#define INITIAL_STATE_NUMBER 18

/***************************************************************/
/* Definition of bit order in control store word.              */
/***************************************************************/
enum CS_BITS {
    IRD,
    COND1, COND0,
    J5, J4, J3, J2, J1, J0,
    LD_MAR,
    LD_MDR,
    LD_IR,
    LD_BEN,
    LD_REG,
    LD_CC,
    LD_PC,
    GATE_PC,
    GATE_MDR,
    GATE_ALU,
    GATE_MARMUX,
    GATE_SHF,
    PCMUX1, PCMUX0,
    DRMUX,
    SR1MUX,
    ADDR1MUX,
    ADDR2MUX1, ADDR2MUX0,
    MARMUX,
    ALUK1, ALUK0,
    MIO_EN,
    R_W,
    DATA_SIZE,
    LSHF1,
    /* MODIFY: you have to add all your new control signals */
    GATE_PSR,
    PSRMUX,
    LD_PSR,
    LD_EXC,
    GATE_VECTOR,
    VECMUX,
    EXCMUX,
    LD_EXCV,
    LD_INTV,
    GATE_SP,
    GATE_SPMUX,
    CONTROL_STORE_BITS
} CS_BITS;

/***************************************************************/
/* Functions to get at the control bits.                       */
/***************************************************************/
int GetIRD(int *x)           { return(x[IRD]); }
int GetCOND(int *x)          { return((x[COND1] << 1) + x[COND0]); }
int GetJ(int *x)             { return((x[J5] << 5) + (x[J4] << 4) +
                                      (x[J3] << 3) + (x[J2] << 2) +
                                      (x[J1] << 1) + x[J0]); }
int GetLD_MAR(int *x)        { return(x[LD_MAR]); }
int GetLD_MDR(int *x)        { return(x[LD_MDR]); }
int GetLD_IR(int *x)         { return(x[LD_IR]); }
int GetLD_BEN(int *x)        { return(x[LD_BEN]); }
int GetLD_REG(int *x)        { return(x[LD_REG]); }
int GetLD_CC(int *x)         { return(x[LD_CC]); }
int GetLD_PC(int *x)         { return(x[LD_PC]); }
int GetGATE_PC(int *x)       { return(x[GATE_PC]); }
int GetGATE_MDR(int *x)      { return(x[GATE_MDR]); }
int GetGATE_ALU(int *x)      { return(x[GATE_ALU]); }
int GetGATE_MARMUX(int *x)   { return(x[GATE_MARMUX]); }
int GetGATE_SHF(int *x)      { return(x[GATE_SHF]); }
int GetPCMUX(int *x)         { return((x[PCMUX1] << 1) + x[PCMUX0]); }
int GetDRMUX(int *x)         { return(x[DRMUX]); }
int GetSR1MUX(int *x)        { return(x[SR1MUX]); }
int GetADDR1MUX(int *x)      { return(x[ADDR1MUX]); }
int GetADDR2MUX(int *x)      { return((x[ADDR2MUX1] << 1) + x[ADDR2MUX0]); }
int GetMARMUX(int *x)        { return(x[MARMUX]); }
int GetALUK(int *x)          { return((x[ALUK1] << 1) + x[ALUK0]); }
int GetMIO_EN(int *x)        { return(x[MIO_EN]); }
int GetR_W(int *x)           { return(x[R_W]); }
int GetDATA_SIZE(int *x)     { return(x[DATA_SIZE]); }
int GetLSHF1(int *x)         { return(x[LSHF1]); }

/***************************************************************/
/* The control store rom.                                      */
/***************************************************************/
int CONTROL_STORE[CONTROL_STORE_ROWS][CONTROL_STORE_BITS];

/***************************************************************/
/* Main memory.                                                */
/***************************************************************/
/* MEMORY[A][0] stores the least significant byte of word at word address A
   MEMORY[A][1] stores the most significant byte of word at word address A 
   There are two write enable signals, one for each byte. WE0 is used for 
   the least significant byte of a word. WE1 is used for the most significant 
   byte of a word. */

#define WORDS_IN_MEM    0x08000
#define MEM_CYCLES      5
int MEMORY[WORDS_IN_MEM][2];

/***************************************************************/

/***************************************************************/

/***************************************************************/
/* LC-3b State info.                                           */
/***************************************************************/
#define LC_3b_REGS 8

int RUN_BIT;	/* run bit */
int BUS;	/* value of the bus */

typedef struct System_Latches_Struct{

    int PC,		/* program counter */
    MDR,	/* memory data register */
    MAR,	/* memory address register */
    IR,		/* instruction register */
    N,		/* n condition bit */
    Z,		/* z condition bit */
    P,		/* p condition bit */
    BEN;        /* ben register */

    int READY;	/* ready bit */
    /* The ready bit is also latched as you dont want the memory system to assert it
       at a bad point in the cycle*/

    int REGS[LC_3b_REGS]; /* register file. */

    int MICROINSTRUCTION[CONTROL_STORE_BITS]; /* The microintruction */

    int STATE_NUMBER; /* Current State Number - Provided for debugging */
} System_Latches;

/* Data Structure for Latch */

System_Latches CURRENT_LATCHES, NEXT_LATCHES;

/***************************************************************/
/* A cycle counter.                                            */
/***************************************************************/
int CYCLE_COUNT;

/***************************************************************/
/*                                                             */
/* Procedure : help                                            */
/*                                                             */
/* Purpose   : Print out a list of commands.                   */
/*                                                             */
/***************************************************************/
void help() {
    printf("----------------LC-3bSIM Help-------------------------\n");
    printf("go               -  run program to completion       \n");
    printf("run n            -  execute program for n cycles    \n");
    printf("mdump low high   -  dump memory from low to high    \n");
    printf("rdump            -  dump the register & bus values  \n");
    printf("?                -  display this help menu          \n");
    printf("quit             -  exit the program                \n\n");
}

/***************************************************************/
/*                                                             */
/* Procedure : cycle                                           */
/*                                                             */
/* Purpose   : Execute a cycle                                 */
/*                                                             */
/***************************************************************/
void cycle() {
    eval_micro_sequencer();
    cycle_memory();
    eval_bus_drivers();
    drive_bus();
    latch_datapath_values();

    CURRENT_LATCHES = NEXT_LATCHES;

    CYCLE_COUNT++;
}

/***************************************************************/
/*                                                             */
/* Procedure : run n                                           */
/*                                                             */
/* Purpose   : Simulate the LC-3b for n cycles.                 */
/*                                                             */
/***************************************************************/
void run(int num_cycles) {
    int i;

    if (RUN_BIT == FALSE) {
        printf("Can't simulate, Simulator is halted\n\n");
        return;
    }

    printf("Simulating for %d cycles...\n\n", num_cycles);
    for (i = 0; i < num_cycles; i++) {
        if (CURRENT_LATCHES.PC == 0x0000) {
            RUN_BIT = FALSE;
            printf("Simulator halted\n\n");
            break;
        }
        cycle();
    }
}

/***************************************************************/
/*                                                             */
/* Procedure : go                                              */
/*                                                             */
/* Purpose   : Simulate the LC-3b until HALTed.                 */
/*                                                             */
/***************************************************************/
void go() {
    if (RUN_BIT == FALSE) {
        printf("Can't simulate, Simulator is halted\n\n");
        return;
    }

    printf("Simulating...\n\n");
    while (CURRENT_LATCHES.PC != 0x0000)
        cycle();
    RUN_BIT = FALSE;
    printf("Simulator halted\n\n");
}

/***************************************************************/
/*                                                             */
/* Procedure : mdump                                           */
/*                                                             */
/* Purpose   : Dump a word-aligned region of memory to the     */
/*             output file.                                    */
/*                                                             */
/***************************************************************/
void mdump(FILE * dumpsim_file, int start, int stop) {
    int address; /* this is a byte address */

    printf("\nMemory content [0x%.4x..0x%.4x] :\n", start, stop);
    printf("-------------------------------------\n");
    for (address = (start >> 1); address <= (stop >> 1); address++)
        printf("  0x%.4x (%d) : 0x%.2x%.2x\n", address << 1, address << 1, MEMORY[address][1], MEMORY[address][0]);
    printf("\n");

    /* dump the memory contents into the dumpsim file */
    fprintf(dumpsim_file, "\nMemory content [0x%.4x..0x%.4x] :\n", start, stop);
    fprintf(dumpsim_file, "-------------------------------------\n");
    for (address = (start >> 1); address <= (stop >> 1); address++)
        fprintf(dumpsim_file, " 0x%.4x (%d) : 0x%.2x%.2x\n", address << 1, address << 1, MEMORY[address][1], MEMORY[address][0]);
    fprintf(dumpsim_file, "\n");
    fflush(dumpsim_file);
}

/***************************************************************/
/*                                                             */
/* Procedure : rdump                                           */
/*                                                             */
/* Purpose   : Dump current register and bus values to the     */
/*             output file.                                    */
/*                                                             */
/***************************************************************/
void rdump(FILE * dumpsim_file) {
    int k;

    printf("\nCurrent register/bus values :\n");
    printf("-------------------------------------\n");
    printf("Cycle Count  : %d\n", CYCLE_COUNT);
    printf("PC           : 0x%.4x\n", CURRENT_LATCHES.PC);
    printf("IR           : 0x%.4x\n", CURRENT_LATCHES.IR);
    printf("STATE_NUMBER : 0x%.4x\n\n", CURRENT_LATCHES.STATE_NUMBER);
    printf("BUS          : 0x%.4x\n", BUS);
    printf("MDR          : 0x%.4x\n", CURRENT_LATCHES.MDR);
    printf("MAR          : 0x%.4x\n", CURRENT_LATCHES.MAR);
    printf("CCs: N = %d  Z = %d  P = %d\n", CURRENT_LATCHES.N, CURRENT_LATCHES.Z, CURRENT_LATCHES.P);
    printf("Registers:\n");
    for (k = 0; k < LC_3b_REGS; k++)
        printf("%d: 0x%.4x\n", k, CURRENT_LATCHES.REGS[k]);
    printf("\n");

    /* dump the state information into the dumpsim file */
    fprintf(dumpsim_file, "\nCurrent register/bus values :\n");
    fprintf(dumpsim_file, "-------------------------------------\n");
    fprintf(dumpsim_file, "Cycle Count  : %d\n", CYCLE_COUNT);
    fprintf(dumpsim_file, "PC           : 0x%.4x\n", CURRENT_LATCHES.PC);
    fprintf(dumpsim_file, "IR           : 0x%.4x\n", CURRENT_LATCHES.IR);
    fprintf(dumpsim_file, "STATE_NUMBER : 0x%.4x\n\n", CURRENT_LATCHES.STATE_NUMBER);
    fprintf(dumpsim_file, "BUS          : 0x%.4x\n", BUS);
    fprintf(dumpsim_file, "MDR          : 0x%.4x\n", CURRENT_LATCHES.MDR);
    fprintf(dumpsim_file, "MAR          : 0x%.4x\n", CURRENT_LATCHES.MAR);
    fprintf(dumpsim_file, "CCs: N = %d  Z = %d  P = %d\n", CURRENT_LATCHES.N, CURRENT_LATCHES.Z, CURRENT_LATCHES.P);
    fprintf(dumpsim_file, "Registers:\n");
    for (k = 0; k < LC_3b_REGS; k++)
        fprintf(dumpsim_file, "%d: 0x%.4x\n", k, CURRENT_LATCHES.REGS[k]);
    fprintf(dumpsim_file, "\n");
    fflush(dumpsim_file);
}

/***************************************************************/
/*                                                             */
/* Procedure : get_command                                     */
/*                                                             */
/* Purpose   : Read a command from standard input.             */
/*                                                             */
/***************************************************************/
void get_command(FILE * dumpsim_file) {
    char buffer[20];
    int start, stop, cycles;

    printf("LC-3b-SIM> ");

    scanf("%s", buffer);
    printf("\n");

    switch(buffer[0]) {
        case 'G':
        case 'g':
            go();
            break;

        case 'M':
        case 'm':
            scanf("%i %i", &start, &stop);
            mdump(dumpsim_file, start, stop);
            break;

        case '?':
            help();
            break;
        case 'Q':
        case 'q':
            printf("Bye.\n");
            exit(0);

        case 'R':
        case 'r':
            if (buffer[1] == 'd' || buffer[1] == 'D')
                rdump(dumpsim_file);
            else {
                scanf("%d", &cycles);
                run(cycles);
            }
            break;

        default:
            printf("Invalid Command\n");
            break;
    }
}

/***************************************************************/
/*                                                             */
/* Procedure : init_control_store                              */
/*                                                             */
/* Purpose   : Load microprogram into control store ROM        */
/*                                                             */
/***************************************************************/
void init_control_store(char *ucode_filename) {
    FILE *ucode;
    int i, j, index;
    char line[200];

    printf("Loading Control Store from file: %s\n", ucode_filename);

    /* Open the micro-code file. */
    if ((ucode = fopen(ucode_filename, "r")) == NULL) {
        printf("Error: Can't open micro-code file %s\n", ucode_filename);
        exit(-1);
    }

    /* Read a line for each row in the control store. */
    for(i = 0; i < CONTROL_STORE_ROWS; i++) {
        if (fscanf(ucode, "%[^\n]\n", line) == EOF) {
            printf("Error: Too few lines (%d) in micro-code file: %s\n",
                   i, ucode_filename);
            exit(-1);
        }

        /* Put in bits one at a time. */
        index = 0;

        for (j = 0; j < CONTROL_STORE_BITS; j++) {
            /* Needs to find enough bits in line. */
            if (line[index] == '\0') {
                printf("Error: Too few control bits in micro-code file: %s\nLine: %d\n",
                       ucode_filename, i);
                exit(-1);
            }
            if (line[index] != '0' && line[index] != '1') {
                printf("Error: Unknown value in micro-code file: %s\nLine: %d, Bit: %d\n",
                       ucode_filename, i, j);
                exit(-1);
            }

            /* Set the bit in the Control Store. */
            CONTROL_STORE[i][j] = (line[index] == '0') ? 0:1;
            index++;
        }

        /* Warn about extra bits in line. */
        if (line[index] != '\0')
            printf("Warning: Extra bit(s) in control store file %s. Line: %d\n",
                   ucode_filename, i);
    }
    printf("\n");
}

/************************************************************/
/*                                                          */
/* Procedure : init_memory                                  */
/*                                                          */
/* Purpose   : Zero out the memory array                    */
/*                                                          */
/************************************************************/
void init_memory() {
    int i;

    for (i=0; i < WORDS_IN_MEM; i++) {
        MEMORY[i][0] = 0;
        MEMORY[i][1] = 0;
    }
}

/**************************************************************/
/*                                                            */
/* Procedure : load_program                                   */
/*                                                            */
/* Purpose   : Load program and service routines into mem.    */
/*                                                            */
/**************************************************************/
void load_program(char *program_filename) {
    FILE * prog;
    int ii, word, program_base;

    /* Open program file. */
    prog = fopen(program_filename, "r");
    if (prog == NULL) {
        printf("Error: Can't open program file %s\n", program_filename);
        exit(-1);
    }

    /* Read in the program. */
    if (fscanf(prog, "%x\n", &word) != EOF)
        program_base = word >> 1;
    else {
        printf("Error: Program file is empty\n");
        exit(-1);
    }

    ii = 0;
    while (fscanf(prog, "%x\n", &word) != EOF) {
        /* Make sure it fits. */
        if (program_base + ii >= WORDS_IN_MEM) {
            printf("Error: Program file %s is too long to fit in memory. %x\n",
                   program_filename, ii);
            exit(-1);
        }

        /* Write the word to memory array. */
        MEMORY[program_base + ii][0] = word & 0x00FF;
        MEMORY[program_base + ii][1] = (word >> 8) & 0x00FF;
        ii++;
    }

    if (CURRENT_LATCHES.PC == 0) CURRENT_LATCHES.PC = (program_base << 1);

    printf("Read %d words from program into memory.\n\n", ii);
}

/***************************************************************/
/*                                                             */
/* Procedure : initialize                                      */
/*                                                             */
/* Purpose   : Load microprogram and machine language program  */
/*             and set up initial state of the machine.        */
/*                                                             */
/***************************************************************/
void initialize(char *ucode_filename, char *program_filename, int num_prog_files) {
    int i;
    init_control_store(ucode_filename);

    init_memory();
    for ( i = 0; i < num_prog_files; i++ ) {
        load_program(program_filename);
        while(*program_filename++ != '\0');
    }
    CURRENT_LATCHES.Z = 1;
    CURRENT_LATCHES.STATE_NUMBER = INITIAL_STATE_NUMBER;
    memcpy(CURRENT_LATCHES.MICROINSTRUCTION, CONTROL_STORE[INITIAL_STATE_NUMBER], sizeof(int)*CONTROL_STORE_BITS);

    NEXT_LATCHES = CURRENT_LATCHES;

    RUN_BIT = TRUE;
}

/***************************************************************/
/*                                                             */
/* Procedure : main                                            */
/*                                                             */
/***************************************************************/
int main(int argc, char *argv[]) {
    FILE * dumpsim_file;

    /* Error Checking */
    if (argc < 3) {
        printf("Error: usage: %s <micro_code_file> <program_file_1> <program_file_2> ...\n",
               argv[0]);
        exit(1);
    }

    printf("LC-3b Simulator\n\n");

    initialize(argv[1], argv[2], argc - 2);

    if ( (dumpsim_file = fopen( "dumpsim", "w" )) == NULL ) {
        printf("Error: Can't open dumpsim file\n");
        exit(-1);
    }

    while (1)
        get_command(dumpsim_file);

}

/***************************************************************/
/* Do not modify the above code.
   You are allowed to use the following global variables in your
   code. These are defined above.

   CONTROL_STORE
   MEMORY
   BUS

   CURRENT_LATCHES
   NEXT_LATCHES

   You may define your own local/global variables and functions.
   You may use the functions to get at the control bits defined
   above.

   Begin your code here 	  			       */
/***************************************************************/
void eval_micro_sequencer() {
//    /*
//     * Evaluate the address of the next state according to the
//     * microsequencer logic. Latch the next microinstruction.
//     */
//    int nextState;
//    int* currentMicroinstruction = CURRENT_LATCHES.MICROINSTRUCTION;
//    if ((CURRENT_LATCHES.IR & 0xF000) == 0x4000) {
//        NEXT_LATCHES.REGS[7] = CURRENT_LATCHES.PC + 2;
//    }
//        //JSR, JSSR
//    else if ((CURRENT_LATCHES.IR & 0xF000) == 0x4000) {
//        NEXT_LATCHES.REGS[7] = CURRENT_LATCHES.PC + 2;
//
//        if (CURRENT_LATCHES.IR & 0x0800) { // JSR
//            int offset = (CURRENT_LATCHES.IR & 0x07FF);
//            if (offset & 0x0400) {
//                offset |= 0xF800;
//            }
//            offset <<= 1;
//            NEXT_LATCHES.PC = CURRENT_LATCHES.PC + (offset << 1);
//        } else { // JSRR
//            int baseReg = (CURRENT_LATCHES.IR >> 6) & 0x07;
//            NEXT_LATCHES.PC = CURRENT_LATCHES.REGS[baseReg];
//        }
//    }
//
//    if (GetIRD(currentMicroinstruction)) {
//        nextState = (CURRENT_LATCHES.IR >> 12) & 0xF;
//    } else {
//        int jBits = (currentMicroinstruction[J5] << 5) |
//                    (currentMicroinstruction[J4] << 4) |
//                    (currentMicroinstruction[J3] << 3) |
//                    ((currentMicroinstruction[J2] || (CURRENT_LATCHES.BEN && currentMicroinstruction[COND1])) << 2) |
//                    ((currentMicroinstruction[J1] || (CURRENT_LATCHES.READY && currentMicroinstruction[COND0])) << 1) |
//                    (currentMicroinstruction[J0] || ((CURRENT_LATCHES.IR & 0x0800) >> 11 && currentMicroinstruction[COND1]));
//
//        nextState = jBits & 0x3F;
//    }
//    NEXT_LATCHES.STATE_NUMBER = nextState;
//    for (int i = 0; i < CONTROL_STORE_BITS; i++) {
//        NEXT_LATCHES.MICROINSTRUCTION[i] = CONTROL_STORE[nextState][i];
//    }
    int nextState;
    int* currentMicroinstruction = CURRENT_LATCHES.MICROINSTRUCTION;
    if ((CURRENT_LATCHES.IR & 0xF000) == 0x4000) {
        NEXT_LATCHES.REGS[7] = CURRENT_LATCHES.PC + 2;

        if (CURRENT_LATCHES.IR & 0x0800) { // JSR
            int offset = (CURRENT_LATCHES.IR & 0x07FF);
            if (offset & 0x0400) offset |= 0xF800;
            NEXT_LATCHES.PC = CURRENT_LATCHES.PC + (offset << 1);
        } else { // JSRR
            int baseReg = (CURRENT_LATCHES.IR >> 6) & 0x07;
            NEXT_LATCHES.PC = CURRENT_LATCHES.REGS[baseReg];
        }
    }
    if (GetIRD(currentMicroinstruction)) {
        nextState = (CURRENT_LATCHES.IR >> 12) & 0xF;
    } else {
        int jBits = (currentMicroinstruction[J5] << 5) |
                    (currentMicroinstruction[J4] << 4) |
                    (currentMicroinstruction[J3] << 3) |
                    ((currentMicroinstruction[J2] || (CURRENT_LATCHES.BEN && currentMicroinstruction[COND1])) << 2) |
                    ((currentMicroinstruction[J1] || (CURRENT_LATCHES.READY && currentMicroinstruction[COND0])) << 1) |
                    (currentMicroinstruction[J0] || ((CURRENT_LATCHES.IR & 0x0800) >> 11 && currentMicroinstruction[COND1]));
        nextState = jBits & 0x3F;
    }
    NEXT_LATCHES.STATE_NUMBER = nextState;
    for (int i = 0; i < CONTROL_STORE_BITS; i++) {
        NEXT_LATCHES.MICROINSTRUCTION[i] = CONTROL_STORE[nextState][i];
    }
}

void cycle_memory() {
    //INCOMPLETE
    /*
     * This function emulates memory and the WE logic.
     * Keep track of which cycle of MEMEN we are dealing with.
     * If fourth, we need to latch Ready bit at the end of
     * cycle to prepare microsequencer for the fifth cycle.
     */
//    static int cycle = 0;
//    if (GetMIO_EN(CURRENT_LATCHES.MICROINSTRUCTION)) {
//        if (++cycle < 5) {
//            if (cycle == 4) {
//                NEXT_LATCHES.READY = 1;
//            }
//        } else {
//            if (CURRENT_LATCHES.READY) {
//                int address = CURRENT_LATCHES.MAR >> 1;
//                int byte = CURRENT_LATCHES.MAR & 0x01;
//                if (GetR_W(CURRENT_LATCHES.MICROINSTRUCTION)) {
//                    if (GetDATA_SIZE(CURRENT_LATCHES.MICROINSTRUCTION)) {
//                        MEMORY[address][0] = CURRENT_LATCHES.MDR & 0xFF;
//                        MEMORY[address][1] = (CURRENT_LATCHES.MDR >> 8) & 0xFF;
//                    } else {
//                        MEMORY[address][byte] = CURRENT_LATCHES.MDR & 0xFF;
//                    }
//                } else {
//                    if (GetDATA_SIZE(CURRENT_LATCHES.MICROINSTRUCTION) && byte == 0) {
//                        NEXT_LATCHES.MDR = (MEMORY[address][1] << 8) | MEMORY[address][0];
//                    } else {
//                        NEXT_LATCHES.MDR = MEMORY[address][byte];
//                    }
//                }
//                cycle = 0;
//                NEXT_LATCHES.READY = 0;
//            }
//        }
//    }
    static int cycle_count = 0;
    if (GetMIO_EN(CURRENT_LATCHES.MICROINSTRUCTION)) {
        cycle_count++;
        if (cycle_count == MEM_CYCLES - 1) {
            NEXT_LATCHES.READY = 1;
        } else if (cycle_count >= MEM_CYCLES && CURRENT_LATCHES.READY) {
            int address = CURRENT_LATCHES.MAR >> 1;
            if (GetR_W(CURRENT_LATCHES.MICROINSTRUCTION)) {
                if (GetDATA_SIZE(CURRENT_LATCHES.MICROINSTRUCTION)) {
                    MEMORY[address][0] = CURRENT_LATCHES.MDR & 0xFF;
                    MEMORY[address][1] = (CURRENT_LATCHES.MDR >> 8) & 0xFF;
                } else {
                    int byteSelect = CURRENT_LATCHES.MAR & 0x01;
                    MEMORY[address][byteSelect] = CURRENT_LATCHES.MDR & 0xFF;
                }
            } else {
                if (GetDATA_SIZE(CURRENT_LATCHES.MICROINSTRUCTION)) {
                    NEXT_LATCHES.MDR = (MEMORY[address][1] << 8) | MEMORY[address][0];
                } else {
                    int byteSelect = CURRENT_LATCHES.MAR & 0x01;
                    int byteValue = MEMORY[address][byteSelect];
                    if (byteValue & 0x80) {
                        byteValue |= 0xFF00;
                    }
                    NEXT_LATCHES.MDR = byteValue;
                }
            }
            cycle_count = 0;
            NEXT_LATCHES.READY = 0;
        }
    } else {
        cycle_count = 0;
    }


}

int Gate_MDR, Gate_ALU, Gate_MARMUX, Gate_SHF;
int tmp;
void eval_bus_drivers() {
    /*
     * Datapath routine emulating operations before driving the bus.
     * Evaluate the input of tristate drivers
     *             Gate_MARMUX,
     *		 Gate_PC,
     *		 Gate_ALU,
     *		 Gate_SHF,
     *		 Gate_MDR.
     */
    int sReg = (CURRENT_LATCHES.IR & 0x01C0) >> 6;
    int sRegVal = Low16bits(CURRENT_LATCHES.REGS[sReg]);
    int PC = Low16bits(CURRENT_LATCHES.PC);

    int input;
    if (GetADDR1MUX(CURRENT_LATCHES.MICROINSTRUCTION) == 1) {
        input = sRegVal;
    } else {
        input = PC;
    }

    int offSet = 0;
    int immVal = CURRENT_LATCHES.IR & 0x03FF;
    int signEXT = (1 << 10);
    int mux = GetADDR2MUX(CURRENT_LATCHES.MICROINSTRUCTION);
    if (mux == 0) {
        offSet = 0;
    } else {
        if (mux == 1) {
            immVal = CURRENT_LATCHES.IR & 0x003F;
            signEXT = (1 << 5);
        } else if (mux == 2) {
            immVal = CURRENT_LATCHES.IR & 0x01FF;
            signEXT = (1 << 8);
        }
        if (immVal & signEXT) {
            offSet = immVal | (~signEXT + 1);
        } else {
            offSet = immVal;
        }
        if (GetLSHF1(CURRENT_LATCHES.MICROINSTRUCTION)) {
            offSet <<= 1;
        }
    }

    offSet = Low16bits(offSet);
    int aluMath = Low16bits(input + offSet);

    if (GetMARMUX(CURRENT_LATCHES.MICROINSTRUCTION) == 1) {
        Gate_MARMUX = aluMath;
    } else {
        Gate_MARMUX = Low16bits((CURRENT_LATCHES.IR & 0xFF) << 1);
    }

    int aluInput;
    if (!(CURRENT_LATCHES.IR & 0x20)) {
        aluInput = Low16bits(CURRENT_LATCHES.REGS[CURRENT_LATCHES.IR & 0x7]);
    } else {
        if (CURRENT_LATCHES.IR & 0x10) {
            aluInput = Low16bits(CURRENT_LATCHES.IR | 0xFFE0);
        } else {
            aluInput = Low16bits(CURRENT_LATCHES.IR & 0x1F);
        }
    }

    if (GetDATA_SIZE(CURRENT_LATCHES.MICROINSTRUCTION) == 1) {
        Gate_MDR = Low16bits(CURRENT_LATCHES.MDR);
    } else {
        int mdrValue;
        if (CURRENT_LATCHES.MAR & 0x1) {
            mdrValue = (CURRENT_LATCHES.MDR >> 8) & 0xFF;
        } else {
            mdrValue = CURRENT_LATCHES.MDR & 0xFF;
        }
        if (mdrValue & 0x80) {
            Gate_MDR = Low16bits(mdrValue | 0xFF00);
        } else {
            Gate_MDR = Low16bits(mdrValue);
        }
    }

    int aluOperation = GetALUK(CURRENT_LATCHES.MICROINSTRUCTION);
    if (aluOperation == 0) {
        Gate_ALU = Low16bits(sRegVal + aluInput);
    } else if (aluOperation == 1) {
        Gate_ALU = Low16bits(sRegVal & aluInput);
    } else if (aluOperation == 2) {
        Gate_ALU = Low16bits(sRegVal ^ aluInput);
    } else {
        int srReg;
        if (GetSR1MUX(CURRENT_LATCHES.MICROINSTRUCTION) == 1) {
            srReg = sReg;
        } else {
            srReg = (CURRENT_LATCHES.IR >> 9) & 0x7;
        }
        Gate_ALU = Low16bits(CURRENT_LATCHES.REGS[srReg]);
    }
    int temp = Low16bits(CURRENT_LATCHES.REGS[(CURRENT_LATCHES.IR & 0x1C0)>>6]);
    int shiftAmount = CURRENT_LATCHES.IR & 0xF;
    int shiftResult = temp;
    switch((CURRENT_LATCHES.IR >> 4) & 0x03) {
        //Lsh
        case 0:
            shiftResult <<= shiftAmount;
            break;
        case 1:
            shiftResult >>= shiftAmount;
            break;
        case 3:
            if (shiftResult & 0x8000) {
                for (int i = 0; i < shiftAmount; ++i) {
                    shiftResult >>= 1;
                    shiftResult |= 0x8000;
                }
            } else {
                shiftResult >>= shiftAmount;
            }
            break;
        default:
            break;
    }
    Gate_SHF = Low16bits(shiftResult);

    tmp = immVal + mux;
}
void drive_bus() {
    /*
     * Datapath routine for driving the bus from one of the 5 possible
     * tristate drivers.
     */
//    bool PC_Gate = GetGATE_PC(CURRENT_LATCHES.MICROINSTRUCTION);
//    bool PC_MDR = GetGATE_MDR(CURRENT_LATCHES.MICROINSTRUCTION);
//    bool PC_ALU = GetGATE_ALU(CURRENT_LATCHES.MICROINSTRUCTION);
//    bool PC_MARMUX = GetGATE_MARMUX(CURRENT_LATCHES.MICROINSTRUCTION);
//    bool PC_SHF = GetGATE_SHF(CURRENT_LATCHES.MICROINSTRUCTION);

    if(GetGATE_PC(CURRENT_LATCHES.MICROINSTRUCTION) == 1){
        BUS = CURRENT_LATCHES.PC;
        return;
    }else if(GetGATE_MDR(CURRENT_LATCHES.MICROINSTRUCTION) == 1){
        BUS = Gate_MDR;
        return;
    }else if(GetGATE_ALU(CURRENT_LATCHES.MICROINSTRUCTION) == 1){
        BUS = Gate_ALU;
        return;
    }else if(GetGATE_MARMUX(CURRENT_LATCHES.MICROINSTRUCTION) == 1){
        BUS = Gate_MARMUX;
        return;
    }else if(GetGATE_SHF(CURRENT_LATCHES.MICROINSTRUCTION) == 1){
        BUS = Gate_SHF;
        return;
    }else{
        BUS = 0;
    }
}


void latch_datapath_values() {
    /*
     * Datapath routine for computing all functions that need to latch
     * values in the data path at the end of this cycle.  Some values
     * require sourcing the bus; therefore, this routine has to come
     * after drive_bus
     */
    if (GetLD_PC(CURRENT_LATCHES.MICROINSTRUCTION)) {
        if (GetPCMUX(CURRENT_LATCHES.MICROINSTRUCTION) == 0) {
            NEXT_LATCHES.PC = Low16bits(CURRENT_LATCHES.PC + 2);

        } else if (GetPCMUX(CURRENT_LATCHES.MICROINSTRUCTION) == 1) {
            int adder = Low16bits(CURRENT_LATCHES.PC + ((CURRENT_LATCHES.IR & 0x07FF) << 1));
            NEXT_LATCHES.PC = adder;
//            NEXT_LATCHES.PC = Low16bits(BUS);
        } else if (GetPCMUX(CURRENT_LATCHES.MICROINSTRUCTION) == 2) {
            int adder = Low16bits(CURRENT_LATCHES.REGS[(CURRENT_LATCHES.IR >> 6) & 0x07]);
            NEXT_LATCHES.PC = adder;
        }else{
            NEXT_LATCHES.PC = Low16bits(tmp);
        }
    }
    //correct
    if (GetLD_IR(CURRENT_LATCHES.MICROINSTRUCTION)) {
        NEXT_LATCHES.IR = Low16bits(BUS);
    }
    if (GetLD_REG(CURRENT_LATCHES.MICROINSTRUCTION)) {
        int reg_num;
        if (GetDRMUX(CURRENT_LATCHES.MICROINSTRUCTION)) {
            reg_num = 7;
        } else {
            reg_num = (CURRENT_LATCHES.IR & 0x0E00) >> 9;
        }
        NEXT_LATCHES.REGS[reg_num] = Low16bits(BUS);

        if (GetLD_CC(CURRENT_LATCHES.MICROINSTRUCTION)) {
            int bus_value = Low16bits(BUS);
            if (bus_value & 0x8000) {
                NEXT_LATCHES.N = 1;
            } else {
                NEXT_LATCHES.N = 0;
            }
            if (bus_value == 0) {
                NEXT_LATCHES.Z = 1;
            } else {
                NEXT_LATCHES.Z = 0;
            }
            if (bus_value > 0 && bus_value <= 0x7FFF) {
                NEXT_LATCHES.P = 1;
            } else {
                NEXT_LATCHES.P = 0;
            }
        } else {
            NEXT_LATCHES.N = CURRENT_LATCHES.N;
            NEXT_LATCHES.Z = CURRENT_LATCHES.Z;
            NEXT_LATCHES.P = CURRENT_LATCHES.P;
        }
    }
    if (GetLD_MAR(CURRENT_LATCHES.MICROINSTRUCTION)) {
        NEXT_LATCHES.MAR = Low16bits(BUS);
    }
    if (GetLD_MDR(CURRENT_LATCHES.MICROINSTRUCTION)){
        if (GetMIO_EN(CURRENT_LATCHES.MICROINSTRUCTION)){
            if (CURRENT_LATCHES.READY){

            }
        }
        if (!GetMIO_EN(CURRENT_LATCHES.MICROINSTRUCTION) && !GetDATA_SIZE(CURRENT_LATCHES.MICROINSTRUCTION)) {
            NEXT_LATCHES.MDR = Low16bits((BUS & 0xFF) << 8);
        } else if (!GetMIO_EN(CURRENT_LATCHES.MICROINSTRUCTION)) {
            NEXT_LATCHES.MDR = Low16bits(BUS);
        }
    }
}

