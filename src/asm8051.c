// asmI8051.c

#define versionName "I8051 assembler"
#include "asmx.h"

enum instrType
{
    OP_None,     // NOP RET RETI - no operands
    OP_AJMP,     // ACALL AJMP - 11-bit jump with A11-A15 unchanged
    OP_LJMP,     // LCALL LJMP - 16-bit jump
    OP_Rel,      // JC JNC JNZ JZ SJMP - relative jump
    OP_BitRel,   // JB JBC JNB - relative jump with bit
    OP_DJNZ,     // DJNZ - decrement and relative jump if not zero
    OP_Arith,    // ADD ADDC SUBB - imm, @R0, @R1, R0-R7, dir
    OP_Logical,  // ANL ORL XRL - imm, @R0, @R1, R0-R7, dir, C-bit, dir-imm, dir-A
    OP_AccA,     // DA RL RLC RR RRC SWAP - only accepts A as parameter (should also have OP_None aliases)
    OP_AccAB,    // DIV MUL - only accepts AB as parameter
    OP_MOV,      // MOV - mucho complicated
    OP_INC_DEC,  // INC DEC - @R0, @R1, A, R0-R7, dir, and DPTR for INC only
    OP_XCHD,     // XCHD A,@R0/@R1
    OP_PushPop,  // POP PUSH - parameter is direct address
    OP_A_bit_C,  // SETB CLR CPL - accepts A, bit, C, except SETB does not accept A
    MOP_JMP,      // JMP @A+DPTR
    OP_MOVC,     // MOVC A,@A+DPTR/PC
    OP_MOVX,     // MOVX A to or from @DPTR, @R0, @R1
    OP_CJNE,     // CJNE @R0, @R1, A, R0-R7, dir with immediate and relative operands
    OP_XCH       // XCH A with @R0, @R1, R0-R7, dir

//  o_Foo = OP_LabelOp,
};

static const struct OpcdRec I8051_opcdTab[] =
{
    {"NOP",  OP_None,  0x00},
    {"RET",  OP_None,  0x22},
    {"RETI", OP_None,  0x32},

    {"ACALL",OP_AJMP,  0x11},
    {"AJMP", OP_AJMP,  0x01},
    {"LCALL",OP_LJMP,  0x12},
    {"LJMP", OP_LJMP,  0x02},

    {"JC",   OP_Rel,   0x40},
    {"JNC",  OP_Rel,   0x50},
    {"JNZ",  OP_Rel,   0x70},
    {"JZ",   OP_Rel,   0x60},
    {"SJMP", OP_Rel,   0x80},

    {"JB",   OP_BitRel, 0x20},
    {"JBC",  OP_BitRel, 0x10},
    {"JNB",  OP_BitRel, 0x30},

    {"DJNZ", OP_DJNZ,  0x00},

    {"ADD",  OP_Arith, 0x20},
    {"ADDC", OP_Arith, 0x30},
    {"SUBB", OP_Arith, 0x90},

    {"ANL",  OP_Logical, 0x8050},
    {"ORL",  OP_Logical, 0x7040},
    {"XRL",  OP_Logical, 0x0060},

    {"DA",   OP_AccA,  0xD4},
    {"RL",   OP_AccA,  0x23},
    {"RLC",  OP_AccA,  0x33},
    {"RR",   OP_AccA,  0x03},
    {"RRC",  OP_AccA,  0x13},
    {"SWAP", OP_AccA,  0xC4},

    // aliases for OP_AccA instructions
//    {"DAA",  OP_None, 0xD4},
//    {"RLA",  OP_None, 0x23},
//    {"RLCA", OP_None, 0x33},
//    {"RRA",  OP_None, 0x03},
//    {"RRCA", OP_None, 0x13},
//    {"SWAPA",OP_None, 0xC4},

    {"DIV",  OP_AccAB, 0x84},
    {"MUL",  OP_AccAB, 0xA4},

    {"MOV",  OP_MOV,   0x00},

    {"INC",  OP_INC_DEC, 0xA300},
    {"DEC",  OP_INC_DEC, 0x0010},

    {"XCHD", OP_XCHD,    0x00},

    {"POP",  OP_PushPop, 0xD0},
    {"PUSH", OP_PushPop, 0xC0},

    {"CLR",  OP_A_bit_C, 0xE4C2},
    {"CPL",  OP_A_bit_C, 0xF4B2},
    {"SETB", OP_A_bit_C, 0x00D2},

    {"JMP",  MOP_JMP,  0x00},

    {"MOVC", OP_MOVC, 0x00},

    {"MOVX", OP_MOVX, 0x00},

    {"CJNE", OP_CJNE, 0x00},

    {"XCH",  OP_XCH,  0x00},

    {"",    OP_Illegal,  0}
};

