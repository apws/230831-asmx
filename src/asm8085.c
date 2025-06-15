// asmI8085.c

#define versionName "I8085 assembler"
#include "asmx.h"

enum
{
    CPU_8080,   // Intel 8080 mnemonics
    CPU_8080Z,  // Intel 8080 mnemonics with Z-80 JR and DJNZ opcodes
    CPU_8085,   // Intel 8085 (adds RIM, SIM)
    CPU_8085U   // Intel 8085 with undocumented instructions
};

enum
{
    I_8085  = 0x100,
    I_8085U = 0x200,
};

enum instrType
{
    OP_None,         // No operands

    OP_Immediate,    // one-byte immediate operand
    OP_LImmediate,   // two-byte immediate operand (mostly JMPs)
    OP_MOV,          // MOV r,r opcode
    OP_RST,          // RST instruction
    OP_Arith,        // Arithmetic instructions
    OP_PushPop,      // PUSH and POP instructions
    OP_MVI,          // MVI instruction
    OP_LXI,          // LXI instruciton
    OP_INR,          // INR/DCR instructions
    OP_INX,          // INX/DCX/DAD instructions
    OP_STAX,         // STAX/LDAX instructions
    OP_JR,           // Z-80 JR instructions
    OP_DJNZ,         // Z-80 DJNZ instruction

//  o_Foo = OP_LabelOp,
};

const char I8085_regs1[]      = "B C D E H L M A";
const char I8085_regs2[]      = "B D H SP";
static const char conds[]     = "NZ Z NC C PO PE P M";

