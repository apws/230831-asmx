// asm8051.c

#define versionName "8051 assembler"
#include "asmx.h"

enum instrType {
    o_None,     // NOP RET RETI - no operands
    o_AJMP,     // ACALL AJMP - 11-bit jump with A11-A15 unchanged
    o_LJMP,     // LCALL LJMP - 16-bit jump
    o_Rel,      // JC JNC JNZ JZ SJMP - relative jump
    o_BitRel,   // JB JBC JNB - relative jump with bit
    o_DJNZ,     // DJNZ - decrement and relative jump if not zero
    o_Arith,    // ADD ADDC SUBB - imm, @R0, @R1, R0-R7, dir
    o_Logical,  // ANL ORL XRL - imm, @R0, @R1, R0-R7, dir, C-bit, dir-imm, dir-A
    o_AccA,     // DA RL RLC RR RRC SWAP - only accepts A as parameter (should also have o_None aliases)
    o_AccAB,    // DIV MUL - only accepts AB as parameter
    o_MOV,      // MOV - mucho complicated
    o_INC_DEC,  // INC DEC - @R0, @R1, A, R0-R7, dir, and DPTR for INC only
    o_XCHD,     // XCHD A,@R0/@R1
    o_PushPop,  // POP PUSH - parameter is direct address
    o_A_bit_C,  // SETB CLR CPL - accepts A, bit, C, except SETB does not accept A
    o_JMP,      // JMP @A+DPTR
    o_MOVC,     // MOVC A,@A+DPTR/PC
    o_MOVX,     // MOVX A to or from @DPTR, @R0, @R1
    o_CJNE,     // CJNE @R0, @R1, A, R0-R7, dir with immediate and relative operands
    o_XCH       // XCH A with @R0, @R1, R0-R7, dir

//  o_Foo = o_LabelOp,
};

static const struct OpcdRec I8051_opcdTab[] = {
    {"NOP",  o_None,  0x00},
    {"RET",  o_None,  0x22},
    {"RETI", o_None,  0x32},

    {"ACALL",o_AJMP,  0x11},
    {"AJMP", o_AJMP,  0x01},
    {"LCALL",o_LJMP,  0x12},
    {"LJMP", o_LJMP,  0x02},

    {"JC",   o_Rel,   0x40},
    {"JNC",  o_Rel,   0x50},
    {"JNZ",  o_Rel,   0x70},
    {"JZ",   o_Rel,   0x60},
    {"SJMP", o_Rel,   0x80},

    {"JB",   o_BitRel, 0x20},
    {"JBC",  o_BitRel, 0x10},
    {"JNB",  o_BitRel, 0x30},

    {"DJNZ", o_DJNZ,  0x00},

    {"ADD",  o_Arith, 0x20},
    {"ADDC", o_Arith, 0x30},
    {"SUBB", o_Arith, 0x90},

    {"ANL",  o_Logical, 0x8050},
    {"ORL",  o_Logical, 0x7040},
    {"XRL",  o_Logical, 0x0060},

    {"DA",   o_AccA,  0xD4},
    {"RL",   o_AccA,  0x23},
    {"RLC",  o_AccA,  0x33},
    {"RR",   o_AccA,  0x03},
    {"RRC",  o_AccA,  0x13},
    {"SWAP", o_AccA,  0xC4},

    // aliases for o_AccA instructions
//    {"DAA",  o_None, 0xD4},
//    {"RLA",  o_None, 0x23},
//    {"RLCA", o_None, 0x33},
//    {"RRA",  o_None, 0x03},
//    {"RRCA", o_None, 0x13},
//    {"SWAPA",o_None, 0xC4},

    {"DIV",  o_AccAB, 0x84},
    {"MUL",  o_AccAB, 0xA4},

    {"MOV",  o_MOV,   0x00},

    {"INC",  o_INC_DEC, 0xA300},
    {"DEC",  o_INC_DEC, 0x0010},

    {"XCHD", o_XCHD,    0x00},

    {"POP",  o_PushPop, 0xD0},
    {"PUSH", o_PushPop, 0xC0},

    {"CLR",  o_A_bit_C, 0xE4C2},
    {"CPL",  o_A_bit_C, 0xF4B2},
    {"SETB", o_A_bit_C, 0x00D2},

    {"JMP",  o_JMP,  0x00},

    {"MOVC", o_MOVC, 0x00},

    {"MOVX", o_MOVX, 0x00},

    {"CJNE", o_CJNE, 0x00},

    {"XCH",  o_XCH,  0x00},

    {"",    o_Illegal,  0}
};

