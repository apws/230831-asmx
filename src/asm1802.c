// asmR1802.c

#define versionName "R1802 assembler"
#include "asmx.h"

enum
{
    OP_None,         // No operands
    OP_Register,     // register operand (with optional "R" in front)
    OP_Immediate,    // 8-bit immediate operand
    OP_Branch,       // short branch
    OP_LBranch,      // long branch
    OP_INPOUT        // INP/OUT instruction

//  o_Foo = OP_LabelOp,
};

static const struct OpcdRec R1802_opcdTab[] =
{
    {"IDL", OP_None,     0x00},
    {"LDN", OP_Register, 0x00},  // note: LDN R0 not allowed
    {"INC", OP_Register, 0x10},
    {"DEC", OP_Register, 0x20},
    {"BR",  OP_Branch,   0x30},
    {"BQ",  OP_Branch,   0x31},
    {"BZ",  OP_Branch,   0x32},
    {"BDF", OP_Branch,   0x33},
    {"BPZ", OP_Branch,   0x33}, // duplicate opcode
    {"BGE", OP_Branch,   0x33}, // duplicate opcode
    {"B1",  OP_Branch,   0x34},
    {"B2",  OP_Branch,   0x35},
    {"B3",  OP_Branch,   0x36},
    {"B4",  OP_Branch,   0x37},
    {"SKP", OP_None,     0x38},
    {"NBR", OP_Branch,   0x38}, // duplicate opcode
    {"BNQ", OP_Branch,   0x39},
    {"BNZ", OP_Branch,   0x3A},
    {"BNF", OP_Branch,   0x3B},
    {"BM",  OP_Branch,   0x3B}, // duplicate opcode
    {"BL",  OP_Branch,   0x3B}, // duplicate opcode
    {"BN1", OP_Branch,   0x3C},
    {"BN2", OP_Branch,   0x3D},
    {"BN3", OP_Branch,   0x3E},
    {"BN4", OP_Branch,   0x3F},
    {"LDA", OP_Register, 0x40},
    {"STR", OP_Register, 0x50},
    {"IRX", OP_None,     0x60},
    {"OUT", OP_INPOUT,   0x60},
    // no opcode for 0x68
    {"INP", OP_INPOUT,   0x68},
    {"RET", OP_None,     0x70},
    {"DIS", OP_None,     0x71},
    {"LDXA",OP_None,     0x72},
    {"STXD",OP_None,     0x73},
    {"ADC", OP_None,     0x74},
    {"SDB", OP_None,     0x75},
    {"SHRC",OP_None,     0x76},
    {"RSHR",OP_None,     0x76}, // duplicate opcode
    {"SMB", OP_None,     0x77},
    {"SAV", OP_None,     0x78},
    {"MARK",OP_None,     0x79},
    {"REQ", OP_None,     0x7A},
    {"SEQ", OP_None,     0x7B},
    {"ADCI",OP_Immediate,0x7C},
    {"SDBI",OP_Immediate,0x7D},
    {"SHLC",OP_None,     0x7E},
    {"RSHL",OP_None,     0x7E}, // duplicate opcode
    {"SMBI",OP_Immediate,0x7F},
    {"GLO", OP_Register, 0x80},
    {"GHI", OP_Register, 0x90},
    {"PLO", OP_Register, 0xA0},
    {"PHI", OP_Register, 0xB0},
    {"LBR", OP_LBranch,  0xC0},
    {"LBQ", OP_LBranch,  0xC1},
    {"LBZ", OP_LBranch,  0xC2},
    {"LBDF",OP_LBranch,  0xC3},
    {"NOP", OP_None,     0xC4},
    {"LSNQ",OP_None,     0xC5},
    {"LSNZ",OP_None,     0xC6},
    {"LSNF",OP_None,     0xC7},
    {"LSKP",OP_None,     0xC8},
    {"NLBR",OP_LBranch,  0xC8}, // duplicate opcode
    {"LBNQ",OP_LBranch,  0xC9},
    {"LBNZ",OP_LBranch,  0xCA},
    {"LBNF",OP_LBranch,  0xCB},
    {"LSIE",OP_None,     0xCC},
    {"LSQ", OP_None,     0xCD},
    {"LSZ", OP_None,     0xCE},
    {"LSDF",OP_None,     0xCF},
    {"SEP", OP_Register, 0xD0},
    {"SEX", OP_Register, 0xE0},
    {"LDX", OP_None,     0xF0},
    {"OR",  OP_None,     0xF1},
    {"AND", OP_None,     0xF2},
    {"XOR", OP_None,     0xF3},
    {"ADD", OP_None,     0xF4},
    {"SD",  OP_None,     0xF5},
    {"SHR", OP_None,     0xF6},
    {"SM",  OP_None,     0xF7},
    {"LDI", OP_Immediate,0xF8},
    {"ORI", OP_Immediate,0xF9},
    {"ANI", OP_Immediate,0xFA},
    {"XRI", OP_Immediate,0xFB},
    {"ADI", OP_Immediate,0xFC},
    {"SDI", OP_Immediate,0xFD},
    {"SHL", OP_None,     0xFE},
    {"SMI", OP_Immediate,0xFF},

    {"",    OP_Illegal,  0}
};


// --------------------------------------------------------------


static int R1802_GetReg(void)
{
    Str255  word;

    char *oldLine = linePtr;
    /*int token =*/ TOKEN_GetWord(word);
    if (word[0] == 'R')
    {
        // R0-R9
        if ('0' <= word[1] && word[1] <= '9' && word[2] == 0)
        {
            return word[1] - '0';
        }
        // RA-RF
        if ('A' <= word[1] && word[1] <= 'F' && word[2] == 0)
        {
            return word[1] - 'A' + 10;
        }
        // R10-R15
        if (word[1] == '1' && '0' <= word[2] && word[2] <= '5' && word[3] == 0)
        {
            return word[2] - '0' + 10;
        }
    }

    // otherwise evaluate an expression
    linePtr = oldLine;
    return EXPR_Eval();
}


static int R1802_DoCPUOpcode(int typ, int parm)
{
    int     val;
//  Str255  word;
//  char    *oldLine;
//  int     token;

    switch (typ)
    {
        case OP_None:
            INSTR_B(parm);
            break;

        case OP_Register:
            val = R1802_GetReg();
            if (val < 0 || val > 15)
            {
                ASMX_IllegalOperand();
            }
            else if (val == 0 && parm == 0x00)
            {
                ASMX_IllegalOperand();   // don't allow LDN R0
            }
            else
            {
                INSTR_B(parm+val);
            }
            break;

        case OP_Immediate:
            val = EXPR_EvalByte();
            INSTR_BB(parm, val);
            break;

        case OP_Branch:
            val = EXPR_Eval();
            // branch must go to same page as second byte of instruction
            if (((locPtr + 1) & 0xFF00) != (val & 0xFF00))
            {
                ASMX_Error("Branch out of range");
            }
            INSTR_BB(parm, val);
            break;

        case OP_LBranch:
            val = EXPR_Eval();
            INSTR_BW(parm, val);
            break;

        case OP_INPOUT:
            val = EXPR_Eval();
            if (val < 1 || val > 7)
            {
                ASMX_IllegalOperand();
            }
            else
            {
                INSTR_B(parm+val);
            }
            break;

        default:
            return 0;
            break;
    }
    return 1;
}


void R1802_AsmInit(void)
{
    void *p = ASMX_AddAsm(versionName, &R1802_DoCPUOpcode, NULL, NULL);

    ASMX_AddCPU(p, "1802", 0, END_BIG, ADDR_16, LIST_24, 8, 0, R1802_opcdTab);
}
