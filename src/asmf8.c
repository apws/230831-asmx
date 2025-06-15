// asmF8.c

#define versionName "Fairchild F8 assembler"
#include "asmx.h"

enum
{
    OP_None,         // No operands
    OP_Immediate,    // 8-bit immediate operand
    OP_Short,        // 4-bit immediate operand
    OP_Register,     // Status register/indexed instruction
    OP_Relative,     // Relative branch
    OP_RegRel,       // Relative branch with embedded register
    OP_Absolute,     // 16-bit absolute address
    OP_SLSR,         // SL and SR instructions
    OP_LR            // LR instruction

//  o_Foo = OP_LabelOp,
};

const char F8_regs[] = "A W J H K Q KU KL QU QL IS DC0 PC0 PC1 DC P0 P1 P";
const char F8_regs_ABSID[] = "A B S I D";

enum regType    // these are keyed to F8_regs[] above
{
    REG_0,          // 0..15 = scratchpad registers
    REG_A = 16,     // 16 - accumulator
    REG_W,          // 17 - status register
    REG_J,          // 18 - 09
    REG_H,          // 19 - 0A 0B
    REG_K,          // 20 - 0C 0D
    REG_Q,          // 21 - 0E 0F
    REG_KU,         // 22 - 0C
    REG_KL,         // 23 - 0D
    REG_QU,         // 24 - 0E
    REG_QL,         // 25 - 0F
    REG_IS,         // 26 - indirect scratchpad address register
    REG_DC0,        // 27 - memory index register
    REG_PC0,        // 28 - program counter
    REG_PC1,        // 29 - "stack pointer"
    REG_DC,         // 30 - alias for DC0
    REG_P0,         // 31 - alias for PC0
    REG_P1,         // 32 - alias for PC1
    REG_P           // 33 - alias for PC1
};

static const struct OpcdRec F8_opcdTab[] =
{
    {"PK",  OP_None,     0x0C},
    {"LM",  OP_None,     0x16},
    {"ST",  OP_None,     0x17},
    {"COM", OP_None,     0x18},
    {"LNK", OP_None,     0x19},
    {"DI",  OP_None,     0x1A},
    {"EI",  OP_None,     0x1B},
    {"POP", OP_None,     0x1C},
    {"INC", OP_None,     0x1F},
    {"NOP", OP_None,     0x2B},
    {"XDC", OP_None,     0x2C},
    {"CLR", OP_None,     0x70},
    {"AM",  OP_None,     0x88},
    {"AMD", OP_None,     0x89},
    {"NM",  OP_None,     0x8A},
    {"OM",  OP_None,     0x8B},
    {"XM",  OP_None,     0x8C},
    {"CM",  OP_None,     0x8D},
    {"ADC", OP_None,     0x8E},

    {"LR",  OP_LR,       0x00},  // LR handles lots of instructions

    {"SR",  OP_SLSR,     0x12},
    {"SL",  OP_SLSR,     0x13},

    {"LI",  OP_Immediate,0x20},
    {"NI",  OP_Immediate,0x21},
    {"OI",  OP_Immediate,0x22},
    {"XI",  OP_Immediate,0x23},
    {"AI",  OP_Immediate,0x24},
    {"CI",  OP_Immediate,0x25},
    {"IN",  OP_Immediate,0x26},
    {"OUT", OP_Immediate,0x27},

    {"PI",  OP_Absolute, 0x28},
    {"JMP", OP_Absolute, 0x29},
    {"DCI", OP_Absolute, 0x2A},

    {"DS",  OP_Register, 0x30},  // note: overrides default "DS" mnemonic
    {"LISU",OP_Short,    0x60},  // reg must be 0..7
    {"LISL",OP_Short,    0x68},  // reg must be 0..7
    {"LIS", OP_Short,    0x70},
    {"INS", OP_Short,    0xA0},
    {"OUTS",OP_Short,    0xB0},
    {"AS",  OP_Register, 0xC0},
    {"ASD", OP_Register, 0xD0},
    {"XS",  OP_Register, 0xE0},
    {"NS",  OP_Register, 0xF0},

    {"BP",  OP_Relative, 0x81},
    {"BC",  OP_Relative, 0x82},
    {"BZ",  OP_Relative, 0x84},
    {"BR7", OP_Relative, 0x8F},
    {"BR",  OP_Relative, 0x90},
    {"BM",  OP_Relative, 0x91},
    {"BNC", OP_Relative, 0x92},
    {"BNZ", OP_Relative, 0x94},
    {"BNO", OP_Relative, 0x98},

    {"BT",  OP_RegRel,   0x80},  // reg must be 0..7
    {"BF",  OP_RegRel,   0x90},

    {"",    OP_Illegal,  0}
};