char regs_8051[] = "@R0 @R1 R0 R1 R2 R3 R4 R5 R6 R7 # A C DPTR @DPTR";

enum {
    reg_xR0   =  0,
    reg_xR1   =  1,
    reg_R0    =  2,
    reg_R1    =  3,
    reg_R2    =  4,
    reg_R3    =  5,
    reg_R4    =  6,
    reg_R5    =  7,
    reg_R6    =  8,
    reg_R7    =  9,
    reg_Imm   = 10,
    reg_A     = 11,
    reg_C     = 12,
    reg_DPTR  = 13,
    reg_xDPTR = 14,
};


// --------------------------------------------------------------


static int Get_8051_Reg(const char *regList)
{
    Str255  word;
    int     token;

    char *oldLine = linePtr;

    if (!(token = GetWord(word))) {
        MissingOperand();
        return reg_EOL;
    }

    // 8051 needs to handle '@' symbols as part of a register name
    if (token == '@') {
        GetWord(word+1);
    }

    token = FindReg(word, regList);
    if (token < 0) {
        // if not in register list, rewind the line pointer
        linePtr = oldLine;
    }
    return token;
}


static int Get_8051_Bit_Reg(int reg)
{
    // Returns the base bit address for a byte address
    //
    // addresses 0x20..0x2F are bits 0x00..0x7F
    // addresses 0xX0 are bits 0xX0..0xX7 (X >= 8)
    // addresses 0xX8 are bits 0xX8..0xXF (X >= 8)
    //
    // all other RAM address are invalid and -1 will be returned

    if (0x20 <= reg && reg <= 0x2F) {
        reg = (reg & 0x1F) * 8;
    } else if ((reg & 0x87) != 0x80) {
        reg = -1;
    }

    return reg;
}