char regs_8051[] = "@R0 @R1 R0 R1 R2 R3 R4 R5 R6 R7 # A C DPTR @DPTR";

enum
{
    REG_xR0   =  0,
    REG_xR1   =  1,
    REG_R0    =  2,
    REG_R1    =  3,
    REG_R2    =  4,
    REG_R3    =  5,
    REG_R4    =  6,
    REG_R5    =  7,
    REG_R6    =  8,
    REG_R7    =  9,
    REG_Imm   = 10,
    REG_A     = 11,
    REG_C     = 12,
    REG_DPTR  = 13,
    REG_xDPTR = 14,
};


// --------------------------------------------------------------


static int I8051_GetReg(const char *regList)
{
    Str255  word;
    int     token;

    char *oldLine = linePtr;

    if (!(token = TOKEN_GetWord(word)))
    {
        ASMX_MissingOperand();
        return reg_EOL;
    }

    // 8051 needs to handle '@' symbols as part of a register name
    if (token == '@')
    {
        TOKEN_GetWord(word+1);
    }

    token = REG_Find(word, regList);
    if (token < 0)
    {
        // if not in register list, rewind the line pointer
        linePtr = oldLine;
    }
    return token;
}


static int I8051_GetBitReg(int reg)
{
    // Returns the base bit address for a byte address
    //
    // addresses 0x20..0x2F are bits 0x00..0x7F
    // addresses 0xX0 are bits 0xX0..0xX7 (X >= 8)
    // addresses 0xX8 are bits 0xX8..0xXF (X >= 8)
    //
    // all other RAM address are invalid and -1 will be returned

    if (0x20 <= reg && reg <= 0x2F)
    {
        reg = (reg & 0x1F) * 8;
    }
    else if ((reg & 0x87) != 0x80)
    {
        reg = -1;
    }

    return reg;
}


static int I8051_EvalBitReg()
{
    // parses a value that might be a bit address
    //
    // If the upcoming expression is followed by a period ("."),
    // it will be assumed to be a bit-accessible register, and is
    // followed by a bit number in the range 0..7. If this is so,
    // the register address is validated, and the computed bit
    // index is returned.
    //
    // Otherwise, the first expression will be assumed to be an
    // absolute bit register address and returned unchanged.
    //
    // Note that 0.0 is not a valid bit address, so it is possible
    // that forward references could cause phase errors. To avoid
    // this, callers should always generate object code even when
    // the bit address is invalid. Therefore, this function will
    // call IllegalOperand() to generate the error, and then return
    // zero to ensure that the caller will always generate code.

    Str255  word;

    int val1 = EXPR_Eval();
    char *oldLine = linePtr;
    if (TOKEN_GetWord(word) == '.')
    {
        // evaluate bit number
        int val2 = EXPR_Eval();

        // determine base bit index
        val1 = I8051_GetBitReg(val1);

        // validate bit index
        if (val1 < 0 || val2 < 0 || val2 > 7)
        {
            ASMX_IllegalOperand();
            return 0;
        }

        // return completed bit index
        return val1 + val2;
    }

    // resulting bit index must be 0..255
    linePtr = oldLine;
    if (val1 & 0xFFFFFF00)
    {
        ASMX_IllegalOperand();
        return 0;
    }
    return val1;
}


