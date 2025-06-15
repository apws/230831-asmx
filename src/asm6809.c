// asmM6809.c

#define versionName "M6809 assembler"
#include "asmx.h"

enum cputype
{
    CPU_6809,        // 6809 instructions only
    CPU_6309         // 6309 instructions enabled
};

enum
{
    OP_Inherent,     // implied instructions
    OP_Immediate,    // immediate instructions
    OP_Relative,     // branch instructions
    OP_LRelative,    // 16-bit branch instructions
    OP_Indexed,      // indexed-only instructions
    OP_PshPul,       // PSHS/PULS instructions
    OP_ExgTfr,       // EXG/TFR instructions
    OP_Logical,      // logical instructions with multiple addressing modes
    OP_Arith,        // arithmetic instructions with multiple addressing modes
    OP_LArith,       // OP_Arith instructions with 16-bit immediate modes
    OP_QArith,       // 6309 OP_Arith with 4-byte immediate
    OP_TFM,          // 6309 TFM instruction
    OP_Bit,          // 6309 bit manipulation instructions

    OP_6309 = 0x8000, // add to parm value for 6309 opcodes

    OP_SETDP = OP_LabelOp // SETDP pseudo-op
};

static const struct OpcdRec M6809_opcdTab[] =
{
    {"NOP",  OP_Inherent, 0x12},
    {"SYNC",  OP_Inherent, 0x13},
    {"DAA",  OP_Inherent, 0x19},
    {"SEX",  OP_Inherent, 0x1D},
    {"RTS",  OP_Inherent, 0x39},
    {"ABX",  OP_Inherent, 0x3A},
    {"RTI",  OP_Inherent, 0x3B},
    {"MUL",  OP_Inherent, 0x3D},
    {"SWI",  OP_Inherent, 0x3F},
    {"NEGA",  OP_Inherent, 0x40},
    {"COMA",  OP_Inherent, 0x43},
    {"LSRA",  OP_Inherent, 0x44},
    {"RORA",  OP_Inherent, 0x46},
    {"ASRA",  OP_Inherent, 0x47},
    {"ASLA",  OP_Inherent, 0x48},
    {"LSLA",  OP_Inherent, 0x48},
    {"ROLA",  OP_Inherent, 0x49},
    {"DECA",  OP_Inherent, 0x4A},
    {"INCA",  OP_Inherent, 0x4C},
    {"TSTA",  OP_Inherent, 0x4D},
    {"CLRA",  OP_Inherent, 0x4F},
    {"NEGB",  OP_Inherent, 0x50},
    {"COMB",  OP_Inherent, 0x53},
    {"LSRB",  OP_Inherent, 0x54},
    {"RORB",  OP_Inherent, 0x56},
    {"ASRB",  OP_Inherent, 0x57},
    {"ASLB",  OP_Inherent, 0x58},
    {"LSLB",  OP_Inherent, 0x58},
    {"ROLB",  OP_Inherent, 0x59},
    {"DECB",  OP_Inherent, 0x5A},
    {"INCB",  OP_Inherent, 0x5C},
    {"TSTB",  OP_Inherent, 0x5D},
    {"CLRB",  OP_Inherent, 0x5F},
    {"SWI2",  OP_Inherent, 0x103F},
    {"SWI3",  OP_Inherent, 0x113F},

    {"ORCC",  OP_Immediate, 0x1A},
    {"ANDCC", OP_Immediate, 0x1C},
    {"CWAI",  OP_Immediate, 0x3C},

    {"BRA",   OP_Relative, 0x20},
    {"BRN",   OP_Relative, 0x21},
    {"BHI",   OP_Relative, 0x22},
    {"BLS",   OP_Relative, 0x23},
    {"BCC",   OP_Relative, 0x24},
    {"BCS",   OP_Relative, 0x25},
    {"BHS",   OP_Relative, 0x24},
    {"BLO",   OP_Relative, 0x25},
    {"BNE",   OP_Relative, 0x26},
    {"BEQ",   OP_Relative, 0x27},
    {"BVC",   OP_Relative, 0x28},
    {"BVS",   OP_Relative, 0x29},
    {"BPL",   OP_Relative, 0x2A},
    {"BMI",   OP_Relative, 0x2B},
    {"BGE",   OP_Relative, 0x2C},
    {"BLT",   OP_Relative, 0x2D},
    {"BGT",   OP_Relative, 0x2E},
    {"BLE",   OP_Relative, 0x2F},
    {"BSR",   OP_Relative, 0x8D},

    {"LBRA",  OP_LRelative, 0x16},
    {"LBSR",  OP_LRelative, 0x17},
    {"LBRN",  OP_LRelative, 0x1021},
    {"LBHI",  OP_LRelative, 0x1022},
    {"LBLS",  OP_LRelative, 0x1023},
    {"LBCC",  OP_LRelative, 0x1024},
    {"LBCS",  OP_LRelative, 0x1025},
    {"LBHS",  OP_LRelative, 0x1024},
    {"LBLO",  OP_LRelative, 0x1025},
    {"LBNE",  OP_LRelative, 0x1026},
    {"LBEQ",  OP_LRelative, 0x1027},
    {"LBVC",  OP_LRelative, 0x1028},
    {"LBVS",  OP_LRelative, 0x1029},
    {"LBPL",  OP_LRelative, 0x102A},
    {"LBMI",  OP_LRelative, 0x102B},
    {"LBGE",  OP_LRelative, 0x102C},
    {"LBLT",  OP_LRelative, 0x102D},
    {"LBGT",  OP_LRelative, 0x102E},
    {"LBLE",  OP_LRelative, 0x102F},

    {"LEAX",  OP_Indexed, 0x30},
    {"LEAY",  OP_Indexed, 0x31},
    {"LEAS",  OP_Indexed, 0x32},
    {"LEAU",  OP_Indexed, 0x33},

    {"PSHS",  OP_PshPul, 0x34},
    {"PULS",  OP_PshPul, 0x35},
    {"PSHU",  OP_PshPul, 0x36},
    {"PULU",  OP_PshPul, 0x37},
    {"EXG",   OP_ExgTfr, 0x1E},
    {"TFR",   OP_ExgTfr, 0x1F},

    {"NEG",   OP_Logical, 0x00}, // OP_Logical: direct = 00; indexed = 60; extended = 70
    {"COM",   OP_Logical, 0x03},
    {"LSR",   OP_Logical, 0x04},
    {"ROR",   OP_Logical, 0x06},
    {"ASR",   OP_Logical, 0x07},
    {"ASL",   OP_Logical, 0x08},
    {"LSL",   OP_Logical, 0x08},
    {"ROL",   OP_Logical, 0x09},
    {"DEC",   OP_Logical, 0x0A},
    {"INC",   OP_Logical, 0x0C},
    {"TST",   OP_Logical, 0x0D},
    {"JMP",   OP_Logical, 0x0E},
    {"CLR",   OP_Logical, 0x0F},

    {"SUBA",  OP_Arith, 0x80},   // iArith: immediate = 00, direct = 10, indexed = 20, extended = 30
    {"CMPA",  OP_Arith, 0x81},
    {"SBCA",  OP_Arith, 0x82},
    {"ANDA",  OP_Arith, 0x84},
    {"BITA",  OP_Arith, 0x85},
    {"LDA",   OP_Arith, 0x86},
    {"STA",   OP_Arith, 0x97},   // * 0x10 = no immediate
    {"EORA",  OP_Arith, 0x88},
    {"ADCA",  OP_Arith, 0x89},
    {"ORA",   OP_Arith, 0x8A},
    {"ADDA",  OP_Arith, 0x8B},
    {"JSR",   OP_Arith, 0x9D},   // *
    {"STX",   OP_Arith, 0x9F},   // *
    {"SUBB",  OP_Arith, 0xC0},
    {"CMPB",  OP_Arith, 0xC1},
    {"SBCB",  OP_Arith, 0xC2},
    {"ANDB",  OP_Arith, 0xC4},
    {"BITB",  OP_Arith, 0xC5},
    {"LDB",   OP_Arith, 0xC6},
    {"STB",   OP_Arith, 0xD7},   // *
    {"EORB",  OP_Arith, 0xC8},
    {"ADCB",  OP_Arith, 0xC9},
    {"ORB",   OP_Arith, 0xCA},
    {"ADDB",  OP_Arith, 0xCB},
    {"JSR",   OP_Arith, 0x9D},   // *
    {"STX",   OP_Arith, 0x9F},   // *
    {"STD",   OP_Arith, 0xDD},   // *
    {"STU",   OP_Arith, 0xDF},   // *
    {"STY",   OP_Arith, 0x109F}, // *
    {"STS",   OP_Arith, 0x10DF}, // *

    {"SUBD", OP_LArith, 0x83},
    {"ADDD", OP_LArith, 0xC3},
    {"CMPX", OP_LArith, 0x8C},
    {"LDX", OP_LArith, 0x8E},
    {"LDD", OP_LArith, 0xCC},
    {"LDU", OP_LArith, 0xCE},
    {"CMPD", OP_LArith, 0x1083},
    {"CMPY", OP_LArith, 0x108C},
    {"LDY", OP_LArith, 0x108E},
    {"LDS", OP_LArith, 0x10CE},
    {"CMPU", OP_LArith, 0x1183},
    {"CMPS", OP_LArith, 0x118C},

    {"SETDP",OP_SETDP,0},

// 6800 compatiblity opcodes

    {"CLC",  OP_Inherent, 0x1CFE},   // ANDCC #$FE
    {"CLI",  OP_Inherent, 0x1CEF},   // ANDCC #$EF
    {"CLV",  OP_Inherent, 0x1CFD},   // ANDCC #$FD
    {"SEC",  OP_Inherent, 0x1A01},   // ORCC  #$01
    {"SEI",  OP_Inherent, 0x1A10},   // ORCC  #$10
    {"SEV",  OP_Inherent, 0x1A02},   // ORCC  #$02
    {"DEX",  OP_Inherent, 0x301F},   // LEAX -1,X
    {"DES",  OP_Inherent, 0x337F},   // LEAS -1,S
    {"INX",  OP_Inherent, 0x3001},   // LEAX  1,X
    {"INS",  OP_Inherent, 0x3361},   // LEAS  1,S
    {"PSHA", OP_Inherent, 0x3402},   // PSHS A
    {"PSHB", OP_Inherent, 0x3504},   // PSHS B
    {"PULA", OP_Inherent, 0x3402},   // PULS A
    {"PULB", OP_Inherent, 0x3504},   // PULS B
    {"ABA",  OP_Inherent, 0xABE0},   // 3504 ABE0 = PSHS B / ADDA ,S+
    {"CBA",  OP_Inherent, 0xA1E0},   // 3504 A1E0 = PSHS B / CMPA ,S+
    {"SBA",  OP_Inherent, 0xA0E0},   // 3504 A0E0 = PSHS B / SUBA ,S+
    {"TAB",  OP_Inherent, 0x1F89},   // TFR A,B
    {"TAP",  OP_Inherent, 0x1F8A},   // TFR A,CC
    {"TBA",  OP_Inherent, 0x1F98},   // TFR B,A
    {"TPA",  OP_Inherent, 0x1FA8},   // TFR CC,A
    {"TSX",  OP_Inherent, 0x1F41},   // TFR S,X
    {"TXS",  OP_Inherent, 0x1F14},   // TFR X,S
    {"WAI",  OP_Inherent, 0x3CFF},   // CWAI #$FF
    {"LDAA", OP_Arith,    0x86},     // alternate mnemonic for LDA
    {"STAA", OP_Arith,    0x97},     // alternate mnemonic for STA
    {"LDAB", OP_Arith,    0xC6},     // alternate mnemonic for LDB
    {"STAB", OP_Arith,    0xD7},     // alternate mnemonic for STB
    {"ORAA", OP_Arith,    0x8A},     // alternate mnemonic for ORA
    {"ORAB", OP_Arith,    0xCA},     // alternate mnemonic for ORB

// 6801 compatibility opcodes

    {"LSRD", OP_Inherent, 0x4456},   // LSRA / RORB
    {"ASLD", OP_Inherent, 0x5849},   // ASLB / ROLA
    {"LSLD", OP_Inherent, 0x5849},   // ASLB / ROLA
    {"PULX", OP_Inherent, 0x3410},   // PULS X
    {"PSHX", OP_Inherent, 0x3510},   // PSHS X
    {"LDAD", OP_LArith,   0xCC},     // alternate mnemonic for LDD
    {"STAD", OP_Arith,    0xDD},     // alternate mnemonic for STD

// 6309 opcodes

    {"ASLD",  OP_Inherent, 0x1048 | OP_6309},
    {"ASRD",  OP_Inherent, 0x1047 | OP_6309},
    {"CLRD",  OP_Inherent, 0x104F | OP_6309},
    {"CLRE",  OP_Inherent, 0x114F | OP_6309},
    {"CLRF",  OP_Inherent, 0x115F | OP_6309},
    {"CLRW",  OP_Inherent, 0x105F | OP_6309},
    {"COMD",  OP_Inherent, 0x1043 | OP_6309},
    {"COME",  OP_Inherent, 0x1143 | OP_6309},
    {"COMF",  OP_Inherent, 0x1153 | OP_6309},
    {"COMW",  OP_Inherent, 0x1053 | OP_6309},
    {"DECD",  OP_Inherent, 0x104A | OP_6309},
    {"DECE",  OP_Inherent, 0x114A | OP_6309},
    {"DECF",  OP_Inherent, 0x115A | OP_6309},
    {"DECW",  OP_Inherent, 0x105A | OP_6309},
    {"INCD",  OP_Inherent, 0x104C | OP_6309},
    {"INCE",  OP_Inherent, 0x114C | OP_6309},
    {"INCF",  OP_Inherent, 0x115C | OP_6309},
    {"INCW",  OP_Inherent, 0x105C | OP_6309},
    {"LSLD",  OP_Inherent, 0x1048 | OP_6309},
    {"LSRD",  OP_Inherent, 0x1044 | OP_6309},
    {"LSRW",  OP_Inherent, 0x1054 | OP_6309},
    {"NEGD",  OP_Inherent, 0x1040 | OP_6309},
    {"PSHSW", OP_Inherent, 0x1038 | OP_6309},
    {"PSHUW", OP_Inherent, 0x103A | OP_6309},
    {"PULSW", OP_Inherent, 0x1039 | OP_6309},
    {"PULUW", OP_Inherent, 0x103B | OP_6309},
    {"ROLD",  OP_Inherent, 0x1049 | OP_6309},
    {"ROLW",  OP_Inherent, 0x1059 | OP_6309},
    {"RORD",  OP_Inherent, 0x1046 | OP_6309},
    {"RORW",  OP_Inherent, 0x1056 | OP_6309},
    {"SEXW",  OP_Inherent, 0x14   | OP_6309},
    {"TSTD",  OP_Inherent, 0x104D | OP_6309},
    {"TSTE",  OP_Inherent, 0x114D | OP_6309},
    {"TSTF",  OP_Inherent, 0x115D | OP_6309},
    {"TSTW",  OP_Inherent, 0x105D | OP_6309},

    {"BITMD", OP_Immediate, 0x113C | OP_6309},
    {"LDMD",  OP_Immediate, 0x113D | OP_6309},

    {"AIM",  OP_Logical, 0x02 | OP_6309},
    {"EIM",  OP_Logical, 0x05 | OP_6309},
    {"OIM",  OP_Logical, 0x01 | OP_6309},
    {"TIM",  OP_Logical, 0x0B | OP_6309},

    {"ADDE",  OP_Arith, 0x118B | OP_6309},
    {"ADDF",  OP_Arith, 0x11CB | OP_6309},
    {"CMPE",  OP_Arith, 0x1181 | OP_6309},
    {"CMPF",  OP_Arith, 0x11C1 | OP_6309},
    {"DIVD",  OP_Arith, 0x118D | OP_6309},
    {"LDE",  OP_Arith, 0x1186 | OP_6309},
    {"LDF",  OP_Arith, 0x11C6 | OP_6309},
    {"STE",  OP_Arith, 0x1197 | OP_6309},
    {"STF",  OP_Arith, 0x11D7 | OP_6309},
    {"STQ",  OP_Arith, 0x10DD | OP_6309},
    {"STS",  OP_Arith, 0x10DF | OP_6309},
    {"SUBE",  OP_Arith, 0x1180 | OP_6309},
    {"SUBF",  OP_Arith, 0x11C0 | OP_6309},

    {"ADCD",  OP_LArith, 0x1089 | OP_6309},
    {"ADDW",  OP_LArith, 0x108B | OP_6309},
    {"ANDD",  OP_LArith, 0x1084 | OP_6309},
    {"BITD",  OP_LArith, 0x1085 | OP_6309},
    {"CMPW",  OP_LArith, 0x1081 | OP_6309},
    {"DIVQ",  OP_LArith, 0x118E | OP_6309},
    {"EORD",  OP_LArith, 0x1088 | OP_6309},
    {"LDW",  OP_LArith, 0x1086 | OP_6309},
    {"MULD",  OP_LArith, 0x118F | OP_6309},
    {"ORD",  OP_LArith, 0x108A | OP_6309},
    {"SBCD",  OP_LArith, 0x1082 | OP_6309},
    {"STW",  OP_LArith, 0x1097 | OP_6309},
    {"SUBW",  OP_LArith, 0x1080 | OP_6309},

    {"LDQ",  OP_QArith, 0x10CC | OP_6309},  // special: immed is 0xCD

    {"ADCR",  OP_ExgTfr, 0x1031 | OP_6309},
    {"ADDR",  OP_ExgTfr, 0x1030 | OP_6309},
    {"ANDR",  OP_ExgTfr, 0x1034 | OP_6309},
    {"CMPR",  OP_ExgTfr, 0x1037 | OP_6309},
    {"EORR",  OP_ExgTfr, 0x1036 | OP_6309},
    {"ORR",  OP_ExgTfr, 0x1035 | OP_6309},
    {"SBCR",  OP_ExgTfr, 0x1033 | OP_6309},
    {"SUBR",  OP_ExgTfr, 0x1032 | OP_6309},

    {"TFM",  OP_TFM, 0x1138 | OP_6309},

    {"BAND",  OP_Bit, 0x1130 | OP_6309},
    {"BIAND", OP_Bit, 0x1131 | OP_6309},
    {"BOR",  OP_Bit, 0x1132 | OP_6309},
    {"BIOR",  OP_Bit, 0x1133 | OP_6309},
    {"BEOR",  OP_Bit, 0x1134 | OP_6309},
    {"BIEOR", OP_Bit, 0x1135 | OP_6309},
    {"LDBT",  OP_Bit, 0x1136 | OP_6309},
    {"STBT",  OP_Bit, 0x1137 | OP_6309},

    {"",     OP_Illegal, 0}
};


