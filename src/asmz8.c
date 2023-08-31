// asmz8.c

#define versionName "Z8 assembler"
#include "asmx.h"

#if 1
#include <stdio.h>
extern FILE *listing; // listing output file
#endif

enum instrType
{
    o_None,     // no operands
    o_LD,       // LD - load a register
    o_JR,       // JR - relative jump
    o_JP,       // JP - absolute or register jump
    o_DJNZ,     // DJNZ - decrement and jump if not zero
    o_CALL,     // CALL - call a subroutine
    o_OneB,     // x0 x1 - single byte operand
    o_OneW,     // x0 x1 - single word operand, must be even address
    o_Two,      // x2..x6 - two operand
    o_LDE_LDC,  // LDE and LDC instructions
    o_LDEI_LDCI,// LDEI and LDCI instructions
    o_SRP,      // SRP instruction

    o_RP = o_LabelOp, // RP pseudo-op
};

struct OpcdRec Z8_opcdTab[] =
{
    {"WDH", o_None, 0x4F},
    {"WDT", o_None, 0x5F},
    {"STOP",o_None, 0x6F},
    {"HALT",o_None, 0x7F},
    {"DI",  o_None, 0x8F},
    {"EI",  o_None, 0x9F},
    {"RET", o_None, 0xAF},
    {"IRET",o_None, 0xBF},
    {"RCF", o_None, 0xCF},
    {"SCF", o_None, 0xDF},
    {"CCF", o_None, 0xEF},
    {"NOP", o_None, 0xFF},

    {"LD",  o_LD,   0x00},
    {"JR",  o_JR,   0x00},
    {"JP",  o_JP,   0x00},
    {"DJNZ",o_DJNZ, 0x00},
    {"CALL",o_CALL, 0x00},

    {"DEC", o_OneB, 0x00},
    {"RLC", o_OneB, 0x10},
    {"INC", o_OneB, 0x20}, // this also has a unique INC Rr form
    {"DA",  o_OneB, 0x40},
    {"POP", o_OneB, 0x50},
    {"COM", o_OneB, 0x60},
    {"PUSH",o_OneB, 0x70},
    {"DECW",o_OneW, 0x80},
    {"RL",  o_OneB, 0x90},
    {"INCW",o_OneW, 0xA0},
    {"CLR", o_OneB, 0xB0},
    {"RRC", o_OneB, 0xC0},
    {"SRA", o_OneB, 0xD0},
    {"RR",  o_OneB, 0xE0},
    {"SWAP",o_OneB, 0xF0},

    {"ADD", o_Two, 0x00},
    {"ADC", o_Two, 0x10},
    {"SUB", o_Two, 0x20},
    {"SBC", o_Two, 0x30},
    {"OR",  o_Two, 0x40},
    {"AND", o_Two, 0x50},
    {"TCM", o_Two, 0x60},
    {"TM",  o_Two, 0x70},
    {"CP",  o_Two, 0xA0},
    {"XOR", o_Two, 0xB0},

    {"LDE", o_LDE_LDC,  0x82}, // 82 LDE  r,@rr   92 LDE  @rr,r
    {"LDC", o_LDE_LDC,  0xC2}, // C2 LDC  r,@rr   D2 LDC  @rr,r
    {"LDEI",o_LDEI_LDCI,0x83}, // 83 LDEI @r,@rr  93 LDEI @rr,@r
    {"LDCI",o_LDEI_LDCI,0xC3}, // C3 LDCI @r,@rr  D3 LDCI @rr,@r

    {"SRP", o_SRP, 0x00},

    {"RP",  o_RP,  0x00},

    {"",    o_Illegal,  0}
};


int rpReg; // current RP register set pointer

// --------------------------------------------------------------


enum { // registers returned by Get_Z8_Reg()
    // reg_EOL and reg_None are also returned by Get_Z8_reg

    // regular registers
    reg_R0 = 0, reg_R1, reg_R2, reg_R3, reg_R4, reg_R5, reg_R6, reg_R7,
    reg_R8, reg_R9, reg_R10, reg_R11, reg_R12, reg_R13, reg_R14, reg_R15,