// --------------------------------------------------------------


static int F8_GetParenIS()
{
    Str255  word;

    TOKEN_GetWord(word);
    if (REG_Find(word, "IS") != 0) return 0;
    if (TOKEN_GetWord(word) != ')') return 0;

    char *oldLine = linePtr;
    switch (TOKEN_GetWord(word))
    {
        case '+':
            return 13; // I: "(IS)+"
        case '-':
            return 14; // D: "(IS)-"
        default:
            break;
    }
    linePtr = oldLine;
    return 12; // S: "(IS)"
}


static int F8_GetSReg(void)
{
    Str255  word;
    int     reg;

    char *oldLine = linePtr;
    int token = TOKEN_GetWord(word);

    // handle special names for registers 10-14
    if ((reg = REG_Find(word, F8_regs_ABSID)) >= 0)
    {
        return reg + 10;
    }

    // (IS) (IS)+ (IS)- form for regs 12-14
    if (token == '(')
    {
        int reg = F8_GetParenIS();
        if (reg) return reg;
    }

    // handle numeric names for registers 0-9
    linePtr = oldLine;
    return EXPR_Eval();
}


static int F8_GetReg(void)
{
    Str255  word;

    char *oldLine = linePtr;
    int token = TOKEN_GetWord(word);

    // handle the random special registers
    int reg = REG_Find(word, F8_regs);
    if (reg >= 0) return reg + REG_A;

    // handle special names for registers 10-14
    if ((reg = REG_Find(word, F8_regs_ABSID)) >= 0)
    {
        // can't use hexadecimal A here!
        if (reg != 0)
        {
            return reg + 10;
        }
    }

    // (IS) (IS)+ (IS)- form for regs 12-14
    if (token == '(')
    {
        reg = F8_GetParenIS();
        if (reg) return reg;
    }

    // handle numeric names for registers 0-9
    linePtr = oldLine;
    return EXPR_Eval();
}