const char tfrRegs[] = "D X Y U S PC X X A B CC DP"; // extra X's are placeholders
const char tfrRegs6309[] = "D X Y U S PC W V A B CC DP 0 00 E F";
const char pshRegs[] = "CC A B DP X Y U PC D S";
const char idxRegs[] = "X Y U S";
const char idxRegsW[] = "X Y U S W";

uint8_t dpReg;


// --------------------------------------------------------------


static void M6809_Indexed(int idxOp, int dirOp, int extOp)
{
//  idxOp is required
//  if dirOp == 0, use extOp
//  if extOp == 0, use idxOp

    Str255  word;
    int     reg;

    int indirect = 0;
    char force = 0;

    char *oldLine = linePtr;
    int token = TOKEN_GetWord(word);
    if (token == '[')
    {
        indirect = 0x10;
        oldLine = linePtr;
        token = TOKEN_GetWord(word);
    }

    // handle A, B, D as tokens
    if (token == -1 && word[1] == 0 && (word[0] == 'A' || word[0] == 'B' || word[0] == 'D'))
    {
        token = word[0];
    }

    // handle E, F, W as tokens
    if (curCPU == CPU_6309 &&
            token == -1 && word[1] == 0 && (word[0] == 'E' || word[0] == 'F' || word[0] == 'W'))
    {
        token = word[0];
    }

    switch (token)
    {
        case ',':
            token = TOKEN_GetWord(word);
            if (token == '-')
            {
                if (*linePtr == '-')
                {
                    linePtr++;
                    TOKEN_GetWord(word);
                    reg = REG_Find(word, idxRegsW);
                    if (curCPU == CPU_6309 && reg == 4)
                    {
                        // ,--W
                        INSTR_XB(idxOp, 0xEF + !!indirect);  // ,--X
                    }
                    else if (reg < 0 || reg > 3)
                    {
                        TOKEN_BadMode();
                    }
                    else
                    {
                        INSTR_XB(idxOp, reg * 0x20 + 0x83 + indirect);  // ,--X
                    }
                }
                else
                {
                    TOKEN_GetWord(word);
                    reg = REG_Find(word, idxRegs);
                    if (reg < 0)
                    {
                        TOKEN_BadMode();
                    }
                    else
                    {
                        INSTR_XB(idxOp, reg * 0x20 + 0x82 + indirect);  // ,-X
                    }
                }
            }
            else
            {
                reg = REG_Find(word, idxRegsW);
                if (curCPU == CPU_6309 && reg == 4)
                {
                    // ,W / ,W++
                    oldLine = linePtr;
                    token = TOKEN_GetWord(word);
                    switch (token)
                    {
                        case ']':
                            linePtr = oldLine;
                            FALLTHROUGH;
                        case 0: // ,W
                            INSTR_XB(idxOp, 0x8F + !!indirect);  // ,X
                            break;
                        case '+': // ,W++
                            if (*linePtr == '+')
                            {
                                linePtr++;
                                INSTR_XB(idxOp, 0xCF + !!indirect);  // ,W++
                                break;
                            }
                            FALLTHROUGH;
                        default:
                            TOKEN_BadMode();
                            break;
                    }
                }
                else if (reg < 0 || reg > 3)
                {
                    TOKEN_BadMode();
                }
                else
                {
                    oldLine = linePtr;
                    token = TOKEN_GetWord(word);
                    switch (token)
                    {
                        case ']':
                            linePtr = oldLine;
                            FALLTHROUGH;
                        case 0:
                            INSTR_XB(idxOp, reg * 0x20 + 0x84 + indirect);  // ,X
                            break;
                        case '+':
                            if (*linePtr == '+')
                            {
                                linePtr++;
                                INSTR_XB(idxOp, reg * 0x20 + 0x81 + indirect);  // ,X++
                            }
                            else
                            {
                                INSTR_XB(idxOp, reg * 0x20 + 0x80 + indirect);  // ,X+
                            }
                            break;
                        default:
                            TOKEN_BadMode();
                            break;
                    }
                }
            }
            break;

        case 'A':
        case 'B':
        case 'D':
        case 'E':
        case 'F':
        case 'W':
            TOKEN_Comma();
            TOKEN_GetWord(word);
            reg = REG_Find(word, idxRegs);
            if (reg < 0)
            {
                TOKEN_BadMode();
            }
            else
            {
                switch (token)
                {
                    case 'A':
                        INSTR_XB(idxOp, reg * 0x20 + 0x86 + indirect);
                        break;        // A,X
                    case 'B':
                        INSTR_XB(idxOp, reg * 0x20 + 0x85 + indirect);
                        break;        // B,X
                    case 'D':
                        INSTR_XB(idxOp, reg * 0x20 + 0x8B + indirect);
                        break;        // D,X
                    case 'E':
                        INSTR_XB(idxOp, reg * 0x20 + 0x87 + indirect);
                        break;        // A,X
                    case 'F':
                        INSTR_XB(idxOp, reg * 0x20 + 0x8A + indirect);
                        break;        // B,X
                    case 'W':
                        INSTR_XB(idxOp, reg * 0x20 + 0x8E + indirect);
                        break;        // D,X
                    default:
                        TOKEN_BadMode();
                }
            }
            break;

        case '<':
        case '>':
            force = token;
            FALLTHROUGH;
        default:
            if (!force)
            {
                linePtr = oldLine;
            }
            int val = EXPR_Eval();
            oldLine = linePtr;
            token = TOKEN_GetWord(word);
            switch (token)
            {
                case ']':
                    linePtr = oldLine;
                    FALLTHROUGH;
                case 0:     // value
                    if (!indirect && (dirOp >= 0 || extOp >= 0))
                    {
                        if (((force != '>' && evalKnown &&
                                (dirOp != 0x0e || force == '<') && // make JMP $00xx be ext!
                                (val & 0xFF00) >> 8 == dpReg) || force == '<')
                                && dirOp >= 0)
                        {
//                          if ((val & 0xFF00) >> 8 != 0 && (val & 0xFF00) >> 8 != dpReg && force == '<')
//                              ASMX_Warning("High byte of operand does not match SETDP value");
                            INSTR_XB(dirOp, val);    // <$xx
                        }
                        else
                        {
                            INSTR_XW(extOp, val);   // >$xxxx
                        }
                    }
                    else
                    {
                        INSTR_XBW(idxOp, 0x8F + indirect, val);  // $xxxx
                    }
                    break;
                case ',':   // value,
                    TOKEN_GetWord(word);
                    reg = REG_Find(word, idxRegsW);
                    if (curCPU == CPU_6309 && reg == 4)
                    {
                        // nnnn,W
                        if (force == '<')
                        {
                            TOKEN_BadMode();
                        }
                        else
                        {
                            INSTR_XBW(idxOp, 0xAF + !!indirect, val); // nnnn,W
                        }
                    }
                    else if (reg < 0 || reg > 3)
                    {
                        if (strcmp(word, "PC") == 0 || strcmp(word, "PCR") == 0)
                        {
                            val = val - locPtr - 3 - (idxOp > 255);
                            if ((force != '>' && evalKnown && -128 <= val && val <= 127)
                                    || force == '<')
                            {
                                INSTR_XBB(idxOp, 0x8C + indirect, val);       // nn,PCR
                            }
                            else
                            {
                                INSTR_XBW(idxOp, 0x8D + indirect, val - 1); // nnnn,PCR
                            }
                        }
                        else
                        {
                            TOKEN_BadMode();
                        }
                    }
                    else
                    {
                        if (force != '>' && evalKnown && !indirect && -16 <= val && val <= 15)
                        {
                            INSTR_XB(idxOp, reg * 0x20 + (val & 0x1F));  // n,X
                        }
                        else if (evalKnown && -128 <= val && val <= 127)
                        {
                            INSTR_XBB(idxOp, reg * 0x20 + 0x88 + indirect, val);  // nn,X
                        }
                        else
                        {
                            INSTR_XBW(idxOp, reg * 0x20 + 0x89 + indirect, val); // nnnn,X
                        }
                    }
                    break;

                default:
                    TOKEN_BadMode();
                    break;
            }
    }

    if (indirect)
    {
        if (!errFlag) TOKEN_Expect("]");  // if we've already had an error, don't bother checking
    }
}