    // register pairs (only even registers are valid)
    reg_RR0, reg_RR1, reg_RR2, reg_RR3, reg_RR4, reg_RR5, reg_RR6, reg_RR7,
    reg_RR8, reg_RR9, reg_RR10, reg_RR11, reg_RR12, reg_RR13, reg_RR14, reg_RR15,

    // indirect registers
    reg_IR0, reg_IR1, reg_IR2, reg_IR3, reg_IR4, reg_IR5, reg_IR6, reg_IR7,
    reg_IR8, reg_IR9, reg_IR10, reg_IR11, reg_IR12, reg_IR13, reg_IR14, reg_IR15,

    // indirect register pairs (only even registers are valid)
    reg_IRR0, reg_IRR1, reg_IRR2, reg_IRR3, reg_IRR4, reg_IRR5, reg_IRR6, reg_IRR7,
    reg_IRR8, reg_IRR9, reg_IRR10, reg_IRR11, reg_IRR12, reg_IRR13, reg_IRR14, reg_IRR15,

    // other stuff
    reg_Imm,    // '#' found for immediate mode
    reg_Ind,    // '@' found without a register name
};

enum { // types returned by RegType()
    // reg_EOL and reg_None are also returned by RegType()
    type_Unk,   // unknown
    type_Reg,   // R0 - R15
    type_RReg,  // RR0 - RR14
    type_Ir,    // @R0 - @R15
    type_Irr,   // @RR0 - @RR14
    type_Ind,   // @
    type_Imm,   // #
};


// Gets a Z8 register name and returns it as a token
// Indirect and immediate have the line pointer ready to call Eval()
static int Get_Z8_Reg()
{
    char    *oldLine;

    oldLine = linePtr;

    int reg = GetReg("R0 R1 R2 R3 R4 R5 R6 R7 R8 R9 R10 R11 R12 R13 R14 R15 "
                     "@ # RR0 RR2 RR4 RR6 RR8 RR10 RR12 RR14");
    switch (reg) {
        case reg_EOL:
        case reg_None:
            linePtr = oldLine;
            return reg;

        default: // Rn RRn
            if (reg > 16) {
                // RRn
                return reg - 18 + reg_RR0;
            }
            // Rn
            return reg + reg_R0;

        case 17: // #
            return reg_Imm;

        case 16: // @
            reg = GetReg("R0 R1 R2 R3 R4 R5 R6 R7 R8 R9 R10 R11 R12 R13 R14 R15 "
                         "RR0 RR2 RR4 RR6 RR8 RR10 RR12 RR14");
            if (reg < 0) {
                // unknown or end of line
                return reg_Ind;
            }
            if (reg < 16) {
                // @Rn
                return reg + reg_IR0;
            }
            // @RRn
            return (reg - 16)*2 + reg_IRR0;
    }
}


// returns the type of a register token
static int RegType(int reg)
{
    // reg_None and reg_EOL
    // reg_None is generally followed by a register address expression
    if (reg < 0) {
        return reg;
    }
    // RP byte registers
    if (reg_R0 <= reg && reg <= reg_R15) {
        return type_Reg;
    }
    // RP word registers
    if (reg_RR0 <= reg && reg <= reg_RR15) {
        return type_RReg;
    }
    // Indirect RP byte registers
    if (reg_IR0 <= reg && reg <= reg_IR15) {
        return type_Ir;
    }
    // Indirect RP word registers
    if (reg_IRR0 <= reg && reg <= reg_IRR15) {
        return type_Irr;
    }
    // Immediate value
    if (reg == reg_Imm) {
        return type_Imm;
    }
    // Indirect register
    if (reg == reg_Ind) {
        return type_Ind;
    }
    return type_Unk;
}


// returns the register number of a register token
static int RegNum(int reg)
{
    if (reg_R0 <= reg && reg <= reg_R15) {
        return reg - reg_R0;
    }
    if (reg_RR0 <= reg && reg <= reg_RR15) {
        return reg - reg_RR0;
    }
    if (reg_IR0 <= reg && reg <= reg_IR15) {
        return reg - reg_IR0;
    }
    if (reg_IRR0 <= reg && reg <= reg_IRR15) {
        return reg - reg_IRR0;
    }
    return reg; // pass reg_None and reg_EOL through
}


