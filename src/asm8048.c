// asmI8048.c

#define versionName "I8048 assembler"
#include "asmx.h"

enum
{
    CPU_8048,
    CPU_8041,
    CPU_8021,
    CPU_8022
};

enum instrType
{
    OP_None,         // No operands

    OP_Arith,        // ADD, ADDC
    OP_Logical,      // ANL, ORL, XRL, XCH
    OP_XCHD,         // XCHD
    OP_JMP,          // JMP, CALL
    OP_RET,          // RET, RETR
    OP_Branch,       // Jcc
    OP_DJNZ,         // DJNZ
    OP_INC_DEC,      // INC, DEC
    OP_MOV,          // MOV
    OP_MOVD,         // MOVD
    OP_LogicalD,     // ANLD, ORLD
    OP_CLR_CPL,      // CLR, CPL
    OP_Accum,        // DA A, RL A, RLC A, RRC A, RR A, SWAP A
    OP_EN_DIS,       // EN, DIS
    OP_MOVX,         // MOVX
    OP_ENT0,         // ENT0 CLK
    OP_MOVP,         // MOVP MOVP3
    OP_JMPP,         // JMPP @A
    OP_STOP,         // STOP TCNT
    OP_STRT,         // STRT CNT/T
    OP_SEL,          // SEL
    OP_IN,           // IN
    OP_INS,          // INS
    OP_OUTL,         // OUTL

//  o_Foo = OP_LabelOp,
};

// Note: there seem to be three primary variations of the instruction set:
// 8048/8049/8050/8035/8039/8040 is the primary variant
// 8021/8022 doesn't have SEL MBx, SEL RBx, MOVX, and a few others
// 8041/8042 doesn't have SEL MBx or MOVX, and some other slight differences
// Oki and Toshiba also have some slight variations.
// These differences are currently handled in the simplest possible way:
// allow all that don't conflict with the main instruction set.

static const struct OpcdRec I8048_opcdTab[] =
{
    {"NOP",  OP_None,  0x00},
    {"HALT", OP_None,  0x01}, // on Oki MSM80Cxx and Toshiba 8048
    {"RET",  OP_RET,   0x83}, // note: 8022 may want "RET I" instead of RETR
    {"RETR", OP_RET,   0x93},
    {"HLTS", OP_None,  0x82}, // on Oki MSM80Cxx
    {"FLT",  OP_None,  0xA2}, // on Oki MSM80Cxx
    {"FLTT", OP_None,  0xC2}, // on Oki MSM80Cxx

    {"ADD",  OP_Arith, 0x00},
    {"ADDC", OP_Arith, 0x10},

    {"ANL",  OP_Logical, 0x50}, // no ports on 8022?
    {"ORL",  OP_Logical, 0x40}, // no ports on 8022?
    {"XRL",  OP_Logical, 0xD0}, // no ports
    {"XCH",  OP_Logical, 0x20}, // no ports or immediate

    {"XCHD", OP_XCHD,    0x30},

    {"JMP",  OP_JMP,     0x04},
    {"CALL", OP_JMP,     0x14},

    {"JB0",  OP_Branch,  0x12}, // 8048 only
    {"JB1",  OP_Branch,  0x32}, // 8048 only
    {"JB2",  OP_Branch,  0x52}, // 8048 only
    {"JB3",  OP_Branch,  0x72}, // 8048 only
    {"JB4",  OP_Branch,  0x92}, // 8048 only
    {"JB5",  OP_Branch,  0xB2}, // 8048 only
    {"JB6",  OP_Branch,  0xD2}, // 8048 only
    {"JB7",  OP_Branch,  0xF2}, // 8048 only

    {"JTF",  OP_Branch,  0x16},
    {"JNT0", OP_Branch,  0x26}, // not 8021?
    {"JT0",  OP_Branch,  0x36}, // not 8021?
    {"JNT1", OP_Branch,  0x46},
    {"JT1",  OP_Branch,  0x56},
    {"JF1",  OP_Branch,  0x76}, // not 8021/8022?
    {"JNI",  OP_Branch,  0x86}, // not 8021/8022?
    {"JF0",  OP_Branch,  0xb6}, // not 8021/8022?

    {"JZ",   OP_Branch,  0xC6},
    {"JNZ",  OP_Branch,  0x96},
    {"JC",   OP_Branch,  0xF6},
    {"JNC",  OP_Branch,  0xE6},

    {"DJNZ", OP_DJNZ,    0xE8},

    {"INC",  OP_INC_DEC, 0x10},
    {"DEC",  OP_INC_DEC, 0xC0},

    {"MOV",  OP_MOV,     0x00},

    {"MOVD", OP_MOVD,    0x00},

    {"ANLD", OP_LogicalD,0x9C},
    {"ORLD", OP_LogicalD,0x8C},

    {"CLR",  OP_CLR_CPL, 0x00},
    {"CPL",  OP_CLR_CPL, 0x10},

    {"SWAP", OP_Accum,   0x47},
    {"DA",   OP_Accum,   0x57},
    {"RRC",  OP_Accum,   0x67},
    {"RR",   OP_Accum,   0x77},
    {"RL",   OP_Accum,   0xE7},
    {"RLC",  OP_Accum,   0xF7},

    {"EN",   OP_EN_DIS,  0x00},
    {"DIS",  OP_EN_DIS,  0x10},

    {"MOVX", OP_MOVX,    0x00}, // 8048 only

    {"ENT0", OP_ENT0,    0x00}, // 8048 only

    {"MOVP", OP_MOVP,    0xA3},
    {"MOVP3",OP_MOVP,    0xE3},

    {"JMPP", OP_JMPP,    0xB3},

    {"STOP", OP_STOP,    0x00},

    {"STRT", OP_STRT,    0x00},

    {"SEL",  OP_SEL,     0x00},

    {"IN",   OP_IN,      0x00},
    {"INS",  OP_INS,     0x00},
    {"OUTL", OP_OUTL,    0x00}, // OUTL DBB,A is 0x90 on 8041/8021/8022


    {"",    OP_Illegal,  0}
};