static int M6809_DoCPUOpcode(int typ, int parm)
{
    int     val, reg1, reg2;
    Str255  word;
    char    *oldLine;
    const char *regs;
    int     token;

    // verify that 6309 instructions are allowed
    if ((parm & OP_6309) && curCPU != CPU_6309) return 0;
    parm &= ~OP_6309;

    switch (typ)
    {
        case OP_Inherent:
            if ((parm & 0xF0FF) == 0xA0E0)
            {
                INSTR_XW(0x3504, parm);
            }
            else
            {
                INSTR_X (parm);
            }
            break;

        case OP_Immediate:
            oldLine = linePtr;
            token = TOKEN_GetWord(word);
            if (token == '#')
            {
                val = EXPR_Eval();
                INSTR_XB(parm, val);
            }
            else
            {
                linePtr = oldLine;
                TOKEN_Expect("#");
            }
            break;

        case OP_Relative:
            val = EXPR_EvalBranch(2);
            INSTR_XB(parm, val);
            break;

        case OP_LRelative:
            val = EXPR_Eval();
            if (parm < 256)
            {
                val = val - locPtr - 3;
            }
            else
            {
                val = val - locPtr - 4;
            }
            INSTR_XW(parm, val);
            break;

        case OP_Indexed:
            M6809_Indexed(parm, -1, -1);
            break;

        case OP_PshPul:
            reg1 = 0;

            token = TOKEN_GetWord(word);
            while (token)
            {
                reg2 = REG_Find(word, pshRegs);
                // don't allow same stack pointer register
                if (reg2 < 0 || ((parm & 0xFE) == 0x34 && reg2 == 9)
                        || ((parm & 0xFE) == 0x36 && reg2 == 6))
                {
                    ASMX_Error("Invalid register");
                    break;
                }
                else
                {
                    switch (reg2)
                    {
                        case 8:
                            reg2 = 0x06;
                            break; // D
                        case 9:
                            reg2 = 0x40;
                            break; // S
                        default:
                            reg2 = 1 << reg2;
                            break;
                    }
                    if (reg1 & reg2)
                    {
                        ASMX_Warning("Repeated register");
                    }
                    reg1 = reg1 | reg2;
                }

                token = TOKEN_GetWord(word);
                if (token)
                {
                    if (token != ',')
                    {
                        ASMX_Error("\",\" expected");
                        token = 0;
                    }
                    else
                    {
                        token = TOKEN_GetWord(word);
                    }
                }
            }

            INSTR_XB(parm, reg1);
            break;

        case OP_ExgTfr:
            regs = tfrRegs;
            if (curCPU == CPU_6309)
            {
                regs = tfrRegs6309;
            }

            // get first register
            TOKEN_GetWord(word);
            reg1 = REG_Find(word, regs);
            if (reg1 < 0)
            {
                ASMX_Error("Invalid register");
                break;
            }

            if (TOKEN_Comma()) break;

            // get second register
            TOKEN_GetWord(word);
            reg2 = REG_Find(word, regs);
            if (reg2 < 0)
            {
                ASMX_Error("Invalid register");
                break;
            }

            if (curCPU != CPU_6309 && (reg1 & 8) != (reg2 & 8))
            {
                ASMX_Error("Register size mismatch");
                break;
            }

            INSTR_XB(parm, reg1*16 + reg2);
            break;

        case OP_Logical:
            M6809_Indexed(parm | 0x60, parm, parm | 0x70);
            break;

        case OP_Arith:
        case OP_LArith:
        case OP_QArith:
            oldLine = linePtr;
            token = TOKEN_GetWord(word);
            if (token == '#')
            {
                if (parm & 0x10)    // immediate
                {
                    ASMX_Error("Invalid addressing mode");
                    break;
                }

                val = EXPR_Eval();
                switch (typ)
                {
                    default:
                    case OP_Arith:
                        INSTR_XB(parm & ~0x10, val);
                        break;
                    case OP_LArith:
                        INSTR_XW(parm & ~0x10, val);
                        break;
                    case OP_QArith:
                        INSTR_X(0xCD);
                        INSTR_AddL(val);
                        break;
                }
            }
            else
            {
                linePtr = oldLine;
                M6809_Indexed((parm & ~0x10) | 0x20, (parm & ~0x10) | 0x10, (parm & ~0x10) | 0x30);
            }
            break;

        case OP_TFM: // works like OP_ExgTfr except for allowing + and - after registers
            regs = tfrRegs;
            if (curCPU == CPU_6309)
            {
                regs = tfrRegs6309;
            }

            // get first register
            TOKEN_GetWord(word);
            reg1 = REG_Find(word, regs);
            if (reg1 < 0)
            {
                ASMX_Error("Invalid register");
                break;
            }

            val = -1;

            token = TOKEN_GetWord(word);
            switch (token)
            {
                case '+': // R+,R+ and R+,R
                    val = 0;
                    if (TOKEN_Comma()) return 1;
                    break;

                case '-': // R-,R- only
                    val = 1;
                    if (TOKEN_Comma()) return 1;
                    break;

                case ',': // R,R+ only
                    val = 3;
                    break;

                default:
                    ASMX_IllegalOperand();
                    return 1;
            }


            // get second register
            TOKEN_GetWord(word);
            reg2 = REG_Find(word, regs);
            if (reg2 < 0)
            {
                ASMX_Error("Invalid register");
                break;
            }

            token = TOKEN_GetWord(word);
            switch (token)
            {
                case '+': // R,R+ and R+,R+
                    switch (val)
                    {
                        case 0: // R+,R+
                        case 3: // R,R+
                            break;

                        default:
                            ASMX_IllegalOperand();
                            return 1;
                    }
                    break;

                case '-': // R-,R- only
                    if (val != 1)
                    {
                        ASMX_IllegalOperand();
                        return 1;
                    }
                    break;

                case 0: // R+,R only
                    if (val != 0)
                    {
                        ASMX_IllegalOperand();
                        return 1;
                    }
                    val = 2;
                    break;

                default:
                    ASMX_IllegalOperand();
                    return 1;
            }

            INSTR_XB(parm + val, reg1*16 + reg2);
            break;

        case OP_Bit:
            // first register = CC, A, B
            token = TOKEN_GetWord(word);
            val = REG_Find(word, pshRegs);
            if (val < 0 || val > 2)
            {
                ASMX_Error("Invalid register");
                break;
            }
            reg1 = val << 6;

            if (TOKEN_Comma()) break;

            // source bit = 0..7
            val = EXPR_Eval();
            if (val < 0 || val > 7)
            {
                ASMX_IllegalOperand();
                break;
            }
            reg1 |= val << 3;

            if (TOKEN_Comma()) break;

            // destination bit = 0..7
            val = EXPR_Eval();
            if (val < 0 || val > 7)
            {
                ASMX_IllegalOperand();
                break;
            }
            reg1 |= val;

            if (TOKEN_Comma()) break;

            // allow '<' force character for direct addressing mode
            oldLine = linePtr;
            token = TOKEN_GetWord(word);
            if (token != '<')
            {
                linePtr = oldLine;
            }

            // direct page address
            val = EXPR_Eval() & 0xFF;

            INSTR_XBB(parm, reg1, val);
            break;

        default:
            return 0;
            break;
    }
    return 1;
}


