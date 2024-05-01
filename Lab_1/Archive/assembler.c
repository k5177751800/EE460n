//Konghwan Shin KS54897
//Hyukjoo Chung HC29268
#include <stdio.h> /* standard input/output library */
#include <stdlib.h> /* Standard C Library */
#include <string.h> /* String operations library */
#include <ctype.h> /* Library for useful character operations */
#include <limits.h> /* Library for definitions of common variable type characteristics */

#define MAX_LINE_LENGTH 255
#define MAX_LABEL_LEN 20
#define MAX_SYMBOLS 255
//TRap, LEA, BR,
int isOpcode(char* input);
int readAndParse( FILE * pInfile, char * pLine, char ** pLabel, char
** pOpcode, char ** pArg1, char ** pArg2, char ** pArg3, char ** pArg4
);
int ImmtoHex(char * input);
enum
{
    DONE, OK, EMPTY_LINE
};
typedef struct {
    int address;
    char label[MAX_LABEL_LEN + 1];	/* Question for the reader: Why do we need to add 1? */
} TableEntry;
TableEntry symbolTable[MAX_SYMBOLS];
int symbolLookUp(char* symbol, int add);
int main(int argc, char* argv[]){
    int *start = malloc(sizeof(int));

    //printf("%d\n", isOpcode("AND")); //Test


    int line = 0;
    int cnt = 0;


    char lLine[MAX_LINE_LENGTH + 1], *lLabel, *lOpcode, *lArg1,
            *lArg2, *lArg3, *lArg4;

    int lRet;
    FILE * lInfile;
    lInfile = fopen( argv[1], "r" );	/* open the input file */
    FILE * pOutfile;
    pOutfile = fopen( argv[2], "w" );
    uint16_t lInstr = 0;
    int p = 0;
    do {

        lRet = readAndParse(lInfile, lLine, &lLabel,
                            &lOpcode, &lArg1, &lArg2, &lArg3, &lArg4);
            if (strcmp(lOpcode,".orig")==0) {
                start[p] = ImmtoHex(lArg1);
                line = ImmtoHex(lArg1);
                p++;
            } else if (isOpcode(lOpcode) != -1 ||strcmp(lOpcode,".fill")==0||strcmp(lOpcode,".end")==0){
                line++;
            }
            if (isalnum(lLabel[0])) {
                cnt++;
                strcpy(symbolTable[cnt].label, lLabel);
                symbolTable[cnt].address = line;
            }
        }
    while( lRet != DONE );
    rewind(lInfile);
    line = 0 ;
    int t = 0;
    p=0;

    do
    {

        lRet = readAndParse( lInfile, lLine, &lLabel,
                             &lOpcode, &lArg1, &lArg2, &lArg3, &lArg4 );
        if( lRet != DONE && lRet != EMPTY_LINE ) {
            lInstr = 0;
            if (strcmp(lOpcode, ".orig") == 0) {
                lInstr += start[p];
                line = start[p];
                p++;
            }
            if (strcmp(lOpcode, ".end") == 0) {
                t++;
            }
            if (p == t+1) {

                if (isOpcode(lOpcode) != -1) {
                    line++;
                    lInstr = isOpcode(lOpcode) << 12;
                    if (lInstr == 0x1000 || lInstr == 0x5000 || lInstr == 0x9000) {
                        //ADD R1, R1, x-10	;x-10 is the negative of x10 DOesn't work

                        //it is imm input 1
                        //DR,Sr here
                        lInstr |= (lArg1[1] - '0') << 9; //DR
                        lInstr |= (lArg2[1] - '0') << 6; //SR
                        if (lArg3[0] == '#' || lArg3[0] == 'x') {//0x?
                            lInstr |= 0x0020;
                            int hVal = ImmtoHex(lArg3);
                            if (hVal >= 0) {
                                lInstr += hVal;
                            } else {
                                lInstr |= 0x0010;
                                if (hVal != -16) {
                                    int tmp = 8;
                                    int ct = 4;
                                    while (ct != 0) {
                                        ct--;
                                        if (hVal <= -tmp) {
                                            hVal += tmp;
                                        } else {
                                            lInstr += 1 << (ct);
                                        }

                                        tmp /= 2;
                                    }
                                    lInstr += 1;
                                }
                            }
                        } else {
                            lInstr += lArg3[1] - '0';
                        }
                    }
                    if (lInstr == 0x0000) {
                        //BR statement
                        //Change it to HEX
                        if (strlen(lOpcode) == 2) {
                            lInstr += 7 << 9;
                        }
                        lInstr += (lOpcode[2] == 'n') << 11;
                        lInstr += (lOpcode[2] == 'z' || lOpcode[3] == 'z') << 10;
                        lInstr += (lOpcode[2] == 'p' || lOpcode[3] == 'p' || lOpcode[4] == 'p') << 9;
                        int off = symbolLookUp(lArg1, line) - 1;
                        if (off < 0) {
                            lInstr |= (1 << 8);
                            int oH = off;
                            oH &= 0x1FF;
                            lInstr |= oH;
                        } else {
                            lInstr += off;
                        }

                    }
                    if (lInstr == 0xC000) {
                        //JMP, RET
                        if (strlen(lArg1) != 0) {
                            lInstr |= (lArg1[1] - '0') << 6;
                        } else {
                            lInstr |= 7 << 6;
                        }

                    }
                    if (lInstr == 0x4000) {
                        //JSR, JSRR
                        if (strlen(lOpcode) == 3) {
                            lInstr |= 1 << 11;
                            uint16_t off11 = symbolLookUp(lArg1, line) - 1;
                            if (off11 < 0) {
                                off11 = ~off11 + 1;
                                off11 ^= 0xF800;
                            }
                            lInstr |= off11;
                        } else {
                            //JSRR
                            lInstr |= (lArg1[1] - '0') << 6;
                        }
                    }
                    if (lInstr == 0xE000) {
                        //LEA
                        lInstr |= (lArg1[1] - '0') << 9;
                        int off = symbolLookUp(lArg2, line) - 1;
                        if (off < 0) {
                            lInstr += 1 << 8;
                            uint8_t oH = -off;
                            oH = ~oH + 1; //2sc
                            lInstr |= oH;
                        } else {
                            lInstr += off;
                        }

                    }
                    // RTI: self-achieved
                    if (lInstr == 0xD000) {
                        lInstr |= (lArg1[1] - '0') << 9;
                        lInstr |= (lArg2[1] - '0') << 6;
                        if (strlen(lOpcode) == 5 && lOpcode[4] == 'L') {
                            lInstr += 1 << 4;
                        } else if (strlen(lOpcode) == 5 && lOpcode[4] == 'A') {
                            lInstr += 3 << 4;
                        }
                        lInstr |= ImmtoHex(lArg3);
                    }
                    if (lInstr == 0xF000) {
                        //TRAP
                        if (strcmp(lOpcode, "halt") == 0) {
                            lInstr |= 0x25;
                        } else {
                            lInstr |= ImmtoHex(lArg1);
                        }
                    }
                    if (lInstr == 0x6000 || lInstr == 0x7000) {
                        lInstr |= (lArg1[1] - '0') << 9;
                        lInstr |= (lArg2[1] - '0') << 6;
                        int8_t ofs = ImmtoHex(lArg3);
                        ofs &= 0x3F;

                        lInstr |= ofs;
                    }
                    if (lInstr == 0x2000) {
                        //LDB
                        lInstr |= (lArg1[1] - '0') << 9;
                        lInstr |= (lArg2[1] - '0') << 6;
                        uint8_t ofs = ImmtoHex(lArg3);
                        if (ofs & 0x20) {
                            ofs |= 0xC0;
                        }
                        lInstr |= ofs;
                    }
                    if (lInstr == 0x3000) {
                        //STB
                        lInstr |= (lArg1[1] - '0') << 9;
                        lInstr |= (lArg2[1] - '0') << 6;
                        uint8_t ofs = ImmtoHex(lArg3);
                        if (ofs & 0x20) {
                            ofs |= 0xC0;
                        }
                        lInstr |= ofs;
                    }

                    // check for the same upcode with different uperand
                    //increment opcode?? and change it to hex at the end????
                    //                if (lArg1 != NULL) {
                    //                    lInstr |= 0x0010;
                    //                }

                } else if (strcmp(lOpcode, ".fill") == 0) {
                    lInstr += ImmtoHex(lArg1);
                }
                fprintf(pOutfile,"0x%.4X\n", lInstr);
            }
        }

    } while( lRet != DONE );
//    printf("%i",ImmtoHex("#15")); //imm to hex test code


    free(start);
    return 0;

//    fprintf(pOutfile, "0x%.4X\n", lInstr);
}
int symbolLookUp(char* symbol, int add){
    //Checks for symbolls of Offsett
    for (int i = 0; i < MAX_SYMBOLS; i++) {
        if (strncmp(symbolTable[i].label, symbol, strlen(symbol))==0) {
            return symbolTable[i].address - add;
        }
    }
    return (int) NULL;
}
int isOpcode(char* input) {
    typedef struct {
        char* name;
        unsigned short value;
    } Opcode;


    Opcode opcodes[] = {
            {"br", 0x0}, {"add", 0x1}, {"jsr", 0x4}, {"and", 0x5}, {"jmp", 0xC}, {"jsrr", 0x4}, {"ldb", 0x2},
            {"ldw", 0x6}, {"lea", 0xE}, {"not", 0x9}, {"ret", 0xC},
            {"lshf", 0xD}, {"rshfl", 0xD}, {"rshfa", 0xD}, {"stb", 0x3},
            {"stw", 0x7}, {"trap", 0xF},{"halt", 0xF}, {"xor", 0x9}
    };
//    for (int i = 0; i < 19; i++) {
//        if (input == opcodes[i].name) {
//            return opcodes[i].value;
//        }
//    }

    for (int i = 0; i < sizeof(opcodes)/sizeof(opcodes[0]); i++) {
        //BRNZP? => works
        if (strcmp(input, opcodes[i].name) == 0) {
            return opcodes[i].value;
        }
        else if(strncmp(input, "br", 2) == 0){
            if (input[2] == 'n'){
                if (input[3] == 'z'||input[3] == 'p'){
                    if (input[4] == 'p' && strlen(input) == 5){
                        return 0;
                    }
                    if (strlen(input) == 4){
                        return 0;
                    }
                }
                if (strlen(input) == 3){
                    return 0;
                }
            }
            if (input[2] == 'z'){
                if (input[3] == 'p' && strlen(input) == 4){
                    return 0;
                }
                if (strlen(input) ==3){
                    return 0;
                }
            }
            if (input[2] == 'p' && strlen(input) == 3){
                return 0;
            }
        }
    }
    return -1;
}
int ImmtoHex(char * input){
    //immediate to HEX. negate when negative.
    int neg = 0;
    int ret = 0;
    if (input[1] == '-') {neg = 1;}//2sc
    for (int i = 1+neg; i < strlen(input); i++) {
        if (input[0] == '#') {
            ret *= 10;
        }
        else if (input[0] == 'x') {
            ret *= 16;
        }
        if (input[i]-'0' <= 9) {
            ret += input[i]-'0';
        }
        else if (input[i]-'a' <= 5) {
            ret += input[i]-'a'+10;
        }
    }
    if (neg) {
        ret = -ret;
    }
    return ret;
}

