// asmZ80.c

#define versionName "Z80 assembler"
//#define NICE_ADD // allow ADD/ADC/SBC without "A,"
#include "asmx.h"

enum
{
    CPU_Z80,    // standard Z80
    CPU_GBZ80,  // Gameboy Z80 variant
    CPU_Z180,   // Z180 - not yet implemented
    CPU_Z80U    // Z80 with undocumented instructions - not yet implemented
};

enum
{
    OP_None,     // No operands
    OP_NoneNGB,  // No operands, not in GBZ80
    OP_NoneGB,   // No operands, GBZ80-only
    OP_None180,  // No operands, Z180 only

    OP_LD,       // Generic LD opcode
    OP_EX,       // Generic EX opcode
    OP_ADD,      // Generic ADD opcode
    OP_ADC_SBC,  // Generic ADC and SBC opcodes
    OP_INC_DEC,  // Generic INC and DEC opcodes
    OP_JP_CALL,  // Generic JP and CALL opcodes
    OP_JR,       // Generic JR opcode
    OP_RET,      // Generic RET opcode
    OP_IN,       // Generic IN opcode
    OP_OUT,      // Generic OUT opcode

    OP_PushPop,  // PUSH and POP instructions
    OP_Arith,    // Arithmetic instructions
    OP_Rotate,   // Z-80 rotate instructions
//  OP_Bit,      // BIT, RES, and SET instructions (moved to LabelOp for SET fall-back)
    OP_IM,       // IM instruction
    OP_DJNZ,     // DJNZ instruction
    OP_RST,      // RST instruction

    OP_SWAP,     // SWAP instruction for GBZ80
    OP_LDH,      // LDH instruction for GBZ80

    OP_IN0,      // IN0 opcode, Z180 only
    OP_OUT0,     // OUT0 opcode, Z180 only
    OP_TST,      // TST opcode, Z180 only
    OP_MLT,      // MLT opcode, Z180 only
    OP_TSTIO,    // TSTIO opcode, Z180 only

    OP_Bit = OP_LabelOp   // BIT, RES, and SET instructions need to be pseudo-op to allow SET fall-back
};

const char conds[] = "NZ Z NC C PO PE P M";
// NZ=0 Z=1 NC=2 C=3 PO=4 PE=5 P=6 M=7

// note: L is in Z80_regs[] twice as a placeholder for (HL)
const char Z80_regs[] = "B C D E H L L A I R BC DE HL SP IX IY AF HLD HLI ("; //mw L L => L M ????

const char Z80_IN0_OUT0_regs[] = "B C D E H L F A";

enum regType    // these are keyed to Z80_regs[] above
{
    REG_B,          //  0
    REG_C,          //  1
    REG_D,          //  2
    REG_E,          //  3
    REG_H,          //  4
    REG_L,          //  5
    REG_M,          //  6
    REG_A,          //  7
    REG_I,          //  8
    REG_R,          //  9
    REG_BC,         // 10
    REG_DE,         // 11
    REG_HL,         // 12
    REG_SP,         // 13
    REG_IX,         // 14
    REG_IY,         // 15
    REG_AF,         // 16
    REG_HLD,        // 17 GBZ80 HL with post-decrement
    REG_HLI,        // 18 GBZ80 HL with post-increment
    REG_Paren       // 19
};

static const struct OpcdRec Z80_opcdTab[] =
{
    {"NOP",  OP_None,    0x00},
    {"RLCA", OP_None,    0x07},
    {"RRCA", OP_None,    0x0F},
    {"RLA",  OP_None,    0x17},
    {"RRA",  OP_None,    0x1F},
    {"DAA",  OP_None,    0x27},
    {"CPL",  OP_None,    0x2F},
    {"SCF",  OP_None,    0x37},
    {"CCF",  OP_None,    0x3F},
    {"HALT", OP_None,    0x76},
    {"DI",   OP_None,    0xF3},
    {"EI",   OP_None,    0xFB},
    {"EXX",  OP_NoneNGB, 0xD9},
    {"NEG",  OP_NoneNGB, 0xED44},
    {"RETN", OP_NoneNGB, 0xED45},
    {"RETI", OP_None,    0xED4D},
    {"RRD",  OP_NoneNGB, 0xED67},
    {"RLD",  OP_NoneNGB, 0xED6F},
    {"LDI",  OP_None,    0xEDA0},
    {"CPI",  OP_NoneNGB, 0xEDA1},
    {"INI",  OP_NoneNGB, 0xEDA2},
    {"OUTI", OP_NoneNGB, 0xEDA3},
    {"LDIR", OP_NoneNGB, 0xEDB0},
    {"CPIR", OP_NoneNGB, 0xEDB1},
    {"INIR", OP_NoneNGB, 0xEDB2},
    {"OTIR", OP_NoneNGB, 0xEDB3},
    {"LDD",  OP_None,    0xEDA8},
    {"CPD",  OP_NoneNGB, 0xEDA9},
    {"IND",  OP_NoneNGB, 0xEDAA},
    {"OUTD", OP_NoneNGB, 0xEDAB},
    {"LDDR", OP_NoneNGB, 0xEDB8},
    {"CPDR", OP_NoneNGB, 0xEDB9},
    {"INDR", OP_NoneNGB, 0xEDBA},
    {"OTDR", OP_NoneNGB, 0xEDBB},

    {"LD",   OP_LD,      0},
    {"EX",   OP_EX,      0},

    {"ADD",  OP_ADD,     0},
    {"ADC",  OP_ADC_SBC, 0},
    {"SBC",  OP_ADC_SBC, 1},
    {"INC",  OP_INC_DEC, 0},
    {"DEC",  OP_INC_DEC, 1},

    {"JP",   OP_JP_CALL, 0xC3C2},
    {"CALL", OP_JP_CALL, 0xCDC4},
    {"JR",   OP_JR,      0},
    {"RET",  OP_RET,     0},

    {"PUSH", OP_PushPop, 0xC5},
    {"POP",  OP_PushPop, 0xC1},

    {"SUB",  OP_Arith,   0xD690},
    {"AND",  OP_Arith,   0xE6A0},
    {"XOR",  OP_Arith,   0xEEA8},
    {"OR",   OP_Arith,   0xF6B0},
    {"CP",   OP_Arith,   0xFEB8},

    {"RLC",  OP_Rotate,  0x00},
    {"RRC",  OP_Rotate,  0x08},
    {"RL",   OP_Rotate,  0x10},
    {"RR",   OP_Rotate,  0x18},
    {"SLA",  OP_Rotate,  0x20},
    {"SRA",  OP_Rotate,  0x28},
    {"SRL",  OP_Rotate,  0x38},

    {"BIT",  OP_Bit,     0x40},
    {"RES",  OP_Bit,     0x80},
    {"SET",  OP_Bit,     0xC0},

    {"IM",   OP_IM,      0},

    {"DJNZ", OP_DJNZ,    0},

    {"IN",   OP_IN,      0},
    {"OUT",  OP_OUT,     0},

    {"RST",  OP_RST,     0},

//  Gameboy Z-80 specific instructions

    {"STOP", OP_NoneGB,  0x10},
//  {"DEBUG",OP_NoneGB,  0xED}, // does this really exist?
    {"SWAP", OP_SWAP,    0x00},
    {"LDH",  OP_LDH,     0x00},

//  Z180 specific instructions

    {"IN0",   OP_IN0,     0xED00 }, // + (r << 3) : bb
    {"OUT0",  OP_OUT0,    0xED01 }, // + (r << 3) : bb
    {"TST",   OP_TST,     0xED04 }, // + (r << 3), or ED64 for TST (C),b
    {"MLT",   OP_MLT,     0xED4C }, // + (rr << 4)
    {"TSTIO", OP_TSTIO,   0xED74 }, // : bb

    {"SLP",   OP_None180, 0xED76 },
    {"OTIM",  OP_None180, 0xED83 },
    {"OTDM",  OP_None180, 0xED8B },
    {"OTIMR", OP_None180, 0xED93 },
    {"OTDMR", OP_None180, 0xED9B },

    {"",    OP_Illegal,  0}
};