static int M6809_DoCPULabelOp(int typ, int parm, char *labl)
{
//  Str255  word;

    (void) parm; // unused parameter

    switch (typ)
    {
        case OP_SETDP:
            if (labl[0])
            {
                ASMX_Error("Label not allowed");
            }

            int val = EXPR_Eval();
            if (!errFlag)
            {
                if ((val & 0xFF) == 0)
                {
                    val = val >> 8;     // handle $XX00 as $00XX
                }
                if (val < 0 || val > 255)
                {
                    ASMX_Error("Operand out of range");
                }
                else
                {
                    dpReg = val;

                    if (pass == 2)
                    {
                        char *p = listLine + 2;
                        p = LIST_Byte(p, val);
                    }
                }
            }
            break;

        default:
            return 0;
            break;
    }
    return 1;
}


static void M6809_PassInit(void)
{
    dpReg = 0;
}


void M6809_AsmInit(void)
{
    void *p = ASMX_AddAsm(versionName, &M6809_DoCPUOpcode, &M6809_DoCPULabelOp, &M6809_PassInit);

    ASMX_AddCPU(p, "6809", CPU_6809, END_BIG, ADDR_16, LIST_24, 8, 0, M6809_opcdTab);
    ASMX_AddCPU(p, "6309", CPU_6309, END_BIG, ADDR_16, LIST_24, 8, 0, M6809_opcdTab);
}