static int I8051_DoCPUOpcode(int typ, int parm)
{
    int     val, reg1, reg2;
    Str255  word;
    char    *oldLine;
    int     token;

    switch (typ)
    {
        case OP_None:
            INSTR_B(parm);
            break;

        case OP_AJMP:
            val = EXPR_Eval();
            if ((val & 0xF800) != ((locPtr + 2) & 0xF800))
            {
                if (parm == 0x01)
                {
                    ASMX_Warning("AJMP out of range");
                }
                else
                {
                    ASMX_Warning("ACALL out of range");
                }
            }
            INSTR_BB(parm + ((val & 0x0700) >> 3), val & 0xFF);
            break;

        case OP_LJMP:
            val = EXPR_Eval();
            INSTR_BW(parm, val);
            break;

        case OP_Rel:
            val = EXPR_EvalBranch(2);
            INSTR_BB(parm, val);
            break;

        case OP_BitRel:
            reg1 = I8051_EvalBitReg();
            if (reg1 < 0) break;
            TOKEN_Comma();
            val = EXPR_EvalBranch(3);
            INSTR_BBB(parm, reg1, val);
            break;

        case OP_DJNZ:
            switch ((reg1 = I8051_GetReg(regs_8051)))
            {
                case REG_R0:    // Rn
                case REG_R1:
                case REG_R2:
                case REG_R3:
                case REG_R4:
                case REG_R5:
                case REG_R6:
                case REG_R7:
                    TOKEN_Comma();
                    val = EXPR_EvalBranch(2);
                    INSTR_BB(0xD8 + reg1 - REG_R0, val);
                    break;

                case reg_None:
                    reg1 = EXPR_EvalByte();
                    TOKEN_Comma();
                    val = EXPR_EvalBranch(3);
                    INSTR_BBB(0xD5, reg1, val);
                    break;

                default:
                    ASMX_IllegalOperand();
                    break;
            }
            break;

        case OP_Arith:
            switch (I8051_GetReg("A"))
            {
                default:
                case reg_None:
                    ASMX_IllegalOperand();
                    break;

                case reg_EOL:
                    break;

                case 0:     // A
                    TOKEN_Comma();
                    switch ((reg1 = I8051_GetReg(regs_8051)))
                    {
                        default:
                            ASMX_IllegalOperand();
                            break;

                        case reg_EOL: // EOL
                            break;

                        case reg_None: // A,dir
                            val = EXPR_EvalByte();
                            INSTR_BB(parm + 0x05, val);
                            break;

                        case REG_xR0:   // A,@Rn
                        case REG_xR1:
                        case REG_R0:    // A,Rn
                        case REG_R1:
                        case REG_R2:
                        case REG_R3:
                        case REG_R4:
                        case REG_R5:
                        case REG_R6:
                        case REG_R7:
                            INSTR_B(parm + 0x06 + reg1 - REG_xR0);
                            break;

                        case REG_Imm:   // A,#imm
                            val = EXPR_EvalByte();
                            INSTR_BB(parm + 0x04, val);
                            break;
                    }
                    break;
            }
            break;

        case OP_Logical:
            switch ((I8051_GetReg("A C")))
            {
                default:
                    ASMX_IllegalOperand();
                    break;

                case reg_EOL:
                    break;

                case reg_None: // dir
                    val = EXPR_EvalByte();

                    TOKEN_Comma();
                    switch (I8051_GetReg("A #"))
                    {
                        default:
                        case reg_None:
                            ASMX_IllegalOperand();
                            break;

                        case reg_EOL:
                            break;

                        case 0:     // dir,A
                            INSTR_BB((parm & 0xFF) + 0x02, val);
                            break;

                        case 1:     // dir,#imm
                            reg1 = val;
                            val = EXPR_EvalByte();
                            INSTR_BBB((parm & 0xFF) + 0x03, reg1, val);
                            break;
                    }
                    break;

                case 0:     // A
                    TOKEN_Comma();
                    switch ((reg1 = I8051_GetReg(regs_8051)))
                    {
                        default:
                            ASMX_IllegalOperand();
                            break;

                        case reg_EOL:
                            break;

                        case reg_None: // A,dir
                            val = EXPR_EvalByte();
                            INSTR_BB((parm & 0xFF) + 0x05, val);
                            break;

                        case REG_xR0:   // A,@Rn
                        case REG_xR1:
                        case REG_R0:    // A,Rn
                        case REG_R1:
                        case REG_R2:
                        case REG_R3:
                        case REG_R4:
                        case REG_R5:
                        case REG_R6:
                        case REG_R7:
                            INSTR_B((parm & 0xFF) + 0x06 + reg1 - REG_xR0);
                            break;

                        case REG_Imm:   // A,#imm
                            val = EXPR_EvalByte();
                            INSTR_BB((parm & 0xFF) + 0x04, val);
                            break;
                    }
                    break;

                case 1:     // C
                    if ((parm & 0xFF00) == 0)
                    {
                        // XRL C,bit
                        ASMX_IllegalOperand();
                        break;
                    }
                    TOKEN_Comma();
                    // "bit" or "/bit"
                    val = 0x02;
                    oldLine = linePtr;
                    token = TOKEN_GetWord(word);
                    if (token == '/')
                    {
                        val = 0x30;
                    }
                    else
                    {
                        linePtr = oldLine;
                    }

                    reg1 = I8051_EvalBitReg();
                    if (reg1 < 0) break;
                    INSTR_BB((parm >> 8) + val, reg1);
                    break;
            }
            break;

        case OP_AccA:
            switch (I8051_GetReg("A"))
            {
                default:
                case reg_None:
                    ASMX_IllegalOperand();
                    break;

                case reg_EOL:
                    break;

                case 0:     // A
                    INSTR_B(parm);
            }
            break;

        case OP_AccAB:
            switch (I8051_GetReg("AB"))
            {
                default:
                case reg_None:
                    ASMX_IllegalOperand();
                    break;

                case reg_EOL:
                    break;

                case 0:     // AB
                    INSTR_B(parm);
                    break;
            }
            break;

        case OP_MOV:
            switch ((reg1 = I8051_GetReg(regs_8051)))
            {
                case reg_EOL: // EOL
                    break;

                case reg_None:    // dir or bit
                    reg1 = EXPR_EvalByte();
                    oldLine = linePtr;
                    token = TOKEN_GetWord(word);
                    if (token == '.')   // bit.b,C
                    {
                        reg1 = I8051_GetBitReg(reg1);
                        reg2 = EXPR_EvalByte();

                        if (reg1 < 0 || reg2 < 0 || reg2 > 7)
                        {
                            ASMX_IllegalOperand();
                            reg1 = 0;
                        }
                        else
                        {
                            reg1 += reg2;
                        }
                        TOKEN_Comma();
                        switch (I8051_GetReg("C"))
                        {
                            default:
                            case reg_None:
                                ASMX_IllegalOperand();
                                break;

                            case reg_EOL:
                                break;

                            case 0:     // bit,C
                                INSTR_BB(0x92, reg1);
                                break;
                        }
                        break;
                    }
                    else if (token != ',')     // dir,
                    {
                        oldLine = linePtr;
                        TOKEN_Expect(",");
                        break;
                    }
                    // dir,Rn or bit,C
                    switch ((reg2 = I8051_GetReg(regs_8051)))
                    {
                        default:
                            ASMX_IllegalOperand();
                            break;

                        case reg_EOL:
                            break;

                        case reg_None: // dir,dir
                            val = EXPR_EvalByte(); // evaluate src
                            // Note that the assembly code is "MOV dst,src"
                            // but the object code is "0x85 src dst"
                            INSTR_BBB(0x85, val, reg1);
                            break;

                        case REG_xR0:   // dir,@Rn
                        case REG_xR1:
                        case REG_R0:    // dir,Rn
                        case REG_R1:
                        case REG_R2:
                        case REG_R3:
                        case REG_R4:
                        case REG_R5:
                        case REG_R6:
                        case REG_R7:
                            INSTR_BB(0x86 + reg2 - REG_xR0, reg1);
                            break;

                        case REG_A:     // dir,A
                            INSTR_BB(0xF5, reg1);
                            break;

                        case REG_Imm:   // dir,#imm
                            val = EXPR_EvalByte();
                            INSTR_BBB(0x75, reg1, val);
                            break;

                        case REG_C:     // bit,C
                            INSTR_BB(0x92, reg1);
                            break;
                    }
                    break;

                case REG_xR0:   // @Rn,
                case REG_xR1:
                case REG_R0:    // Rn,
                case REG_R1:
                case REG_R2:
                case REG_R3:
                case REG_R4:
                case REG_R5:
                case REG_R6:
                case REG_R7:
                    TOKEN_Comma();
                    switch (I8051_GetReg("A #"))
                    {
                        default:
                            ASMX_IllegalOperand();
                            break;

                        case reg_EOL:
                            break;

                        case reg_None:  // Rn,dir
                            val = EXPR_EvalByte();
                            INSTR_BB(0xA6 + reg1, val);
                            break;

                        case 0:         // Rn,A
                            INSTR_B(0xF6 + reg1);
                            break;

                        case 1:         // Rn,#imm
                            val = EXPR_EvalByte();
                            INSTR_BB(0x76 + reg1 - REG_xR0, val);
                            break;
                    }
                    break;

                case REG_A:     // A,
                    TOKEN_Comma();
                    switch ((reg1 = I8051_GetReg(regs_8051)))
                    {
                        default:
                            ASMX_IllegalOperand();
                            break;

                        case reg_EOL:
                            break;

                        case reg_None:  // A,dir
                            val = EXPR_EvalByte();
                            INSTR_BB(0xE5, val);
                            break;

                        case REG_xR0:   // A,@Rn
                        case REG_xR1:
                        case REG_R0:    // A,Rn
                        case REG_R1:
                        case REG_R2:
                        case REG_R3:
                        case REG_R4:
                        case REG_R5:
                        case REG_R6:
                        case REG_R7:
                            INSTR_B(0xE6 + reg1 - REG_xR0);
                            break;

                        case REG_Imm:   // A,#imm
                            val = EXPR_EvalByte();
                            INSTR_BB(0x74, val);
                            break;
                    }
                    break;

                case REG_C:     // C,bit
                    TOKEN_Comma();
                    reg1 = I8051_EvalBitReg();
                    if (reg1 < 0) break;
                    INSTR_BB(0xA2, reg1);
                    break;

                case REG_DPTR:  // DPTR,#
                    TOKEN_Comma();
                    TOKEN_Expect("#");
                    val = EXPR_Eval();
                    INSTR_BW(0x90, val);
                    break;

                default:
                    ASMX_IllegalOperand();
                    break;
            }
            break;

        case OP_INC_DEC:
            switch ((reg1 = I8051_GetReg(regs_8051)))
            {
                case reg_EOL: // EOL
                    break;

                case reg_None: // dir
                    val = EXPR_EvalByte();
                    INSTR_BB(parm + 0x05, val);
                    break;

                case REG_A:     // A
                    INSTR_B((parm & 0xFF) + 0x04);
                    break;

                case REG_xR0:   // @Rn,
                case REG_xR1:
                case REG_R0:    // Rn,
                case REG_R1:
                case REG_R2:
                case REG_R3:
                case REG_R4:
                case REG_R5:
                case REG_R6:
                case REG_R7:
                    INSTR_B((parm & 0xFF) + 0x06 + reg1 - REG_xR0);
                    break;

                case REG_DPTR:  // DPTR
                    if (parm & 0xFF00)
                    {
                        INSTR_B(parm >> 8);
                    }
                    else
                    {
                        ASMX_IllegalOperand();
                    }
                    break;

                default:
                    ASMX_IllegalOperand();
                    break;
            }
            break;

        case OP_XCHD:
            switch (I8051_GetReg("A"))
            {
                default:
                case reg_None:
                    ASMX_IllegalOperand();
                    break;

                case reg_EOL:
                    break;

                case 0:     // A
                    TOKEN_Comma();
                    switch ((reg1 = I8051_GetReg("@R0 @R1")))
                    {
                        default:
                        case reg_None:
                            ASMX_IllegalOperand();
                            break;

                        case reg_EOL:
                            break;

                        case 0: // A,@R0
                        case 1: // A,@R1
                            INSTR_B(0xD6 + reg1);
                            break;
                    }
                    break;
            }
            break;

        case OP_PushPop:
            val = EXPR_Eval();
            INSTR_BB(parm, val);
            break;

        case OP_A_bit_C:
            switch ((reg1 = I8051_GetReg("A C")))
            {
                case 0:     // A
                    if ((parm & 0xFF00) == 0)
                    {
                        ASMX_IllegalOperand();
                    }
                    else
                    {
                        INSTR_B(parm >> 8);
                    }
                    break;

                case 1:     // C
                    INSTR_B((parm & 0xFF) + 1);
                    break;

                case reg_None: // bit
                    reg1 = I8051_EvalBitReg();
                    if (reg1 < 0) break;
                    INSTR_BB(parm & 0xFF, reg1);
                    break;

                default:
                    break;
            }
            break;

        case MOP_JMP:
            switch (I8051_GetReg("@A"))
            {
                default:
                case reg_None:
                    ASMX_IllegalOperand();
                    break;

                case reg_EOL:
                    break;

                case 0:     // @A
                    TOKEN_Expect("+");
                    switch (I8051_GetReg("DPTR"))
                    {
                        default:
                        case reg_None:
                            ASMX_IllegalOperand();
                            break;

                        case reg_EOL:
                            break;

                        case 0:     // @A+DPTR
                            INSTR_B(0x73);
                            break;
                    }
                    break;
            }
            break;

        case OP_MOVC:
            switch (I8051_GetReg("A"))
            {
                default:
                case reg_None:
                    ASMX_IllegalOperand();
                    break;

                case reg_EOL:
                    break;

                case 0:     // A
                    TOKEN_Comma();
                    switch (I8051_GetReg("@A"))
                    {
                        default:
                        case reg_None:
                            ASMX_IllegalOperand();
                            break;

                        case reg_EOL:
                            break;

                        case 0:     // A,@A+
                            TOKEN_Expect("+");

                            switch (I8051_GetReg("PC DPTR"))
                            {
                                default:
                                case reg_None:
                                    ASMX_IllegalOperand();
                                    break;

                                case reg_EOL:
                                    break;

                                case 0: // A,@A+PC
                                    INSTR_B(0x83);
                                    break;

                                case 1: // A,@A+DPTR
                                    INSTR_B(0x93);
                                    break;
                            }
                            break;
                    }
                    break;
            }
            break;

        case OP_MOVX:
            switch ((reg1 = I8051_GetReg("@DPTR A @R0 @R1")))
            {
                default:
                case reg_None:
                    ASMX_IllegalOperand();
                    break;

                case reg_EOL:
                    break;

                case 1:     // A
                    TOKEN_Comma();
                    switch ((reg1 = I8051_GetReg("@DPTR A @R0 @R1")))
                    {
                        case 1:     // A,A
                        default:
                        case reg_None:
                            ASMX_IllegalOperand();
                            break;

                        case reg_EOL:
                            break;

                        case 0:     // A,@DPTR
                        case 2:     // A,@R0
                        case 3:     // A,@R1
                            INSTR_B(0xE0 + reg1);
                            break;
                    }
                    break;

                case 0:     // @DPTR
                case 2:     // @R0
                case 3:     // @R1
                    TOKEN_Comma();
                    switch (I8051_GetReg("A"))
                    {
                        default:
                        case reg_None:
                            ASMX_IllegalOperand();
                            break;

                        case reg_EOL:
                            break;

                        case 0:     // ,A
                            break;
                    }
                    INSTR_B(0xF0 + reg1);
                    break;
            }
            break;

        case OP_CJNE:
            switch ((reg1 = I8051_GetReg(regs_8051)))
            {
                case reg_None:
                    ASMX_IllegalOperand();
                    break;

                case REG_A:     // A
                    TOKEN_Comma();
                    switch ((reg1 = I8051_GetReg("#")))
                    {
                        default:
                            ASMX_IllegalOperand();
                            break;

                        case reg_EOL:
                            break;

                        case reg_None: // A,dir,rel
                            reg1 = EXPR_EvalByte();
                            TOKEN_Comma();
                            val = EXPR_EvalBranch(3);
                            INSTR_BBB(0xB5, reg1, val);
                            break;

                        case 0: // A,#imm,rel
                            reg1 = EXPR_EvalByte();
                            TOKEN_Comma();
                            val = EXPR_EvalBranch(3);
                            INSTR_BBB(0xB4, reg1, val);
                            break;
                    }
                    break;

                case REG_xR0:   // @Rn,#imm,rel
                case REG_xR1:
                case REG_R0:    // Rn,#imm,rel
                case REG_R1:
                case REG_R2:
                case REG_R3:
                case REG_R4:
                case REG_R5:
                case REG_R6:
                case REG_R7:
                    TOKEN_Comma();
                    TOKEN_Expect("#");
                    reg2 = EXPR_EvalByte();
                    TOKEN_Comma();
                    val = EXPR_EvalBranch(3);
                    INSTR_BBB(0xB6 + reg1 - REG_xR0, reg2, val);
                    break;

                default:
                    break;
            }
            break;

        case OP_XCH:
            switch (I8051_GetReg("A"))
            {
                default:
                case reg_None:
                    ASMX_IllegalOperand();
                    break;

                case reg_EOL:
                    break;

                case 0:     // A
                    TOKEN_Comma();
                    switch ((reg1 = I8051_GetReg(regs_8051)))
                    {
                        default:
                            ASMX_IllegalOperand();
                            break;

                        case reg_EOL:
                            break;

                        case reg_None: // dir
                            val = EXPR_EvalByte();
                            INSTR_BB(0xC5, val);
                            break;

                        case REG_xR0:   // A,@Rn
                        case REG_xR1:
                        case REG_R0:    // A,Rn
                        case REG_R1:
                        case REG_R2:
                        case REG_R3:
                        case REG_R4:
                        case REG_R5:
                        case REG_R6:
                        case REG_R7:
                            INSTR_B(0xC6 + reg1 - REG_xR0);
                            break;
                    }
                    break;
            }
            break;

        default:
            return 0;
            break;
    }
    return 1;
}


