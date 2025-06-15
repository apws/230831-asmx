// asmI8008.c

#define versionName "I8008 assembler"
#include "asmx.h"

enum
{
    CPU_8008,
};

enum instrType
{
    OP_None,         // No operands

    OP_Immediate,    // one-byte immediate operand
    OP_LImmediate,   // two-byte immediate operand (mostly JMPs)
    OP_MOV,          // MOV r,r opcode
    OP_RST,          // RST instruction
    OP_Arith,        // Arithmetic instructions
    OP_MVI,          // MVI instruction
    OP_LXI,          // LXI instruciton
    OP_INR,          // INR/DCR instructions
    OP_IN,           // IN instructions
    OP_OUT,          // OUT instructions

//  o_Foo = OP_LabelOp,
};

const char I8008_regs1[]      = "A B C D E H L M";
const char I8008_regs2[]      = "B D H";

static const struct OpcdRec I8008_opcdTab[] =
{
    {"RLC", OP_None, 0x02},
    {"RNC", OP_None, 0x03}, // also RFC
    {"RET", OP_None, 0x07},
    {"RRC", OP_None, 0x0A},
    {"RNZ", OP_None, 0x0B}, // also RFZ
    {"RAL", OP_None, 0x12},
    {"RP",  OP_None, 0x13}, // also RFS
    {"RPO", OP_None, 0x1B}, // also RFP
    {"RAR", OP_None, 0x1A},
    {"RC",  OP_None, 0x23}, // also RTC
    {"RZ",  OP_None, 0x2B}, // also RTZ
    {"RM",  OP_None, 0x33}, // also RTS
    {"RPE", OP_None, 0x3B}, // also RTP
    {"NOP", OP_None, 0xC0},
    {"HLT", OP_None, 0xFF},

    {"ADI", OP_Immediate, 0x04},
    {"ACI", OP_Immediate, 0x0C},
    {"SUI", OP_Immediate, 0x14},
    {"SCI", OP_Immediate, 0x1C}, // also SBI
    {"ANI", OP_Immediate, 0x24}, // also NDI
    {"XRI", OP_Immediate, 0x2C},
    {"ORI", OP_Immediate, 0x34},
    {"CPI", OP_Immediate, 0x3C},

    {"JNC", OP_LImmediate, 0x40}, // also JFC
    {"CNC", OP_LImmediate, 0x42}, // also CFC
    {"JMP", OP_LImmediate, 0x44},
    {"CALL",OP_LImmediate, 0x46}, // also CAL
    {"JNZ", OP_LImmediate, 0x48}, // also JFZ
    {"CNZ", OP_LImmediate, 0x4A}, // also CFZ
    {"JP",  OP_LImmediate, 0x50}, // also JFS
    {"CP",  OP_LImmediate, 0x52}, // also CFS
    {"JPO", OP_LImmediate, 0x58}, // also JFP
    {"CPO", OP_LImmediate, 0x5A}, // also CFP
    {"JC",  OP_LImmediate, 0x60}, // also JTC
    {"CC",  OP_LImmediate, 0x62}, // also CTC
    {"JZ",  OP_LImmediate, 0x68}, // also JTZ
    {"CZ",  OP_LImmediate, 0x6A}, // also CTZ
    {"JM",  OP_LImmediate, 0x70}, // also JTS
    {"CM",  OP_LImmediate, 0x72}, // also CTS
    {"JPE", OP_LImmediate, 0x78}, // also JTP
    {"CPE", OP_LImmediate, 0x7A}, // also CTP

    {"MOV", OP_MOV,   0xC0},

    {"RST", OP_RST,   0x05},

    {"ADD", OP_Arith, 0x80}, // also ADA..ADM
    {"ADC", OP_Arith, 0x88}, // also ACA..ACM
    {"SUB", OP_Arith, 0x90}, // also SUA..SUM
    {"SBB", OP_Arith, 0x98}, // also SBA..SBM
    {"ANA", OP_Arith, 0xA0}, // also NDA..NDM
    {"XRA", OP_Arith, 0xA8}, // also XRA..XRM
    {"ORA", OP_Arith, 0xB0}, // also ORA..ORM
    {"CMP", OP_Arith, 0xB8}, // also CRA..CRM

    {"MVI", OP_MVI,   0x06},

    {"LXI", OP_LXI,   0x16},

    {"INR", OP_INR,   0x00}, // also INA..INM
    {"DCR", OP_INR,   0x01}, // also DCA..DCM

    {"IN",  OP_IN,    0x41}, // also INP

    {"OUT", OP_OUT,   0x41},

    {"",    OP_Illegal,  0}
};