// checks if a register address is in the current RP range
// note that this depends on evalKnown and must be done immediately after Eval()
static bool IsRP(int reg)
{
    return evalKnown && rpReg >= 0 && (reg & 0xF0) == rpReg;
}


// checks that a register number is valid as an operand
static void CheckZ8Reg(int reg)
{
    // register number must be in 0x00 - 0xFF range
    if (reg < 0 || reg > 0xFF ||
    // register number must not be in 0xE0 - 0xEF range
        (0xE0 <= reg && reg <= 0xEF)) {
        // could also check 80-DF range on 124-byte RAM cores
        Error("Invalid register");
    }
}


// get the condition code for a JP or JR instruction
static int GetZ8Cond()
{
    int reg = GetReg("F LT LE ULE OV MI EQ C T GE GT UGT NOV PL NZ NC ULT UGE");
    switch (reg) {
        case 16: reg =  7; break; // ULT -> C
        case 17: reg = 15; break; // UGE -> NC
    }

    if (reg >= 0) {
        Comma();
    } else {
        reg = 8; // no condition => T
    }
    
    return reg;
}


int Z8_DoCPUOpcode(int typ, int parm)
{
    int     val;
    int     reg1;
    int     reg2;
    bool    isRP1;
    Str255  word;
    char    *oldLine;
//  int     token;

    switch (typ) {
        case o_None:
            InstrB(parm);
            break;

        case o_LD:
            // LD - load a register
            reg1 = Get_Z8_Reg();
            switch (RegType(reg1)) {
                case type_Reg:
                    // LD Rr,
                    Comma();
                    reg2 = Get_Z8_Reg();
                    switch (RegType(reg2)) {
                        case type_Imm:
                            // LD Rr,#imm => rC ii
                            val = Eval();
                            InstrBB(RegNum(reg1)*16 + 0x0C, val);
                            break;

                        case type_Reg:
                            // LD Rr,Rr => r8 Er
                            InstrBB(RegNum(reg1)*16 + 0x08, 0xE0 + (reg2 & 0x0F));
                            break;

                        case type_Ir:
                            // LD Rr,@Rr => E3 ds
                            InstrBB(0xE3, RegNum(reg1)*16 + RegNum(reg2));
                            break;

                        case reg_None:
                            // LD Rr,reg
                            reg2 = Eval();
                            // can't use GetReg because it will complain about EOL
                            oldLine = linePtr;
                            if (GetWord(word) == '(') {
                                // LD Rr,ofs(Rr)
                                val = reg2;
                                reg2 = Get_Z8_Reg();
                                switch (RegType(reg2)) {
                                    case type_Reg:
                                        // LD Rr,ofs(Rr) => C7 ds oo
                                        Expect(")");
                                        InstrBBB(0xC7, RegNum(reg1)*16 + RegNum(reg2), val);
                                        break;

                                    case reg_EOL:
                                        break;

                                    default:
                                        IllegalOperand();
                                        break;
                                }
                            } else {
                                linePtr = oldLine;
                                if (IsRP(reg2)) {
                                    // LD Rr,Rr => r8 Er
                                    InstrBB(RegNum(reg1)*16 + 0x08, 0xE0 + (reg2 & 0x0F));
                                } else {
                                    // LD Rr,reg => r8 rr
                                    CheckZ8Reg(reg2);
                                    InstrBB(RegNum(reg1)*16 + 0x08, reg2);
                                }
                            }
                            break;

                        case type_Ind:
                            // LD Rr,@reg
                            reg2 = Eval();
                            CheckZ8Reg(reg2);
                            if (IsRP(reg2)) {
                                // LD Rr,@Rs => E3 ds
                                InstrBB(0xE3, RegNum(reg1)*16 + (reg2 & 0x0F));
                            } else {
                                // LD dst,@src => E5 ss dd
                                InstrBBB(0xE5, reg2, 0xE0 + RegNum(reg1));
                            }
                            break;

                        case reg_EOL:
                            break;

                        default:
                            IllegalOperand();
                            break;
                    }
                    break;

                case type_Ir:
                    // LD @Rr,
                    Comma();
                    reg2 = Get_Z8_Reg();
                    switch (RegType(reg2)) {
                        case type_Imm:
                            // LD @Rr,#imm => E7 Er ii
                            val = Eval();
                            InstrBBB(0xE7, 0xE0 + RegNum(reg1), val);
                            break;

                        case reg_None:
                            // LD @Rr,reg => F5 ss Er
                            reg2 = Eval();
                            CheckZ8Reg(reg2);
                            InstrBBB(0xF5, reg2, 0xE0 + RegNum(reg1));
                            break;

                        case type_Reg:
                            // LD @Rr,Rr => F3 ds
                            InstrBB(0xF3, RegNum(reg1)*16 + RegNum(reg2));
                            break;

                        case reg_EOL:
                            break;

                        default:
                            IllegalOperand();
                            break;
                    }
                    break;

                case reg_None:
                    // LD reg,
                    reg1 = Eval();
                    if (GetReg("(") == 0) {
                        // LD ofs(
                        val = reg1;
                        reg1 = Get_Z8_Reg();
                        switch (RegType(reg1)) {
                            case type_Reg:
                                // LD ofs(Rr),
                                Expect(")");
                                Comma();
                                reg2 = Get_Z8_Reg();
                                switch (RegType(reg2)) {
                                    case type_Reg:
                                        // LD ofs(Rr),Rr => D7 sd oo
                                        InstrBBB(0xD7, RegNum(reg2)*16 + RegNum(reg1), val);
                                        break;

                                    case reg_EOL:
                                        break;

                                    default:
                                        IllegalOperand();
                                        break;
                                }
                                break;

                            case reg_EOL:
                                break;

                            default:
                                IllegalOperand();
                                break;
                        }
                    } else {
                        isRP1 = IsRP(reg1);
                        Comma();
                        reg2 = Get_Z8_Reg();
                        switch (RegType(reg2)) {
                            case type_Reg:
                                // LD reg,Rr => x9 rr
                                // Note that in this form, registers Ex are allowed!
                                if (reg1 < 0 || reg1 > 255) {
                                    Error("Invalid register");
                                }
                                InstrBB(RegNum(reg2)*16 + 0x09, reg1);
                                break;

                            case reg_None:
                                // LD dst,src
                                reg2 = Eval();
#if 0
                                CheckZ8Reg(reg1);
#else
                                // It is possible that this form also allows registers Ex
                                if (reg1 < 0 || reg1 > 255) {
                                    Error("Invalid register");
                                }
#endif
                                CheckZ8Reg(reg2);
                                if (isRP1) {
                                    // LD Rr,Rr => r8 rr
                                    InstrBB(RegNum(reg1)*16 + 0x08, reg2);
                                } else if (IsRP(reg2)) {
                                    // LD reg,Rr => x9 rr
                                    InstrBB(reg2*16 + 0x09, reg1);
                                } else {
                                    // LD dst,src => E4 ss dd
                                    InstrBBB(0xE4, reg2, reg1);
                                }
                                break;

                            case type_Ir:
                                // LD dst,@Rs
                                CheckZ8Reg(reg1);
                                if (isRP1 ) {
                                    // LD Rd,@Rs => E3 ds
                                    InstrBB(0xE3, (reg1 & 0x0F)*16 + RegNum(reg2));
                                } else {
                                    // LD dst,@Rs => E5 Es dd
                                    InstrBBB(0xE5, 0xE0 + RegNum(reg2), reg1);
                                }
                                break;

                            case type_Ind:
                                // LD dst,@src
                                reg2 = Eval();
                                CheckZ8Reg(reg1);
                                CheckZ8Reg(reg2);
                                if (isRP1 && IsRP(reg2)) {
                                    // LD Rr,@Rr => E3 ds
                                    InstrBB(0xE3, (reg1 & 0x0F)*16 + (reg2 & 0x0F));
                                } else {
                                    // LD dst,@src => E5 ss dd
                                    InstrBBB(0xE5, reg2, reg1);
                                }
                                break;

                            case type_Imm:
                                // LD dst,#imm
                                val = Eval();
                                if (isRP1) {
                                    // LD Rr,#imm => rC ii
                                    InstrBB((reg1 & 0x0F)*16 + 0x0C, val);
                                } else {
                                    // LD dst,#imm => E6 rr ii
                                    InstrBBB(0xE6, reg1, val);
                                }
                                break;

                            case reg_EOL:
                                break;

                            default:
                                IllegalOperand();
                                break;
                        }
                    }
                    break;

                case type_Ind:
                    // LD @reg,
                    reg1 = Eval();
                    Comma();
                    reg2 = Get_Z8_Reg();
                    switch (RegType(reg2)) {
                        case type_Imm:
                            // LD @reg,#imm => E7 rr ii
                            val = Eval();
                            InstrBBB(0xE7, reg1, val);
                            break;

                        case type_Reg:
                            // LD @reg,Rs
                            if (IsRP(reg1)) {
                                // LD @Rd,Rs => F3 ds
                                InstrBB(0xF3, (reg1 & 0x0F)*16 + (reg2 & 0x0F));
                            } else {
                                // LD @reg,Rs => F5 Es dd
                                InstrBBB(0xF5, reg2 | 0xE0, reg1);
                            }
                            break;

                        case reg_None:
                            // LD @reg,reg
                            isRP1 = IsRP(reg1);
                            reg2 = Eval();
                            CheckZ8Reg(reg2);
                            if (isRP1 && IsRP(reg2)) {
                                // LD @Rr,Rr => F3 ds
                                InstrBB(0xF3, (reg1 & 0x0F)*16 + (reg2 & 0x0F));
                            } else {
                                // LD @reg,Rs => F5 ss dd
                                InstrBBB(0xF5, reg2, reg1);
                            }
                            break;

                        case reg_EOL:
                            break;

                        default:
                            IllegalOperand();
                            break;
                    }
                    break;

                case reg_EOL:
                    break;

                default:
                    IllegalOperand();
                    break;
            }
            break;

        case o_JR:
            // JR - relative jump
            // JR [C,]rel => cB dd
            reg1 = GetZ8Cond();
            val = EvalBranch(2);
            InstrBB(reg1*16 + 0x0B, val);
            break;

        case o_JP:
            // JP - absolute or register jump
            reg1 = GetZ8Cond();
            reg2 = Get_Z8_Reg();
            switch (RegType(reg2)) {
                case reg_None:
                    // JP C,aaaa => cD aaaa
                    val = Eval();
                    InstrBW(reg1*16 + 0x0D, val);
                    break;

                case type_Ir:
                    // JP @Rr => 30 Er
                    if ((RegNum(reg2) & 0x01) != 0) {
                        Error("Register must be even");
                    }
                    // fall through
                case type_Irr:
                    // JP @RRr => 30 Er
                    InstrBB(0x30, 0xE0 + RegNum(reg2));
                    break;

                case type_Ind:
                    // JP @rr => 30 rr
                    reg2 = Eval();
                    CheckZ8Reg(reg2);
                    if ((reg1 & 0x01) != 0) {
                        Error("Register must be even");
                    }
                    InstrBB(0x30, reg2);
                    break;

                case reg_EOL:
                    break;

                default:
                    IllegalOperand();
                    break;
            }
            break;

        case o_DJNZ:
            // DJNZ - decrement and jump if not zero
            reg1 = Get_Z8_Reg();
            switch (RegType(reg1)) {
                case type_Reg:
                    // DJNZ Rr,rel => rA dd
                    Comma();
                    val = EvalBranch(2);
                    InstrBB(RegNum(reg1)*16 + 0x0A, val);
                    break;

                case reg_EOL:
                    break;

                default:
                    IllegalOperand();
                    break;
            }
            break;

        case o_CALL:
            // CALL - call a subroutine
            reg1 = Get_Z8_Reg();
            switch (RegType(reg1)) {
                case type_Ir:
                    // @Rr => D4 Er
                    if ((RegNum(reg1) & 0x01) != 0) {
                        Error("Register must be even");
                    }
                    // fall through
                case type_Irr:
                    // @RRr => D4 Er
                    InstrBB(0xD4, 0xE0 + RegNum(reg1));
                    break;

                case type_Ind:
                    // CALL @reg => D4 rr
                    reg1 = Eval();
                    CheckZ8Reg(reg1);
                    if ((RegNum(reg1) & 0x01) != 0) {
                        Error("Register must be even");
                    }
                    InstrBB(0xD4, reg1);
                    break;

                case reg_None: // CALL aaaa => D6 aaaa
                    val = Eval();
                    InstrBW(0xD6, val);
                    break;

                case reg_EOL:
                    break;

                default:
                    IllegalOperand();
                    break;
            }
            break;

        case o_OneB:
            // x0 x1 - single byte operand
        case o_OneW:
            // x0 x1 - single word operand, must be even register address
            // OP Rr => x0 Er
            // OP reg => x0 rr
            // OP @reg => x1 @rr
            reg1 = Get_Z8_Reg();
            switch (RegType(reg1)) {
                case type_RReg:
                    // RRr
                    if (typ != o_OneW) {
                        IllegalOperand();
                    }
                    FALLTHROUGH;
                case type_Reg:
                    // Rr
                    if (typ == o_OneW && (RegNum(reg1) & 0x01) != 0) {
                        Error("Register must be even");
                    }
                    if (parm == 0x20) {
                        // special case for INC Rr => rE
                        InstrB(RegNum(reg1)*16 + 0x0E);
                        break;
                    }
                    // OP Rr => x0 Er
                    InstrBB(parm, 0xE0 + RegNum(reg1));
                    break;

                case type_Irr:
                    // @RRr
                    if (typ != o_OneW) {
                        IllegalOperand();
                    }
                    FALLTHROUGH;
                case type_Ir:
                    // @Rr
                    if (typ == o_OneW && (RegNum(reg1) & 0x01)) {
                        Error("Register must be even");
                    }
                    // OP @Rr => x1 Er
                    InstrBB(parm + 0x01, 0xE0 + RegNum(reg1));
                    break;

                case type_Ind:
                    // OP @reg => x1 rr
                    reg1 = Eval();
                    CheckZ8Reg(reg1);
                    if (typ == o_OneW && (reg1 & 0x01) != 0) {
                        Error("Register must be even");
                    }
                    InstrBB(parm + 0x01, reg1);
                    break;

                case reg_None:
                    // OP reg  => x0 rr
                    reg1 = Eval();
                    CheckZ8Reg(reg1);
                    if (typ == o_OneW && (reg1 & 0x01) != 0) {
                        Error("Register must be even");
                    }
                    InstrBB(parm + 0x00, reg1);
                    break;

                case reg_EOL:
                    break;

                default:
                    IllegalOperand();
                    break;
            }
            break;

        case o_Two:
            // x2..x6 - two operand
            // OP Rd,Rs => x2 ds
            // OP Rd,@Rs => x3 ds
            // OP dst,src => x4 src dst
            // OP dst,@src => x5 src dst
            // OP reg,#imm => x6 rr ii
            // OP @reg,#imm => x7 rr ii
            reg1 = Get_Z8_Reg();
            switch (RegType(reg1)) {
                case type_Reg:
                    // OP Rr,
                    Comma();
                    reg2 = Get_Z8_Reg();
                    switch (RegType(reg2)) {
                        case type_Reg:
                            // OP Rd,Rs => x2 ds
                            InstrBB(parm + 0x02, RegNum(reg1)*16 + RegNum(reg2));
                            break;

                        case type_Ir:
                            // OP Rd,@Rs => x3 ds
                            InstrBB(parm + 0x03, RegNum(reg1)*16 + RegNum(reg2));
                            break;

                        case reg_None:
                            // OP Rd,reg => x4 ss Ed
                            reg2 = Eval();
                            CheckZ8Reg(reg2);
                            InstrBBB(parm + 0x04, reg2, 0xE0 + RegNum(reg1));
                            break;

                        case type_Ind:
                            // OP Rd,@reg => x5 ss Ed
                            reg2 = Eval();
                            CheckZ8Reg(reg2);
                            InstrBBB(parm + 0x05, reg2, 0xE0 + RegNum(reg1));
                            break;

                        case type_Imm:
                            // OP Rd,#imm => 06 Ed ii
                            val = Eval();
                            InstrBBB(parm + 0x06, 0xE0 + RegNum(reg1), val);
                            break;

                        case reg_EOL:
                            break;

                        default:
                            IllegalOperand();
                            break;
                    }
                    break;

                case type_Ir:
                    // OP @Rr,#imm
                    Comma();
                    reg2 = Get_Z8_Reg();
                    switch (RegType(reg2)) {
                        case type_Imm:
                            // OP @Rr,#imm => 07 Er ii
                            val = Eval();
                            InstrBBB(parm + 0x07, 0xE0 + RegNum(reg1), val);
                            break;

                        case reg_EOL:
                            break;

                        default:
                            IllegalOperand();
                            break;
                    }
                    break;

                case reg_None:
                    // OP reg,
                    reg1 = Eval();
                    isRP1 = IsRP(reg1);
                    Comma();
                    CheckZ8Reg(reg1);
                    InstrBB(parm, reg1);
                    reg2 = Get_Z8_Reg();
                    switch (RegType(reg2)) {
                        case type_Reg:
                            // OP dd,Rs => 04 Es dd
                            InstrBBB(parm + 0x04, 0xE0 + RegNum(reg2), reg1);
                            break;

                        case reg_None:
                            // OP dd,ss => 04 ss dd
                            reg2 = Eval();
                            CheckZ8Reg(reg2);
                            // OP dd,Rs
                            if (isRP1 && IsRP(reg2)) {
                                // OP Rd,Rs => 02 ds
                                InstrBB(parm + 0x02, (reg1 & 0x0F)*16 + (reg2 & 0x0F));
                            } else {
                                InstrBBB(parm + 0x04, reg2, reg1);
                            }
                            break;

                        case type_Ir:
                            // OP dd,@Rs => 05 Es dd
                            InstrBBB(parm + 0x05, 0xE0 + RegNum(reg2), reg1);
                            break;

                        case type_Ind:
                            // OP dd,@ss
                            reg2 = Eval();
                            CheckZ8Reg(reg2);
                            if (isRP1 && IsRP(reg2)) {
                                // OP Rd,@Rs => 03 ds
                                InstrBB(parm + 0x03, (reg1 & 0x0F)*16 + (reg2 & 0x0F));
                            } else {
                                // OP dd,@ss => 05 ss dd
                                InstrBBB(parm + 0x05, reg2, reg1);
                            }
                            break;

                        case type_Imm:
                            // OP reg,#imm => 06 rr ii
                            CheckZ8Reg(reg1);
                            val = Eval();
                            InstrBBB(parm + 0x06, reg1, val);
                            break;

                        case reg_EOL:
                            break;

                        default:
                            IllegalOperand();
                            break;
                    }
                    break;

                case type_Ind:
                    // OP @reg,#imm
                    reg1 = Eval();
                    Comma();
                    reg2 = Get_Z8_Reg();
                    switch (RegType(reg2)) {
                        case type_Imm:
                            // OP @reg,#imm => 07 rr ii
                            CheckZ8Reg(reg1);
                            val = Eval();
                            InstrBBB(parm + 0x07, reg1, val);
                            break;

                        case reg_EOL:
                            break;

                        default:
                            IllegalOperand();
                            break;
                    }
                    break;

                case reg_EOL:
                    break;

                default:
                    IllegalOperand();
                    break;
            }
            break;

        case o_LDE_LDC:
            // LDE and LDC instructions
            reg1 = Get_Z8_Reg();
            switch (RegType(reg1)) {
                case type_Reg:
                    // LDE/LDC Rr,@RRr
                    Comma();
                    reg2 = Get_Z8_Reg();
                    switch (RegType(reg2)) {
                        case type_Irr:
                            // LDE/LDC Rr,@RRr => 82/C2 ds
                            InstrBB(parm + 0x00, RegNum(reg1)*16 + RegNum(reg2));
                            break;

                        case reg_EOL:
                            break;

                        default:
                            IllegalOperand();
                            break;
                    }
                    break;

                case type_Irr:
                    // LDE/LDC @RRr,Rr
                    Comma();
                    reg2 = Get_Z8_Reg();
                    switch (RegType(reg2)) {
                        case type_Reg:
                            // LDE/LDC @RRr,Rr => 92/D2 sd
                            // NOTE: some Zilog manuals show LDC as D2 ds
                            InstrBB(parm + 0x10, RegNum(reg2)*16 + RegNum(reg1));
                            break;

                        case reg_EOL:
                            break;

                        default:
                            IllegalOperand();
                            break;
                    }
                    break;

                case reg_EOL:
                    break;

                default:
                    IllegalOperand();
                    break;
            }
            break;

        case o_LDEI_LDCI:
            // LDEI and LDCI instructions
            reg1 = Get_Z8_Reg();
            switch (RegType(reg1)) {
                case type_Ir:
                    // LDEI/LDCI @Rr,@RRr
                    Comma();
                    reg2 = Get_Z8_Reg();
                    switch (RegType(reg2)) {
                        case type_Irr:
                            // LDEI/LDCI @Rr,@RRr => 83/C3 ds
                            InstrBB(parm + 0x00, RegNum(reg1)*16 + RegNum(reg2));
                            break;

                        case reg_EOL:
                            break;

                        default:
                            IllegalOperand();
                            break;
                    }
                    break;

                case type_Irr:
                    // LDEI/LDCI @RRr,@Rr
                    // NOTE: some Zilog manuals show LDCI as D3 ds
                    Comma();
                    reg2 = Get_Z8_Reg();
                    switch (RegType(reg2)) {
                        case type_Ir:
                            // LDEI/LDCI @RRr,@Rr => 93/D3 sd
                            InstrBB(parm + 0x10, RegNum(reg2)*16 + RegNum(reg1));
                            break;

                        case reg_EOL:
                            break;

                        default:
                            IllegalOperand();
                            break;
                    }
                    break;

                case reg_EOL:
                    break;

                default:
                    IllegalOperand();
                    break;
            }
            break;

        case o_SRP:
            // SRP instruction
            reg1 = Get_Z8_Reg();
            switch (RegType(reg1)) {
                case type_Imm:
                    // SRP #ii => 31 ii
                    val = Eval();
                    InstrBB(0x31, val);
                    break;

                case reg_EOL:
                    break;

                default:
                    IllegalOperand();
                    break;
            }
            break;

        default:
            return 0;
            break;
    }
    return 1;
}


