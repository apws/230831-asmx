// asmz80.c

#define versionName "Z-80 assembler"
//#define NICE_ADD // allow ADD/ADC/SBC without "A,"
#include "asmx.h"

enum {
    CPU_Z80,    // standard Z80
    CPU_GBZ80,  // Gameboy Z80 variant
    CPU_Z180,   // Z180 - not yet implemented
    CPU_Z80U    // Z80 with undocumented instructions - not yet implemented
};

enum {
    o_None,     // No operands
    o_NoneNGB,  // No operands, not in GBZ80
    o_NoneGB,   // No operands, GBZ80-only
    o_None180,  // No operands, Z180 only

    o_LD,       // Generic LD opcode
    o_EX,       // Generic EX opcode
    o_ADD,      // Generic ADD opcode
    o_ADC_SBC,  // Generic ADC and SBC opcodes
    o_INC_DEC,  // Generic INC and DEC opcodes
    o_JP_CALL,  // Generic JP and CALL opcodes
    o_JR,       // Generic JR opcode
    o_RET,      // Generic RET opcode
    o_IN,       // Generic IN opcode
    o_OUT,      // Generic OUT opcode

    o_PushPop,  // PUSH and POP instructions
    o_Arith,    // Arithmetic instructions
    o_Rotate,   // Z-80 rotate instructions
//  o_Bit,      // BIT, RES, and SET instructions (moved to LabelOp for SET fall-back)
    o_IM,       // IM instruction
    o_DJNZ,     // DJNZ instruction
    o_RST,      // RST instruction

    o_SWAP,     // SWAP instruction for GBZ80
    o_LDH,      // LDH instruction for GBZ80

    o_IN0,      // IN0 opcode, Z180 only
    o_OUT0,     // OUT0 opcode, Z180 only
    o_TST,      // TST opcode, Z180 only
    o_MLT,      // MLT opcode, Z180 only
    o_TSTIO,    // TSTIO opcode, Z180 only

    o_Bit = o_LabelOp   // BIT, RES, and SET instructions need to be pseudo-op to allow SET fall-back
};

const char conds[] = "NZ Z NC C PO PE P M";
// NZ=0 Z=1 NC=2 C=3 PO=4 PE=5 P=6 M=7

// note: L is in Z80_regs[] twice as a placeholder for (HL)
const char Z80_regs[] = "B C D E H L L A I R BC DE HL SP IX IY AF HLD HLI (";

const char Z80_IN0_OUT0_regs[] = "B C D E H L F A";

enum regType {  // these are keyed to Z80_regs[] above
    reg_B,          //  0
    reg_C,          //  1
    reg_D,          //  2
    reg_E,          //  3
    reg_H,          //  4
    reg_L,          //  5
    reg_M,          //  6
    reg_A,          //  7
    reg_I,          //  8
    reg_R,          //  9
    reg_BC,         // 10
    reg_DE,         // 11
    reg_HL,         // 12
    reg_SP,         // 13
    reg_IX,         // 14
    reg_IY,         // 15
    reg_AF,         // 16
    reg_HLD,        // 17 GBZ80 HL with post-decrement
    reg_HLI,        // 18 GBZ80 HL with post-increment
    reg_Paren       // 19
};

static const struct OpcdRec Z80_opcdTab[] = {
    {"NOP",  o_None,    0x00},
    {"RLCA", o_None,    0x07},
    {"RRCA", o_None,    0x0F},
    {"RLA",  o_None,    0x17},
    {"RRA",  o_None,    0x1F},
    {"DAA",  o_None,    0x27},
    {"CPL",  o_None,    0x2F},
    {"SCF",  o_None,    0x37},
    {"CCF",  o_None,    0x3F},
    {"HALT", o_None,    0x76},
    {"DI",   o_None,    0xF3},
    {"EI",   o_None,    0xFB},
    {"EXX",  o_NoneNGB, 0xD9},
    {"NEG",  o_NoneNGB, 0xED44},
    {"RETN", o_NoneNGB, 0xED45},
    {"RETI", o_None,    0xED4D},
    {"RRD",  o_NoneNGB, 0xED67},
    {"RLD",  o_NoneNGB, 0xED6F},
    {"LDI",  o_None,    0xEDA0},
    {"CPI",  o_NoneNGB, 0xEDA1},
    {"INI",  o_NoneNGB, 0xEDA2},
    {"OUTI", o_NoneNGB, 0xEDA3},
    {"LDIR", o_NoneNGB, 0xEDB0},
    {"CPIR", o_NoneNGB, 0xEDB1},
    {"INIR", o_NoneNGB, 0xEDB2},
    {"OTIR", o_NoneNGB, 0xEDB3},
    {"LDD",  o_None,    0xEDA8},
    {"CPD",  o_NoneNGB, 0xEDA9},
    {"IND",  o_NoneNGB, 0xEDAA},
    {"OUTD", o_NoneNGB, 0xEDAB},
    {"LDDR", o_NoneNGB, 0xEDB8},
    {"CPDR", o_NoneNGB, 0xEDB9},
    {"INDR", o_NoneNGB, 0xEDBA},
    {"OTDR", o_NoneNGB, 0xEDBB},

    {"LD",   o_LD,      0},
    {"EX",   o_EX,      0},

    {"ADD",  o_ADD,     0},
    {"ADC",  o_ADC_SBC, 0},
    {"SBC",  o_ADC_SBC, 1},
    {"INC",  o_INC_DEC, 0},
    {"DEC",  o_INC_DEC, 1},

    {"JP",   o_JP_CALL, 0xC3C2},
    {"CALL", o_JP_CALL, 0xCDC4},
    {"JR",   o_JR,      0},
    {"RET",  o_RET,     0},

    {"PUSH", o_PushPop, 0xC5},
    {"POP",  o_PushPop, 0xC1},

    {"SUB",  o_Arith,   0xD690},
    {"AND",  o_Arith,   0xE6A0},
    {"XOR",  o_Arith,   0xEEA8},
    {"OR",   o_Arith,   0xF6B0},
    {"CP",   o_Arith,   0xFEB8},

    {"RLC",  o_Rotate,  0x00},
    {"RRC",  o_Rotate,  0x08},
    {"RL",   o_Rotate,  0x10},
    {"RR",   o_Rotate,  0x18},
    {"SLA",  o_Rotate,  0x20},
    {"SRA",  o_Rotate,  0x28},
    {"SRL",  o_Rotate,  0x38},

    {"BIT",  o_Bit,     0x40},
    {"RES",  o_Bit,     0x80},
    {"SET",  o_Bit,     0xC0},

    {"IM",   o_IM,      0},

    {"DJNZ", o_DJNZ,    0},

    {"IN",   o_IN,      0},
    {"OUT",  o_OUT,     0},

    {"RST",  o_RST,     0},

//  Gameboy Z-80 specific instructions

    {"STOP", o_NoneGB,  0x10},
//  {"DEBUG",o_NoneGB,  0xED}, // does this really exist?
    {"SWAP", o_SWAP,    0x00},
    {"LDH",  o_LDH,     0x00},

//  Z180 specific instructions

    {"IN0",   o_IN0,     0xED00 }, // + (r << 3) : bb
    {"OUT0",  o_OUT0,    0xED01 }, // + (r << 3) : bb
    {"TST",   o_TST,     0xED04 }, // + (r << 3), or ED64 for TST (C),b
    {"MLT",   o_MLT,     0xED4C }, // + (rr << 4)
    {"TSTIO", o_TSTIO,   0xED74 }, // : bb

    {"SLP",   o_None180, 0xED76 },
    {"OTIM",  o_None180, 0xED83 },
    {"OTDM",  o_None180, 0xED8B },
    {"OTIMR", o_None180, 0xED93 },
    {"OTDMR", o_None180, 0xED9B },

    {"",    o_Illegal,  0}
};