static int I8051_DoCPULabelOp(int typ, int parm, char *labl)
{
    char    *oldLine;
    Str255  word;

    switch (typ)
    {
        case OP_EQU:
            if (labl[0] == 0)
            {
                ASMX_Error("Missing label");
            }
            else
            {
                int val = EXPR_Eval();

                // allow EQU to 8051 register bit references
                oldLine = linePtr;
                if (TOKEN_GetWord(word) == '.')
                {
                    val = I8051_GetBitReg(val);
                    int val2 = EXPR_Eval();

                    // validate bit index
                    if (val < 0 || val2 < 0 || val2 > 7)
                    {
                        ASMX_IllegalOperand();
                        break;
                    }

                    val = val + val2;
                }
                else
                {
                    linePtr = oldLine;
                }

                sprintf(word, "---- = %.4X", val & 0xFFFF);
                for (int i = 5; i < 11; i++)
                {
                    listLine[i] = word[i];
                }
                SYM_Def(labl, val, /*setSym*/ parm == 1, /*equSym*/ parm == 0);
            }
            break;

        default:
            return 0;
            break;
    }
    return 1;
}


void I8051_AsmInit(void)
{
    void *p = ASMX_AddAsm(versionName, &I8051_DoCPUOpcode, &I8051_DoCPULabelOp, NULL);

    ASMX_AddCPU(p, "8051", 0, END_BIG, ADDR_16, LIST_24, 8, 0, I8051_opcdTab);
    ASMX_AddCPU(p, "8052", 0, END_BIG, ADDR_16, LIST_24, 8, 0, I8051_opcdTab);
    ASMX_AddCPU(p, "8031", 0, END_BIG, ADDR_16, LIST_24, 8, 0, I8051_opcdTab);
    ASMX_AddCPU(p, "8032", 0, END_BIG, ADDR_16, LIST_24, 8, 0, I8051_opcdTab);
}