// --------------------------------------------------------------


static int Z80_IXOffset()
{
    Str255  word;

    int val = 0;
    char *oldLine = linePtr;
    int token = TOKEN_GetWord(word);
    if (token == '+' || token == '-')
    {
        val = EXPR_Eval();
        if (token == '-')
        {
            val = -val;
        }
    }
    else
    {
        linePtr = oldLine;
    }

    TOKEN_RParen();

    return val;
}


static int Z80_DDFD(int reg)
{
    switch (reg)
    {
        default:
        case REG_HL:
            return      0;
        case REG_IX:
            return 0xDD00;
        case REG_IY:
            return 0xFD00;
    }
}


static void Z80_DoArith(int imm, int reg)
{
    int         reg2;
    int         val;

    char *oldLine = linePtr; // save position because of (HL) vs (expr)

    switch ((reg2 = REG_Get(Z80_regs)))
    {
        case reg_EOL:
            break;

        case reg_None:  // ADD A,nn
            val = EXPR_Eval();
            INSTR_BB(imm, val);
            break;

        case REG_B:
        case REG_C:
        case REG_D:
        case REG_E:
        case REG_H:
        case REG_L:
        case REG_A:     // ADD A,r
            INSTR_B(reg + reg2);
            break;

        case REG_Paren: // ADD A,(
            switch ((reg2 = REG_Get(Z80_regs)))
            {
                case reg_EOL:
                    break;

                case REG_HL:
                    if (TOKEN_RParen()) break;
                    INSTR_B(reg+REG_M);
                    break;

                case REG_IX:
                case REG_IY:
                    if (curCPU == CPU_GBZ80)
                    {
                        ASMX_IllegalOperand();
                        break;
                    }
                    val = Z80_IXOffset();
                    INSTR_XB(Z80_DDFD(reg2) + reg + REG_M, val);
                    break;

                default:
                    // must be constant "(expression)" so try again that way
                    linePtr = oldLine;
                    val = EXPR_Eval();
                    INSTR_BB(imm, val);
            }
            break;

        default:
            ASMX_IllegalOperand();
            break;
    }
}