static int F8_DoCPUOpcode(int typ, int parm)
{
    int     val, reg1, reg2;
//  Str255  word;
//  char    *oldLine;
//  int     token;

    switch (typ)
    {
        case OP_None:
            INSTR_B(parm);
            break;

        case OP_Immediate:
            val = EXPR_EvalByte();
            INSTR_BB(parm, val);
            break;

        case OP_Register:
            val = F8_GetSReg();
            if (val < 0 || val >= 15)
            {
                ASMX_IllegalOperand();
            }
            else
            {
                INSTR_B(parm + val);
            }
            break;

        case OP_Short:
            val = EXPR_Eval();
            if (val < 0 || val > 15 || ((parm == 0x60 || parm == 0x68) && val > 7))
            {
                ASMX_IllegalOperand();
            }
            else
            {
                INSTR_B(parm + val);
            }
            break;

        case OP_Relative:
            val = EXPR_EvalBranch(1);
            INSTR_BB(parm, val);
            break;

        case OP_RegRel:
            reg1 = EXPR_Eval();
            TOKEN_Comma();
            val = EXPR_EvalBranch(1);
            if (parm == 0x80 && reg1 > 7)
            {
                ASMX_IllegalOperand();
            }
            else
            {
                INSTR_BB(parm + reg1, val);
            }
            break;

        case OP_Absolute:
            val = EXPR_Eval();
            INSTR_BW(parm, val);
            break;

        case OP_SLSR:
            val = EXPR_Eval();
            switch (val)
            {
                case 1:
                    INSTR_B(parm);
                    break;
                case 4:
                    INSTR_B(parm + 2);
                    break;
                default:
                    ASMX_IllegalOperand();
                    break;
            }
            break;

        case OP_LR:
            val = -1;
            reg1 = F8_GetReg();
            TOKEN_Comma();
            reg2 = F8_GetReg();

            switch (reg1)
            {
                case REG_A:
                    if (reg2 == REG_KU)     val = 0x00; // LR A,KU
                    if (reg2 == REG_KL)     val = 0x01; // LR A,KL
                    if (reg2 == REG_QU)     val = 0x02; // LR A,QU
                    if (reg2 == REG_QL)     val = 0x03; // LR A,QL
                    if (reg2 == REG_IS)     val = 0x0A; // LR A,IS
                    if (REG_0 <= reg2 && reg2 <= REG_0+14)
                    {
                        val = 0x40 + reg2; // LR A,n
                    }
                    break;

                case REG_K:
                    if (reg2 == REG_PC1)    val = 0x08; // LR K,PC1
                    if (reg2 == REG_P1)     val = 0x08; // LR K,PC1
                    if (reg2 == REG_P)      val = 0x08; // LR K,PC1
                    break;

                case REG_W:
                    if (reg2 == REG_J)      val = 0x1D; // LR W,J
                    break;

                case REG_J:
                    if (reg2 == REG_W)      val = 0x1E; // LR J,W
                    break;

                case REG_Q:
                    if (reg2 == REG_DC0)    val = 0x0E; // LR Q,DC0
                    if (reg2 == REG_DC)     val = 0x0E; // LR Q,DC0
                    break;

                case REG_H:
                    if (reg2 == REG_DC0)    val = 0x11; // LR H,DC0
                    if (reg2 == REG_DC)     val = 0x11; // LR H,DC0
                    break;

                case REG_KU:
                    if (reg2 == REG_A)      val = 0x04; // LR KU,A
                    break;

                case REG_KL:
                    if (reg2 == REG_A)      val = 0x05; // LR KL,A
                    break;

                case REG_QU:
                    if (reg2 == REG_A)      val = 0x06; // LR QU,A
                    break;

                case REG_QL:
                    if (reg2 == REG_A)      val = 0x07; // LR QL,A
                    break;

                case REG_IS:
                    if (reg2 == REG_A)      val = 0x0B; // LR IS,A
                    break;

                case REG_PC1:
                case REG_P1:
                case REG_P:
                    if (reg2 == REG_K)      val = 0x09; // LR PC1,K
                    break;

                case REG_PC0:
                case REG_P0:
                    if (reg2 == REG_Q)      val = 0x0D; // LR PC0,Q
                    break;

                case REG_DC0:
                case REG_DC:
                    if (reg2 == REG_Q)      val = 0x0F; // LR DC0,H
                    if (reg2 == REG_H)      val = 0x10; // LR DC0,H
                    break;

                default:
                    if (REG_0 <= reg1 && reg1 <= REG_0+14 && reg2 == REG_A)
                    {
                        val = 0x50 + reg1;  // LR n,A
                    }
            }

            if (val < 0)
            {
                ASMX_IllegalOperand();
            }
            else
            {
                INSTR_B(val);
            }
            break;

        default:
            return 0;
            break;
    }
    return 1;
}


void F8_AsmInit(void)
{
    void *p = ASMX_AddAsm(versionName, &F8_DoCPUOpcode, NULL, NULL);

    ASMX_AddCPU(p, "F8", 0, END_BIG, ADDR_16, LIST_24, 8, 0, F8_opcdTab);
}