char regs_8048[] = "R0 R1 R2 R3 R4 R5 R6 R7 @R0 @R1 # A PSW T";

enum
{
    REG_R0    =  0,
    REG_R1    =  1,
    REG_R2    =  2,
    REG_R3    =  3,
    REG_R4    =  4,
    REG_R5    =  5,
    REG_R6    =  6,
    REG_R7    =  7,
    REG_xR0   =  8,
    REG_xR1   =  9,
    REG_Imm   = 10,
    REG_A     = 11,
    REG_PSW   = 12,
    REG_T     = 13,
};


// This tracks the last SEL MB page. It is reset to -1 after an
// unconditional JMP or RET instruction. If it is unknown, the current
// page is used instead.
// This can be subverted through bad programming style, but should be
// sufficient if SEL MB is always set before long jumps/calls. (And
// presumably set back after long calls as well.)

int selmb;

// --------------------------------------------------------------


static int I8048_GetReg(const char *regList)
{
    Str255  word;
    int     token;

    if (!(token = TOKEN_GetWord(word)))
    {
        ASMX_MissingOperand();
        return reg_EOL;
    }

    // 8048 needs to handle '@' symbols as part of a register name
    if (token == '@')
    {
        TOKEN_GetWord(word+1);
    }

    return REG_Find(word, regList);
}


