// asmTHUMB.c

#define versionName "ARM THUMB assembler"
#include "asmx.h"

enum
{
    OP_TwoOp,        // two-operand arithmetic/logical instructions
    OP_ADD,          // ADD opcode
    OP_SUB,          // SUB opcode
    OP_Shift,        // ASR/LSL/LSR
    OP_Immed8,       // BKPT/SWI
    OP_Branch,       // B
    OP_BranchCC,     // Bcc
    OP_BLX,          // BLX
    OP_BL,           // BL
    OP_BX,           // BX
    OP_CMP_MOV,      // CMP/MOV
    OP_LDMIA,        // LDMIA/STMIA
    OP_LDR,          // LDR/STR
    OP_LDRBH,        // LDRB/LDRH/STRB/STRH
    OP_LDRSBH,       // LDRSB/LDRSH
    OP_PUSH_POP,     // PUSH/POP
    OP_Implied,      // NOP

//  o_Foo = OP_LabelOp,
};

const char regs_0_7 [] = "R0 R1 R2 R3 R4 R5 R6 R7";
const char regs_0_7L[] = "R0 R1 R2 R3 R4 R5 R6 R7 R14 LR"; // for PUSH/POP
const char regs_0_15[] = "R0 R1 R2 R3 R4 R5 R6 R7 R8 R9 R10 R11 R12 R13 R14 R15 SP LR PC";

enum
{
    REG_R0 = 0,
    REG_R7 = 7,
    REG_SP = 13,
    REG_LR = 14,
    REG_PC = 15,
};

static const struct OpcdRec THUMB_opcdTab[] =
{
    {"ADC",   OP_TwoOp,      0x4140},
    {"AND",   OP_TwoOp,      0x4000},
    {"BIC",   OP_TwoOp,      0x4380},
    {"CMN",   OP_TwoOp,      0x42C0},
    {"EOR",   OP_TwoOp,      0x4040},
    {"MUL",   OP_TwoOp,      0x4340},
    {"MVN",   OP_TwoOp,      0x43C0},
    {"NEG",   OP_TwoOp,      0x4240},
    {"ORR",   OP_TwoOp,      0x4300},
    {"ROR",   OP_TwoOp,      0x41C0},
    {"SBC",   OP_TwoOp,      0x4180},
    {"TST",   OP_TwoOp,      0x4200},

    {"ADD",   OP_ADD,        0},
    {"SUB",   OP_SUB,        0},

    {"ASR",   OP_Shift,      0x10004100}, // immed5; two-reg
    {"ASL",   OP_Shift,      0x00004080},
    {"LSL",   OP_Shift,      0x00004080},
    {"LSR",   OP_Shift,      0x080040C0},

    {"BKPT",  OP_Immed8,     0xBE00},
    {"SWI",   OP_Immed8,     0xDF00},

    {"B",     OP_Branch,     0},

    {"BEQ",   OP_BranchCC,   0xD000},
    {"BNE",   OP_BranchCC,   0xD100},
    {"BCS",   OP_BranchCC,   0xD200},
    {"BHS",   OP_BranchCC,   0xD200},
    {"BCC",   OP_BranchCC,   0xD300},
    {"BLO",   OP_BranchCC,   0xD300},
    {"BMI",   OP_BranchCC,   0xD400},
    {"BPL",   OP_BranchCC,   0xD500},
    {"BVS",   OP_BranchCC,   0xD600},
    {"BVC",   OP_BranchCC,   0xD700},
    {"BHI",   OP_BranchCC,   0xD800},
    {"BLS",   OP_BranchCC,   0xD900},
    {"BGE",   OP_BranchCC,   0xDA00},
    {"BLT",   OP_BranchCC,   0xDB00},
    {"BGT",   OP_BranchCC,   0xDC00},
    {"BLE",   OP_BranchCC,   0xDD00},
    {"BAL",   OP_BranchCC,   0xDE00},
//  {"BNV",   OP_BranchCC,   0xDF00}, // ILLEGAL - opcode reused for SWI!

    {"BL",    OP_BL,         0xF800},
    {"BLX",   OP_BLX,        0xE800},

    {"BX",    OP_BX,         0x4700},

    {"CMP",   OP_CMP_MOV,    0x28428045}, // high bytes of immed8; two-reg; high-reg
    {"MOV",   OP_CMP_MOV,    0x201C0046},

    {"LDMIA", OP_LDMIA,      0xC800},
    {"STMIA", OP_LDMIA,      0xC000},

    {"LDR",   OP_LDR,        0x0800},
    {"STR",   OP_LDR,        0x0000},

    {"LDRB",  OP_LDRBH,      0x78005C00}, // immed offset; reg offset
    {"LDRH",  OP_LDRBH,      0x88005A00},
    {"STRB",  OP_LDRBH,      0x70005400},
    {"STRH",  OP_LDRBH,      0x80005200},

    {"LDRSB", OP_LDRSBH,     0x5600},
    {"LDRSH", OP_LDRSBH,     0x5E00},

    {"PUSH",  OP_PUSH_POP,   0xBC00},
    {"POP",   OP_PUSH_POP,   0xB400},

    {"NOP",   OP_Implied,    0x46C0},

    {"",      OP_Illegal,    0}
};

