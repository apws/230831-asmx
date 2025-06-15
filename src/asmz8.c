// asmZ8.c

#define versionName "Z8 assembler"
#include "asmx.h"

#if 1
#include <stdio.h>
extern FILE *listing; // listing output file
#endif

enum instrType
{
    OP_None,     // no operands
    OP_LD,       // LD - load a register
    OP_JR,       // JR - relative jump
    OP_JP,       // JP - absolute or register jump
    OP_DJNZ,     // DJNZ - decrement and jump if not zero
    OP_CALL,     // CALL - call a subroutine
    OP_OneB,     // x0 x1 - single byte operand
    OP_OneW,     // x0 x1 - single word operand, must be even address
    OP_Two,      // x2..x6 - two operand
    OP_LDE_LDC,  // LDE and LDC instructions
    OP_LDEI_LDCI,// LDEI and LDCI instructions
    OP_SRP,      // SRP instruction

    OP_RP = OP_LabelOp, // RP pseudo-op
};

struct OpcdRec Z8_opcdTab[] =
{
    {"WDH", OP_None, 0x4F},
    {"WDT", OP_None, 0x5F},
    {"STOP",OP_None, 0x6F},
    {"HALT",OP_None, 0x7F},
    {"DI",  OP_None, 0x8F},
    {"EI",  OP_None, 0x9F},
    {"RET", OP_None, 0xAF},
    {"IRET",OP_None, 0xBF},
    {"RCF", OP_None, 0xCF},
    {"SCF", OP_None, 0xDF},
    {"CCF", OP_None, 0xEF},
    {"NOP", OP_None, 0xFF},

    {"LD",  OP_LD,   0x00},
    {"JR",  OP_JR,   0x00},
    {"JP",  OP_JP,   0x00},
    {"DJNZ",OP_DJNZ, 0x00},
    {"CALL",OP_CALL, 0x00},

    {"DEC", OP_OneB, 0x00},
    {"RLC", OP_OneB, 0x10},
    {"INC", OP_OneB, 0x20}, // this also has a unique INC Rr form
    {"DA",  OP_OneB, 0x40},
    {"POP", OP_OneB, 0x50},
    {"COM", OP_OneB, 0x60},
    {"PUSH",OP_OneB, 0x70},
    {"DECW",OP_OneW, 0x80},
    {"RL",  OP_OneB, 0x90},
    {"INCW",OP_OneW, 0xA0},
    {"CLR", OP_OneB, 0xB0},
    {"RRC", OP_OneB, 0xC0},
    {"SRA", OP_OneB, 0xD0},
    {"RR",  OP_OneB, 0xE0},
    {"SWAP",OP_OneB, 0xF0},

    {"ADD", OP_Two, 0x00},
    {"ADC", OP_Two, 0x10},
    {"SUB", OP_Two, 0x20},
    {"SBC", OP_Two, 0x30},
    {"OR",  OP_Two, 0x40},
    {"AND", OP_Two, 0x50},
    {"TCM", OP_Two, 0x60},
    {"TM",  OP_Two, 0x70},
    {"CP",  OP_Two, 0xA0},
    {"XOR", OP_Two, 0xB0},

    {"LDE", OP_LDE_LDC,  0x82}, // 82 LDE  r,@rr   92 LDE  @rr,r
    {"LDC", OP_LDE_LDC,  0xC2}, // C2 LDC  r,@rr   D2 LDC  @rr,r
    {"LDEI",OP_LDEI_LDCI,0x83}, // 83 LDEI @r,@rr  93 LDEI @rr,@r
    {"LDCI",OP_LDEI_LDCI,0xC3}, // C3 LDCI @r,@rr  D3 LDCI @rr,@r

    {"SRP", OP_SRP, 0x00},

    {"RP",  OP_RP,  0x00},

    {"",    OP_Illegal,  0}
};


int rpReg; // current RP register set pointer

// --------------------------------------------------------------