// --------------------------------------------------------------


static int I8008_DoCPUOpcode(int typ, int parm)
{
    int     val, reg1, reg2;
    Str255  word;
    char    *oldLine;
//  int     token;

    switch (typ)
    {
        case OP_None:
            INSTR_B(parm & 255);
            break;

        case OP_Immediate:
            val = EXPR_Eval();
            INSTR_BB(parm & 0xFF, val);
            break;

        case OP_LImmediate:
            val = EXPR_Eval();
            INSTR_BW(parm, val);
            break;

        case OP_MOV:
            TOKEN_GetWord(word);
            reg1 = REG_Find(word, I8008_regs1);
            if (reg1 < 0)
            {
                ASMX_IllegalOperand();
            }
            else
            {
                oldLine = linePtr;
                if (TOKEN_GetWord(word) != ',')
                {
                    linePtr = oldLine;
                    TOKEN_Comma();
                }
                else
                {
                    TOKEN_GetWord(word);
                    reg2 = REG_Find(word, I8008_regs1);
                    if (reg2 < 0 || (reg1 == 7 && reg2 == 7))
                    {
                        ASMX_IllegalOperand();
                    }
                    else
                    {
                        INSTR_B(parm + (reg1 << 3) + reg2);
                    }
                }
            }
            break;

        case OP_RST:
            val = EXPR_Eval();
            if (0 <= val && val <= 7)
            {
                INSTR_B(parm + val*8);
            }
            else
            {
                ASMX_IllegalOperand();
            }
            break;

        case OP_Arith:
            TOKEN_GetWord(word);
            reg1 = REG_Find(word, I8008_regs1);
            if (reg1 < 0)
            {
                ASMX_IllegalOperand();
            }
            else
            {
                INSTR_B(parm + reg1);
            }
            break;

        case OP_MVI:
            TOKEN_GetWord(word);
            reg1 = REG_Find(word, I8008_regs1);
            if (reg1 < 0)
            {
                ASMX_IllegalOperand();
            }
            else
            {
                oldLine = linePtr;
                if (TOKEN_GetWord(word) != ',')
                {
                    linePtr = oldLine;
                    TOKEN_Comma();
                }
                else
                {
                    val = EXPR_Eval();
                    INSTR_BB(parm + (reg1 << 3), val);
                }
            }
            break;

        case OP_LXI:
            TOKEN_GetWord(word);
            reg1 = REG_Find(word, I8008_regs2);
            if (reg1 < 0)
            {
                ASMX_IllegalOperand();
            }
            else
            {
                oldLine = linePtr;
                if (TOKEN_GetWord(word) != ',')
                {
                    linePtr = oldLine;
                    TOKEN_Comma();
                }
                else
                {
                    val = EXPR_Eval();
                    INSTR_BBBB(parm + (reg1 << 4), val & 0xFF, parm + (reg1 << 4) - 8, val >> 8);
                }
            }
            break;

        case OP_INR:
            TOKEN_GetWord(word);
            reg1 = REG_Find(word, I8008_regs1);
            if ((reg1 < 0) || (reg1 == 0) || (reg1 == 7))
            {
                ASMX_IllegalOperand();
            }
            else
            {
                INSTR_B(parm + (reg1 << 3));
            }
            break;

        case OP_IN:
            val = EXPR_Eval();
            if ((val < 0) || (val > 7))
            {
                ASMX_IllegalOperand();
            }
            else
            {
                INSTR_B(parm + val*2);
            }
            break;

        case OP_OUT:
            val = EXPR_Eval();
            if ((val < 8) || (val > 31))
            {
                ASMX_IllegalOperand();
            }
            else
            {
                INSTR_B(parm + val*2);
            }
            break;

        default:
            return 0;
            break;
    }
    return 1;
}


void I8008_AsmInit(void)
{
    void *p = ASMX_AddAsm(versionName, &I8008_DoCPUOpcode, NULL, NULL);

    ASMX_AddCPU(p, "8008",  CPU_8008,  END_LITTLE, ADDR_16, LIST_24, 8, 0, I8008_opcdTab);
}