// --------------------------------------------------------------


static void THUMB_SetMultiReg(int reg, uint16_t *regbits, bool *warned)
{
    if (!*warned && *regbits & (1 << reg))
    {
        ASMX_Warning("register specified twice");
        *warned = true;
    }
    *regbits |= 1 << reg;
}


static int THUMB_GetMultiRegs(bool useLR, uint16_t *regbits)
{
    Str255  word;

    *regbits = 0;
    bool warned = false;

    // looking for {r0,r2-r5,r6,lr}

    if (TOKEN_Expect("{")) return 1;

    // FIXME: at least one reg must be specified

    char *oldLine = linePtr;
    int token = ',';
    while (token == ',')
    {
        int reg1 = REG_Get(regs_0_7L);
        if (reg1 == 9) reg1 = 8; // LR/R14 -> bit 8

        if ((reg1 > 7) && !useLR)   // error if LR not allowed
        {
            ASMX_IllegalOperand();
            break;
        }

        if (reg1 < 0)
        {
            ASMX_IllegalOperand();      // abort if not valid register
            break;
        }

        // set single register bit
        THUMB_SetMultiReg(reg1, regbits, &warned);

        // check for - or ,
        oldLine = linePtr;
        token = TOKEN_GetWord(word);

        if (token == '-')       // register range
        {
            int reg2 = REG_Get(regs_0_7L); // check for second register
            oldLine = linePtr;  // commit line position
            if (reg2 == 9) reg2 = 8; // LR/R14 -> bit 8

            if ((reg2 > 7) && !useLR)   // error if LR not allowed
            {
                ASMX_IllegalOperand();
                break;
            }

            if (reg2 < 0)
            {
                ASMX_IllegalOperand();      // abort if not valid register
                break;
            }
            if (reg1 < reg2)
            {
                for (int i = reg1 + 1; i <= reg2; i++)
                {
                    THUMB_SetMultiReg(i, regbits, &warned);
                }
            }
            else if (reg1 > reg2)
            {
                for (int i = reg1 - 1; i >= reg2; i--)
                {
                    THUMB_SetMultiReg(i, regbits, &warned);
                }
            }
            oldLine = linePtr;  // commit line position
            token = TOKEN_GetWord(word);
        }
        if (token == ',')   // is there another register?
        {
            oldLine = linePtr;  // commit line position
        }
    }
    linePtr = oldLine;

    TOKEN_Expect("}");

    return 0;
}


// get two registers R0-R7, returns -1 if error
static uint32_t THUMB_TwoRegs()
{
    int reg1 = REG_Get(regs_0_7);
    if (REG_Check(reg1)) return -1;

    if (TOKEN_Comma()) return -1;

    int reg2 = REG_Get(regs_0_7);
    if (REG_Check(reg2)) return -1;

    return (reg2 << 3) | reg1;
}


static int THUMB_EvalBranch(int width, int instrLen) // instrLen should be 4
{
    long limit = (1 << width) - 1;

    long val = EXPR_Eval();
    val = val - locPtr - instrLen;
    if (!errFlag && ((val & 1) || val < ~limit || val > limit))
    {
        ASMX_Error("Long branch out of range");
    }

    return val;
}