enum   // registers returned by Z8_GetReg()
{
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

enum   // types returned by Z8_RegType()
{
    // reg_EOL and reg_None are also returned by Z8_RegType()
    RTYP_Unk,   // unknown
    RTYP_Reg,   // R0 - R15
    RTYP_RReg,  // RR0 - RR14
    RTYP_Ir,    // @R0 - @R15
    RTYP_Irr,   // @RR0 - @RR14
    RTYP_Ind,   // @
    RTYP_Imm,   // #
};


// Gets a Z8 register name and returns it as a token
// Indirect and immediate have the line pointer ready to call Eval()
static int Z8_GetReg()
{
    char    *oldLine;

    oldLine = linePtr;

    int reg = REG_Get("R0 R1 R2 R3 R4 R5 R6 R7 R8 R9 R10 R11 R12 R13 R14 R15 "
                     "@ # RR0 RR2 RR4 RR6 RR8 RR10 RR12 RR14");
    switch (reg)
    {
        case reg_EOL:
        case reg_None:
            linePtr = oldLine;
            return reg;

        default: // Rn RRn
            if (reg > 16)
            {
                // RRn
                return reg - 18 + reg_RR0;
            }
            // Rn
            return reg + reg_R0;

        case 17: // #
            return reg_Imm;

        case 16: // @
            reg = REG_Get("R0 R1 R2 R3 R4 R5 R6 R7 R8 R9 R10 R11 R12 R13 R14 R15 "
                         "RR0 RR2 RR4 RR6 RR8 RR10 RR12 RR14");
            if (reg < 0)
            {
                // unknown or end of line
                return reg_Ind;
            }
            if (reg < 16)
            {
                // @Rn
                return reg + reg_IR0;
            }
            // @RRn
            return (reg - 16)*2 + reg_IRR0;
    }
}


// returns the type of a register token
static int Z8_RegType(int reg)
{
    // reg_None and reg_EOL
    // reg_None is generally followed by a register address expression
    if (reg < 0)
    {
        return reg;
    }
    // RP byte registers
    if (reg_R0 <= reg && reg <= reg_R15)
    {
        return RTYP_Reg;
    }
    // RP word registers
    if (reg_RR0 <= reg && reg <= reg_RR15)
    {
        return RTYP_RReg;
    }
    // Indirect RP byte registers
    if (reg_IR0 <= reg && reg <= reg_IR15)
    {
        return RTYP_Ir;
    }
    // Indirect RP word registers
    if (reg_IRR0 <= reg && reg <= reg_IRR15)
    {
        return RTYP_Irr;
    }
    // Immediate value
    if (reg == reg_Imm)
    {
        return RTYP_Imm;
    }
    // Indirect register
    if (reg == reg_Ind)
    {
        return RTYP_Ind;
    }
    return RTYP_Unk;
}


// returns the register number of a register token
static int Z8_RegNum(int reg)
{
    if (reg_R0 <= reg && reg <= reg_R15)
    {
        return reg - reg_R0;
    }
    if (reg_RR0 <= reg && reg <= reg_RR15)
    {
        return reg - reg_RR0;
    }
    if (reg_IR0 <= reg && reg <= reg_IR15)
    {
        return reg - reg_IR0;
    }
    if (reg_IRR0 <= reg && reg <= reg_IRR15)
    {
        return reg - reg_IRR0;
    }
    return reg; // pass reg_None and reg_EOL through
}


// checks if a register address is in the current RP range
// note that this depends on evalKnown and must be done immediately after Eval()
static bool Z8_IsRP(int reg)
{
    return evalKnown && rpReg >= 0 && (reg & 0xF0) == rpReg;
}


// checks that a register number is valid as an operand
static void Z8_CheckReg(int reg)
{
    // register number must be in 0x00 - 0xFF range
    if (reg < 0 || reg > 0xFF ||
            // register number must not be in 0xE0 - 0xEF range
            (0xE0 <= reg && reg <= 0xEF))
    {
        // could also check 80-DF range on 124-byte RAM cores
        ASMX_Error("Invalid register");
    }
}


// get the condition code for a JP or JR instruction
static int Z8_GetCond()
{
    int reg = REG_Get("F LT LE ULE OV MI EQ C T GE GT UGT NOV PL NZ NC ULT UGE");
    switch (reg)
    {
        case 16:
            reg =  7;
            break; // ULT -> C
        case 17:
            reg = 15;
            break; // UGE -> NC
    }

    if (reg >= 0)
    {
        TOKEN_Comma();
    }
    else
    {
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

    switch (typ)
    {
        case OP_None:
            INSTR_B(parm);
            break;

        case OP_LD:
            // LD - load a register
            reg1 = Z8_GetReg();
            switch (Z8_RegType(reg1))
            {
                case RTYP_Reg:
                    // LD Rr,
                    TOKEN_Comma();
                    reg2 = Z8_GetReg();
                    switch (Z8_RegType(reg2))
                    {
                        case RTYP_Imm:
                            // LD Rr,#imm => rC ii
                            val = EXPR_Eval();
                            INSTR_BB(Z8_RegNum(reg1)*16 + 0x0C, val);
                            break;

                        case RTYP_Reg:
                            // LD Rr,Rr => r8 Er
                            INSTR_BB(Z8_RegNum(reg1)*16 + 0x08, 0xE0 + (reg2 & 0x0F));
                            break;

                        case RTYP_Ir:
                            // LD Rr,@Rr => E3 ds
                            INSTR_BB(0xE3, Z8_RegNum(reg1)*16 + Z8_RegNum(reg2));
                            break;

                        case reg_None:
                            // LD Rr,reg
                            reg2 = EXPR_Eval();
                            // can't use GetReg because it will complain about EOL
                            oldLine = linePtr;
                            if (TOKEN_GetWord(word) == '(')
                            {
                                // LD Rr,ofs(Rr)
                                val = reg2;
                                reg2 = Z8_GetReg();
                                switch (Z8_RegType(reg2))
                                {
                                    case RTYP_Reg:
                                        // LD Rr,ofs(Rr) => C7 ds oo
                                        TOKEN_Expect(")");
                                        INSTR_BBB(0xC7, Z8_RegNum(reg1)*16 + Z8_RegNum(reg2), val);
                                        break;

                                    case reg_EOL:
                                        break;

                                    default:
                                        ASMX_IllegalOperand();
                                        break;
                                }
                            }
                            else
                            {
                                linePtr = oldLine;
                                if (Z8_IsRP(reg2))
                                {
                                    // LD Rr,Rr => r8 Er
                                    INSTR_BB(Z8_RegNum(reg1)*16 + 0x08, 0xE0 + (reg2 & 0x0F));
                                }
                                else
                                {
                                    // LD Rr,reg => r8 rr
                                    Z8_CheckReg(reg2);
                                    INSTR_BB(Z8_RegNum(reg1)*16 + 0x08, reg2);
                                }
                            }
                            break;

                        case RTYP_Ind:
                            // LD Rr,@reg
                            reg2 = EXPR_Eval();
                            Z8_CheckReg(reg2);
                            if (Z8_IsRP(reg2))
                            {
                                // LD Rr,@Rs => E3 ds
                                INSTR_BB(0xE3, Z8_RegNum(reg1)*16 + (reg2 & 0x0F));
                            }
                            else
                            {
                                // LD dst,@src => E5 ss dd
                                INSTR_BBB(0xE5, reg2, 0xE0 + Z8_RegNum(reg1));
                            }
                            break;

                        case reg_EOL:
                            break;

                        default:
                            ASMX_IllegalOperand();
                            break;
                    }
                    break;

                case RTYP_Ir:
                    // LD @Rr,
                    TOKEN_Comma();
                    reg2 = Z8_GetReg();
                    switch (Z8_RegType(reg2))
                    {
                        case RTYP_Imm:
                            // LD @Rr,#imm => E7 Er ii
                            val = EXPR_Eval();
                            INSTR_BBB(0xE7, 0xE0 + Z8_RegNum(reg1), val);
                            break;

                        case reg_None:
                            // LD @Rr,reg => F5 ss Er
                            reg2 = EXPR_Eval();
                            Z8_CheckReg(reg2);
                            INSTR_BBB(0xF5, reg2, 0xE0 + Z8_RegNum(reg1));
                            break;

                        case RTYP_Reg:
                            // LD @Rr,Rr => F3 ds
                            INSTR_BB(0xF3, Z8_RegNum(reg1)*16 + Z8_RegNum(reg2));
                            break;

                        case reg_EOL:
                            break;

                        default:
                            ASMX_IllegalOperand();
                            break;
                    }
                    break;

                case reg_None:
                    // LD reg,
                    reg1 = EXPR_Eval();
                    if (REG_Get("(") == 0)
                    {
                        // LD ofs(
                        val = reg1;
                        reg1 = Z8_GetReg();
                        switch (Z8_RegType(reg1))
                        {
                            case RTYP_Reg:
                                // LD ofs(Rr),
                                TOKEN_Expect(")");
                                TOKEN_Comma();
                                reg2 = Z8_GetReg();
                                switch (Z8_RegType(reg2))
                                {
                                    case RTYP_Reg:
                                        // LD ofs(Rr),Rr => D7 sd oo
                                        INSTR_BBB(0xD7, Z8_RegNum(reg2)*16 + Z8_RegNum(reg1), val);
                                        break;

                                    case reg_EOL:
                                        break;

                                    default:
                                        ASMX_IllegalOperand();
                                        break;
                                }
                                break;

                            case reg_EOL:
                                break;

                            default:
                                ASMX_IllegalOperand();
                                break;
                        }
                    }
                    else
                    {
                        isRP1 = Z8_IsRP(reg1);
                        TOKEN_Comma();
                        reg2 = Z8_GetReg();
                        switch (Z8_RegType(reg2))
                        {
                            case RTYP_Reg:
                                // LD reg,Rr => x9 rr
                                // Note that in this form, registers Ex are allowed!
                                if (reg1 < 0 || reg1 > 255)
                                {
                                    ASMX_Error("Invalid register");
                                }
                                INSTR_BB(Z8_RegNum(reg2)*16 + 0x09, reg1);
                                break;

                            case reg_None:
                                // LD dst,src
                                reg2 = EXPR_Eval();
#if 0
                                Z8_CheckReg(reg1);
#else
                                // It is possible that this form also allows registers Ex
                                if (reg1 < 0 || reg1 > 255)
                                {
                                    ASMX_Error("Invalid register");
                                }
#endif
                                Z8_CheckReg(reg2);
                                if (isRP1)
                                {
                                    // LD Rr,Rr => r8 rr
                                    INSTR_BB(Z8_RegNum(reg1)*16 + 0x08, reg2);
                                }
                                else if (Z8_IsRP(reg2))
                                {
                                    // LD reg,Rr => x9 rr
                                    INSTR_BB(reg2*16 + 0x09, reg1);
                                }
                                else
                                {
                                    // LD dst,src => E4 ss dd
                                    INSTR_BBB(0xE4, reg2, reg1);
                                }
                                break;

                            case RTYP_Ir:
                                // LD dst,@Rs
                                Z8_CheckReg(reg1);
                                if (isRP1 )
                                {
                                    // LD Rd,@Rs => E3 ds
                                    INSTR_BB(0xE3, (reg1 & 0x0F)*16 + Z8_RegNum(reg2));
                                }
                                else
                                {
                                    // LD dst,@Rs => E5 Es dd
                                    INSTR_BBB(0xE5, 0xE0 + Z8_RegNum(reg2), reg1);
                                }
                                break;

                            case RTYP_Ind:
                                // LD dst,@src
                                reg2 = EXPR_Eval();
                                Z8_CheckReg(reg1);
                                Z8_CheckReg(reg2);
                                if (isRP1 && Z8_IsRP(reg2))
                                {
                                    // LD Rr,@Rr => E3 ds
                                    INSTR_BB(0xE3, (reg1 & 0x0F)*16 + (reg2 & 0x0F));
                                }
                                else
                                {
                                    // LD dst,@src => E5 ss dd
                                    INSTR_BBB(0xE5, reg2, reg1);
                                }
                                break;

                            case RTYP_Imm:
                                // LD dst,#imm
                                val = EXPR_Eval();
                                if (isRP1)
                                {
                                    // LD Rr,#imm => rC ii
                                    INSTR_BB((reg1 & 0x0F)*16 + 0x0C, val);
                                }
                                else
                                {
                                    // LD dst,#imm => E6 rr ii
                                    INSTR_BBB(0xE6, reg1, val);
                                }
                                break;

                            case reg_EOL:
                                break;

                            default:
                                ASMX_IllegalOperand();
                                break;
                        }
                    }
                    break;

                case RTYP_Ind:
                    // LD @reg,
                    reg1 = EXPR_Eval();
                    TOKEN_Comma();
                    reg2 = Z8_GetReg();
                    switch (Z8_RegType(reg2))
                    {
                        case RTYP_Imm:
                            // LD @reg,#imm => E7 rr ii
                            val = EXPR_Eval();
                            INSTR_BBB(0xE7, reg1, val);
                            break;

                        case RTYP_Reg:
                            // LD @reg,Rs
                            if (Z8_IsRP(reg1))
                            {
                                // LD @Rd,Rs => F3 ds
                                INSTR_BB(0xF3, (reg1 & 0x0F)*16 + (reg2 & 0x0F));
                            }
                            else
                            {
                                // LD @reg,Rs => F5 Es dd
                                INSTR_BBB(0xF5, reg2 | 0xE0, reg1);
                            }
                            break;

                        case reg_None:
                            // LD @reg,reg
                            isRP1 = Z8_IsRP(reg1);
                            reg2 = EXPR_Eval();
                            Z8_CheckReg(reg2);
                            if (isRP1 && Z8_IsRP(reg2))
                            {
                                // LD @Rr,Rr => F3 ds
                                INSTR_BB(0xF3, (reg1 & 0x0F)*16 + (reg2 & 0x0F));
                            }
                            else
                            {
                                // LD @reg,Rs => F5 ss dd
                                INSTR_BBB(0xF5, reg2, reg1);
                            }
                            break;

                        case reg_EOL:
                            break;

                        default:
                            ASMX_IllegalOperand();
                            break;
                    }
                    break;

                case reg_EOL:
                    break;

                default:
                    ASMX_IllegalOperand();
                    break;
            }
            break;

        case OP_JR:
            // JR - relative jump
            // JR [C,]rel => cB dd
            reg1 = Z8_GetCond();
            val = EXPR_EvalBranch(2);
            INSTR_BB(reg1*16 + 0x0B, val);
            break;

        case OP_JP:
            // JP - absolute or register jump
            reg1 = Z8_GetCond();
            reg2 = Z8_GetReg();
            switch (Z8_RegType(reg2))
            {
                case reg_None:
                    // JP C,aaaa => cD aaaa
                    val = EXPR_Eval();
                    INSTR_BW(reg1*16 + 0x0D, val);
                    break;

                case RTYP_Ir:
                    // JP @Rr => 30 Er
                    if ((Z8_RegNum(reg2) & 0x01) != 0)
                    {
                        ASMX_Error("Register must be even");
                    }
                // fall through
                case RTYP_Irr:
                    // JP @RRr => 30 Er
                    INSTR_BB(0x30, 0xE0 + Z8_RegNum(reg2));
                    break;

                case RTYP_Ind:
                    // JP @rr => 30 rr
                    reg2 = EXPR_Eval();
                    Z8_CheckReg(reg2);
                    if ((reg1 & 0x01) != 0)
                    {
                        ASMX_Error("Register must be even");
                    }
                    INSTR_BB(0x30, reg2);
                    break;

                case reg_EOL:
                    break;

                default:
                    ASMX_IllegalOperand();
                    break;
            }
            break;

        case OP_DJNZ:
            // DJNZ - decrement and jump if not zero
            reg1 = Z8_GetReg();
            switch (Z8_RegType(reg1))
            {
                case RTYP_Reg:
                    // DJNZ Rr,rel => rA dd
                    TOKEN_Comma();
                    val = EXPR_EvalBranch(2);
                    INSTR_BB(Z8_RegNum(reg1)*16 + 0x0A, val);
                    break;

                case reg_EOL:
                    break;

                default:
                    ASMX_IllegalOperand();
                    break;
            }
            break;

        case OP_CALL:
            // CALL - call a subroutine
            reg1 = Z8_GetReg();
            switch (Z8_RegType(reg1))
            {
                case RTYP_Ir:
                    // @Rr => D4 Er
                    if ((Z8_RegNum(reg1) & 0x01) != 0)
                    {
                        ASMX_Error("Register must be even");
                    }
                // fall through
                case RTYP_Irr:
                    // @RRr => D4 Er
                    INSTR_BB(0xD4, 0xE0 + Z8_RegNum(reg1));
                    break;

                case RTYP_Ind:
                    // CALL @reg => D4 rr
                    reg1 = EXPR_Eval();
                    Z8_CheckReg(reg1);
                    if ((Z8_RegNum(reg1) & 0x01) != 0)
                    {
                        ASMX_Error("Register must be even");
                    }
                    INSTR_BB(0xD4, reg1);
                    break;

                case reg_None: // CALL aaaa => D6 aaaa
                    val = EXPR_Eval();
                    INSTR_BW(0xD6, val);
                    break;

                case reg_EOL:
                    break;

                default:
                    ASMX_IllegalOperand();
                    break;
            }
            break;

        case OP_OneB:
        // x0 x1 - single byte operand
        case OP_OneW:
            // x0 x1 - single word operand, must be even register address
            // OP Rr => x0 Er
            // OP reg => x0 rr
            // OP @reg => x1 @rr
            reg1 = Z8_GetReg();
            switch (Z8_RegType(reg1))
            {
                case RTYP_RReg:
                    // RRr
                    if (typ != OP_OneW)
                    {
                        ASMX_IllegalOperand();
                    }
                    FALLTHROUGH;
                case RTYP_Reg:
                    // Rr
                    if (typ == OP_OneW && (Z8_RegNum(reg1) & 0x01) != 0)
                    {
                        ASMX_Error("Register must be even");
                    }
                    if (parm == 0x20)
                    {
                        // special case for INC Rr => rE
                        INSTR_B(Z8_RegNum(reg1)*16 + 0x0E);
                        break;
                    }
                    // OP Rr => x0 Er
                    INSTR_BB(parm, 0xE0 + Z8_RegNum(reg1));
                    break;

                case RTYP_Irr:
                    // @RRr
                    if (typ != OP_OneW)
                    {
                        ASMX_IllegalOperand();
                    }
                    FALLTHROUGH;
                case RTYP_Ir:
                    // @Rr
                    if (typ == OP_OneW && (Z8_RegNum(reg1) & 0x01))
                    {
                        ASMX_Error("Register must be even");
                    }
                    // OP @Rr => x1 Er
                    INSTR_BB(parm + 0x01, 0xE0 + Z8_RegNum(reg1));
                    break;

                case RTYP_Ind:
                    // OP @reg => x1 rr
                    reg1 = EXPR_Eval();
                    Z8_CheckReg(reg1);
                    if (typ == OP_OneW && (reg1 & 0x01) != 0)
                    {
                        ASMX_Error("Register must be even");
                    }
                    INSTR_BB(parm + 0x01, reg1);
                    break;

                case reg_None:
                    // OP reg  => x0 rr
                    reg1 = EXPR_Eval();
                    Z8_CheckReg(reg1);
                    if (typ == OP_OneW && (reg1 & 0x01) != 0)
                    {
                        ASMX_Error("Register must be even");
                    }
                    INSTR_BB(parm + 0x00, reg1);
                    break;

                case reg_EOL:
                    break;

                default:
                    ASMX_IllegalOperand();
                    break;
            }
            break;

        case OP_Two:
            // x2..x6 - two operand
            // OP Rd,Rs => x2 ds
            // OP Rd,@Rs => x3 ds
            // OP dst,src => x4 src dst
            // OP dst,@src => x5 src dst
            // OP reg,#imm => x6 rr ii
            // OP @reg,#imm => x7 rr ii
            reg1 = Z8_GetReg();
            switch (Z8_RegType(reg1))
            {
                case RTYP_Reg:
                    // OP Rr,
                    TOKEN_Comma();
                    reg2 = Z8_GetReg();
                    switch (Z8_RegType(reg2))
                    {
                        case RTYP_Reg:
                            // OP Rd,Rs => x2 ds
                            INSTR_BB(parm + 0x02, Z8_RegNum(reg1)*16 + Z8_RegNum(reg2));
                            break;

                        case RTYP_Ir:
                            // OP Rd,@Rs => x3 ds
                            INSTR_BB(parm + 0x03, Z8_RegNum(reg1)*16 + Z8_RegNum(reg2));
                            break;

                        case reg_None:
                            // OP Rd,reg => x4 ss Ed
                            reg2 = EXPR_Eval();
                            Z8_CheckReg(reg2);
                            INSTR_BBB(parm + 0x04, reg2, 0xE0 + Z8_RegNum(reg1));
                            break;

                        case RTYP_Ind:
                            // OP Rd,@reg => x5 ss Ed
                            reg2 = EXPR_Eval();
                            Z8_CheckReg(reg2);
                            INSTR_BBB(parm + 0x05, reg2, 0xE0 + Z8_RegNum(reg1));
                            break;

                        case RTYP_Imm:
                            // OP Rd,#imm => 06 Ed ii
                            val = EXPR_Eval();
                            INSTR_BBB(parm + 0x06, 0xE0 + Z8_RegNum(reg1), val);
                            break;

                        case reg_EOL:
                            break;

                        default:
                            ASMX_IllegalOperand();
                            break;
                    }
                    break;

                case RTYP_Ir:
                    // OP @Rr,#imm
                    TOKEN_Comma();
                    reg2 = Z8_GetReg();
                    switch (Z8_RegType(reg2))
                    {
                        case RTYP_Imm:
                            // OP @Rr,#imm => 07 Er ii
                            val = EXPR_Eval();
                            INSTR_BBB(parm + 0x07, 0xE0 + Z8_RegNum(reg1), val);
                            break;

                        case reg_EOL:
                            break;

                        default:
                            ASMX_IllegalOperand();
                            break;
                    }
                    break;

                case reg_None:
                    // OP reg,
                    reg1 = EXPR_Eval();
                    isRP1 = Z8_IsRP(reg1);
                    TOKEN_Comma();
                    Z8_CheckReg(reg1);
                    INSTR_BB(parm, reg1);
                    reg2 = Z8_GetReg();
                    switch (Z8_RegType(reg2))
                    {
                        case RTYP_Reg:
                            // OP dd,Rs => 04 Es dd
                            INSTR_BBB(parm + 0x04, 0xE0 + Z8_RegNum(reg2), reg1);
                            break;

                        case reg_None:
                            // OP dd,ss => 04 ss dd
                            reg2 = EXPR_Eval();
                            Z8_CheckReg(reg2);
                            // OP dd,Rs
                            if (isRP1 && Z8_IsRP(reg2))
                            {
                                // OP Rd,Rs => 02 ds
                                INSTR_BB(parm + 0x02, (reg1 & 0x0F)*16 + (reg2 & 0x0F));
                            }
                            else
                            {
                                INSTR_BBB(parm + 0x04, reg2, reg1);
                            }
                            break;

                        case RTYP_Ir:
                            // OP dd,@Rs => 05 Es dd
                            INSTR_BBB(parm + 0x05, 0xE0 + Z8_RegNum(reg2), reg1);
                            break;

                        case RTYP_Ind:
                            // OP dd,@ss
                            reg2 = EXPR_Eval();
                            Z8_CheckReg(reg2);
                            if (isRP1 && Z8_IsRP(reg2))
                            {
                                // OP Rd,@Rs => 03 ds
                                INSTR_BB(parm + 0x03, (reg1 & 0x0F)*16 + (reg2 & 0x0F));
                            }
                            else
                            {
                                // OP dd,@ss => 05 ss dd
                                INSTR_BBB(parm + 0x05, reg2, reg1);
                            }
                            break;

                        case RTYP_Imm:
                            // OP reg,#imm => 06 rr ii
                            Z8_CheckReg(reg1);
                            val = EXPR_Eval();
                            INSTR_BBB(parm + 0x06, reg1, val);
                            break;

                        case reg_EOL:
                            break;

                        default:
                            ASMX_IllegalOperand();
                            break;
                    }
                    break;

                case RTYP_Ind:
                    // OP @reg,#imm
                    reg1 = EXPR_Eval();
                    TOKEN_Comma();
                    reg2 = Z8_GetReg();
                    switch (Z8_RegType(reg2))
                    {
                        case RTYP_Imm:
                            // OP @reg,#imm => 07 rr ii
                            Z8_CheckReg(reg1);
                            val = EXPR_Eval();
                            INSTR_BBB(parm + 0x07, reg1, val);
                            break;

                        case reg_EOL:
                            break;

                        default:
                            ASMX_IllegalOperand();
                            break;
                    }
                    break;

                case reg_EOL:
                    break;

                default:
                    ASMX_IllegalOperand();
                    break;
            }
            break;

        case OP_LDE_LDC:
            // LDE and LDC instructions
            reg1 = Z8_GetReg();
            switch (Z8_RegType(reg1))
            {
                case RTYP_Reg:
                    // LDE/LDC Rr,@RRr
                    TOKEN_Comma();
                    reg2 = Z8_GetReg();
                    switch (Z8_RegType(reg2))
                    {
                        case RTYP_Irr:
                            // LDE/LDC Rr,@RRr => 82/C2 ds
                            INSTR_BB(parm + 0x00, Z8_RegNum(reg1)*16 + Z8_RegNum(reg2));
                            break;

                        case reg_EOL:
                            break;

                        default:
                            ASMX_IllegalOperand();
                            break;
                    }
                    break;

                case RTYP_Irr:
                    // LDE/LDC @RRr,Rr
                    TOKEN_Comma();
                    reg2 = Z8_GetReg();
                    switch (Z8_RegType(reg2))
                    {
                        case RTYP_Reg:
                            // LDE/LDC @RRr,Rr => 92/D2 sd
                            // NOTE: some Zilog manuals show LDC as D2 ds
                            INSTR_BB(parm + 0x10, Z8_RegNum(reg2)*16 + Z8_RegNum(reg1));
                            break;

                        case reg_EOL:
                            break;

                        default:
                            ASMX_IllegalOperand();
                            break;
                    }
                    break;

                case reg_EOL:
                    break;

                default:
                    ASMX_IllegalOperand();
                    break;
            }
            break;

        case OP_LDEI_LDCI:
            // LDEI and LDCI instructions
            reg1 = Z8_GetReg();
            switch (Z8_RegType(reg1))
            {
                case RTYP_Ir:
                    // LDEI/LDCI @Rr,@RRr
                    TOKEN_Comma();
                    reg2 = Z8_GetReg();
                    switch (Z8_RegType(reg2))
                    {
                        case RTYP_Irr:
                            // LDEI/LDCI @Rr,@RRr => 83/C3 ds
                            INSTR_BB(parm + 0x00, Z8_RegNum(reg1)*16 + Z8_RegNum(reg2));
                            break;

                        case reg_EOL:
                            break;

                        default:
                            ASMX_IllegalOperand();
                            break;
                    }
                    break;

                case RTYP_Irr:
                    // LDEI/LDCI @RRr,@Rr
                    // NOTE: some Zilog manuals show LDCI as D3 ds
                    TOKEN_Comma();
                    reg2 = Z8_GetReg();
                    switch (Z8_RegType(reg2))
                    {
                        case RTYP_Ir:
                            // LDEI/LDCI @RRr,@Rr => 93/D3 sd
                            INSTR_BB(parm + 0x10, Z8_RegNum(reg2)*16 + Z8_RegNum(reg1));
                            break;

                        case reg_EOL:
                            break;

                        default:
                            ASMX_IllegalOperand();
                            break;
                    }
                    break;

                case reg_EOL:
                    break;

                default:
                    ASMX_IllegalOperand();
                    break;
            }
            break;

        case OP_SRP:
            // SRP instruction
            reg1 = Z8_GetReg();
            switch (Z8_RegType(reg1))
            {
                case RTYP_Imm:
                    // SRP #ii => 31 ii
                    val = EXPR_Eval();
                    INSTR_BB(0x31, val);
                    break;

                case reg_EOL:
                    break;

                default:
                    ASMX_IllegalOperand();
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

    switch (typ)
    {
        case OP_RP:
            // RP pseudo-op
            // This sets the expected value of the register pointer to
            // optimize Rxx addressing modes to Rn, or you can turn it OFF

            if (labl[0])
            {
                ASMX_Error("Label not allowed");
            }

            if (REG_Get("OFF") == 0)
            {
                // RP OFF to disable RP optimizations
                rpReg = -1;
                char *p = listLine + 2;
                p = LIST_Str(p, "--");
                break;
            }
            int val = EXPR_Eval();
            if (!errFlag)
            {
                if ((val & 0xFF) == 0)
                {
                    val = val >> 8;     // handle $XX00 as $00XX
                }
                if (val < 0 || val > 255 || (val > 0x0F && (val & 0x0F) != 0))
                {
                    ASMX_Error("Operand out of range");
                }
                else
                {
                    // convert low-nibble form to high-nibble form
                    if (val < 0x0F)
                    {
                        val = val*16;
                    }
                    rpReg = val;

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


void Z8_PassInit(void)
{
    // start each pass with RP optimizations disabled
    rpReg = -1;
}


void Z8_AsmInit(void)
{
    void *p = ASMX_AddAsm(versionName, &Z8_DoCPUOpcode, &Z8_DoCPULabelOp, &Z8_PassInit);

    ASMX_AddCPU(p, "Z8", 0, END_BIG, ADDR_16, LIST_24, 8, 0, Z8_opcdTab);
}