static const struct OpcdRec I8085_opcdTab[] =
{
    {"NOP", OP_None, 0x00},
    {"RLC", OP_None, 0x07},
    {"RRC", OP_None, 0x0F},
    {"RAL", OP_None, 0x17},
    {"RAR", OP_None, 0x1F},
    {"RIM", OP_None, 0x20 + I_8085}, // 8085-only
    {"DAA", OP_None, 0x27},
    {"CMA", OP_None, 0x2F},
    {"SIM", OP_None, 0x30 + I_8085}, // 8085-only
    {"STC", OP_None, 0x37},
    {"CMC", OP_None, 0x3F},
    {"HLT", OP_None, 0x76},
    {"RNZ", OP_None, 0xC0},
    {"RZ",  OP_None, 0xC8},
    {"RET", OP_None, 0xC9},
    {"RNC", OP_None, 0xD0},
    {"RC",  OP_None, 0xD8},
    {"RPO", OP_None, 0xE0},
    {"XTHL",OP_None, 0xE3},
    {"RPE", OP_None, 0xE8},
    {"PCHL",OP_None, 0xE9},
    {"XCHG",OP_None, 0xEB},
    {"RP",  OP_None, 0xF0},
    {"DI",  OP_None, 0xF3},
    {"RM",  OP_None, 0xF8},
    {"SPHL",OP_None, 0xF9},
    {"EI",  OP_None, 0xFB},

    {"ADI", OP_Immediate, 0xC6},
    {"ACI", OP_Immediate, 0xCE},
    {"OUT", OP_Immediate, 0xD3},
    {"SUI", OP_Immediate, 0xD6},
    {"IN",  OP_Immediate, 0xDB},
    {"SBI", OP_Immediate, 0xDE},
    {"ANI", OP_Immediate, 0xE6},
    {"XRI", OP_Immediate, 0xEE},
    {"ORI", OP_Immediate, 0xF6},
    {"CPI", OP_Immediate, 0xFE},

    {"SHLD",OP_LImmediate, 0x22},
    {"LHLD",OP_LImmediate, 0x2A},
    {"STA", OP_LImmediate, 0x32},
    {"LDA", OP_LImmediate, 0x3A},
    {"JNZ", OP_LImmediate, 0xC2},
    {"JMP", OP_LImmediate, 0xC3},
    {"CNZ", OP_LImmediate, 0xC4},
    {"JZ",  OP_LImmediate, 0xCA},
    {"CZ",  OP_LImmediate, 0xCC},
    {"CALL",OP_LImmediate, 0xCD},
    {"JNC", OP_LImmediate, 0xD2},
    {"CNC", OP_LImmediate, 0xD4},
    {"JC",  OP_LImmediate, 0xDA},
    {"CC",  OP_LImmediate, 0xDC},
    {"JPO", OP_LImmediate, 0xE2},
    {"CPO", OP_LImmediate, 0xE4},
    {"JPE", OP_LImmediate, 0xEA},
    {"CPE", OP_LImmediate, 0xEC},
    {"JP",  OP_LImmediate, 0xF2},
    {"CP",  OP_LImmediate, 0xF4},
    {"JM",  OP_LImmediate, 0xFA},
    {"CM",  OP_LImmediate, 0xFC},

    {"MOV", OP_MOV,     0},

    {"RST", OP_RST,     0},

    {"ADD", OP_Arith, 0x80},
    {"ADC", OP_Arith, 0x88},
    {"SUB", OP_Arith, 0x90},
    {"SBB", OP_Arith, 0x98},
    {"ANA", OP_Arith, 0xA0},
    {"XRA", OP_Arith, 0xA8},
    {"ORA", OP_Arith, 0xB0},
    {"CMP", OP_Arith, 0xB8},

    {"PUSH",OP_PushPop, 0xC5},
    {"POP", OP_PushPop, 0xC1},

    {"MVI", OP_MVI,   0x06},
    {"LXI", OP_LXI,   0x01},
    {"INR", OP_INR,   0x04},
    {"DCR", OP_INR,   0x05},
    {"INX", OP_INX,   0x03},
    {"DAD", OP_INX,   0x09},
    {"DCX", OP_INX,   0x0B},
    {"STAX",OP_STAX,  0x02},
    {"LDAX",OP_STAX,  0x0A},

//  Undocumented 8085 instructions:
//  These are in most "real" 8085 versions,
//  but are likely to not be found in VHDL cores, etc.

    {"DSUB",OP_None,       0x08 + I_8085U}, // HL = HL - BC
    {"ARHL",OP_None,       0x10 + I_8085U}, // arithmetic shift right HL
    {"RDEL",OP_None,       0x18 + I_8085U}, // rotate DE left through carry
    {"LDHI",OP_Immediate,  0x28 + I_8085U}, // DE = HL + imm
    {"LDSI",OP_Immediate,  0x38 + I_8085U}, // DE = SP + imm
    {"RSTV",OP_None,       0xCB + I_8085U}, // call 0x40 if overflow
    {"LHLX",OP_None,       0xED + I_8085U}, // L = (DE), H = (DE+1)
    {"SHLX",OP_None,       0xD9 + I_8085U}, // (DE) = L, (DE+1) = H
    {"JNX5",OP_LImmediate, 0xDD + I_8085U},
    {"JX5", OP_LImmediate, 0xFD + I_8085U},

// Z80 "JR" opcode
// This was used in a historical version of the H-19 source code

    {"JR",  OP_JR,       0x18},
    {"DJNZ",OP_DJNZ,     0x10},

    {"",    OP_Illegal,  0}
};


// --------------------------------------------------------------