static int Z80_DoCPUOpcode(int typ, int parm)
{
    int     val, reg1, reg2;
    Str255  word;
    int     token;

    char *oldLine = linePtr;
    switch (typ)
    {
        case OP_None180:
            if (curCPU != CPU_Z180) return 0;
            FALLTHROUGH;
        case OP_None:
        case OP_NoneGB:
        case OP_NoneNGB:
            if (curCPU != CPU_GBZ80 || (parm & 0xFFF7) != 0xEDA0)
            {
                // LDI/LDD 0xEDA0/0xEDA8
                if ((typ == OP_NoneGB)  && curCPU != CPU_GBZ80) return 0;
                if ((typ == OP_NoneNGB) && curCPU == CPU_GBZ80) return 0;
                if (curCPU == CPU_GBZ80 && parm == 0xED4D)
                {
                    parm = 0xD9; // GBZ80 RETI
                }
                INSTR_X(parm & 0xFFFF);
                break;
            }

            // fall-through for GBZ80 LDI/LDD:
            parm = ((parm & 0x08) << 1) | 0x22;

            switch ((reg1 = REG_Get(Z80_regs)))
            {
                case reg_EOL:
                    break;

                case REG_A:     // A,(HL)
                    if (TOKEN_Comma()) break;
                    if (TOKEN_Expect("(")) break;
                    switch ((reg1 = REG_Get(Z80_regs)))
                    {
                        case reg_EOL:
                            break;

                        case REG_HL: // A,(HL)
                            if (TOKEN_RParen()) break;
                            INSTR_B(parm + 8);
                            break;

                        default:
                            ASMX_IllegalOperand();
                            break;
                    }
                    break;

                case REG_Paren: // (HL),A
                    switch ((reg1 = REG_Get(Z80_regs)))
                    {
                        case reg_EOL:
                            break;

                        case REG_HL: // (HL),A
                            if (TOKEN_RParen()) break;
                            if (TOKEN_Comma()) break;
                            if (TOKEN_Expect("A")) break;
                            INSTR_B(parm);
                            break;

                        default:
                            ASMX_IllegalOperand();
                            break;
                    }
                    break;

                default:
                    ASMX_IllegalOperand();
                    break;
            }
            break;

        case OP_LD:
            switch ((reg1 = REG_Get(Z80_regs)))
            {
                case reg_EOL:
                    break;

                case reg_None:  // LD nnnn,?
                    ASMX_IllegalOperand();
                    break;

                case REG_B:
                case REG_C:
                case REG_D:
                case REG_E:
                case REG_H:
                case REG_L:
                case REG_A:     // LD r,?
                    if (TOKEN_Comma()) break;
                    switch ((reg2 = REG_Get(Z80_regs)))
                    {
                        case reg_EOL:
                            break;

                        case REG_B:
                        case REG_C:
                        case REG_D:
                        case REG_E:
                        case REG_H:
                        case REG_L:
                        case REG_A:     // LD r,r
                            INSTR_B(0x40 + reg1*8 + reg2);
                            break;

                        case REG_I:     // LD A,I
                            if (reg1 != REG_A || curCPU == CPU_GBZ80)
                            {
                                ASMX_IllegalOperand();
                                break;
                            }
                            INSTR_X(0xED57);
                            break;

                        case REG_R:     // LD A,R
                            if (reg1 != REG_A || curCPU == CPU_GBZ80)
                            {
                                ASMX_IllegalOperand();
                                break;
                            }
                            INSTR_X(0xED5F);
                            break;

                        case REG_Paren:     // LD r,(?)
                            switch ((reg2 = REG_Get(Z80_regs)))
                            {
                                case reg_EOL:
                                    break;

                                case REG_BC:    // LD A,(BC)
                                case REG_DE:    // LD A,(DE)
                                    if (reg1 != REG_A)
                                    {
                                        ASMX_IllegalOperand();
                                    }
                                    else
                                    {
                                        if (TOKEN_RParen()) break;
                                        INSTR_B(0x0A + (reg2-REG_BC)*16);
                                    }
                                    break;

                                case REG_C:
                                    if (curCPU != CPU_GBZ80)
                                    {
                                        ASMX_IllegalOperand();
                                    }
                                    else
                                    {
                                        if (TOKEN_RParen()) break;
                                        INSTR_B(0xF2); // LD A,(C)
                                    }
                                    break;

                                case REG_HLI:
                                    if (curCPU != CPU_GBZ80)
                                    {
                                        ASMX_IllegalOperand();
                                    }
                                    else
                                    {
                                        if (TOKEN_RParen()) break;
                                        INSTR_B(0x2A); // LD A,(HLI)
                                    }
                                    break;

                                case REG_HLD:
                                    if (curCPU != CPU_GBZ80)
                                    {
                                        ASMX_IllegalOperand();
                                    }
                                    else
                                    {
                                        if (TOKEN_RParen()) break;
                                        INSTR_B(0x3A); // LD A,(HLD)
                                    }
                                    break;

                                case REG_HL:    // LD r,(HL)
                                    if (curCPU == CPU_GBZ80 && *linePtr == '+')
                                    {
                                        linePtr++;
                                        if (TOKEN_RParen()) break;
                                        INSTR_B(0x2A); // LD A,(HL+)
                                    }
                                    else if (curCPU == CPU_GBZ80 && *linePtr == '-')
                                    {
                                        linePtr++;
                                        if (TOKEN_RParen()) break;
                                        INSTR_B(0x3A); // LD A,(HL-)
                                    }
                                    else
                                    {
                                        if (TOKEN_RParen()) break;
                                        INSTR_B(0x40 + reg1*8 + REG_M);
                                    }
                                    break;

                                case REG_IX:    // LD r,(IX+d)
                                case REG_IY:    // LD r,(IY+d)
                                    if (curCPU == CPU_GBZ80)
                                    {
                                        ASMX_IllegalOperand();
                                        break;
                                    }
                                    val = Z80_IXOffset();
                                    INSTR_XB(Z80_DDFD(reg2) + 0x46 + reg1*8, val);
                                    break;

                                case reg_None:  // LD A,(nnnn)
                                    if (reg1 != REG_A)
                                    {
                                        ASMX_IllegalOperand();
                                    }
                                    else
                                    {
                                        val = EXPR_Eval();
                                        if (TOKEN_RParen()) break;
                                        if (curCPU == CPU_GBZ80 && evalKnown && (val & 0xFF00) == 0xFF00)
                                        {
                                            INSTR_BB(0xF0, 0xFF);
                                        }
                                        else if (curCPU == CPU_GBZ80)
                                        {
                                            INSTR_BW(0xFA, val);
                                        }
                                        else
                                        {
                                            INSTR_BW(0x3A, val);
                                        }
                                    }
                                    break;

                                default:
                                    ASMX_IllegalOperand();
                                    break;
                            }
                            break;

                        case reg_None:  // LD r,nn
                            INSTR_BB(0x06 + reg1*8, EXPR_Eval());
                            break;

                        default:
                            ASMX_IllegalOperand();
                            break;
                    }
                    break;

                case REG_I:     // LD I,A
                    if (curCPU == CPU_GBZ80)
                    {
                        ASMX_IllegalOperand();
                        break;
                    }
                    if (TOKEN_Comma()) break;
                    if (TOKEN_Expect("A")) break;
                    INSTR_X(0xED47);
                    break;

                case REG_R:     // LD R,A
                    if (curCPU == CPU_GBZ80)
                    {
                        ASMX_IllegalOperand();
                        break;
                    }
                    if (TOKEN_Comma()) break;
                    if (TOKEN_Expect("A")) break;
                    INSTR_X(0xED4F);
                    break;

                case REG_BC:
                case REG_DE:
                case REG_HL:
                case REG_SP:    // LD rr,?
                    if (TOKEN_Comma()) break;
                    oldLine = linePtr; // save linePtr in case of backtracking
                    switch ((reg2 = REG_Get(Z80_regs)))
                    {
                        case reg_EOL:
                            break;

                        case REG_HL:    // LD SP,HL/IX/IY
                        case REG_IX:
                        case REG_IY:
                            if (reg1 != REG_SP)
                            {
                                ASMX_IllegalOperand();
                            }
                            else if (reg2 != REG_HL && curCPU == CPU_GBZ80)
                            {
                                ASMX_IllegalOperand();
                            }
                            else
                            {
                                INSTR_X(Z80_DDFD(reg2) + 0xF9);
                            }
                            break;

                        case REG_Paren:
                            if (curCPU == CPU_GBZ80)
                            {
                                ASMX_IllegalOperand();
                            }
                            else if (reg1 == REG_HL)
                            {
                                val = EXPR_Eval();   // LD HL,(nnnn)
                                if (TOKEN_RParen()) break;
                                INSTR_BW(0x2A, val);
                            }
                            else
                            {
                                val = EXPR_Eval();   // LD BC/DE/SP,(nnnn)
                                if (TOKEN_RParen()) break;
                                INSTR_XW(0xED4B + (reg1-REG_BC)*16, val);
                            }

                            // at this point, if there is any extra stuff on the line,
                            // backtrack and try again with reg_None case
                            // to handle the case of LD BC,(foo + 1) * 256, etc.
                            token = TOKEN_GetWord(word);
                            if (token == 0) break;
                            // note that if an error occurs, it will be repeated twice,
                            // and we have to return to avoid that, but we need to make sure
                            // the instruction length is the same in both passes or there
                            // will be a phase error problem
                            instrLen = 3;
                            if (errFlag) break;
                            linePtr = oldLine; // restore line position before left paren
                            // now fall through...
                            FALLTHROUGH;

                        case reg_None:  // LD rr,nnnn
                            val = EXPR_Eval();
                            INSTR_BW(0x01 + (reg1-REG_BC)*16, val);
                            break;

                        case REG_SP:
                            if (curCPU == CPU_GBZ80)
                            {
                                INSTR_B(0xF8); // LD HL,SP
                                break;
                            }
                            FALLTHROUGH;
                        default:
                            ASMX_IllegalOperand();
                            break;
                    }
                    break;

                case REG_IX:    // LD IX,?
                case REG_IY:    // LD IY,?
                    if (curCPU == CPU_GBZ80)
                    {
                        ASMX_IllegalOperand();
                        break;
                    }
                    if (TOKEN_Comma()) break;
                    switch ((reg2 = REG_Get(Z80_regs)))
                    {
                        case reg_EOL:
                            break;

                        case REG_Paren: // LD IX,(nnnn)
                            val = EXPR_Eval();
                            if (TOKEN_RParen()) break;
                            INSTR_XW(Z80_DDFD(reg1) + 0x2A, val);

                            // at this point, if there is any extra stuff on the line,
                            // backtrack and try again with reg_None case
                            // to handle the case of LD IX,(foo + 1) * 256, etc.
                            token = TOKEN_GetWord(word);
                            if (token == 0) break;
                            // note that if an error occurs, it will be repeated twice,
                            // and we have to return to avoid that
                            if (errFlag) break;
                            // now fall through...
                            FALLTHROUGH;

                        case reg_None:  // LD IX,nnnn
                            val = EXPR_Eval();
                            INSTR_XW(Z80_DDFD(reg1) + 0x21, val);
                            break;

                        default:
                            ASMX_IllegalOperand();
                            break;
                    }
                    break;

                case REG_Paren:     // LD (?),?
                    switch ((reg1 = REG_Get(Z80_regs)))
                    {
                        case reg_EOL:
                            break;

                        case reg_None:  // LD (nnnn),?
                            val = EXPR_Eval();
                            if (TOKEN_RParen()) break;
                            if (TOKEN_Comma()) break;
                            switch ((reg2 = REG_Get(Z80_regs)))
                            {
                                case reg_EOL:
                                    break;

                                case REG_A:
                                    if (curCPU == CPU_GBZ80 && evalKnown && (val & 0xFF00) == 0xFF00)
                                    {
                                        INSTR_BB(0xE0, val);
                                    }
                                    else if (curCPU == CPU_GBZ80)
                                    {
                                        INSTR_BW(0xEA, val);
                                    }
                                    else
                                    {
                                        INSTR_BW(0x32, val);
                                    }
                                    break;

                                case REG_HL:
                                    if (curCPU == CPU_GBZ80)
                                    {
                                        ASMX_IllegalOperand();
                                    }
                                    else
                                    {
                                        INSTR_BW(0x22, val);
                                    }
                                    break;

                                case REG_BC:
                                case REG_DE:
                                case REG_SP:
                                    if (reg2 == REG_SP && curCPU == CPU_GBZ80)
                                    {
                                        INSTR_BW(0x08, val);
                                    }
                                    else if (curCPU == CPU_GBZ80)
                                    {
                                        ASMX_IllegalOperand();
                                    }
                                    else
                                    {
                                        INSTR_XW(0xED43 + (reg2-REG_BC)*16, val);
                                    }
                                    break;

                                case REG_IX:
                                case REG_IY:
                                    if (curCPU == CPU_GBZ80)
                                    {
                                        ASMX_IllegalOperand();
                                    }
                                    else
                                    {
                                        INSTR_XW(Z80_DDFD(reg2) + 0x22, val);
                                    }
                                    break;

                                default:
                                    ASMX_IllegalOperand();
                                    break;
                            }
                            break;

                        case REG_BC:
                        case REG_DE:
                            if (TOKEN_RParen()) break;
                            if (TOKEN_Comma()) break;
                            if (TOKEN_Expect("A")) break;
                            INSTR_B(0x02 + (reg1-REG_BC)*16);
                            break;

                        case REG_C:
                            if (curCPU != CPU_GBZ80)
                            {
                                ASMX_IllegalOperand();
                            }
                            else
                            {
                                if (TOKEN_RParen()) break;
                                if (TOKEN_Comma()) break;
                                if (TOKEN_Expect("A")) break;
                                INSTR_B(0xE2); // LD (C),A
                            }
                            break;

                        case REG_HLI:
                            if (curCPU != CPU_GBZ80)
                            {
                                ASMX_IllegalOperand();
                            }
                            else
                            {
                                if (TOKEN_RParen()) break;
                                if (TOKEN_Comma()) break;
                                if (TOKEN_Expect("A")) break;
                                INSTR_B(0x22); // LD (HLI),A
                            }
                            break;

                        case REG_HLD:
                            if (curCPU != CPU_GBZ80)
                            {
                                ASMX_IllegalOperand();
                            }
                            else
                            {
                                if (TOKEN_RParen()) break;
                                if (TOKEN_Comma()) break;
                                if (TOKEN_Expect("A")) break;
                                INSTR_B(0x32); // LD (HLD),A
                            }
                            break;

                        case REG_HL: // LD (HL),?
                            if (curCPU == CPU_GBZ80 && *linePtr == '+')
                            {
                                linePtr++;
                                if (TOKEN_RParen()) break;
                                if (TOKEN_Comma()) break;
                                if (TOKEN_Expect("A")) break;
                                INSTR_B(0x22); // LD (HL+),A
                                break;
                            }
                            else if (curCPU == CPU_GBZ80 && *linePtr == '-')
                            {
                                linePtr++;
                                if (TOKEN_RParen()) break;
                                if (TOKEN_Comma()) break;
                                if (TOKEN_Expect("A")) break;
                                INSTR_B(0x32); // LD (HL-),A
                                break;
                            }
                            if (TOKEN_RParen()) break;
                            if (TOKEN_Comma()) break;
                            switch ((reg2 = REG_Get(Z80_regs)))
                            {
                                case reg_EOL:
                                    break;

                                case reg_None:
                                    val = EXPR_Eval();
                                    INSTR_BB(0x36, val);
                                    break;

                                case REG_B:
                                case REG_C:
                                case REG_D:
                                case REG_E:
                                case REG_H:
                                case REG_L:
                                case REG_A:
                                    INSTR_B(0x70 + reg2);
                                    break;

                                default:
                                    ASMX_IllegalOperand();
                                    break;
                            }
                            break;

                        case REG_IX:
                        case REG_IY:    // LD (IX),?
                            if (curCPU == CPU_GBZ80)
                            {
                                ASMX_IllegalOperand();
                                break;
                            }
                            val = Z80_IXOffset();
                            if (TOKEN_Comma()) break;
                            switch ((reg2 = REG_Get(Z80_regs)))
                            {
                                case reg_EOL:
                                    break;

                                case reg_None:
                                    reg2 = EXPR_Eval();
                                    INSTR_XBB(Z80_DDFD(reg1) + 0x36, val, reg2);
                                    break;

                                case REG_B:
                                case REG_C:
                                case REG_D:
                                case REG_E:
                                case REG_H:
                                case REG_L:
                                case REG_A:
                                    INSTR_XB(Z80_DDFD(reg1) + 0x70 + reg2, val);
                                    break;

                                default:
                                    ASMX_IllegalOperand();
                                    break;
                            }
                            break;

                        default:
                            ASMX_IllegalOperand();
                            break;
                    }
                    break;

                default:
                    ASMX_IllegalOperand();
                    break;
            }
            break;

        case OP_EX:
            if (curCPU == CPU_GBZ80)
            {
                return 0;
            }
            switch ((reg1 = REG_Get(Z80_regs)))
            {
                case reg_EOL:
                    break;

                case REG_DE:    // EX DE,HL }
                    if (TOKEN_Comma()) break;
                    if (TOKEN_Expect("HL")) break;
                    INSTR_B(0xEB);
                    break;

                case REG_AF:    // EX AF,AF' }
                    if (TOKEN_Comma()) break;
                    if (TOKEN_Expect("AF")) break;
                    if (TOKEN_Expect("'")) break;
                    INSTR_B(0x08);
                    break;

                case REG_Paren: // EX (SP),? }
                    if (TOKEN_Expect("SP")) break;
                    if (TOKEN_RParen()) break;
                    if (TOKEN_Comma()) break;
                    switch ((reg2 = REG_Get(Z80_regs)))
                    {
                        case reg_EOL:
                            break;

                        case REG_HL:
                        case REG_IX:
                        case REG_IY:
                            INSTR_X(Z80_DDFD(reg2) + 0xE3);
                            break;

                        default:
                            ASMX_IllegalOperand();
                            break;
                    }
                    break;

                default:
                    ASMX_IllegalOperand();
                    break;
            }
            break;

        case OP_ADD:
            switch ((reg1 = REG_Get(Z80_regs)))
            {
                case reg_EOL:
                    break;

#ifdef NICE_ADD
                case REG_Paren:
                case reg_None:
                case REG_B: // ADD r
                case REG_C:
                case REG_D:
                case REG_E:
                case REG_H:
                case REG_L:
                    linePtr = oldLine;
                    Z80_DoArith(0xC6, 0x80);
                    break;
#endif

                case REG_A:
#ifdef NICE_ADD
                    oldLine = linePtr;
                    token = GetWord(word);
                    if (token == 0)
                    {
                        InstrB(0x87); // ADD A
                        break;
                    }
                    linePtr = oldLine;
#endif
                    if (TOKEN_Comma()) break;
                    Z80_DoArith(0xC6, 0x80);
                    break;

                case REG_SP:
                    if (curCPU != CPU_GBZ80)
                    {
                        ASMX_IllegalOperand();
                    }
                    else
                    {
                        if (TOKEN_Comma()) break;
                        val = EXPR_Eval();
                        if (val < -128 || val > 127)
                        {
                            ASMX_Error("Offset out of range");
                        }
                        INSTR_BB(0xE8, val);
                    }
                    break;


                case REG_IX:
                case REG_IY:
                    if (curCPU == CPU_GBZ80)
                    {
                        ASMX_IllegalOperand();
                        break;
                    }
                    FALLTHROUGH;
                case REG_HL:
                    if (TOKEN_Comma()) break;
                    switch ((reg2 = REG_Get(Z80_regs)))
                    {
                        case reg_EOL:
                            break;

                        case REG_HL:
                        case REG_IX:
                        case REG_IY:
                            if (reg1 != reg2)
                            {
                                ASMX_IllegalOperand();
                                break;
                            }
                            reg2 = REG_HL;
                            FALLTHROUGH;

                        case REG_BC:
                        case REG_DE:
                        case REG_SP:
                            INSTR_X(Z80_DDFD(reg1) + 0x09 + (reg2-REG_BC)*16);
                            break;

                        default:
                            ASMX_IllegalOperand();
                            break;
                    }
                    break;

                default:
                    ASMX_IllegalOperand();
                    break;
            }
            break;

        case OP_ADC_SBC:
            switch ((reg1 = REG_Get(Z80_regs)))
            {
                case reg_EOL:
                    break;

#ifdef NICE_ADD
                case REG_Paren:
                case reg_None:
                case REG_B: // ADD r
                case REG_C:
                case REG_D:
                case REG_E:
                case REG_H:
                case REG_L:
                    linePtr = oldLine;
                    Z80_DoArith(parm*16 + 0xCE, 0x88 + parm*16);
                    break;
#endif

                case REG_A:
#ifdef NICE_ADD
                    oldLine = linePtr;
                    token = GetWord(word);
                    if (token == 0)
                    {
                        InstrB(0x8F + parm*16); // ADD A
                        break;
                    }
                    linePtr = oldLine;
#endif
                    if (TOKEN_Comma()) break;
                    Z80_DoArith(parm*16 + 0xCE, 0x88 + parm*16); // had to move 0xCE because GCC complained?
                    break;

                case REG_HL:
                    if (curCPU == CPU_GBZ80)
                    {
                        ASMX_IllegalOperand();
                        break;
                    }
                    if (TOKEN_Comma()) break;
                    switch ((reg2 = REG_Get(Z80_regs)))
                    {
                        case reg_EOL:
                            break;

                        case REG_BC:
                        case REG_DE:
                        case REG_HL:
                        case REG_SP:
                            INSTR_X(0xED4A + (reg2-REG_BC)*16 - parm*8);
                            break;

                        default:
                            ASMX_IllegalOperand();
                            break;
                    }
                    break;

                default:
                    ASMX_IllegalOperand();
                    break;
            }
            break;

        case OP_INC_DEC:
            switch ((reg1 = REG_Get(Z80_regs)))
            {
                case reg_EOL:
                    break;

                case REG_B:
                case REG_C:
                case REG_D:
                case REG_E:
                case REG_H:
                case REG_L:
                case REG_A: // INC r
                    INSTR_B(0x04 + reg1*8 + parm);
                    break;

                case REG_BC:
                case REG_DE:
                case REG_HL:
                case REG_SP:    // INC rr
                    INSTR_B(0x03 + (reg1-REG_BC)*16 + parm*8);
                    break;

                case REG_IX:
                case REG_IY:
                    if (curCPU == CPU_GBZ80)
                    {
                        ASMX_IllegalOperand();
                    }
                    else
                    {
                        INSTR_X(Z80_DDFD(reg1) + 0x23 + parm*8);
                    }
                    break;

                case REG_Paren: // INC (HL)
                    switch ((reg1 = REG_Get(Z80_regs)))
                    {
                        case reg_EOL:
                            break;

                        case REG_HL:
                            if (TOKEN_RParen()) break;
                            INSTR_B(0x34 + parm);
                            break;

                        case REG_IX:
                        case REG_IY:
                            if (curCPU == CPU_GBZ80)
                            {
                                ASMX_IllegalOperand();
                            }
                            else
                            {
                                val = Z80_IXOffset();
                                INSTR_XB(Z80_DDFD(reg1) + 0x34 + parm, val);
                            }
                            break;

                        default:
                            ASMX_IllegalOperand();
                            break;
                    }
                    break;

                default:
                    ASMX_IllegalOperand();
                    break;
            }
            break;

        case OP_JP_CALL:
            oldLine = linePtr;
            token = TOKEN_GetWord(word);
            if (token == '(')
            {
                // JP (HL/IX/IY)
                if (parm >> 8 != 0xC3)
                {
                    ASMX_IllegalOperand();
                }
                else
                {
                    reg1 = REG_Get(Z80_regs);
                    if (TOKEN_RParen()) break;
                    switch (reg1)
                    {
                        case reg_EOL:
                            break;

                        case REG_HL:
                            INSTR_B(0xE9);
                            break;

                        case REG_IX:
                        case REG_IY:
                            if (curCPU == CPU_GBZ80)
                            {
                                ASMX_IllegalOperand();
                            }
                            else
                            {
                                INSTR_X(Z80_DDFD(reg1) + 0xE9);
                            }
                            break;

                        default:
                            ASMX_IllegalOperand();
                            break;
                    }
                }
            }
            else
            {
                reg1 = REG_Find(word, conds);
                if (reg1 < 0)
                {
                    linePtr = oldLine;
                    val = EXPR_Eval();
                    INSTR_BW(parm >> 8, val);
                }
                else
                {
                    if (TOKEN_Comma()) return 1;
                    val = EXPR_Eval();
                    if (curCPU == CPU_GBZ80 && reg1 > 3)
                    {
                        ASMX_IllegalOperand();
                    }
                    else
                    {
                        INSTR_BW((parm & 255) + reg1*8, val);
                    }
                }
                if ((parm >> 8) == 0xC3 && reg1 <= 3)
                {
                    val = locPtr + 2 - val;
                    if (-128 <= val && val <= 128 && !exactFlag)
                    {
                        // max is +128 because JR will save a byte
                        ASMX_Warning("JR instruction could be used here");
                    }
                }
            }
            break;

        case OP_JR:
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

        case OP_RET:
            if (TOKEN_GetWord(word) == 0)
            {
                INSTR_B(0xC9);
            }
            else
            {
                reg1 = REG_Find(word, conds);
                if (reg1 < 0)
                {
                    ASMX_IllegalOperand();
                }
                else if (curCPU == CPU_GBZ80 && reg1 > 3)
                {
                    ASMX_IllegalOperand();
                }
                else
                {
                    INSTR_B(0xC0 + reg1*8);
                }
            }
            break;

        case OP_IN:
            if (curCPU == CPU_GBZ80)
            {
                return 0;
            }
            switch ((reg1 = REG_Get(Z80_regs)))
            {
                case reg_EOL:
                    break;

                case REG_B:
                case REG_C:
                case REG_D:
                case REG_E:
                case REG_H:
                case REG_L:
                case REG_A:
                    if (TOKEN_Comma()) break;
                    if (TOKEN_Expect("(")) break;
                    switch ((reg2 = REG_Get(Z80_regs)))
                    {
                        case reg_EOL:
                            break;

                        case reg_None:
                            if (reg1 != REG_A)
                            {
                                ASMX_IllegalOperand();
                            }
                            else
                            {
                                val = EXPR_Eval();
                                if (TOKEN_RParen()) break;
                                INSTR_BB(0xDB, val);
                            }
                            break;

                        case REG_C:
                            if (TOKEN_RParen()) break;
                            INSTR_X(0xED40 + reg1*8);
                            break;

                        default:
                            ASMX_IllegalOperand();
                            break;
                    }
                    break;

                default:
                    ASMX_IllegalOperand();
                    break;
            }
            break;

        case OP_OUT:
            if (curCPU == CPU_GBZ80)
            {
                return 0;
            }
            if (TOKEN_Expect("(")) break;
            switch ((reg1 = REG_Get(Z80_regs)))
            {
                case reg_EOL:
                    break;

                case reg_None:
                    val = EXPR_Eval();
                    if (TOKEN_RParen()) break;
                    if (TOKEN_Comma()) break;
                    if (TOKEN_Expect("A")) break;
                    INSTR_BB(0xD3, val);
                    break;

                case REG_C:
                    if (TOKEN_RParen()) break;
                    if (TOKEN_Comma()) break;
                    switch ((reg2 = REG_Get(Z80_regs)))
                    {
                        case reg_EOL:
                            break;

                        case REG_B:
                        case REG_C:
                        case REG_D:
                        case REG_E:
                        case REG_H:
                        case REG_L:
                        case REG_A:
                            INSTR_X(0xED41 + reg2*8);
                            break;

                        default:
                            ASMX_IllegalOperand();
                            break;
                    }
                    break;

                default:
                    ASMX_IllegalOperand();
                    break;
            }
            break;

        case OP_PushPop:
            switch ((reg1 = REG_Get(Z80_regs)))
            {
                case reg_EOL:
                    break;

                case REG_BC:
                case REG_DE:
                case REG_HL:
                    INSTR_B(parm + (reg1-REG_BC)*16);
                    break;

                case REG_AF:
                    INSTR_B(parm + 0x30);
                    break;

                case REG_IX:
                case REG_IY:
                    if (curCPU == CPU_GBZ80)
                    {
                        ASMX_IllegalOperand();
                    }
                    else
                    {
                        INSTR_X(Z80_DDFD(reg1) + 0x20 + parm);
                    }
                    break;

                default:
                    ASMX_IllegalOperand();
                    break;
            }
            break;

        case OP_Arith:
            Z80_DoArith(parm >> 8, parm & 255);
            break;

        case OP_Rotate:
            switch ((reg1 = REG_Get(Z80_regs)))
            {
                case reg_EOL:
                    break;

                case REG_B:
                case REG_C:
                case REG_D:
                case REG_E:
                case REG_H:
                case REG_L:
                case REG_A:     // RLC r
                    INSTR_X(0xCB00 + parm + reg1);
                    break;

                case REG_Paren:
                    switch ((reg1 = REG_Get(Z80_regs)))
                    {
                        case reg_EOL:
                            break;

                        case REG_HL:
                            if (TOKEN_RParen()) break;
                            INSTR_X(0xCB00 + parm + REG_M);
                            break;

                        case REG_IX:
                        case REG_IY:
                            if (curCPU == CPU_GBZ80)
                            {
                                ASMX_IllegalOperand();
                                break;
                            }
                            val = Z80_IXOffset();
                            INSTR_XBB(Z80_DDFD(reg1) + 0xCB, val, parm + REG_M);
                            break;

                        default:
                            ASMX_IllegalOperand();
                            break;
                    }
                    break;

                default:
                    ASMX_IllegalOperand();
                    break;
            }
            break;

        case OP_IM:
            if (curCPU == CPU_GBZ80)
            {
                return 0;
            }
            switch (REG_Get("0 1 2"))
            {
                case 0:
                    INSTR_X(0xED46);
                    break;
                case 1:
                    INSTR_X(0xED56);
                    break;
                case 2:
                    INSTR_X(0xED5E);
                    break;
                default:
                    ASMX_IllegalOperand();
                    break;
            }
            break;

        case OP_DJNZ:
            if (curCPU == CPU_GBZ80)
            {
                return 0;
            }
            val = EXPR_EvalBranch(2);
            INSTR_BB(0x10, val);
            break;

        case OP_RST:
            val = EXPR_Eval();
            if (0 <= val && val <= 7)
            {
                INSTR_B(0xC7 + val*8);
            }
            else if ((val & 0xC7) == 0)
            {
                // [$00,$08,$10,$18,$20,$28,$30,$38]
                INSTR_B(0xC7 + val);
            }
            else
            {
                ASMX_IllegalOperand();
                break;
            }

            // allow bytes to follow the RST address because
            // RST vectors are used this way so much
            oldLine = line;
            token = TOKEN_GetWord(word);
            if (token == ',')
            {
                while (token == ',' && instrLen < MAX_BYTSTR)
                {
                    bytStr[instrLen++] = EXPR_Eval();
                    oldLine = line;
                    token = TOKEN_GetWord(word);
                }
                instrLen = -instrLen;   // negative means we're using long DB listing format
            }
            if (token)
            {
                linePtr = oldLine;  // ensure a too many operands error
            }
            break;

        case OP_SWAP:
            if (curCPU != CPU_GBZ80)
            {
                return 0;
            }
            switch ((reg1 = REG_Get(Z80_regs)))
            {
                case reg_EOL:
                    break;

                case REG_B:
                case REG_C:
                case REG_D:
                case REG_E:
                case REG_H:
                case REG_L:
                case REG_A:     // SWAP r
                    INSTR_X(0xCB30 + reg1);
                    break;

                case REG_Paren:
                    switch ((reg1 = REG_Get(Z80_regs)))
                    {
                        case reg_EOL:
                            break;

                        case REG_HL: // SWAP (HL)
                            if (TOKEN_RParen()) break;
                            INSTR_X(0xCB36);
                            break;

                        default:
                            ASMX_IllegalOperand();
                            break;
                    }
                    break;

                default:
                    ASMX_IllegalOperand();
                    break;
            }
            break;

        case OP_LDH:
            if (curCPU != CPU_GBZ80)
            {
                return 0;
            }
            switch ((reg1 = REG_Get(Z80_regs)))
            {
                case reg_EOL:
                    break;

                case REG_A:     // A,(nn)
                    if (TOKEN_Comma()) break;
                    if (TOKEN_Expect("(")) break;
                    val = EXPR_Eval();
                    if (TOKEN_RParen()) break;
                    if ((val & 0xFF00) != 0 && (val & 0xFF00) != 0xFF00)
                    {
                        ASMX_Error("LDH address out of range");
                    }
                    INSTR_BB(0xF0, val);
                    break;

                case REG_Paren: // (nn),A
                    val = EXPR_Eval();
                    if (TOKEN_RParen()) break;
                    if (TOKEN_Comma()) break;
                    if (TOKEN_Expect("A")) break;
                    if ((val & 0xFF00) != 0 && (val & 0xFF00) != 0xFF00)
                    {
                        ASMX_Error("LDH address out of range");
                    }
                    INSTR_BB(0xE0, val);
                    break;

                default:
                    ASMX_IllegalOperand();
                    break;
            }
            break;

        case OP_IN0:
            if (curCPU != CPU_Z180)
            {
                return 0;
            }
            switch ((reg1 = REG_Get(Z80_IN0_OUT0_regs)))
            {
                case reg_EOL:
                    break;

                case REG_B:
                case REG_C:
                case REG_D:
                case REG_E:
                case REG_H:
                case REG_L:
                case REG_M: // Note: "F" becomes Reg_M
                case REG_A:
                    if (TOKEN_Comma()) break;
                    if (TOKEN_Expect("(")) break;
                    val = EXPR_Eval();
                    if (TOKEN_RParen()) break;
                    INSTR_XB(parm + reg1*8, val);
                    break;

                default:
                    ASMX_IllegalOperand();
                    break;
            }
            break;

        case OP_OUT0:
            if (curCPU != CPU_Z180)
            {
                return 0;
            }
            if (TOKEN_Expect("(")) break;
            val = EXPR_Eval();
            if (TOKEN_RParen()) break;
            if (TOKEN_Comma()) break;
            switch ((reg1 = REG_Get(Z80_IN0_OUT0_regs)))
            {
                case reg_EOL:
                    break;

                case REG_B:
                case REG_C:
                case REG_D:
                case REG_E:
                case REG_H:
                case REG_L:
                case REG_M: // Note: "F" becomes Reg_M
                case REG_A:
                    INSTR_XB(parm + reg1*8, val);
                    break;

                default:
                    ASMX_IllegalOperand();
                    break;
            }
            break;

        case OP_TST:
            if (curCPU != CPU_Z180)
            {
                return 0;
            }
            switch ((reg1 = REG_Get(Z80_regs)))
            {
                case reg_EOL:
                    break;

                case REG_B:
                case REG_C:
                case REG_D:
                case REG_E:
                case REG_H:
                case REG_L:
                case REG_A:
                    INSTR_X(parm + reg1*8);
                    break;

                case REG_Paren:
                    switch ((reg1 = REG_Get("HL C")))
                    {
                        case reg_EOL:
                            break;

                        case 0: // TST (HL)
                            if (TOKEN_RParen()) break;
                            INSTR_X(parm + REG_M*8);
                            break;

                        case 1: // TST (C),b
                            if (TOKEN_RParen()) break;
                            if (TOKEN_Comma()) break;
                            val = EXPR_Eval();
                            INSTR_XB(0xED64, val);
                            break;

                        default:
                            ASMX_IllegalOperand();
                            break;
                    }
                    break;

                default:
                    ASMX_IllegalOperand();
                    break;
            }
            break;

        case OP_MLT:
            if (curCPU != CPU_Z180)
            {
                return 0;
            }
            switch ((reg1 = REG_Get(Z80_regs)))
            {
                case reg_EOL:
                    break;

                case REG_BC:
                case REG_DE:
                case REG_HL:
                case REG_SP:
                    INSTR_X(parm + (reg1-REG_BC)*16);
                    break;

                default:
                    ASMX_IllegalOperand();
                    break;
            }
            break;

        case OP_TSTIO:
            if (curCPU != CPU_Z180)
            {
                return 0;
            }
            val = EXPR_Eval();
            INSTR_XB(parm, val);
            break;

        default:
            return 0;
            break;
    }
    return 1;
}