int Z8_DoCPULabelOp(int typ, int parm, char *labl)
{
    (void) parm; // unused parameter

    switch (typ) {
        case o_RP:
            // RP pseudo-op
            // This sets the expected value of the register pointer to
            // optimize Rxx addressing modes to Rn, or you can turn it OFF

            if (labl[0]) {
                Error("Label not allowed");
            }

            if (GetReg("OFF") == 0) {
                // RP OFF to disable RP optimizations
                rpReg = -1;
                char *p = listLine + 2;
                p = ListStr(p, "--");
                break;
            }
            int val = Eval();
            if (!errFlag) {
                if ((val & 0xFF) == 0) {
                    val = val >> 8;     // handle $XX00 as $00XX
                }
                if (val < 0 || val > 255 || (val > 0x0F && (val & 0x0F) != 0)) {
                    Error("Operand out of range");
                } else {
                    // convert low-nibble form to high-nibble form
                    if (val < 0x0F) {
                        val = val*16;
                    }
                    rpReg = val;

                    if (pass == 2) {
                        char *p = listLine + 2;
                        p = ListByte(p, val);
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


void Z8_PassInit(void)
{
    // start each pass with RP optimizations disabled
    rpReg = -1;
}


void AsmZ8Init(void)
{
    void *p = AddAsm(versionName, &Z8_DoCPUOpcode, &Z8_DoCPULabelOp, &Z8_PassInit);

    AddCPU(p, "Z8", 0, BIG_END, ADDR_16, LIST_24, 8, 0, Z8_opcdTab);
}