// --------------------------------------------------------------


static int IXOffset()
{
    Str255  word;

    int val = 0;
    char *oldLine = linePtr;
    int token = GetWord(word);
    if (token == '+' || token == '-') {
        val = Eval();
        if (token == '-') {
            val = -val;
        }
    } else {
        linePtr = oldLine;
    }

    RParen();

    return val;
}


static int DDFD(int reg)
{
    switch (reg) {
        default:
        case reg_HL: return      0;
        case reg_IX: return 0xDD00;
        case reg_IY: return 0xFD00;
    }
}


static void DoArith(int imm, int reg)
{
    int         reg2;
    int         val;

    char *oldLine = linePtr; // save position because of (HL) vs (expr)

    switch ((reg2 = GetReg(Z80_regs))) {
        case reg_EOL:
            break;

        case reg_None:  // ADD A,nn
            val = Eval();
            InstrBB(imm, val);
            break;

        case reg_B:
        case reg_C:
        case reg_D:
        case reg_E:
        case reg_H:
        case reg_L:
        case reg_A:     // ADD A,r
            InstrB(reg + reg2);
            break;

        case reg_Paren: // ADD A,(
            switch ((reg2 = GetReg(Z80_regs))) {
                case reg_EOL:
                    break;

                case reg_HL:
                    if (RParen()) break;
                    InstrB(reg+reg_M);
                    break;

                case reg_IX:
                case reg_IY:
                    if (curCPU == CPU_GBZ80) {
                        IllegalOperand();
                        break;
                    }
                    val = IXOffset();
                    InstrXB(DDFD(reg2) + reg + reg_M, val);
                    break;

                default:
                    // must be constant "(expression)" so try again that way
                    linePtr = oldLine;
                    val = Eval();
                    InstrBB(imm, val);
            }
            break;

         default:
            IllegalOperand();
            break;
    }
}