static int EvalBitReg()
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

    int val1 = Eval();
    char *oldLine = linePtr;
    if (GetWord(word) == '.') {
        // evaluate bit number
        int val2 = Eval();

        // determine base bit index
        val1 = Get_8051_Bit_Reg(val1);

        // validate bit index
        if (val1 < 0 || val2 < 0 || val2 > 7) {
            IllegalOperand();
            return 0;
        }

        // return completed bit index
        return val1 + val2;
    }

    // resulting bit index must be 0..255
    linePtr = oldLine;
    if (val1 & 0xFFFFFF00) {
        IllegalOperand();
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

    switch (typ) {
        case o_None:
            InstrB(parm);
            break;

        case o_AJMP:
            val = Eval();
            if ((val & 0xF800) != ((locPtr + 2) & 0xF800)) {
                if (parm == 0x01) {
                    Warning("AJMP out of range");
                } else {
                    Warning("ACALL out of range");
                }
            }
            InstrBB(parm + ((val & 0x0700) >> 3), val & 0xFF);
            break;

        case o_LJMP:
            val = Eval();
            InstrBW(parm, val);
            break;

        case o_Rel:
            val = EvalBranch(2);
            InstrBB(parm, val);
            break;

        case o_BitRel:
            reg1 = EvalBitReg();
            if (reg1 < 0) break;
            Comma();
            val = EvalBranch(3);
            InstrBBB(parm, reg1, val);
            break;

        case o_DJNZ:
            switch ((reg1 = Get_8051_Reg(regs_8051))) {
                case reg_R0:    // Rn
                case reg_R1:
                case reg_R2:
                case reg_R3:
                case reg_R4:
                case reg_R5:
                case reg_R6:
                case reg_R7:
                    Comma();
                    val = EvalBranch(2);
                    InstrBB(0xD8 + reg1 - reg_R0, val);
                    break;

                case reg_None:
                    reg1 = EvalByte();
                    Comma();
                    val = EvalBranch(3);
                    InstrBBB(0xD5, reg1, val);
                    break;

                default:
                    IllegalOperand();
                    break;
            }
            break;

        case o_Arith:
            switch (Get_8051_Reg("A")) {
                default:
                case reg_None:
                    IllegalOperand();
                    break;

                case reg_EOL:
                    break;

                case 0:     // A
                    Comma();
                    switch ((reg1 = Get_8051_Reg(regs_8051))) {
                        default:
                            IllegalOperand();
                            break;

                        case reg_EOL: // EOL
                            break;

                        case reg_None: // A,dir
                            val = EvalByte();
                            InstrBB(parm + 0x05, val);
                            break;

                        case reg_xR0:   // A,@Rn
                        case reg_xR1:
                        case reg_R0:    // A,Rn
                        case reg_R1:
                        case reg_R2:
                        case reg_R3:
                        case reg_R4:
                        case reg_R5:
                        case reg_R6:
                        case reg_R7:
                            InstrB(parm + 0x06 + reg1 - reg_xR0);
                            break;

                        case reg_Imm:   // A,#imm
                            val = EvalByte();
                            InstrBB(parm + 0x04, val);
                            break;
                    }
                    break;
            }
            break;

        case o_Logical:
            switch ((Get_8051_Reg("A C"))) {
                default:
                    IllegalOperand();
                    break;

                case reg_EOL:
                    break;

                case reg_None: // dir
                    val = EvalByte();

                    Comma();
                    switch (Get_8051_Reg("A #")) {
                        default:
                        case reg_None:
                            IllegalOperand();
                            break;

                        case reg_EOL:
                            break;

                        case 0:     // dir,A
                            InstrBB((parm & 0xFF) + 0x02, val);
                            break;

                        case 1:     // dir,#imm
                            reg1 = val;
                            val = EvalByte();
                            InstrBBB((parm & 0xFF) + 0x03, reg1, val);
                            break;
                    }
                    break;

                case 0:     // A
                    Comma();
                    switch ((reg1 = Get_8051_Reg(regs_8051))) {
                        default:
                            IllegalOperand();
                            break;

                        case reg_EOL:
                            break;

                        case reg_None: // A,dir
                            val = EvalByte();
                            InstrBB((parm & 0xFF) + 0x05, val);
                            break;

                        case reg_xR0:   // A,@Rn
                        case reg_xR1:
                        case reg_R0:    // A,Rn
                        case reg_R1:
                        case reg_R2:
                        case reg_R3:
                        case reg_R4:
                        case reg_R5:
                        case reg_R6:
                        case reg_R7:
                            InstrB((parm & 0xFF) + 0x06 + reg1 - reg_xR0);
                            break;

                        case reg_Imm:   // A,#imm
                            val = EvalByte();
                            InstrBB((parm & 0xFF) + 0x04, val);
                            break;
                    }
                    break;

                case 1:     // C
                    if ((parm & 0xFF00) == 0) {
                        // XRL C,bit
                        IllegalOperand();
                        break;
                    }
                    Comma();
                    // "bit" or "/bit"
                    val = 0x02;
                    oldLine = linePtr;
                    token = GetWord(word);
                    if (token == '/') {
                        val = 0x30;
                    } else {
                        linePtr = oldLine;
                    }

                    reg1 = EvalBitReg();
                    if (reg1 < 0) break;
                    InstrBB((parm >> 8) + val, reg1);
                    break;
            }
            break;

        case o_AccA:
            switch (Get_8051_Reg("A")) {
                default:
                case reg_None:
                    IllegalOperand();
                    break;

                case reg_EOL:
                    break;

                case 0:     // A
                    InstrB(parm);
            }
            break;

        case o_AccAB:
            switch (Get_8051_Reg("AB")) {
                default:
                case reg_None:
                    IllegalOperand();
                    break;

                case reg_EOL:
                    break;

                case 0:     // AB
                    InstrB(parm);
                    break;
            }
            break;

        case o_MOV:
            switch ((reg1 = Get_8051_Reg(regs_8051))) {
                case reg_EOL: // EOL
                    break;

                case reg_None:    // dir or bit
                    reg1 = EvalByte();
                    oldLine = linePtr;
                    token = GetWord(word);
                    if (token == '.') { // bit.b,C
                        reg1 = Get_8051_Bit_Reg(reg1);
                        reg2 = EvalByte();

                        if (reg1 < 0 || reg2 < 0 || reg2 > 7) {
                            IllegalOperand();
                            reg1 = 0;
                        } else {
                            reg1 += reg2;
                        }
                        Comma();
                        switch (Get_8051_Reg("C")) {
                            default:
                            case reg_None:
                                IllegalOperand();
                                break;

                            case reg_EOL:
                                break;

                            case 0:     // bit,C
                                InstrBB(0x92, reg1);
                                break;
                        }
                        break;
                    } else if (token != ',') { // dir,
                        oldLine = linePtr;
                        Expect(",");
                        break;
                    }
                    // dir,Rn or bit,C
                    switch ((reg2 = Get_8051_Reg(regs_8051))) {
                        default:
                            IllegalOperand();
                            break;

                        case reg_EOL:
                            break;

                        case reg_None: // dir,dir
                            val = EvalByte(); // evaluate src
                            // Note that the assembly code is "MOV dst,src"
                            // but the object code is "0x85 src dst"
                            InstrBBB(0x85, val, reg1);
                            break;

                        case reg_xR0:   // dir,@Rn
                        case reg_xR1:
                        case reg_R0:    // dir,Rn
                        case reg_R1:
                        case reg_R2:
                        case reg_R3:
                        case reg_R4:
                        case reg_R5:
                        case reg_R6:
                        case reg_R7:
                            InstrBB(0x86 + reg2 - reg_xR0, reg1);
                            break;

                        case reg_A:     // dir,A
                            InstrBB(0xF5, reg1);
                            break;

                        case reg_Imm:   // dir,#imm
                            val = EvalByte();
                            InstrBBB(0x75, reg1, val);
                            break;

                        case reg_C:     // bit,C
                            InstrBB(0x92, reg1);
                            break;
                    }
                    break;

                case reg_xR0:   // @Rn,
                case reg_xR1:
                case reg_R0:    // Rn,
                case reg_R1:
                case reg_R2:
                case reg_R3:
                case reg_R4:
                case reg_R5:
                case reg_R6:
                case reg_R7:
                    Comma();
                    switch (Get_8051_Reg("A #")) {
                        default:
                            IllegalOperand();
                            break;

                        case reg_EOL:
                            break;

                        case reg_None:  // Rn,dir
                            val = EvalByte();
                            InstrBB(0xA6 + reg1, val);
                            break;

                        case 0:         // Rn,A
                            InstrB(0xF6 + reg1);
                            break;

                        case 1:         // Rn,#imm
                            val = EvalByte();
                            InstrBB(0x76 + reg1 - reg_xR0, val);
                            break;
                    }
                    break;

                case reg_A:     // A,
                    Comma();
                    switch ((reg1 = Get_8051_Reg(regs_8051))) {
                        default:
                            IllegalOperand();
                            break;

                        case reg_EOL:
                            break;

                        case reg_None:  // A,dir
                            val = EvalByte();
                            InstrBB(0xE5, val);
                            break;

                        case reg_xR0:   // A,@Rn
                        case reg_xR1:
                        case reg_R0:    // A,Rn
                        case reg_R1:
                        case reg_R2:
                        case reg_R3:
                        case reg_R4:
                        case reg_R5:
                        case reg_R6:
                        case reg_R7:
                            InstrB(0xE6 + reg1 - reg_xR0);
                            break;

                        case reg_Imm:   // A,#imm
                            val = EvalByte();
                            InstrBB(0x74, val);
                            break;
                    }
                    break;

                case reg_C:     // C,bit
                    Comma();
                    reg1 = EvalBitReg();
                    if (reg1 < 0) break;
                    InstrBB(0xA2, reg1);
                    break;

                case reg_DPTR:  // DPTR,#
                    Comma();
                    Expect("#");
                    val = Eval();
                    InstrBW(0x90, val);
                    break;

                default:
                    IllegalOperand();
                    break;
            }
            break;

        case o_INC_DEC:
            switch ((reg1 = Get_8051_Reg(regs_8051))) {
                case reg_EOL: // EOL
                    break;

                case reg_None: // dir
                    val = EvalByte();
                    InstrBB(parm + 0x05, val);
                    break;

                case reg_A:     // A
                    InstrB((parm & 0xFF) + 0x04);
                    break;

                case reg_xR0:   // @Rn,
                case reg_xR1:
                case reg_R0:    // Rn,
                case reg_R1:
                case reg_R2:
                case reg_R3:
                case reg_R4:
                case reg_R5:
                case reg_R6:
                case reg_R7:
                    InstrB((parm & 0xFF) + 0x06 + reg1 - reg_xR0);
                    break;

                case reg_DPTR:  // DPTR
                    if (parm & 0xFF00) {
                        InstrB(parm >> 8);
                    } else {
                        IllegalOperand();
                    }
                    break;

                default:
                    IllegalOperand();
                    break;
            }
            break;

        case o_XCHD:
            switch (Get_8051_Reg("A")) {
                default:
                case reg_None:
                    IllegalOperand();
                    break;

                case reg_EOL:
                    break;

                case 0:     // A
                    Comma();
                    switch ((reg1 = Get_8051_Reg("@R0 @R1"))) {
                        default:
                        case reg_None:
                            IllegalOperand();
                            break;

                        case reg_EOL:
                            break;

                        case 0: // A,@R0
                        case 1: // A,@R1
                            InstrB(0xD6 + reg1);
                            break;
                    }
                    break;
            }
            break;

        case o_PushPop:
            val = Eval();
            InstrBB(parm, val);
            break;

        case o_A_bit_C:
            switch ((reg1 = Get_8051_Reg("A C"))) {
                case 0:     // A
                    if ((parm & 0xFF00) == 0) {
                        IllegalOperand();
                    } else {
                        InstrB(parm >> 8);
                    }
                    break;

                case 1:     // C
                    InstrB((parm & 0xFF) + 1);
                    break;

                case reg_None: // bit
                    reg1 = EvalBitReg();
                    if (reg1 < 0) break;
                    InstrBB(parm & 0xFF, reg1);
                    break;

                default:
                    break;
            }
            break;

        case o_JMP:
            switch (Get_8051_Reg("@A")) {
                default:
                case reg_None:
                    IllegalOperand();
                    break;

                case reg_EOL:
                    break;

                case 0:     // @A
                    Expect("+");
                    switch (Get_8051_Reg("DPTR")) {
                        default:
                        case reg_None:
                            IllegalOperand();
                            break;

                        case reg_EOL:
                            break;

                        case 0:     // @A+DPTR
                            InstrB(0x73);
                            break;
                    }
                    break;
            }
            break;

        case o_MOVC:
            switch (Get_8051_Reg("A")) {
                default:
                case reg_None:
                    IllegalOperand();
                    break;

                case reg_EOL:
                    break;

                case 0:     // A
                    Comma();
                    switch (Get_8051_Reg("@A")) {
                        default:
                        case reg_None:
                            IllegalOperand();
                            break;

                        case reg_EOL:
                            break;

                        case 0:     // A,@A+
                            Expect("+");

                            switch (Get_8051_Reg("PC DPTR")) {
                                default:
                                case reg_None:
                                    IllegalOperand();
                                    break;

                                case reg_EOL:
                                    break;

                                case 0: // A,@A+PC
                                    InstrB(0x83);
                                    break;

                                case 1: // A,@A+DPTR
                                    InstrB(0x93);
                                    break;
                            }
                            break;
                    }
                    break;
            }
            break;

        case o_MOVX:
            switch ((reg1 = Get_8051_Reg("@DPTR A @R0 @R1"))) {
                default:
                case reg_None:
                    IllegalOperand();
                    break;

                case reg_EOL:
                    break;

                case 1:     // A
                    Comma();
                    switch ((reg1 = Get_8051_Reg("@DPTR A @R0 @R1"))) {
                        case 1:     // A,A
                        default:
                        case reg_None:
                            IllegalOperand();
                            break;

                        case reg_EOL:
                            break;

                        case 0:     // A,@DPTR
                        case 2:     // A,@R0
                        case 3:     // A,@R1
                            InstrB(0xE0 + reg1);
                            break;
                    }
                    break;

                case 0:     // @DPTR
                case 2:     // @R0
                case 3:     // @R1
                    Comma();
                    switch (Get_8051_Reg("A")) {
                        default:
                        case reg_None:
                            IllegalOperand();
                            break;

                        case reg_EOL:
                            break;

                        case 0:     // ,A
                            break;
                    }
                    InstrB(0xF0 + reg1);
                    break;
            }
            break;

        case o_CJNE:
            switch ((reg1 = Get_8051_Reg(regs_8051))) {
                case reg_None:
                    IllegalOperand();
                    break;

                case reg_A:     // A
                    Comma();
                    switch ((reg1 = Get_8051_Reg("#"))) {
                        default:
                            IllegalOperand();
                            break;

                        case reg_EOL:
                            break;

                        case reg_None: // A,dir,rel
                            reg1 = EvalByte();
                            Comma();
                            val = EvalBranch(3);
                            InstrBBB(0xB5, reg1, val);
                            break;

                        case 0: // A,#imm,rel
                            reg1 = EvalByte();
                            Comma();
                            val = EvalBranch(3);
                            InstrBBB(0xB4, reg1, val);
                            break;
                    }
                    break;

                case reg_xR0:   // @Rn,#imm,rel
                case reg_xR1:
                case reg_R0:    // Rn,#imm,rel
                case reg_R1:
                case reg_R2:
                case reg_R3:
                case reg_R4:
                case reg_R5:
                case reg_R6:
                case reg_R7:
                    Comma();
                    Expect("#");
                    reg2 = EvalByte();
                    Comma();
                    val = EvalBranch(3);
                    InstrBBB(0xB6 + reg1 - reg_xR0, reg2, val);
                    break;

                default:
                    break;
            }
            break;

        case o_XCH:
            switch (Get_8051_Reg("A")) {
                default:
                case reg_None:
                    IllegalOperand();
                    break;

                case reg_EOL:
                    break;

                case 0:     // A
                    Comma();
                    switch ((reg1 = Get_8051_Reg(regs_8051))) {
                        default:
                            IllegalOperand();
                            break;

                        case reg_EOL:
                            break;

                        case reg_None: // dir
                            val = EvalByte();
                            InstrBB(0xC5, val);
                            break;

                        case reg_xR0:   // A,@Rn
                        case reg_xR1:
                        case reg_R0:    // A,Rn
                        case reg_R1:
                        case reg_R2:
                        case reg_R3:
                        case reg_R4:
                        case reg_R5:
                        case reg_R6:
                        case reg_R7:
                            InstrB(0xC6 + reg1 - reg_xR0);
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

    switch (typ) {
        case o_EQU:
            if (labl[0] == 0) {
                Error("Missing label");
            } else {
                int val = Eval();

                // allow EQU to 8051 register bit references
                oldLine = linePtr;
                if (GetWord(word) == '.') {
                    val = Get_8051_Bit_Reg(val);
                    int val2 = Eval();

                    // validate bit index
                    if (val < 0 || val2 < 0 || val2 > 7) {
                        IllegalOperand();
                        break;
                    }

                    val = val + val2;
                } else {
                    linePtr = oldLine;
                }

                sprintf(word, "---- = %.4X", val & 0xFFFF);
                for (int i = 5; i < 11; i++) {
                    listLine[i] = word[i];
                }
                DefSym(labl, val, /*setSym*/ parm == 1, /*equSym*/ parm == 0);
            }
            break;

        default:
            return 0;
            break;
    }
    return 1;
}


void Asm8051Init(void)
{
    void *p = AddAsm(versionName, &I8051_DoCPUOpcode, &I8051_DoCPULabelOp, NULL);

    AddCPU(p, "8051", 0, BIG_END, ADDR_16, LIST_24, 8, 0, I8051_opcdTab);
    AddCPU(p, "8052", 0, BIG_END, ADDR_16, LIST_24, 8, 0, I8051_opcdTab);
    AddCPU(p, "8031", 0, BIG_END, ADDR_16, LIST_24, 8, 0, I8051_opcdTab);
    AddCPU(p, "8032", 0, BIG_END, ADDR_16, LIST_24, 8, 0, I8051_opcdTab);
}