static int Z80_DoCPULabelOp(int typ, int parm, char *labl)
{
    int     val;
    Str255  word;
    char    *oldLine;
    int     token;
    int     reg1;
    int     reg2;

    switch (typ)
    {
        // OP_Bit needs to be implemented like a pseudo-op so that
        //        SET can fall back to the standard SET pseudo-op
        case OP_Bit:
            oldLine = linePtr;
            reg1 = EXPR_Eval();                  // get bit number
            token = TOKEN_GetWord(word);          // attempt to get comma
            // if end of line and SET opcode
            if (token == 0 && parm == 0xC0)
            {
                linePtr = oldLine;
                if (!errFlag || pass < 2)   // don't double-error on second pass
                {
                    ASMX_DoLabelOp(OP_EQU, 1, labl);  // fall back to SET pseudo-op
                }
            }
            else
            {
                SYM_Def(labl, locPtr, false, false); // define label if present
                showAddr = true;
                if (token != ',')           // validate that comma is present
                {
                    ASMX_Error("\",\" expected");
                    break;
                }
                switch ((reg2 = REG_Get(Z80_regs)))
                {
                    case reg_EOL:
                        break;

                    case REG_B:
                    case REG_C:
                    case REG_D:
                    case REG_E:
                    case REG_H:
                    case REG_L:
                    case REG_A:         // BIT n,r
                        INSTR_X(0xCB00 + parm + reg1*8 + reg2);
                        break;

                    case REG_Paren:     // BIT n,(HL)
                        switch ((reg2 = REG_Get(Z80_regs)))
                        {
                            case reg_EOL:
                                break;

                            case REG_HL:
                                if (TOKEN_RParen()) break;
                                INSTR_X(0xCB00 + parm + reg1*8 + REG_M);
                                break;

                            case REG_IX:
                            case REG_IY:
                                if (curCPU == CPU_GBZ80)
                                {
                                    ASMX_IllegalOperand();
                                    break;
                                }
                                val = Z80_IXOffset();
                                INSTR_XBB(Z80_DDFD(reg2) + 0xCB, val, parm + reg1*8 + REG_M);
                                break;

                            default:
                                ASMX_IllegalOperand();
                                break;
                        }
                        break;

                    default:            // invalid or unknown register
                        ASMX_IllegalOperand();
                        break;
                }
            }
            break;

        default:
            return 0;
            break;
    }
    return 1;
}


void Z80_AsmInit(void)
{
    void *p = ASMX_AddAsm(versionName, &Z80_DoCPUOpcode, &Z80_DoCPULabelOp, NULL);

    enum   // standard CPU options
    {
        OPT = (OPT_ATSYM | OPT_DOLLARSYM),
    };

    ASMX_AddCPU(p, "Z80",   CPU_Z80,   END_LITTLE, ADDR_16, LIST_24, 8, OPT, Z80_opcdTab);
    ASMX_AddCPU(p, "Z180",  CPU_Z180,  END_LITTLE, ADDR_16, LIST_24, 8, OPT, Z80_opcdTab);
    ASMX_AddCPU(p, "GBZ80", CPU_GBZ80, END_LITTLE, ADDR_16, LIST_24, 8, OPT, Z80_opcdTab);
}