int readAndParse( FILE * pInfile, char * pLine, char ** pLabel, char
** pOpcode, char ** pArg1, char ** pArg2, char ** pArg3, char ** pArg4
){
    char * lRet, * lPtr;
    int i;
    if( !fgets( pLine, MAX_LINE_LENGTH, pInfile ) )
        return( DONE );
    for( i = 0; i < strlen( pLine ); i++ )
        pLine[i] = tolower( pLine[i] );

    /* convert entire line to lowercase */
    *pLabel = *pOpcode = *pArg1 = *pArg2 = *pArg3 = *pArg4 = pLine + strlen(pLine);

    /* ignore the comments */
    lPtr = pLine;

    while( *lPtr != ';' && *lPtr != '\0' &&
           *lPtr != '\n' )
        lPtr++;

    *lPtr = '\0';
    if( !(lPtr = strtok( pLine, "\t\n ," ) ) )
        return( EMPTY_LINE );

    if( isOpcode( lPtr ) == -1 && lPtr[0] != '.' ) /* found a label */
    {
        *pLabel = lPtr;
        if( !( lPtr = strtok( NULL, "\t\n ," ) ) ) return( OK );
    }

    *pOpcode = lPtr;

    if( !( lPtr = strtok( NULL, "\t\n ," ) ) ) return( OK );

    *pArg1 = lPtr;

    if( !( lPtr = strtok( NULL, "\t\n ," ) ) ) return( OK );

    *pArg2 = lPtr;
    if( !( lPtr = strtok( NULL, "\t\n ," ) ) ) return( OK );

    *pArg3 = lPtr;

    if( !( lPtr = strtok( NULL, "\t\n ," ) ) ) return( OK );

    *pArg4 = lPtr;

    return( OK );
}