static int I8085_DoCPUOpcode(int typ, int parm)
{
    int     val, reg1, reg2;
    Str255  word;
    char    *oldLine;
//  int     token;

    switch (typ)
    {
        case OP_None:
            if ((parm & I_8085)  && curCPU == CPU_8080)  return 0;
            if ((parm & I_8085U) && curCPU != CPU_8085U) return 0;

            INSTR_B(parm & 255);
            break;

        case OP_Immediate:
            if ((parm & I_8085U) && curCPU != CPU_8085U) return 0;

            val = EXPR_Eval();
            INSTR_BB(parm & 0xFF, val);
            break;

        case OP_LImmediate:
            if ((parm & I_8085U) && curCPU != CPU_8085U) return 0;

            val = EXPR_Eval();
            INSTR_BW(parm, val);
            break;

        case OP_MOV:
            TOKEN_GetWord(word);
            reg1 = REG_Find(word, I8085_regs1);
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
                    reg2 = REG_Find(word, I8085_regs1);
                    if (reg2 < 0 || (reg1 == 6 && reg2 == 6))
                    {
                        ASMX_IllegalOperand();
                    }
                    else
                    {
                        INSTR_B(0x40 + (reg1 << 3) + reg2);
                    }
                }
            }
            break;

        case OP_RST:
            val = EXPR_Eval();
            if (0 <= val && val <= 7)
            {
                INSTR_B(0xC7 + val*8);
            }
            else
            {
                ASMX_IllegalOperand();
            }
            break;

        case OP_Arith:
            TOKEN_GetWord(word);
            reg1 = REG_Find(word, I8085_regs1);
            if (reg1 < 0)
            {
                ASMX_IllegalOperand();
            }
            else
            {
                INSTR_B(parm + reg1);
            }
            break;

        case OP_PushPop:
            TOKEN_GetWord(word);
            reg1 = REG_Find(word, "B D H PSW");
            if (reg1 < 0)
            {
                ASMX_IllegalOperand();
            }
            else
            {
                INSTR_B(parm + (reg1 << 4));
            }
            break;

        case OP_MVI:
            TOKEN_GetWord(word);
            reg1 = REG_Find(word, I8085_regs1);
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
            reg1 = REG_Find(word, I8085_regs2);
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
                    INSTR_BW(parm + (reg1 << 4), val);
                }
            }
            break;

        case OP_INR:
            TOKEN_GetWord(word);
            reg1 = REG_Find(word, I8085_regs1);
            if (reg1 < 0)
            {
                ASMX_IllegalOperand();
            }
            else
            {
                INSTR_B(parm + (reg1 << 3));
            }
            break;

        case OP_INX:
            TOKEN_GetWord(word);
            reg1 = REG_Find(word, I8085_regs2);
            if (reg1 < 0)
            {
                ASMX_IllegalOperand();
            }
            else
            {
                INSTR_B(parm + (reg1 << 4));
            }
            break;

        case OP_STAX:
            TOKEN_GetWord(word);
            reg1 = REG_Find(word, "B D");
            if (reg1 < 0)
            {
                ASMX_IllegalOperand();
            }
            else
            {
                INSTR_B(parm + (reg1 << 4));
            }
            break;

        case OP_JR:
            if (curCPU != CPU_8080Z)  return 0;

            switch ((reg1 = REG_Get(conds)))
            {
                case reg_EOL:
                    break;

                case reg_None:
                    val = EXPR_EvalBranch(2);
                    INSTR_BB(0x18, val);
                    break;

                case 0:
                case 1:
                case 2:
                case 3:
                    if (TOKEN_Comma()) break;
                    val = EXPR_EvalBranch(2);
                    INSTR_BB(0x20 + reg1*8, val);
                    break;

                default:
                    ASMX_IllegalOperand();
                    break;
            }
            break;

        case OP_DJNZ:
            if (curCPU != CPU_8080Z)  return 0;

            val = EXPR_EvalBranch(2);
            INSTR_BB(0x10, val);
            break;

        default:
            return 0;
            break;
    }
    return 1;
}


void I8085_AsmInit(void)
{
    void *p = ASMX_AddAsm(versionName, &I8085_DoCPUOpcode, NULL, NULL);

    ASMX_AddCPU(p, "8080",  CPU_8080,  END_LITTLE, ADDR_16, LIST_24, 8, 0, I8085_opcdTab);
    ASMX_AddCPU(p, "8080Z", CPU_8080Z, END_LITTLE, ADDR_16, LIST_24, 8, 0, I8085_opcdTab);
    ASMX_AddCPU(p, "8085",  CPU_8085,  END_LITTLE, ADDR_16, LIST_24, 8, 0, I8085_opcdTab);
    ASMX_AddCPU(p, "8085U", CPU_8085U, END_LITTLE, ADDR_16, LIST_24, 8, 0, I8085_opcdTab);
}