static int Z80_DoCPUOpcode(int typ, int parm)
{
    int     val, reg1, reg2;
    Str255  word;
    int     token;

    char *oldLine = linePtr;
    switch (typ) {
        case o_None180:
            if (curCPU != CPU_Z180) return 0;
            FALLTHROUGH;
        case o_None:
        case o_NoneGB:
        case o_NoneNGB:
            if (curCPU != CPU_GBZ80 || (parm & 0xFFF7) != 0xEDA0) {
                // LDI/LDD 0xEDA0/0xEDA8
                if ((typ == o_NoneGB)  && curCPU != CPU_GBZ80) return 0;
                if ((typ == o_NoneNGB) && curCPU == CPU_GBZ80) return 0;
                if (curCPU == CPU_GBZ80 && parm == 0xED4D) {
                    parm = 0xD9; // GBZ80 RETI
                }
                InstrX(parm & 0xFFFF);
                break;
            }

            // fall-through for GBZ80 LDI/LDD:
            parm = ((parm & 0x08) << 1) | 0x22;

            switch ((reg1 = GetReg(Z80_regs))) {
                case reg_EOL:
                    break;

                case reg_A:     // A,(HL)
                    if (Comma()) break;
                    if (Expect("(")) break;
                    switch ((reg1 = GetReg(Z80_regs))) {
                        case reg_EOL:
                            break;

                        case reg_HL: // A,(HL)
                            if (RParen()) break;
                            InstrB(parm + 8);
                            break;

                        default:
                            IllegalOperand();
                            break;
                    }
                    break;

                case reg_Paren: // (HL),A
                    switch ((reg1 = GetReg(Z80_regs))) {
                        case reg_EOL:
                            break;

                        case reg_HL: // (HL),A
                            if (RParen()) break;
                            if (Comma()) break;
                            if (Expect("A")) break;
                            InstrB(parm);
                            break;

                        default:
                            IllegalOperand();
                            break;
                    }
                    break;

                default:
                    IllegalOperand();
                    break;
            }
            break;

        case o_LD:
            switch ((reg1 = GetReg(Z80_regs))) {
                case reg_EOL:
                    break;

                case reg_None:  // LD nnnn,?
                    IllegalOperand();
                    break;

                case reg_B:
                case reg_C:
                case reg_D:
                case reg_E:
                case reg_H:
                case reg_L:
                case reg_A:     // LD r,?
                    if (Comma()) break;
                    switch ((reg2 = GetReg(Z80_regs))) {
                        case reg_EOL:
                            break;

                        case reg_B:
                        case reg_C:
                        case reg_D:
                        case reg_E:
                        case reg_H:
                        case reg_L:
                        case reg_A:     // LD r,r
                            InstrB(0x40 + reg1*8 + reg2);
                            break;

                        case reg_I:     // LD A,I
                            if (reg1 != reg_A || curCPU == CPU_GBZ80) {
                                IllegalOperand();
                                break;
                            }
                            InstrX(0xED57);
                            break;

                        case reg_R:     // LD A,R
                            if (reg1 != reg_A || curCPU == CPU_GBZ80) {
                                IllegalOperand();
                                break;
                            }
                            InstrX(0xED5F);
                            break;

                        case reg_Paren:     // LD r,(?)
                            switch ((reg2 = GetReg(Z80_regs))) {
                                case reg_EOL:
                                    break;

                                case reg_BC:    // LD A,(BC)
                                case reg_DE:    // LD A,(DE)
                                    if (reg1 != reg_A) {
                                        IllegalOperand();
                                    } else {
                                        if (RParen()) break;
                                        InstrB(0x0A + (reg2-reg_BC)*16);
                                    }
                                    break;

                                case reg_C:
                                    if (curCPU != CPU_GBZ80) {
                                        IllegalOperand();
                                    } else {
                                        if (RParen()) break;
                                        InstrB(0xF2); // LD A,(C)
                                    }
                                    break;

                                case reg_HLI:
                                    if (curCPU != CPU_GBZ80) {
                                        IllegalOperand();
                                    } else {
                                        if (RParen()) break;
                                        InstrB(0x2A); // LD A,(HLI)
                                    }
                                    break;

                                case reg_HLD:
                                    if (curCPU != CPU_GBZ80) {
                                        IllegalOperand();
                                    } else {
                                        if (RParen()) break;
                                        InstrB(0x3A); // LD A,(HLD)
                                    }
                                    break;

                                case reg_HL:    // LD r,(HL)
                                    if (curCPU == CPU_GBZ80 && *linePtr == '+') {
                                        linePtr++;
                                        if (RParen()) break;
                                        InstrB(0x2A); // LD A,(HL+)
                                    } else if (curCPU == CPU_GBZ80 && *linePtr == '-') {
                                        linePtr++;
                                        if (RParen()) break;
                                        InstrB(0x3A); // LD A,(HL-)
                                    } else {
                                        if (RParen()) break;
                                        InstrB(0x40 + reg1*8 + reg_M);
                                    }
                                    break;

                                case reg_IX:    // LD r,(IX+d)
                                case reg_IY:    // LD r,(IY+d)
                                    if (curCPU == CPU_GBZ80) {
                                        IllegalOperand();
                                        break;
                                    }
                                    val = IXOffset();
                                    InstrXB(DDFD(reg2) + 0x46 + reg1*8, val);
                                    break;

                                case reg_None:  // LD A,(nnnn)
                                    if (reg1 != reg_A) {
                                        IllegalOperand();
                                    } else {
                                        val = Eval();
                                        if (RParen()) break;
                                        if (curCPU == CPU_GBZ80 && evalKnown && (val & 0xFF00) == 0xFF00) {
                                            InstrBB(0xF0, 0xFF);
                                        } else if (curCPU == CPU_GBZ80) {
                                            InstrBW(0xFA, val);
                                        } else {
                                            InstrBW(0x3A, val);
                                        }
                                    }
                                    break;

                                default:
                                    IllegalOperand();
                                    break;
                            }
                            break;

                        case reg_None:  // LD r,nn
                            InstrBB(0x06 + reg1*8, Eval());
                            break;

                        default:
                            IllegalOperand();
                            break;
                    }
                    break;

                case reg_I:     // LD I,A
                    if (curCPU == CPU_GBZ80) {
                        IllegalOperand();
                        break;
                    }
                    if (Comma()) break;
                    if (Expect("A")) break;
                    InstrX(0xED47);
                    break;

                case reg_R:     // LD R,A
                    if (curCPU == CPU_GBZ80) {
                        IllegalOperand();
                        break;
                    }
                    if (Comma()) break;
                    if (Expect("A")) break;
                    InstrX(0xED4F);
                    break;

                case reg_BC:
                case reg_DE:
                case reg_HL:
                case reg_SP:    // LD rr,?
                    if (Comma()) break;
                    oldLine = linePtr; // save linePtr in case of backtracking
                    switch ((reg2 = GetReg(Z80_regs))) {
                        case reg_EOL:
                            break;

                        case reg_HL:    // LD SP,HL/IX/IY
                        case reg_IX:
                        case reg_IY:
                            if (reg1 != reg_SP) {
                                IllegalOperand();
                            } else if (reg2 != reg_HL && curCPU == CPU_GBZ80) {
                                IllegalOperand();
                            } else {
                                InstrX(DDFD(reg2) + 0xF9);
                            }
                            break;

                        case reg_Paren:
                            if (curCPU == CPU_GBZ80) {
                                IllegalOperand();
                            } else if (reg1 == reg_HL) {
                                val = Eval();   // LD HL,(nnnn)
                                if (RParen()) break;
                                InstrBW(0x2A, val);
                            } else {
                                val = Eval();   // LD BC/DE/SP,(nnnn)
                                if (RParen()) break;
                                InstrXW(0xED4B + (reg1-reg_BC)*16, val);
                            }

                            // at this point, if there is any extra stuff on the line,
                            // backtrack and try again with reg_None case
                            // to handle the case of LD BC,(foo + 1) * 256, etc.
                            token = GetWord(word);
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
                            val = Eval();
                            InstrBW(0x01 + (reg1-reg_BC)*16, val);
                            break;

                        case reg_SP:
                            if (curCPU == CPU_GBZ80) {
                                InstrB(0xF8); // LD HL,SP
                                break;
                            }
                            FALLTHROUGH;
                        default:
                            IllegalOperand();
                            break;
                    }
                    break;

                case reg_IX:    // LD IX,?
                case reg_IY:    // LD IY,?
                    if (curCPU == CPU_GBZ80) {
                        IllegalOperand();
                        break;
                    }
                    if (Comma()) break;
                    switch ((reg2 = GetReg(Z80_regs))) {
                        case reg_EOL:
                            break;

                        case reg_Paren: // LD IX,(nnnn)
                            val = Eval();
                            if (RParen()) break;
                            InstrXW(DDFD(reg1) + 0x2A, val);

                            // at this point, if there is any extra stuff on the line,
                            // backtrack and try again with reg_None case
                            // to handle the case of LD IX,(foo + 1) * 256, etc.
                            token = GetWord(word);
                            if (token == 0) break;
                            // note that if an error occurs, it will be repeated twice,
                            // and we have to return to avoid that
                            if (errFlag) break;
                            // now fall through...
                            FALLTHROUGH;

                        case reg_None:  // LD IX,nnnn
                            val = Eval();
                            InstrXW(DDFD(reg1) + 0x21, val);
                            break;

                        default:
                            IllegalOperand();
                            break;
                    }
                    break;

                case reg_Paren:     // LD (?),?
                    switch ((reg1 = GetReg(Z80_regs))) {
                        case reg_EOL:
                            break;

                        case reg_None:  // LD (nnnn),?
                            val = Eval();
                            if (RParen()) break;
                            if (Comma()) break;
                            switch ((reg2 = GetReg(Z80_regs))) {
                                case reg_EOL:
                                    break;

                                case reg_A:
                                    if (curCPU == CPU_GBZ80 && evalKnown && (val & 0xFF00) == 0xFF00) {
                                        InstrBB(0xE0, val);
                                    } else if (curCPU == CPU_GBZ80) {
                                        InstrBW(0xEA, val);
                                    } else {
                                        InstrBW(0x32, val);
                                    }
                                    break;

                                case reg_HL:
                                    if (curCPU == CPU_GBZ80) {
                                        IllegalOperand();
                                    } else {
                                        InstrBW(0x22, val);
                                    }
                                    break;

                                case reg_BC:
                                case reg_DE:
                                case reg_SP:
                                    if (reg2 == reg_SP && curCPU == CPU_GBZ80) {
                                        InstrBW(0x08, val);
                                    } else if (curCPU == CPU_GBZ80) {
                                        IllegalOperand();
                                    } else {
                                        InstrXW(0xED43 + (reg2-reg_BC)*16, val);
                                    }
                                    break;

                                case reg_IX:
                                case reg_IY:
                                    if (curCPU == CPU_GBZ80) {
                                        IllegalOperand();
                                    } else {
                                        InstrXW(DDFD(reg2) + 0x22, val);
                                    }
                                    break;

                                default:
                                    IllegalOperand();
                                    break;
                            }
                            break;

                            case reg_BC:
                            case reg_DE:
                                if (RParen()) break;
                                if (Comma()) break;
                                if (Expect("A")) break;
                                InstrB(0x02 + (reg1-reg_BC)*16);
                                break;

                            case reg_C:
                                if (curCPU != CPU_GBZ80) {
                                    IllegalOperand();
                                } else {
                                    if (RParen()) break;
                                    if (Comma()) break;
                                    if (Expect("A")) break;
                                    InstrB(0xE2); // LD (C),A
                                }
                                break;

                            case reg_HLI:
                                if (curCPU != CPU_GBZ80) {
                                    IllegalOperand();
                                } else {
                                    if (RParen()) break;
                                    if (Comma()) break;
                                    if (Expect("A")) break;
                                    InstrB(0x22); // LD (HLI),A
                                }
                                break;

                            case reg_HLD:
                                if (curCPU != CPU_GBZ80) {
                                    IllegalOperand();
                                } else {
                                    if (RParen()) break;
                                    if (Comma()) break;
                                    if (Expect("A")) break;
                                    InstrB(0x32); // LD (HLD),A
                                }
                                break;

                            case reg_HL: // LD (HL),?
                                if (curCPU == CPU_GBZ80 && *linePtr == '+') {
                                    linePtr++;
                                    if (RParen()) break;
                                    if (Comma()) break;
                                    if (Expect("A")) break;
                                    InstrB(0x22); // LD (HL+),A
                                    break;
                                } else if (curCPU == CPU_GBZ80 && *linePtr == '-') {
                                    linePtr++;
                                    if (RParen()) break;
                                    if (Comma()) break;
                                    if (Expect("A")) break;
                                    InstrB(0x32); // LD (HL-),A
                                    break;
                                }
                                if (RParen()) break;
                                if (Comma()) break;
                                switch ((reg2 = GetReg(Z80_regs))) {
                                    case reg_EOL:
                                        break;

                                    case reg_None:
                                        val = Eval();
                                        InstrBB(0x36, val);
                                        break;

                                    case reg_B:
                                    case reg_C:
                                    case reg_D:
                                    case reg_E:
                                    case reg_H:
                                    case reg_L:
                                    case reg_A:
                                        InstrB(0x70 + reg2);
                                        break;

                                    default:
                                        IllegalOperand();
                                        break;
                                }
                                break;

                            case reg_IX:
                            case reg_IY:    // LD (IX),?
                                if (curCPU == CPU_GBZ80) {
                                    IllegalOperand();
                                    break;
                                }
                                val = IXOffset();
                                if (Comma()) break;
                                switch ((reg2 = GetReg(Z80_regs))) {
                                    case reg_EOL:
                                        break;

                                    case reg_None:
                                        reg2 = Eval();
                                        InstrXBB(DDFD(reg1) + 0x36, val, reg2);
                                        break;

                                    case reg_B:
                                    case reg_C:
                                    case reg_D:
                                    case reg_E:
                                    case reg_H:
                                    case reg_L:
                                    case reg_A:
                                        InstrXB(DDFD(reg1) + 0x70 + reg2, val);
                                        break;

                                    default:
                                        IllegalOperand();
                                        break;
                                }
                                break;

                            default:
                                IllegalOperand();
                                break;
                        }
                    break;

                default:
                    IllegalOperand();
                    break;
            }
            break;

        case o_EX:
            if (curCPU == CPU_GBZ80) {
                return 0;
            }
            switch ((reg1 = GetReg(Z80_regs))) {
                case reg_EOL:
                    break;

                case reg_DE:    // EX DE,HL }
                    if (Comma()) break;
                    if (Expect("HL")) break;
                    InstrB(0xEB);
                    break;

                case reg_AF:    // EX AF,AF' }
                    if (Comma()) break;
                    if (Expect("AF")) break;
                    if (Expect("'")) break;
                    InstrB(0x08);
                    break;

                case reg_Paren: // EX (SP),? }
                    if (Expect("SP")) break;
                    if (RParen()) break;
                    if (Comma()) break;
                    switch ((reg2 = GetReg(Z80_regs))) {
                        case reg_EOL:
                            break;

                        case reg_HL:
                        case reg_IX:
                        case reg_IY:
                            InstrX(DDFD(reg2) + 0xE3);
                            break;

                        default:
                            IllegalOperand();
                            break;
                    }
                    break;

                default:
                    IllegalOperand();
                    break;
            }
            break;

        case o_ADD:
            switch ((reg1 = GetReg(Z80_regs))) {
                case reg_EOL:
                    break;

#ifdef NICE_ADD
                case reg_Paren:
                case reg_None:
                case reg_B: // ADD r
                case reg_C:
                case reg_D:
                case reg_E:
                case reg_H:
                case reg_L:
                    linePtr = oldLine;
                    DoArith(0xC6, 0x80);
                    break;
#endif

                case reg_A:
#ifdef NICE_ADD
                    oldLine = linePtr;
                    token = GetWord(word);
                    if (token == 0) {
                        InstrB(0x87); // ADD A
                        break;
                    }
                    linePtr = oldLine;
#endif
                    if (Comma()) break;
                    DoArith(0xC6, 0x80);
                    break;

                case reg_SP:
                    if (curCPU != CPU_GBZ80) {
                        IllegalOperand();
                    } else {
                        if (Comma()) break;
                        val = Eval();
                        if (val < -128 || val > 127) {
                            Error("Offset out of range");
                        }
                        InstrBB(0xE8, val);
                    }
                    break;


                case reg_IX:
                case reg_IY:
                    if (curCPU == CPU_GBZ80) {
                        IllegalOperand();
                        break;
                    }
                    FALLTHROUGH;
                case reg_HL:
                    if (Comma()) break;
                    switch ((reg2 = GetReg(Z80_regs))) {
                        case reg_EOL:
                            break;

                        case reg_HL:
                        case reg_IX:
                        case reg_IY:
                            if (reg1 != reg2) {
                                IllegalOperand();
                                break;
                            }
                            reg2 = reg_HL;
                            FALLTHROUGH;

                        case reg_BC:
                        case reg_DE:
                        case reg_SP:
                            InstrX(DDFD(reg1) + 0x09 + (reg2-reg_BC)*16);
                            break;

                        default:
                            IllegalOperand();
                            break;
                    }
                    break;

                default:
                    IllegalOperand();
                    break;
            }
            break;

        case o_ADC_SBC:
            switch ((reg1 = GetReg(Z80_regs))) {
                case reg_EOL:
                    break;

#ifdef NICE_ADD
                case reg_Paren:
                case reg_None:
                case reg_B: // ADD r
                case reg_C:
                case reg_D:
                case reg_E:
                case reg_H:
                case reg_L:
                    linePtr = oldLine;
                    DoArith(parm*16 + 0xCE, 0x88 + parm*16);
                    break;
#endif

                case reg_A:
#ifdef NICE_ADD
                    oldLine = linePtr;
                    token = GetWord(word);
                    if (token == 0) {
                        InstrB(0x8F + parm*16); // ADD A
                        break;
                    }
                    linePtr = oldLine;
#endif
                    if (Comma()) break;
                    DoArith(parm*16 + 0xCE, 0x88 + parm*16); // had to move 0xCE because GCC complained?
                    break;

                case reg_HL:
                    if (curCPU == CPU_GBZ80) {
                        IllegalOperand();
                        break;
                    }
                    if (Comma()) break;
                    switch ((reg2 = GetReg(Z80_regs))) {
                        case reg_EOL:
                            break;

                        case reg_BC:
                        case reg_DE:
                        case reg_HL:
                        case reg_SP:
                            InstrX(0xED4A + (reg2-reg_BC)*16 - parm*8);
                            break;

                        default:
                            IllegalOperand();
                            break;
                    }
                    break;

                default:
                    IllegalOperand();
                    break;
            }
            break;

        case o_INC_DEC:
            switch ((reg1 = GetReg(Z80_regs))) {
                case reg_EOL:
                    break;

                case reg_B:
                case reg_C:
                case reg_D:
                case reg_E:
                case reg_H:
                case reg_L:
                case reg_A: // INC r
                    InstrB(0x04 + reg1*8 + parm);
                    break;

                case reg_BC:
                case reg_DE:
                case reg_HL:
                case reg_SP:    // INC rr
                    InstrB(0x03 + (reg1-reg_BC)*16 + parm*8);
                    break;

                case reg_IX:
                case reg_IY:
                    if (curCPU == CPU_GBZ80) {
                        IllegalOperand();
                    } else {
                        InstrX(DDFD(reg1) + 0x23 + parm*8);
                    }
                    break;

                case reg_Paren: // INC (HL)
                    switch ((reg1 = GetReg(Z80_regs))) {
                        case reg_EOL:
                            break;

                        case reg_HL:
                            if (RParen()) break;
                            InstrB(0x34 + parm);
                            break;

                        case reg_IX:
                        case reg_IY:
                            if (curCPU == CPU_GBZ80) {
                                IllegalOperand();
                            } else {
                                val = IXOffset();
                                InstrXB(DDFD(reg1) + 0x34 + parm, val);
                            }
                            break;

                        default:
                            IllegalOperand();
                            break;
                    }
                    break;

                default:
                    IllegalOperand();
                    break;
            }
            break;

        case o_JP_CALL:
            oldLine = linePtr;
            token = GetWord(word);
            if (token == '(') {
                // JP (HL/IX/IY)
                if (parm >> 8 != 0xC3) {
                    IllegalOperand();
                } else {
                    reg1 = GetReg(Z80_regs);
                    if (RParen()) break;
                    switch (reg1) {
                        case reg_EOL:
                            break;

                        case reg_HL:
                            InstrB(0xE9);
                            break;

                        case reg_IX:
                        case reg_IY:
                            if (curCPU == CPU_GBZ80) {
                                IllegalOperand();
                            } else {
                                InstrX(DDFD(reg1) + 0xE9);
                            }
                            break;

                        default:
                            IllegalOperand();
                            break;
                    }
                }
            } else {
                reg1 = FindReg(word, conds);
                if (reg1 < 0) {
                    linePtr = oldLine;
                    val = Eval();
                    InstrBW(parm >> 8, val);
                } else {
                    if (Comma()) return 1;
                    val = Eval();
                    if (curCPU == CPU_GBZ80 && reg1 > 3) {
                        IllegalOperand();
                    } else {
                        InstrBW((parm & 255) + reg1*8, val);
                    }
                }
                if ((parm >> 8) == 0xC3 && reg1 <= 3) {
                    val = locPtr + 2 - val;
                    if (-128 <= val && val <= 128 && !exactFlag) {
                        // max is +128 because JR will save a byte
                        Warning("JR instruction could be used here");
                    }
                }
            }
            break;

        case o_JR:
            switch ((reg1 = GetReg(conds))) {
                case reg_EOL:
                    break;

                case reg_None:
                    val = EvalBranch(2);
                    InstrBB(0x18, val);
                    break;

                case 0:
                case 1:
                case 2:
                case 3:
                    if (Comma()) break;
                    val = EvalBranch(2);
                    InstrBB(0x20 + reg1*8, val);
                    break;

                default:
                    IllegalOperand();
                    break;
            }
            break;

        case o_RET:
            if (GetWord(word) == 0) {
                InstrB(0xC9);
            } else {
                reg1 = FindReg(word, conds);
                if (reg1 < 0) {
                    IllegalOperand();
                } else if (curCPU == CPU_GBZ80 && reg1 > 3) {
                    IllegalOperand();
                } else {
                    InstrB(0xC0 + reg1*8);
                }
            }
            break;

        case o_IN:
            if (curCPU == CPU_GBZ80) {
                return 0;
            }
            switch ((reg1 = GetReg(Z80_regs))) {
                case reg_EOL:
                    break;

                case reg_B:
                case reg_C:
                case reg_D:
                case reg_E:
                case reg_H:
                case reg_L:
                case reg_A:
                    if (Comma()) break;
                    if (Expect("(")) break;
                    switch ((reg2 = GetReg(Z80_regs))) {
                        case reg_EOL:
                            break;

                        case reg_None:
                            if (reg1 != reg_A) {
                                IllegalOperand();
                            } else {
                                val = Eval();
                                if (RParen()) break;
                                InstrBB(0xDB, val);
                            }
                            break;

                        case reg_C:
                            if (RParen()) break;
                            InstrX(0xED40 + reg1*8);
                            break;

                        default:
                            IllegalOperand();
                            break;
                    }
                    break;

                default:
                    IllegalOperand();
                    break;
            }
            break;

        case o_OUT:
            if (curCPU == CPU_GBZ80) {
                return 0;
            }
            if (Expect("(")) break;
            switch ((reg1 = GetReg(Z80_regs))) {
                case reg_EOL:
                    break;

                case reg_None:
                    val = Eval();
                    if (RParen()) break;
                    if (Comma()) break;
                    if (Expect("A")) break;
                    InstrBB(0xD3, val);
                    break;

                case reg_C:
                    if (RParen()) break;
                    if (Comma()) break;
                    switch ((reg2 = GetReg(Z80_regs))) {
                        case reg_EOL:
                            break;

                        case reg_B:
                        case reg_C:
                        case reg_D:
                        case reg_E:
                        case reg_H:
                        case reg_L:
                        case reg_A:
                            InstrX(0xED41 + reg2*8);
                            break;

                        default:
                            IllegalOperand();
                            break;
                    }
                    break;

                default:
                    IllegalOperand();
                    break;
            }
            break;

        case o_PushPop:
            switch ((reg1 = GetReg(Z80_regs))) {
                case reg_EOL:
                    break;

                case reg_BC:
                case reg_DE:
                case reg_HL:
                    InstrB(parm + (reg1-reg_BC)*16);
                    break;

                case reg_AF:
                    InstrB(parm + 0x30);
                    break;

                case reg_IX:
                case reg_IY:
                    if (curCPU == CPU_GBZ80) {
                        IllegalOperand();
                    } else {
                        InstrX(DDFD(reg1) + 0x20 + parm);
                    }
                    break;

                default:
                    IllegalOperand();
                    break;
            }
            break;

        case o_Arith:
            DoArith(parm >> 8, parm & 255);
            break;

        case o_Rotate:
            switch ((reg1 = GetReg(Z80_regs))) {
                case reg_EOL:
                    break;

                case reg_B:
                case reg_C:
                case reg_D:
                case reg_E:
                case reg_H:
                case reg_L:
                case reg_A:     // RLC r
                    InstrX(0xCB00 + parm + reg1);
                    break;

                case reg_Paren:
                    switch ((reg1 = GetReg(Z80_regs))) {
                        case reg_EOL:
                            break;

                        case reg_HL:
                            if (RParen()) break;
                            InstrX(0xCB00 + parm + reg_M);
                            break;

                        case reg_IX:
                        case reg_IY:
                            if (curCPU == CPU_GBZ80) {
                                IllegalOperand();
                                break;
                            }
                            val = IXOffset();
                            InstrXBB(DDFD(reg1) + 0xCB, val, parm + reg_M);
                            break;

                        default:
                            IllegalOperand();
                            break;
                    }
                    break;

                default:
                    IllegalOperand();
                    break;
            }
            break;

        case o_IM:
            if (curCPU == CPU_GBZ80) {
                return 0;
            }
            switch (GetReg("0 1 2")) {
                case 0: InstrX(0xED46); break;
                case 1: InstrX(0xED56); break;
                case 2: InstrX(0xED5E); break;
                default:
                    IllegalOperand();
                    break;
            }
            break;

        case o_DJNZ:
            if (curCPU == CPU_GBZ80) {
                return 0;
            }
            val = EvalBranch(2);
            InstrBB(0x10, val);
            break;

        case o_RST:
            val = Eval();
            if (0 <= val && val <= 7) {
                InstrB(0xC7 + val*8);
            } else if ((val & 0xC7) == 0) {
                // [$00,$08,$10,$18,$20,$28,$30,$38]
                InstrB(0xC7 + val);
            } else {
                IllegalOperand();
                break;
            }

            // allow bytes to follow the RST address because
            // RST vectors are used this way so much
            oldLine = line;
            token = GetWord(word);
            if (token == ',') {
                while (token == ',' && instrLen < MAX_BYTSTR) {
                    bytStr[instrLen++] = Eval();
                    oldLine = line;
                    token = GetWord(word);
                }
                instrLen = -instrLen;   // negative means we're using long DB listing format
            }
            if (token) {
                linePtr = oldLine;  // ensure a too many operands error
            }
            break;

        case o_SWAP:
            if (curCPU != CPU_GBZ80) {
                return 0;
            }
            switch ((reg1 = GetReg(Z80_regs))) {
                case reg_EOL:
                    break;

                case reg_B:
                case reg_C:
                case reg_D:
                case reg_E:
                case reg_H:
                case reg_L:
                case reg_A:     // SWAP r
                    InstrX(0xCB30 + reg1);
                    break;

                case reg_Paren:
                    switch ((reg1 = GetReg(Z80_regs))) {
                        case reg_EOL:
                            break;

                        case reg_HL: // SWAP (HL)
                            if (RParen()) break;
                            InstrX(0xCB36);
                            break;

                        default:
                            IllegalOperand();
                            break;
                    }
                    break;

                default:
                    IllegalOperand();
                    break;
            }
            break;

        case o_LDH:
            if (curCPU != CPU_GBZ80) {
                return 0;
            }
            switch ((reg1 = GetReg(Z80_regs))) {
                case reg_EOL:
                    break;

                case reg_A:     // A,(nn)
                    if (Comma()) break;
                    if (Expect("(")) break;
                    val = Eval();
                    if (RParen()) break;
                    if ((val & 0xFF00) != 0 && (val & 0xFF00) != 0xFF00) {
                        Error("LDH address out of range");
                    }
                    InstrBB(0xF0, val);
                    break;

                case reg_Paren: // (nn),A
                    val = Eval();
                    if (RParen()) break;
                    if (Comma()) break;
                    if (Expect("A")) break;
                    if ((val & 0xFF00) != 0 && (val & 0xFF00) != 0xFF00) {
                        Error("LDH address out of range");
                    }
                    InstrBB(0xE0, val);
                    break;

                default:
                    IllegalOperand();
                    break;
            }
            break;

        case o_IN0:
            if (curCPU != CPU_Z180) {
                return 0;
            }
            switch ((reg1 = GetReg(Z80_IN0_OUT0_regs))) {
                case reg_EOL:
                    break;

                case reg_B:
                case reg_C:
                case reg_D:
                case reg_E:
                case reg_H:
                case reg_L:
                case reg_M: // Note: "F" becomes Reg_M
                case reg_A:
                    if (Comma()) break;
                    if (Expect("(")) break;
                    val = Eval();
                    if (RParen()) break;
                    InstrXB(parm + reg1*8, val);
                    break;

                default:
                    IllegalOperand();
                    break;
            }
            break;

        case o_OUT0:
            if (curCPU != CPU_Z180) {
                return 0;
            }
            if (Expect("(")) break;
            val = Eval();
            if (RParen()) break;
            if (Comma()) break;
            switch ((reg1 = GetReg(Z80_IN0_OUT0_regs))) {
                case reg_EOL:
                    break;

                case reg_B:
                case reg_C:
                case reg_D:
                case reg_E:
                case reg_H:
                case reg_L:
                case reg_M: // Note: "F" becomes Reg_M
                case reg_A:
                    InstrXB(parm + reg1*8, val);
                    break;

                default:
                    IllegalOperand();
                    break;
            }
            break;

        case o_TST:
            if (curCPU != CPU_Z180) {
                return 0;
            }
            switch ((reg1 = GetReg(Z80_regs))) {
                case reg_EOL:
                    break;

                case reg_B:
                case reg_C:
                case reg_D:
                case reg_E:
                case reg_H:
                case reg_L:
                case reg_A:
                    InstrX(parm + reg1*8);
                    break;

                case reg_Paren:
                    switch ((reg1 = GetReg("HL C"))) {
                        case reg_EOL:
                            break;

                        case 0: // TST (HL)
                            if (RParen()) break;
                            InstrX(parm + reg_M*8);
                            break;

                        case 1: // TST (C),b
                            if (RParen()) break;
                            if (Comma()) break;
                            val = Eval();
                            InstrXB(0xED64, val);
                            break;

                        default:
                            IllegalOperand();
                            break;
                    }
                    break;

                default:
                    IllegalOperand();
                    break;
            }
            break;

        case o_MLT:
            if (curCPU != CPU_Z180) {
                return 0;
            }
            switch ((reg1 = GetReg(Z80_regs))) {
                case reg_EOL:
                    break;

                case reg_BC:
                case reg_DE:
                case reg_HL:
                case reg_SP:
                    InstrX(parm + (reg1-reg_BC)*16);
                    break;

                default:
                    IllegalOperand();
                    break;
            }
            break;

        case o_TSTIO:
            if (curCPU != CPU_Z180) {
                return 0;
            }
            val = Eval();
            InstrXB(parm, val);
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

    switch (typ) {
        // o_Bit needs to be implemented like a pseudo-op so that
        //        SET can fall back to the standard SET pseudo-op
        case o_Bit:
            oldLine = linePtr;
            reg1 = Eval();                  // get bit number
            token = GetWord(word);          // attempt to get comma
            // if end of line and SET opcode
            if (token == 0 && parm == 0xC0) {
                linePtr = oldLine;
                if (!errFlag || pass < 2) { // don't double-error on second pass
                    DoLabelOp(o_EQU, 1, labl);  // fall back to SET pseudo-op
                }
            } else {
                DefSym(labl, locPtr, false, false); // define label if present
                showAddr = true;
                if (token != ',') {         // validate that comma is present
                    Error("\",\" expected");
                    break;
                }
                switch ((reg2 = GetReg(Z80_regs))) {
                    case reg_EOL:
                        break;

                    case reg_B:
                    case reg_C:
                    case reg_D:
                    case reg_E:
                    case reg_H:
                    case reg_L:
                    case reg_A:         // BIT n,r
                        InstrX(0xCB00 + parm + reg1*8 + reg2);
                        break;

                    case reg_Paren:     // BIT n,(HL)
                        switch ((reg2 = GetReg(Z80_regs))) {
                            case reg_EOL:
                                break;

                            case reg_HL:
                                if (RParen()) break;
                                InstrX(0xCB00 + parm + reg1*8 + reg_M);
                                break;

                            case reg_IX:
                            case reg_IY:
                                if (curCPU == CPU_GBZ80) {
                                    IllegalOperand();
                                    break;
                                }
                                val = IXOffset();
                                InstrXBB(DDFD(reg2) + 0xCB, val, parm + reg1*8 + reg_M);
                                break;

                            default:
                                IllegalOperand();
                                break;
                        }
                        break;

                    default:            // invalid or unknown register
                        IllegalOperand();
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


void AsmZ80Init(void)
{
    void *p = AddAsm(versionName, &Z80_DoCPUOpcode, &Z80_DoCPULabelOp, NULL);

    enum { // standard CPU options
        OPT = (OPT_ATSYM | OPT_DOLLARSYM),
    };

    AddCPU(p, "Z80",   CPU_Z80,   LITTLE_END, ADDR_16, LIST_24, 8, OPT, Z80_opcdTab);
    AddCPU(p, "Z180",  CPU_Z180,  LITTLE_END, ADDR_16, LIST_24, 8, OPT, Z80_opcdTab);
    AddCPU(p, "GBZ80", CPU_GBZ80, LITTLE_END, ADDR_16, LIST_24, 8, OPT, Z80_opcdTab);
}