static int I8048_DoCPUOpcode(int typ, int parm)
{
    int     val, reg1, reg2;
//  Str255  word;
//  char    *oldLine;
//  int     token;

    switch (typ)
    {
        case OP_RET:
            // reset selmb after unconditional returns
            selmb = -1;
        // fall through...
        case OP_None:
            INSTR_B(parm);
            break;

        case OP_Arith:
            if (TOKEN_Expect("A")) break;
            if (TOKEN_Comma()) break;
            reg1 = I8048_GetReg(regs_8048);
            switch (reg1)
            {
                case REG_R0: // A,Rn = parm + 0x68 + reg
                case REG_R1:
                case REG_R2:
                case REG_R3:
                case REG_R4:
                case REG_R5:
                case REG_R6:
                case REG_R7:
                    INSTR_B(parm + 0x68 + reg1 - REG_R0);
                    break;

                case REG_xR0: // A,@Rn = parm + 0x60 + reg
                case REG_xR1:
                    INSTR_B(parm + 0x60 + reg1 - REG_xR0);
                    break;

                case REG_Imm: // A,#imm = parm + 0x03
                    val = EXPR_EvalByte();
                    INSTR_BB(parm + 0x03, val);
                    break;

                case reg_EOL:
                    break;

                default:
                    ASMX_IllegalOperand();
                    break;
            }
            break;

        case OP_Logical:
            reg1 = I8048_GetReg("A P1 P2 BUS");
            switch (reg1)
            {
                case 0:
                    if (TOKEN_Comma()) break;
                    reg1 = I8048_GetReg(regs_8048);
                    switch (reg1)
                    {
                        case REG_R0: // A,Rn = parm + 0x08 + reg
                        case REG_R1:
                        case REG_R2:
                        case REG_R3:
                        case REG_R4:
                        case REG_R5:
                        case REG_R6:
                        case REG_R7:
                            INSTR_B(parm + 0x08 + reg1 - REG_R0);
                            break;

                        case REG_xR0: // A,@Rn = parm + reg
                        case REG_xR1:
                            INSTR_B(parm + reg1 - REG_xR0);
                            break;

                        case REG_Imm: // A,#imm = parm + 0x03 (not 0x20)
                            if (parm == 0x20)
                            {
                                ASMX_IllegalOperand();
                                break;
                            }
                            val = EXPR_EvalByte();
                            INSTR_BB(parm + 0x03, val);
                            break;

                        case reg_EOL:
                            break;

                        default:
                            ASMX_IllegalOperand();
                            break;
                    }
                    break;

                case 1: // Pn,#imm = parm + 0x48 + port (not 0xD0 or 0x20)
                case 2:
                    if (TOKEN_Comma()) break;
                    if (TOKEN_Expect("#")) break;
                    val = EXPR_EvalByte();
                    if (parm == 0xD0 || parm == 0x20)
                    {
                        ASMX_IllegalOperand();
                        break;
                    }
                    INSTR_BB(parm + 0x48 + reg1, val);
                    break;

                case 3: // BUS,#imm = parm + 0x48 (not 0xD0 or 0x20)
                    if (TOKEN_Comma()) break;
                    if (TOKEN_Expect("#")) break;
                    val = EXPR_EvalByte();
                    if (parm == 0xD0 || parm == 0x20)
                    {
                        ASMX_IllegalOperand();
                        break;
                    }
                    INSTR_BB(parm + 0x48, val);
                    break;

                case reg_EOL:
                    break;

                default:
                    ASMX_IllegalOperand();
                    break;
            }
            break;

        case OP_XCHD:
            if (TOKEN_Expect("A")) break;
            if (TOKEN_Comma()) break;
            reg1 = I8048_GetReg("@R0 @R1");
            switch (reg1)
            {
                case 0: // A,@Rn = parm + reg
                case 1:
                    INSTR_B(parm + reg1);
                    break;

                case reg_EOL:
                    break;

                default:
                    ASMX_IllegalOperand();
                    break;
            }
            break;

        case OP_JMP:
            val = EXPR_Eval();

            // check for jumps to the other SEL MB area
            if (selmb == -1)
            {
                // if no SEL MBx seen recently, assume PC is correct
                if ((val & 0xF800) != ((locPtr + 2) & 0xF800))
                {
                    ASMX_Warning("Jump across code bank boundary");
                }
            }
            else
            {
                // if SEL MBx has been seen, use it for bank number
                if (selmb != ((val & 0xF800) >> 11))
                {
                    ASMX_Warning("Jump across code bank boundary");
                }
            }

            // reset selmb after unconditional jumps
            if (parm == 0x04)
            {
                selmb = -1;
            }

            INSTR_BB(parm + ((val & 0x0700) >> 3), val & 0xFF);
            break;

        case OP_Branch:
            val = EXPR_Eval();
            if ((val & 0xFF00) != ((locPtr + 2) & 0xFF00))
            {
                ASMX_Warning("Branch out of range");
            }
            INSTR_BB(parm, val & 0xFF);
            break;

        case OP_DJNZ:
            reg1 = I8048_GetReg(regs_8048);
            switch (reg1)
            {
                case REG_R0: // Rn,addr = parm + reg
                case REG_R1:
                case REG_R2:
                case REG_R3:
                case REG_R4:
                case REG_R5:
                case REG_R6:
                case REG_R7:
                    if (TOKEN_Comma()) break;
                    val = EXPR_Eval();
                    if ((val & 0xFF00) != ((locPtr + 2) & 0xFF00))
                    {
                        ASMX_Warning("Branch out of range");
                    }
                    INSTR_BB(parm + reg1 - REG_R0, val & 0xFF);
                    break;

                case reg_EOL:
                    break;

                default:
                    ASMX_IllegalOperand();
                    break;

            }
            break;

        case OP_INC_DEC:
            reg1 = I8048_GetReg(regs_8048);
            switch (reg1)
            {
                case REG_R0: // Rn = parm + 0x08 + reg (no 0xC0 on 8021/8022)
                case REG_R1:
                case REG_R2:
                case REG_R3:
                case REG_R4:
                case REG_R5:
                case REG_R6:
                case REG_R7:
                    INSTR_B(parm + 0x08 + reg1 - REG_R0);
                    break;

                case REG_xR0: // @Rn = parm + reg
                case REG_xR1:
                    INSTR_B(parm + reg1 - REG_xR0);
                    break;

                case REG_A: // A = (parm & 0x10) + 0x07
                    INSTR_B((parm & 0x10) + 0x07);
                    break;

                case reg_EOL:
                    break;

                default:
                    ASMX_IllegalOperand();
                    break;
            }
            break;

        case OP_MOV:
            reg1 = I8048_GetReg(regs_8048);
            switch (reg1)
            {
                case REG_R0: // Rn,
                case REG_R1:
                case REG_R2:
                case REG_R3:
                case REG_R4:
                case REG_R5:
                case REG_R6:
                case REG_R7:
                    if (TOKEN_Comma()) break;
                    reg2 = I8048_GetReg("A #");
                    switch (reg2)
                    {
                        case 0: // Rn,A = 0xA8 + reg1
                            INSTR_B(0xA8 + reg1 - REG_R0);
                            break;

                        case 1: // Rn,#imm = 0xB8 + reg1
                            val = EXPR_EvalByte();
                            INSTR_BB(0xB8 + reg1 - REG_R0, val);
                            break;

                        case reg_EOL:
                            break;

                        default:
                            ASMX_IllegalOperand();
                            break;
                    }
                    break;

                case REG_xR0: // @Rn,
                case REG_xR1:
                    if (TOKEN_Comma()) break;
                    reg2 = I8048_GetReg("A #");
                    switch (reg2)
                    {
                        case 0: // @Rn,A = 0xA0 + reg1
                            INSTR_B(0xA0 + reg1 - REG_xR0);
                            break;

                        case 1: // @Rn,#imm = 0xB0 + reg1
                            val = EXPR_EvalByte();
                            INSTR_BB(0xB0 + reg1 - REG_xR0, val);
                            break;

                        case reg_EOL:
                            break;

                        default:
                            ASMX_IllegalOperand();
                            break;
                    }
                    break;

                case REG_A: // A,
                    if (TOKEN_Comma()) break;
                    reg2 = I8048_GetReg(regs_8048);
                    switch (reg2)
                    {
                        case REG_R0: // A,Rn = 0xF8 + reg2
                        case REG_R1:
                        case REG_R2:
                        case REG_R3:
                        case REG_R4:
                        case REG_R5:
                        case REG_R6:
                        case REG_R7:
                            INSTR_B(0xF8 + reg2 - REG_R0);
                            break;

                        case REG_xR1: // @Rn = 0xF0 + reg2
                        case REG_xR0:
                            INSTR_B(0xF0 + reg2 - REG_xR0);
                            break;

                        case REG_Imm: // A,#imm
                            val = EXPR_EvalByte();
                            INSTR_BB(0x23, val);
                            break;

                        case REG_PSW: // A,PSW
                            INSTR_B(0xC7);
                            break;

                        case REG_T: // A,T
                            INSTR_B(0x42);
                            break;

                        case reg_EOL:
                            break;

                        default:
                            ASMX_IllegalOperand();
                            break;
                    }
                    break;

                case REG_PSW: // PSW,A
                    if (TOKEN_Comma()) break;
                    if (TOKEN_Expect("A")) break;
                    INSTR_B(0xD7);
                    break;

                case REG_T: // T,A
                    if (TOKEN_Comma()) break;
                    if (TOKEN_Expect("A")) break;
                    INSTR_B(0x62);
                    break;

                case reg_EOL:
                    break;

                default:
                    ASMX_IllegalOperand();
                    break;
            }
            break;

        case OP_MOVD:
            reg1 = I8048_GetReg("P4 P5 P6 P7 A");
            switch (reg1)
            {
                case 0: // Pn,A = 0x3C + reg
                case 1:
                case 2:
                case 3:
                    if (TOKEN_Comma()) break;
                    if (TOKEN_Expect("A")) break;
                    INSTR_B(0x3C + reg1);
                    break;

                case 4: // A,Pn
                    if (TOKEN_Comma()) break;
                    reg1 = I8048_GetReg("P4 P5 P6 P7");
                    switch (reg1)
                    {
                        case 0: // A,Pn = 0x0C + reg
                        case 1:
                        case 2:
                        case 3:
                            INSTR_B(0x0C + reg1);
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

        case OP_LogicalD:
            reg1 = I8048_GetReg("P4 P5 P6 P7");
            switch (reg1)
            {
                case 0: // Pn,A = parm + reg
                case 1:
                case 2:
                case 3:
                    if (TOKEN_Comma()) break;
                    if (TOKEN_Expect("A")) break;
                    INSTR_B(parm + reg1);
                    break;

                case reg_EOL:
                    break;

                default:
                    ASMX_IllegalOperand();
                    break;
            }
            break;

        case OP_CLR_CPL:
            reg1 = I8048_GetReg("A F0 C F1");
            switch (reg1)
            {
                case 0: // A  = parm + 0x27
                    INSTR_B(parm + 0x27);
                    break;

                case 1: // F0 = parm + 0x85 (not on 8021/8022)
                    INSTR_B(parm + 0x85);
                    break;

                case 2: // C  = parm + 0x97
                    INSTR_B(parm + 0x97);
                    break;

                case 3: // F1 = parm + 0xA5 (not on 8021/8022)
                    INSTR_B(parm + 0xA5);
                    break;

                case reg_EOL:
                    break;

                default:
                    ASMX_IllegalOperand();
                    break;
            }
            break;

        case OP_Accum:
            if (TOKEN_Expect("A")) break;
            INSTR_B(parm);
            break;

        case OP_EN_DIS:
            reg1 = I8048_GetReg("I TCNTI");
            switch (reg1)
            {
                case 0: // I  = parm + 0x05
                    INSTR_B(parm + 0x05);
                    break;

                case 1: // TCNTI = parm + 0x25
                    INSTR_B(parm + 0x25);
                    break;

                case reg_EOL:
                    break;

                default:
                    ASMX_IllegalOperand();
                    break;
            }
            break;

        case OP_MOVX: // 8048 only
            reg1 = I8048_GetReg("@R0 @R1 A");
            switch (reg1)
            {
                case 0: // @Rn,A = 0x90 + reg
                case 1:
                    if (TOKEN_Comma()) break;
                    if (TOKEN_Expect("A")) break;
                    INSTR_B(0x90 + reg1);
                    break;

                case 2: // A,@Rn
                    if (TOKEN_Comma()) break;
                    reg1 = I8048_GetReg("@R0 @R1");
                    switch (reg1)
                    {
                        case 0: // @A,Rn = 0x80 + reg
                        case 1:
                            INSTR_B(0x80 + reg1);
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

        case OP_ENT0: // 8048 only
            reg1 = I8048_GetReg("CLK");
            switch (reg1)
            {
                case 0: // CLK
                    INSTR_B(0x75);
                    break;

                case reg_EOL:
                    break;

                default:
                    ASMX_IllegalOperand();
                    break;
            }
            break;

        case OP_MOVP:
            if (TOKEN_Expect("A")) break;
            if (TOKEN_Comma()) break;
        // fall through...
        case OP_JMPP:
            // reset selmb after unconditional jumps
            if ((typ == OP_JMPP) && (parm == 0xB3))
            {
                selmb = -1;
            }

            reg1 = I8048_GetReg("@A");
            switch (reg1)
            {
                case 0: // @A
                    INSTR_B(parm);
                    break;

                case reg_EOL:
                    break;

                default:
                    ASMX_IllegalOperand();
                    break;
            }
            break;

        case OP_STOP:
            reg1 = I8048_GetReg("TCNT");
            switch (reg1)
            {
                case 0: // TCNT
                    INSTR_B(0x65);
                    break;

                case reg_EOL:
                    break;

                default:
                    ASMX_IllegalOperand();
                    break;
            }
            break;

        case OP_STRT:
            reg1 = I8048_GetReg("CNT T");
            switch (reg1)
            {
                case 0: // CNT
                    INSTR_B(0x45);
                    break;

                case 1: // T
                    INSTR_B(0x55);
                    break;

                case reg_EOL:
                    break;

                default:
                    ASMX_IllegalOperand();
                    break;
            }
            break;

        case OP_SEL:
            reg1 = I8048_GetReg("RB0 RB1 MB0 MB1");
            switch (reg1)
            {
                case 2: // MB0
                case 3: // MB1
                    // remember last SEL MBx
                    selmb = reg1 - 2;
                    break;
            }
            switch (reg1)
            {
                case 0: // RB0 = 0xC5 not 8022?
                case 1: // RB1 = 0xD5 not 8022?
                case 2: // MB0 = 0xE5 8048 only
                case 3: // MB1 = 0XF5 8048 only
                    INSTR_B(0xC5 + reg1*16);
                    break;

                case reg_EOL:
                    break;

                default:
                    ASMX_IllegalOperand();
                    break;
            }
            break;

        case OP_IN:
        case OP_INS:
            if (TOKEN_Expect("A")) break;
            if (TOKEN_Comma()) break;
            reg1 = I8048_GetReg("BUS P1 P2");
            switch (reg1)
            {
                case 0: // A,BUS
                    if (typ == OP_IN)
                    {
                        ASMX_IllegalOperand();
                        break;
                    }
                    INSTR_B(0x08);
                    break;

                case 1: // A,P1
                case 2: // A,P2
                    if (typ != OP_IN)
                    {
                        ASMX_IllegalOperand();
                        break;
                    }
                    INSTR_B(0x08 + reg1);
                    break;

                case reg_EOL:
                    break;

                default:
                    ASMX_IllegalOperand();
                    break;
            }
            break;

        case OP_OUTL:
            reg1 = I8048_GetReg("BUS P1 P2");
            switch (reg1)
            {
                case 0: // BUS,A
                    if (TOKEN_Comma()) break;
                    if (TOKEN_Expect("A")) break;
                    INSTR_B(0x02); // 0x90 for 8041/8021/8022 "OUTL DBB,A"
                    break;

                case 1: // P1,A
                case 2: // P2,A
                    if (TOKEN_Comma()) break;
                    if (TOKEN_Expect("A")) break;
                    INSTR_B(0x38 + reg1);
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


void I8048_PassInit(void)
{
    // start each pass with last selmb page undefined
    selmb = -1;
}


void I8048_AsmInit(void)
{
    void *p = ASMX_AddAsm(versionName, &I8048_DoCPUOpcode, NULL, &I8048_PassInit);

    ASMX_AddCPU(p, "8048",  CPU_8048, END_LITTLE, ADDR_16, LIST_24, 8, 0, I8048_opcdTab);
//  ASMX_AddCPU(p, "8041",  CPU_8041, LITTLE_END, ADDR_16, LIST_24, 8, 0, I8048_opcdTab);
//  ASMX_AddCPU(p, "8021",  CPU_8021, LITTLE_END, ADDR_16, LIST_24, 8, 0, I8048_opcdTab);
//  ASMX_AddCPU(p, "8022",  CPU_8022, LITTLE_END, ADDR_16, LIST_24, 8, 0, I8048_opcdTab);
}