static int THUMB_DoCPUOpcode(int typ, int parm)
{
    int     val, reg1, reg2;
    Str255  word;
    char    *oldLine;
    int     token;
    uint16_t regbits;

    switch (typ)
    {
        case OP_TwoOp:        // two-operand arithmetic/logical instructions
            if ((val = THUMB_TwoRegs()) < 0) break;
            INSTR_W(parm | val);
            break;

        case OP_ADD:          // ADD opcode
            reg1 = REG_Get(regs_0_15);
            if (reg1 > 15) reg1 = reg1 - 3; // SP LR PC -> R13 R14 R15
            if (REG_Check(reg1)) break;

            if (TOKEN_Comma()) break;

            oldLine = linePtr;
            token = TOKEN_GetWord(word);
            if (token == '#')
            {
                // Rd,#immed8 / SP,#immed7
                if (reg1 <= 7)
                {
                    val = EXPR_EvalByte();
                    INSTR_W(0x3000 | (reg1 << 8) | (val & 0xFF));
                }
                else
                {
                    val = EXPR_Eval();
                    // FIXME: check immed7 range
                    INSTR_W(0xD000 | ((val >> 2) & 0x7F));
                }
            }
            else
            {
                // Rd,Rn,#immed3 / Rd,Rn,Rm / RD,RM / Rd,PC,#immed8 / Rd,PC/SP,#immed8
                linePtr = oldLine;

                reg2 = REG_Get(regs_0_15);
                if (reg2 > 15) reg2 = reg2 - 3; // SP LR PC -> R13 R14 R15
                if (REG_Check(reg2)) break;

                oldLine = linePtr;
                token = TOKEN_GetWord(word);
                if (token == ',')
                {
                    token = TOKEN_GetWord(word);
                    if (token == '#')
                    {
                        val = EXPR_Eval();
                        switch (reg2)
                        {
                            case REG_PC:
                                // FIXME: need to check immed8 range
                                INSTR_W(0xA000 | (reg1 << 8) | ((val >> 2) & 0xFF));
                                break;

                            case REG_SP:
                                // FIXME: need to check immed8 range
                                INSTR_W(0xA800 | (reg1 << 8) | ((val >> 2) & 0xFF));
                                break;
                            default:
                                if (reg2 <= REG_R7)   // Rm
                                {
                                    // FIXME: need to check immed3 range 0..7
                                    INSTR_W(0x1C00 | ((val & 7) << 6) | (reg2 << 3) | reg1);
                                }
                                else
                                {
                                    ASMX_IllegalOperand();
                                }
                        }
                    }
                    else
                    {
                        val = REG_Find(word, regs_0_7);
                        if (REG_Check(val)) break;

                        INSTR_W(0x1800 | (val << 6) | (reg2 << 3) | reg1);
                    }
                }
                else     // RD,RM
                {
                    INSTR_W(0x4400 | ((reg1 << 4) & 0x80) | (reg2 << 3) | (reg1 & 0x03));
                }
            }
            break;

        case OP_SUB:          // SUB opcode
            reg1 = REG_Get(regs_0_15);
            if (reg1 > 15) reg1 = reg1 - 3; // SP LR PC -> R13 R14 R15
            if (REG_Check(reg1)) break;

            if (reg1 > 7 && reg1 != 13)    // only R0-R7 and SP are allowed here
            {
                ASMX_IllegalOperand();
                break;
            }

            if (TOKEN_Comma()) break;

            oldLine = linePtr;
            token = TOKEN_GetWord(word);
            if (token == '#')
            {
                // Rd,#immed8 / SP,#immed7
                if (reg1 <= 7)
                {
                    val = EXPR_EvalByte();
                    INSTR_W(0x3800 | (reg1 << 8) | (val & 0xFF));
                }
                else
                {
                    val = EXPR_Eval();
                    // FIXME: check immed7 range
                    INSTR_W(0xB080 | ((val >> 2) & 0x7F));
                }
            }
            else
            {
                // Rd,Rn,#immed3 / Rd,Rn,Rm
                linePtr = oldLine;
                reg2 = REG_Get(regs_0_7);
                if (REG_Check(reg2)) break;

                if (TOKEN_Comma()) break;

                token = TOKEN_GetWord(word);
                if (token == '#')
                {
                    val = EXPR_Eval();
                    // FIXME: need to check immed3 range 0..7
                    INSTR_W(0x1E00 | ((val & 7) << 6) | (reg2 << 3) | reg1);
                }
                else
                {
                    val = REG_Find(word, regs_0_7);
                    if (REG_Check(val)) break;

                    INSTR_W(0x1A00 | (val << 6) | (reg2 << 3) | reg1);
                }
            }
            break;

        case OP_Shift:        // ASR/LSL/LSR
            reg1 = REG_Get(regs_0_7);
            if (REG_Check(reg1)) break;

            if (TOKEN_Comma()) break;

            reg2 = REG_Get(regs_0_7);
            if (REG_Check(reg2)) break;

            oldLine = linePtr;
            token = TOKEN_GetWord(word);
            if (token == ',')
            {
                if (TOKEN_Expect("#")) break;
                val = EXPR_EvalByte();
                // FIXME: need to confirm that EXPR_EvalByte is sufficient here for immed8
                INSTR_W((parm >> 16) | ((val & 0x1F) << 6) | (reg2 << 3) | reg1);
            }
            else
            {
                linePtr = oldLine;
                INSTR_W((parm & 0xFFFF) | (reg2 << 3) | reg1);
            }

            break;

        case OP_Immed8:       // BKPT/SWI
            val = EXPR_EvalByte();
            INSTR_W(parm | (val & 0xFF));
            break;

        case OP_Branch:       // B short=DE00 long=E000
            val = THUMB_EvalBranch(11, 4);
            if (evalKnown && (-256 <= val) && (val <= 255))
            {
                INSTR_W(0xDE00 | ((val >> 1) & 0xFF));
            }
            else
            {
                INSTR_W(0xE000 | ((val >> 1) & 0x07FF));
            }
            break;

        case OP_BranchCC:     // Bcc
            val = THUMB_EvalBranch(8, 4);
            INSTR_W(parm | ((val >> 1) & 0xFF));
            break;

        case OP_BLX:          // BLX
            oldLine = linePtr;
            reg1 = REG_Get(regs_0_15);
            if (reg1 > 15) reg1 = reg1 - 3; // SP LR PC -> R13 R14 R15
            if (reg1 >= 0)
            {
                INSTR_W(0x4780 | (reg1 << 4));
                break;
            }
            linePtr = oldLine;
            // fall through to BL form
            FALLTHROUGH;

        case OP_BL:           // BL
            val = THUMB_EvalBranch(22, 4);
            INSTR_WW(0xF000 | ((val >> 12) & 0x07FF), parm | ((val >> 1) & 0x07FF));
            break;

        case OP_BX:           // BX
            reg1 = REG_Get(regs_0_15);
            if (reg1 > 15) reg1 = reg1 - 3; // SP LR PC -> R13 R14 R15
            if (REG_Check(reg1)) break;

            INSTR_W(parm | reg1 << 3);
            break;

        case OP_CMP_MOV:      // CMP/MOV
            reg1 = REG_Get(regs_0_15);
            if (reg1 > 15) reg1 = reg1 - 3; // SP LR PC -> R13 R14 R15
            if (REG_Check(reg1)) break;

            if (TOKEN_Comma()) break;

            oldLine = linePtr;
            token = TOKEN_GetWord(word);
            if (token == '#')
            {
                if (reg1 > 7)
                {
                    ASMX_IllegalOperand();
                    break;
                }
                val = EXPR_EvalByte();
                // FIXME: need to confirm that EXPR_EvalByte is sufficient here for immed8
                INSTR_W(((parm >> 16) & 0xFF00) | (reg1 << 8) | (val & 0xFF));
            }
            else
            {
                linePtr = oldLine;

                reg2 = REG_Get(regs_0_15);
                if (reg2 > 15) reg2 = reg2 - 3; // SP LR PC -> R13 R14 R15
                if (REG_Check(reg2)) break;

                if ((reg1 < 8) && (reg2 < 8))
                {
                    INSTR_W(((parm >> 8) & 0xFFFF) | ((reg1 << 4) & 0x80) | (reg2 << 3) | (reg1 & 0x03));
                }
                else
                {
                    INSTR_W(((parm << 8) & 0xFF00) | ((reg1 << 4) & 0x80) | (reg2 << 3) | (reg1 & 0x03));
                }
            }

            break;

        case OP_LDMIA:        // LDMIA/STMIA
            // FIXME: at least one reg must be specified
            reg1 = REG_Get(regs_0_7);
            if (REG_Check(reg1)) break;

            if (TOKEN_Expect("!")) break;
            if (TOKEN_Comma()) break;

            if (THUMB_GetMultiRegs(false, &regbits)) break;

            INSTR_W(parm | (reg1 << 8) | regbits);
            break;

        case OP_LDR:          // LDR/STR
//    {"LDR",   OP_LDR,        0x0800},
//    {"STR",   OP_LDR,        0x0000},
            reg1 = REG_Get(regs_0_7);
            if (REG_Check(reg1)) break;

            if (TOKEN_Comma()) break;
            if (TOKEN_Expect("[")) break;

            reg2 = REG_Get(regs_0_15);
            if (reg2 > 15) reg2 = reg2 - 3; // SP LR PC -> R13 R14 R15
            if (REG_Check(reg2)) break;

            if ((reg2 > 7) && (reg2 != 13) && (reg2 != 15))
            {
                ASMX_IllegalOperand();
                break;
            }
            if (TOKEN_Comma()) break;

            switch (reg2)
            {
                case REG_PC:
                    if (!parm)   // can't use PC relative on STR
                    {
                        ASMX_IllegalOperand();
                        break;
                    }
                    TOKEN_Expect("#");
                    val = EXPR_Eval();
                    // FIXME: need to check immed8 range
                    INSTR_W(0x4000 | parm | (reg1 << 8) | ((val >> 2) & 0xFF));
                    break;

                case REG_SP:
                    TOKEN_Expect("#");
                    val = EXPR_Eval();
                    // FIXME: need to check immed8 range
                    INSTR_W(0x9000 | parm | (reg1 << 8) | ((val >> 2) & 0xFF));
                    break;

                default:
                    token = TOKEN_GetWord(word);
                    if (token == '#')
                    {
                        // #immed5
                        val = EXPR_Eval();
                        // FIXME: need to check immed5 range
                        INSTR_W(0x6000 | parm | (((val >> 2) & 0x1F) << 6) | (reg2 << 3) | reg1);
                    }
                    else
                    {
                        // Rm
                        val = REG_Find(word, regs_0_7);
                        if (REG_Check(val)) break;

                        INSTR_W(0x5000 | parm | (val << 6) | (reg2 << 3) | reg1);
                    }
            }
            TOKEN_Expect("]");
            break;

        case OP_LDRBH:        // LDRB/LDRH/STRB/STRH
            reg1 = REG_Get(regs_0_7);
            if (REG_Check(reg1)) break;

            if (TOKEN_Comma()) break;
            if (TOKEN_Expect("[")) break;

            reg2 = REG_Get(regs_0_7);
            if (REG_Check(reg2)) break;

            if (TOKEN_Comma()) break;

            token = TOKEN_GetWord(word);
            if (token == '#')
            {
                val = EXPR_Eval();
                if (parm & 0x80000000)
                {
                    // halfword
                    // FIXME: need to check immed5 range
                    INSTR_W((parm >> 16) | (((val >> 1) & 0x1F) << 6) | (reg2 << 3) | reg1);
                }
                else
                {
                    // byte
                    // FIXME: need to check immed5 range
                    INSTR_W((parm >> 16) | ((val & 0x1F) << 6) | (reg2 << 3) | reg1);
                }
            }
            else
            {
                val = REG_Find(word, regs_0_7);
                if (REG_Check(val)) break;

                INSTR_W((parm & 0xFFFF) | ((val & 0x1F) << 6) | (reg2 << 3) | reg1);
            }
            TOKEN_Expect("]");
            break;

        case OP_LDRSBH:       // LDRSB/LDRSH
            reg1 = REG_Get(regs_0_7);
            if (REG_Check(reg1)) break;

            if (TOKEN_Comma()) break;
            if (TOKEN_Expect("[")) break;

            reg2 = THUMB_TwoRegs();
            if (reg2 < 0) break;

            INSTR_W(parm | (reg2 << 3) | reg1);

            TOKEN_Expect("]");

            break;

        case OP_PUSH_POP:     // PUSH/POP
            // FIXME: at least one reg must be specified
            if (THUMB_GetMultiRegs(true, &regbits)) break;

            INSTR_W(parm | (regbits & 0x1FF));
            break;

        case OP_Implied:      // NOP
            INSTR_W(parm);
            break;

        default:
            return 0;
            break;
    }

    if (locPtr & 1)
    {
        ASMX_Error("Code at non-word-aligned address");
        // deposit an extra byte to reset alignment and prevent further errors
        INSTR_AddB(0);
        // note: Inserting bytes in front won't work because offsets have already been assembled.
        // The line could be re-assembled by recursively calling DoCPUOpcode, but then
        // the label before the instruction would still be at the odd address.
    }

    return 1;
}


void THUMB_AsmInit(void)
{
    void *p = ASMX_AddAsm(versionName, &THUMB_DoCPUOpcode, NULL, NULL);

    ASMX_AddCPU(p, "THUMB", 0, END_LITTLE, ADDR_24, LIST_24, 8, 0, THUMB_opcdTab);
    ASMX_AddCPU(p, "THUMB_BE", 0, END_BIG,    ADDR_24, LIST_24, 8, 0, THUMB_opcdTab);
    ASMX_AddCPU(p, "THUMB_LE", 0, END_LITTLE, ADDR_24, LIST_24, 8, 0, THUMB_opcdTab);
}
