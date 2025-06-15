// asmARM.c

#define versionName "ARM assembler"
#include "asmx.h"

enum
{
    OP_Branch,       // Bcc / BLcc
    OP_OneReg,       // Rm (only used for BX?)
    OP_OneRegShift,  // Rd,shifter
    OP_OneRegShiftS, // Rd,shifter with optional S
    OP_TwoRegShiftS, // Rd,Rn,shifter with optional S
    OP_SWI,          // SWI immed24
    OP_BKPT,         // BKPT immed16
    OP_BLX,          // BLX
    OP_CLZ,          // CLZ
    OP_MUL,          // MUL
    OP_SWP,          // SWP
    OP_LDM_STM,      // LDM/STM
    OP_LDR_STR,      // LDR/STR
    OP_MRS,          // MRS
    OP_MSR,          // MSR
    OP_MLA,          // MLA
    OP_MLAL,         // SMLAL/SMULL/UMLAL/UMULL
    OP_CDP,          // CDP
    OP_MCR_MRC,      // MCR/MRC
    OP_LDC_STC,      // LDC/STC
    OP_ADR,          // ADR/ADRL
    OP_Implied,      // NOP

//  o_Foo = OP_LabelOp,
};

const char regs[] = "R0 R1 R2 R3 R4 R5 R6 R7 R8 R9 R10 R11 R12 R13 R14 R15 SP LR PC";
const char pregs[] = "P0 P1 P2 P3 P4 P5 P6 P7 P8 P9 P10 P11 P12 P13 P14 P15";
const char cregs[] = "CR0 CR1 CR2 CR3 CR4 CR5 CR6 CR7 CR8 CR9 CR10 CR11 CR12 CR13 CR14 CR15";
const char shifts[] = "LSL LSR ASR ROR RRX ASL";

enum   // shifts
{
    TYP_LSL = 0,
    TYP_LSR = 1,
    TYP_ASR = 2,
    TYP_ROR = 3,
    TYP_RRX = 4,
    TYP_ASL = 5, // same as 0
};

static const struct OpcdRec ARM_opcdTab[] =
{
    // B and BL don't use wildcards to avoid false matches with BL, BLX, and BX
    {"B",       OP_Branch,       0xEA000000},
    {"BEQ",     OP_Branch,       0x0A000000},
    {"BNE",     OP_Branch,       0x1A000000},
    {"BCS",     OP_Branch,       0x2A000000},
    {"BHS",     OP_Branch,       0x2A000000},
    {"BCC",     OP_Branch,       0x3A000000},
    {"BLO",     OP_Branch,       0x3A000000},
    {"BMI",     OP_Branch,       0x4A000000},
    {"BPL",     OP_Branch,       0x5A000000},
    {"BVS",     OP_Branch,       0x6A000000},
    {"BVC",     OP_Branch,       0x7A000000},
    {"BHI",     OP_Branch,       0x8A000000},
    {"BLS",     OP_Branch,       0x9A000000},
    {"BGE",     OP_Branch,       0xAA000000},
    {"BLT",     OP_Branch,       0xBA000000},
    {"BGT",     OP_Branch,       0xCA000000},
    {"BLE",     OP_Branch,       0xDA000000},
    {"BAL",     OP_Branch,       0xEA000000},

    {"BL",      OP_Branch,       0xEB000000},
    {"BLEQ",    OP_Branch,       0x0B000000},
    {"BLNE",    OP_Branch,       0x1B000000},
    {"BLCS",    OP_Branch,       0x2B000000},
    {"BLHS",    OP_Branch,       0x2B000000},
    {"BLCC",    OP_Branch,       0x3B000000},
    {"BLLO",    OP_Branch,       0x3B000000},
    {"BLMI",    OP_Branch,       0x4B000000},
    {"BLPL",    OP_Branch,       0x5B000000},
    {"BLVS",    OP_Branch,       0x6B000000},
    {"BLVC",    OP_Branch,       0x7B000000},
    {"BLHI",    OP_Branch,       0x8B000000},
    {"BLLS",    OP_Branch,       0x9B000000},
    {"BLGE",    OP_Branch,       0xAB000000},
    {"BLLT",    OP_Branch,       0xBB000000},
    {"BLGT",    OP_Branch,       0xCB000000},
    {"BLLE",    OP_Branch,       0xDB000000},
    {"BLAL",    OP_Branch,       0xEB000000},

    {"BX*",     OP_OneReg,       0x012FFF10}, // ARM5/ARM4T

    {"CMN*",    OP_OneRegShift,  0x01700000},
    {"CMP*",    OP_OneRegShift,  0x01500000},
    {"TEQ*",    OP_OneRegShift,  0x01300000},
    {"TST*",    OP_OneRegShift,  0x01100000},

    {"MOV*",    OP_OneRegShiftS, 0x01A00000},
    {"MVN*",    OP_OneRegShiftS, 0x01E00000},

    {"ADC*",    OP_TwoRegShiftS, 0x00A00000},
    {"ADD*",    OP_TwoRegShiftS, 0x00800000},
    {"AND*",    OP_TwoRegShiftS, 0x00000000},
    {"BIC*",    OP_TwoRegShiftS, 0x01C00000},
    {"EOR*",    OP_TwoRegShiftS, 0x00200000},
    {"ORR*",    OP_TwoRegShiftS, 0x01800000},
    {"RSB*",    OP_TwoRegShiftS, 0x00600000},
    {"RSC*",    OP_TwoRegShiftS, 0x00E00000},
    {"SBC*",    OP_TwoRegShiftS, 0x00C00000},
    {"SUB*",    OP_TwoRegShiftS, 0x00400000},

    {"SWI*",    OP_SWI,          0x0F000000},

    {"BKPT",    OP_BKPT,         0xE1200070},

    {"BLX*",    OP_BLX,          0x012FFF30}, // ARM5

    {"CLZ*",    OP_CLZ,          0x016F0F10}, // ARM5

    {"MUL*",    OP_MUL,          0x00000090}, // ARM2

    {"SWP*",    OP_SWP,          0x01000090}, // ARM2a/ARM3

    {"LDM*",    OP_LDM_STM,      0x00100000},
    {"STM*",    OP_LDM_STM,      0x00000000},

    {"LDR*",    OP_LDR_STR,      0x00100000}, // LDRH/LDRSB/LDRSH = ARM4
    {"STR*",    OP_LDR_STR,      0x00000000}, // STRH = ARM4

    {"MRS*",    OP_MRS,          0x010F0000}, // ARM3

    {"MSR*",    OP_MSR,          0x0120F000}, // ARM3

    {"MLA*",    OP_MLA,          0x00200090}, // ARM2

    {"SMLAL*",  OP_MLAL,         0x00E00090}, // ARM-M
    {"SMULL*",  OP_MLAL,         0x00C00090}, // ARM-M
    {"UMLAL*",  OP_MLAL,         0x00A00090}, // ARM-M
    {"UMULL*",  OP_MLAL,         0x00800090}, // ARM-M

    {"CDP*",    OP_CDP,          0x0E000000}, // ARM2/5

    {"MCR*",    OP_MCR_MRC,      0x0E000010}, // ARM2/5
    {"MRC*",    OP_MCR_MRC,      0x0E100010}, // ARM2/5

    {"LDC*",    OP_LDC_STC,      0x0C100000}, // ARM2/5
    {"STC*",    OP_LDC_STC,      0x0C000000}, // ARM2/5

    {"ADR*",    OP_ADR,          0},          // pseudo-instruction

    {"NOP",     OP_Implied,      0xE1A00000}, // pseudo-instruction = MOV R0,R0

    {"",        OP_Illegal,      0}
};


// --------------------------------------------------------------


static long ARM_Immed(uint32_t val)
{
    // note: can't abort assembling instruction because it may cause phase errors

    if ((val & 0xFFFFFF00) == 0)
    {
        return (val & 0xFF);
    }
    else
    {
        // FIXME: need to handle shifts
        // bit 25 (I) = 1
        // bits 8-11 = rotate (immed8 is rotated right by 2 * this value)
        // bits 0-7 = immed8
        // the smallest value of rotate should be chosen
    }

    ASMX_Error("Invalid immediate constant");
    return 0;
}


static bool ARM_Shifter(uint32_t *shift)
{
    int     reg1, reg2, typ;
    Str255  word;
    char    *oldLine;
    int     token;
    int     val;

    *shift = 0;

    token = TOKEN_GetWord(word);
    if (token == '#')
    {
        val = EXPR_Eval();
        *shift = (1 << 25) | ARM_Immed(val);
//  } else if (token == '=') {
//      // I think this is where "=value" for deferred constants needs to go
//
    }
    else
    {
        reg1 = REG_Find(word, regs);
        if (reg1 > 15) reg1 = reg1 - 3; // SP LR PC -> R13 R14 R15
        if (REG_Check(reg1)) return 1;

        oldLine = linePtr;
        token = TOKEN_GetWord(word);
        if (token != ',')
        {
            linePtr = oldLine;
            *shift = reg1;
        }
        else
        {
            // shifts and rotates
            typ = REG_Get(shifts);
            if (typ == TYP_ASL) typ = TYP_LSL;
            if (typ < 0)
            {
                ASMX_IllegalOperand();
            }
            else if (typ == TYP_RRX)
            {
                *shift = 0x60 | reg1;
            }
            else
            {
                token = TOKEN_GetWord(word);
                if (token == '#')
                {
                    val = EXPR_Eval();
                    // FIXME: need to range check immed5
                    *shift = (typ << 5) | reg1 | ((val & 0x1F) << 7);
                }
                else
                {
                    reg2 = REG_Find(word, regs);
                    if (reg2 > 15) reg2 = reg2 - 3; // SP LR PC -> R13 R14 R15
                    if (REG_Check(reg2)) return 1;

                    *shift = (typ << 5) | (1 << 4) | reg1 | (reg2 << 8);
                }
            }
        }
    }

    return 0;
}


static int ARM_PlusMinus(void)
{
    Str255  word;
    char    *oldLine;
    int     token;

    oldLine = linePtr;
    token = TOKEN_GetWord(word);
    switch (token)
    {
        case '+':
            return 1;

        case '-':
            return 0;

        default:
            linePtr = oldLine;
            return 1;
    }
}


static int ARM_Writeback(void)
{
    Str255  word;
    char    *oldLine;
    int     token;

    oldLine = linePtr;
    token = TOKEN_GetWord(word);
    if (token == '!') return 1;

    linePtr = oldLine;
    return 0;
}


// addressing mode for LDR/STR/B/T/BT (post flag is true for T opcode)
static bool ARM_AddrMode2(uint32_t *mode, bool post)
{
    int     reg1, reg2;
    Str255  word;
    char    *oldLine;
    int     token;
    int     sign;
    int     wb;
    int     val;
    int     typ;

    *mode = 0;

    if (TOKEN_Expect("[")) return 1;

    reg1 = REG_Get(regs);
    if (reg1 > 15) reg1 = reg1 - 3; // SP LR PC -> R13 R14 R15
    if (REG_Check(reg1)) return 1;

    oldLine = linePtr;
    token = TOKEN_GetWord(word);
    if (token == ',')
    {
        if (post)
        {
            TOKEN_BadMode();
            return 1; // these modes not allowed for post-addrmode
        }

        oldLine = linePtr;
        token = TOKEN_GetWord(word);
        if (token == '#')
        {
            // [Rn, #+/- ofs12
            val = EXPR_Eval();
            if (TOKEN_Expect("]")) return 1;
            wb = ARM_Writeback();

            // FIXME: need to check range

            if (val < 0)
            {
                *mode = (reg1 << 16) | (1 << 24) | (wb << 21) | (-val & 0x0FFF);
            }
            else
            {
                *mode = (reg1 << 16) | (1 << 24) | (1 << 23) | (wb << 21) | (val & 0x0FFF);
            }
        }
        else
        {
            linePtr = oldLine;
            sign = ARM_PlusMinus();

            reg2 = REG_Get(regs);
            if (reg2 > 15) reg2 = reg2 - 3; // SP LR PC -> R13 R14 R15
            if (REG_Check(reg2)) return 1;

            oldLine = linePtr;
            token = TOKEN_GetWord(word);
            if (token == ',')
            {
                // FIXME ,shift #immed5
                typ = REG_Get(shifts);
                if (typ == TYP_ASL) typ = TYP_LSL;
                if (typ < 0)
                {
                    ASMX_IllegalOperand();
                }
                else if (typ == TYP_RRX)
                {
                    if (TOKEN_Expect("]")) return 1;
                    wb = ARM_Writeback();

                    *mode = (1 << 25) | (1 << 24) | (sign << 23) | (wb << 21) | (reg1 << 16) | (3 << 5) | reg2;
                }
                else
                {
                    TOKEN_Expect("#");
                    val = EXPR_Eval();
                    // FIXME need to range check immed5, must be != 0
                    if (TOKEN_Expect("]")) return 1;
                    wb = ARM_Writeback();

                    *mode = (1 << 25) | (1 << 24) | (sign << 23) | (wb << 21) | (reg1 << 16) | ((val & 31) << 7) | (typ << 5) | reg2;
                }
            }
            else
            {
                linePtr = oldLine;
                if (TOKEN_Expect("]")) return 1;
                wb = ARM_Writeback();

                *mode = (1 << 25) | (1 << 24) | (sign << 23) | (wb << 21) | (reg1 << 16) | reg2;
            }

            // FIXME need to set bits
        }
    }
    else
    {
        linePtr = oldLine;
        if (TOKEN_Expect("]")) return 1;
        oldLine = linePtr;
        token = TOKEN_GetWord(word);
        if (!token)
        {
            // various possible ways of representing this
            // [Rn] => [Rn,#0]
            //*mode = (reg1 << 16) | (1 << 24) | (1 << 23);
            // [Rn] => [Rn],#0
            //*mode = (1 << 23) | (reg1 << 16);
            // [Rn] => [Rn,#-0]
            *mode = (reg1 << 16) | (1 << 24);
        }
        else if (token == '!')
        {
            if (post)
            {
                TOKEN_BadMode();
                return 1; // writeback modes not allowed for post-addrmode
            }

            // [Rn]! => [Rn,#0]!
            *mode = (1 << 24) | (1 << 23) | (1 << 21) | (reg1 << 16);
        }
        else
        {
            linePtr = oldLine;
            if (TOKEN_Comma()) return 1;

            oldLine = linePtr;
            token = TOKEN_GetWord(word);
            if (token == '#')
            {
                val = EXPR_Eval();
                // FIXME: need to check range

                if (val < 0)
                {
                    *mode = (reg1 << 16) | (-val & 0x0FFF);
                }
                else
                {
                    *mode = (1 << 23) | (reg1 << 16) | (val & 0x0FFF);
                }
            }
            else
            {
                linePtr = oldLine;
                sign = ARM_PlusMinus();

                reg2 = REG_Get(regs);
                if (reg2 > 15) reg2 = reg2 - 3; // SP LR PC -> R13 R14 R15
                if (REG_Check(reg2)) return 1;

                oldLine = linePtr;
                token = TOKEN_GetWord(word);
                if (token == ',')
                {
                    // FIXME ,shift #immed5
                    typ = REG_Get(shifts);
                    if (typ == TYP_ASL) typ = TYP_LSL;
                    if (typ < 0)
                    {
                        ASMX_IllegalOperand();
                    }
                    else if (typ == TYP_RRX)
                    {
                        *mode = (1 << 25) | (sign << 23) | (reg1 << 16) | (3 << 5) | reg2;
                    }
                    else
                    {
                        TOKEN_Expect("#");
                        val = EXPR_Eval();
                        // FIXME need to range check immed5
                        *mode = (1 << 25) | (sign << 23) | (reg1 << 16) | ((val & 31) << 7) | (typ << 5) | reg2;
                    }
                }
                else
                {
                    linePtr = oldLine;

                    *mode = (1 << 25) | (sign << 23) | (reg1 << 16) | reg2;
                }

                // FIXME need to set bits
            }
        }
    }

    return 0;
}


// addressing mode for LDRH/LDRSB/LDRSH/STRH
static bool ARM_AddrMode3(uint32_t *mode)
{
    int     reg1, reg2;
    Str255  word;
    char    *oldLine;
    int     token;
    int     sign;
    int     wb;
    int     val;

    *mode = 0;

    if (TOKEN_Expect("[")) return 1;

    reg1 = REG_Get(regs);
    if (reg1 > 15) reg1 = reg1 - 3; // SP LR PC -> R13 R14 R15
    if (REG_Check(reg1)) return 1;

    oldLine = linePtr;
    token = TOKEN_GetWord(word);
    if (token == ',')
    {
        oldLine = linePtr;
        token = TOKEN_GetWord(word);
        if (token == '#')
        {
            val = EXPR_Eval();
            if (TOKEN_Expect("]")) return 1;
            wb = ARM_Writeback();

            // FIXME: need to check range
            if (val < 0)
            {
                *mode = (1 << 24) | (1 << 22) | (wb << 21) | (reg1 << 16) | ((-val & 0xF0) << 4) | (-val & 0x0F);
            }
            else
            {
                *mode = (1 << 24) | (1 << 23) | (1 << 22) | (wb << 21) | (reg1 << 16) | ((val & 0xF0) << 4) | (val & 0x0F);
            }
        }
        else
        {
            linePtr = oldLine;
            sign = ARM_PlusMinus();

            reg2 = REG_Get(regs);
            if (reg2 > 15) reg2 = reg2 - 3; // SP LR PC -> R13 R14 R15
            if (REG_Check(reg2)) return 1;

            if (TOKEN_Expect("]")) return 1;

            wb = ARM_Writeback();

            *mode = (1 << 24) | (sign << 23) | (wb << 21) | (reg1 << 16) | reg2;
        }
    }
    else
    {
        linePtr = oldLine;
        if (TOKEN_Expect("]")) return 1;

        oldLine = linePtr;
        token = TOKEN_GetWord(word);
        if (!token)
        {
            // various possible ways of representing this
            // [Rn] => [Rn,#0]
            //*mode = (1 << 24) | (1 << 23) | (1 << 22) | (reg1 << 16);
            // [Rn] => [Rn],#0
            //*mode = (1 << 23) | (1 << 22) | (reg1 << 16);
            // [Rn] => [Rn,#-0]
            *mode = (1 << 24) | (1 << 22) | (reg1 << 16);
        }
        else if (token == '!')
        {
            // [Rn]! => [Rn,#0]!
            *mode = (1 << 24) | (1 << 23) | (1 << 22) | (1 << 21) | (reg1 << 16);
        }
        else
        {
            linePtr = oldLine;
            if (TOKEN_Comma()) return 1;

            oldLine = linePtr;
            token = TOKEN_GetWord(word);
            if (token == '#')
            {
                val = EXPR_Eval();
                // FIXME: need to check range

                if (val < 0)
                {
                    *mode = (1 << 22) | (reg1 << 16) | ((-val & 0xF0) << 4) | (-val & 0x0F);
                }
                else
                {
                    *mode = (1 << 23) | (1 << 22) | (reg1 << 16) | ((val & 0xF0) << 4) | (val & 0x0F);
                }
            }
            else
            {
                linePtr = oldLine;
                sign = ARM_PlusMinus();

                reg2 = REG_Get(regs);
                if (reg2 > 15) reg2 = reg2 - 3; // SP LR PC -> R13 R14 R15
                if (REG_Check(reg2)) return 1;

                *mode =  (sign << 23) | (reg1 << 16) | reg2;
            }
        }
    }

    return 0;
}


// addressing mode for LDC/STC
static bool ARM_AddrMode5(uint32_t *mode)
{
    int     reg1;//, reg2;
    Str255  word;
    char    *oldLine;
    int     token;
//  int     sign;
    int     wb;
    int     val;

    *mode = 0;

    if (TOKEN_Expect("[")) return 1;

    reg1 = REG_Get(regs);
    if (reg1 > 15) reg1 = reg1 - 3; // SP LR PC -> R13 R14 R15
    if (REG_Check(reg1)) return 1;

    oldLine = linePtr;
    token = TOKEN_GetWord(word);
    if (token == ',')
    {
        if (TOKEN_Expect("#")) return 1;

        val = EXPR_Eval();
        if (TOKEN_Expect("]")) return 1;
        wb = ARM_Writeback();

        // FIXME: need to check range
        if (val < 0)
        {
            *mode = (1 << 24) | (wb << 21) | (reg1 << 16) | (((-val) >> 2) & 0xFF);
        }
        else
        {
            *mode = (1 << 24) | (1 << 23) | (wb << 21) | (reg1 << 16) | ((val >> 2) & 0xFF);
        }
    }
    else
    {
        linePtr = oldLine;
        if (TOKEN_Expect("]")) return 1;

        oldLine = linePtr;
        token = TOKEN_GetWord(word);
        if (!token)
        {
            // various possible ways of representing this
            // [Rn] => [Rn],#0
            //*mode = (1 << 23) | (1 << 21) | (reg1 << 16);
            // [Rn] => [Rn],#-0
            //*mode = (1 << 21) | (reg1 << 16);
            // [Rn] => [Rn,#-0]
            *mode = (1 << 24) | (reg1 << 16);
        }
        else if (token == '!')
        {
            // [Rn]! => [Rn,#0]!
            *mode = (1 << 24) | (1 << 23) | (1 << 21) | (reg1 << 16);
        }
        else
        {
            linePtr = oldLine;

            if (TOKEN_Comma()) return 1;

            token = TOKEN_GetWord(word);
            switch (token)
            {
                case '#':
                    val = EXPR_Eval();
                    // FIXME: need to check range
                    if (val < 0)
                    {
                        *mode = (1 << 21) | (reg1 << 16) | ((-val >> 2) & 0xFF);
                    }
                    else
                    {
                        *mode = (1 << 23) | (1 << 21) | (reg1 << 16) | ((val >> 2) & 0xFF);
                    }
                    break;

                case '{':
                    val = EXPR_Eval();
                    if (TOKEN_Expect("}")) return 1;
                    // FIXME: need to check range
                    *mode = (1 << 23) | (reg1 << 16) | (val & 0xFF);
                    break;

                default:
                    ASMX_Error("\"#\" or \"{\" expected");
                    return 1;
            }
        }
    }

    return 0;
}


static void ARM_SetMultiReg(int reg, uint16_t *regbits, bool *warned)
{
    if (!*warned && *regbits & (1 << reg))
    {
        ASMX_Warning("register specified twice");
        *warned = true;
    }
    *regbits |= 1 << reg;
}


static int ARM_GetMultiRegs(uint16_t *regbits)
{
    int     reg1, reg2;
    Str255  word;
    char    *oldLine;
    int     token;
    bool    warned;

    *regbits = 0;
    warned = false;

    // looking for {r0,r2-r5,r6,lr}

    if (TOKEN_Expect("{")) return 1;

    // FIXME: at least one reg must be specified

    oldLine = linePtr;
    token = ',';
    while (token == ',')
    {
        reg1 = REG_Get(regs);
        if (reg1 > 15) reg1 = reg1 - 3; // SP LR PC -> R13 R14 R15

        if (reg1 < 0)
        {
            ASMX_IllegalOperand();      // abort if not valid register
            break;
        }

        // set single register bit
        ARM_SetMultiReg(reg1, regbits, &warned);

        // check for - or ,
        oldLine = linePtr;
        token = TOKEN_GetWord(word);

        if (token == '-')       // register range
        {
            oldLine = linePtr;  // commit line position
            reg2 = REG_Get(regs); // check for second register
            if (reg2 > 15) reg2 = reg2 - 3; // SP LR PC -> R13 R14 R15

            if (reg2 < 0)
            {
                ASMX_IllegalOperand();      // abort if not valid register
                break;
            }
            if (reg1 < reg2)
            {
                for (int i = reg1 + 1; i <= reg2; i++)
                {
                    ARM_SetMultiReg(i, regbits, &warned);
                }
            }
            else if (reg1 > reg2)
            {
                for (int i = reg1 - 1; i >= reg2; i--)
                {
                    ARM_SetMultiReg(i, regbits, &warned);
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


// returns the LDM/STM type string
// returns -1 if invalid type string
// returns 0-3 for DA/IA/DB/IB, 4-7 for FA/EA/FD/ED, (value & 3) << 23 to opcode
// if LDM and return >= 4, XOR result by 3 first
static int ARM_GetLDMType(char *word)
{
    int i, ofs;

    ofs = 0;

    if (!word[0] || !word[1])   // type string less than 2 bytes
    {
        return -1;
    }

    if (word[2] && word[3] && !word[4])   // 4-char type
    {
        ofs = 2;
//  } else if (word[2]) { // not a 2-char type
//      return -1;
    }

    i = REG_Find(word + ofs, "DA IA DB IB ED EA FD FA");
    if (i >= 0)
    {
        word[ofs] = 0; // remove LDM/STM type and leave condition code intact
        return i;
    }

    return -1;
}


enum { LDR_none, LDR_B, LDR_BT, LDR_H, LDR_SB, LDR_SH, LDR_T };

static int ARM_GetLDRType(char *word)
{
    int i, ofs;

    ofs = 0;

    if (!word[0]) return 0; // empty type string

    if (word[1] && word[2])   // 3- or 4-char type
    {
        ofs = 2;
    }

    i = REG_Find(word + ofs, "B BT H SB SH T");
    if (i >= 0)
    {
        word[ofs] = 0; // remove LDR type and leave condition code intact
        return i + 1;
    }

    return 0;
}


static long ARM_GetMSRReg(char *word)
{
    char *p;
    int  bit;
    long mask;

    // CPSR_flags or SPSR_flags
    if (word[0] != 'C' && word[0] != 'S') return -1;
    if (word[1] != 'P' || word[2] != 'S' || word[3] != 'R' || word[4] != '_' || !word[5]) return -1;

    mask = 0;
    p = word + 5;
    while (*p)
    {
        switch (*p++)
        {
            case 'C':
                bit = 16;
                break;
            case 'X':
                bit = 17;
                break;
            case 'S':
                bit = 18;
                break;
            case 'F':
                bit = 19;
                break;
            default:
                return -1;
        }
        if (mask & (1 << bit)) return -1; // don't allow repeated fields
        mask |= 1 << bit;
    }

    return ((word[0] == 'S') << 22) | mask;
}


static bool ARM_Cond(int *cond, char *word)
{
    int reg1;

    *cond = 14; // default to ALways
    if (word[0])
    {
        reg1 = REG_Find(word, "EQ NE CS CC MI PL VS VC HI LS GE LT GT LE AL HS LO");
        if (reg1 < 0) return true;
        if (reg1 > 14) reg1 = reg1 - 14 + 2; // HS -> CS / LO -> CC
        *cond = reg1;
    }

    return false;
}


static bool ARM_OpcodeFlag(char *word, char flag)
{
    if ((word[0] == flag) && (word[1] == 0))
    {
        word[0] = 0;
        return 1;
    }

    if (word[0] && word[1] && (word[2] == flag) && (word[3] == 0))
    {
        word[2] = 0;
        return 1;
    }

    if (word[0] && word[1] && word[2] && (word[3] == flag) && (word[4] == 0))
    {
        word[2] = 0;
        return 1;
    }

    return 0;
}


static int ARM_EvalBranch(int width, int instrLen) // instrLen should be 8
{
    long val;
    long limit;

    limit = (1 << (width+1)) - 1;

    val = EXPR_Eval();
    val = val - locPtr - instrLen;
    if (!errFlag && ((val & 3) || val < ~limit || val > limit))
    {
        ASMX_Error("Long branch out of range");
    }

    return val;
}


static int ARM_DoCPUOpcode(int typ, int parm)
{
    int     val, val2, i;
    Str255  word;
    char    *oldLine;
    int     token;
    int     reg1, reg2, reg3, reg4;
    int     cond;
    uint32_t mode;
    uint16_t regbits;

    // get condition from opcode wildcard
    word[0] = 0;
    if (isalnum(*linePtr))
    {
        TOKEN_GetWord(word);
    }

    switch (typ)
    {
        case OP_Branch:       // Bcc / BLcc
            val = ARM_EvalBranch(24, 8);
            INSTR_L(parm | ((val >> 2) & 0x00FFFFFF));
            break;

        case OP_OneReg:       // Rm
            if (ARM_Cond(&cond, word)) return 0;

            reg1 = REG_Get(regs);
            if (reg1 > 15) reg1 = reg1 - 3; // SP LR PC -> R13 R14 R15
            if (REG_Check(reg1)) break;

            INSTR_L(parm | (cond << 28) | reg1);
            break;

        case OP_OneRegShiftS: // Rd,shifter with optional S
            val = ARM_OpcodeFlag(word, 'S');
            if (ARM_Cond(&cond, word)) return 0;

            reg1 = REG_Get(regs);
            if (reg1 > 15) reg1 = reg1 - 3; // SP LR PC -> R13 R14 R15
            if (REG_Check(reg1)) break;

            if (TOKEN_Comma()) break;

            if (ARM_Shifter(&mode)) break;

            INSTR_L(parm | (cond << 28) | (val << 20) | (reg1 << 12) | mode);
            break;

        case OP_OneRegShift:  // Rd,shifter
            if (ARM_Cond(&cond, word)) return 0;

            reg1 = REG_Get(regs);
            if (reg1 > 15) reg1 = reg1 - 3; // SP LR PC -> R13 R14 R15
            if (REG_Check(reg1)) break;

            if (TOKEN_Comma()) break;

            if (ARM_Shifter(&mode)) break;

            INSTR_L(parm | (cond << 28) | (1 << 20) | (reg1 << 16) | mode);
            break;

        case OP_TwoRegShiftS: // Rd,Rn,shifter with optional S
            val = ARM_OpcodeFlag(word, 'S');
            if (ARM_Cond(&cond, word)) return 0;

            reg1 = REG_Get(regs);
            if (reg1 > 15) reg1 = reg1 - 3; // SP LR PC -> R13 R14 R15
            if (REG_Check(reg1)) break;

            if (TOKEN_Comma()) break;

            reg2 = REG_Get(regs);
            if (reg2 > 15) reg2 = reg2 - 3; // SP LR PC -> R13 R14 R15
            if (REG_Check(reg2)) break;

            if (TOKEN_Comma()) break;

            if (ARM_Shifter(&mode)) break;

            INSTR_L(parm | (cond << 28) | (val << 20) | (reg2 << 16) | (reg1 << 12) | mode);
            break;

        case OP_SWI:          // SWI immed24
            if (ARM_Cond(&cond, word)) return 0;

            val = EXPR_Eval();
            // FIXME: need to check immed24 range

            INSTR_L(parm | (cond << 28) | (val & 0x00FFFFFF));
            break;

        case OP_BKPT:         // BKPT immed16
            val = EXPR_Eval();
            // FIXME: need to check immed16 range

            INSTR_L(parm  | ((val << 4) & 0x000FFF00) | (val & 0x0F));
            break;

        case OP_BLX:          // BLX
            if (ARM_Cond(&cond, word)) return 0;

            oldLine = linePtr;

            reg1 = REG_Get(regs);
            if (reg1 > 15) reg1 = reg1 - 3; // SP LR PC -> R13 R14 R15

            if (reg1 >= 0)
            {
                INSTR_L(parm | (cond << 28) | reg1);
            }
            else
            {
                linePtr = oldLine;

                if (cond != 14) return 0; // this form of BLX is always unconditional

                val = ARM_EvalBranch(25, 8);
                INSTR_L(0xFA000000 | ((val << 23) & 0x01000000) | ((val >> 2) & 0x00FFFFFF));
            }

            break;

        case OP_CLZ:          // CLZ
            if (ARM_Cond(&cond, word)) return 0;

            reg1 = REG_Get(regs);
            if (reg1 > 15) reg1 = reg1 - 3; // SP LR PC -> R13 R14 R15
            if (REG_Check(reg1)) break;

            if (TOKEN_Comma()) break;

            reg2 = REG_Get(regs);
            if (reg2 > 15) reg2 = reg2 - 3; // SP LR PC -> R13 R14 R15
            if (REG_Check(reg2)) break;

            INSTR_L(parm | (cond << 28) | (reg1 << 12) | reg2);
            break;

        case OP_MUL:          // MUL
            val = ARM_OpcodeFlag(word, 'S');
            if (ARM_Cond(&cond, word)) return 0;

            reg1 = REG_Get(regs);
            if (reg1 > 15) reg1 = reg1 - 3; // SP LR PC -> R13 R14 R15
            if (REG_Check(reg1)) break;

            if (TOKEN_Comma()) break;

            reg2 = REG_Get(regs);
            if (reg2 > 15) reg2 = reg2 - 3; // SP LR PC -> R13 R14 R15
            if (REG_Check(reg2)) break;

            if (TOKEN_Comma()) break;

            reg3 = REG_Get(regs);
            if (reg3 > 15) reg3 = reg3 - 3; // SP LR PC -> R13 R14 R15
            if (REG_Check(reg3)) break;

            INSTR_L(parm | (cond << 28) | (val << 20) | (reg1 << 16) | (reg3 << 8) | reg2);
            break;

        case OP_SWP:          // SWP
            val = ARM_OpcodeFlag(word, 'B');
            if (ARM_Cond(&cond, word)) return 0;

            reg1 = REG_Get(regs);
            if (reg1 > 15) reg1 = reg1 - 3; // SP LR PC -> R13 R14 R15
            if (REG_Check(reg1)) break;

            if (TOKEN_Comma()) break;

            reg2 = REG_Get(regs);
            if (reg2 > 15) reg2 = reg2 - 3; // SP LR PC -> R13 R14 R15
            if (REG_Check(reg2)) break;

            if (TOKEN_Comma()) break;
            if (TOKEN_Expect("[")) break;

            reg3 = REG_Get(regs);
            if (reg3 > 15) reg3 = reg3 - 3; // SP LR PC -> R13 R14 R15
            if (REG_Check(reg3)) break;

            if (TOKEN_Expect("]")) break;

            INSTR_L(parm | (cond << 28) | (val << 22) | (reg3 << 16) | (reg1 << 12) | reg2);
            break;

        case OP_LDM_STM:      // LDM/STM
            if ((reg2 = ARM_GetLDMType(word)) < 0) return 0;
            if (ARM_Cond(&cond, word)) return 0;

            if (reg2 > 4 && parm) reg2 = reg2 ^ 3; // invert ED/FD/EA/MA for LDM
            reg2 = reg2 & 3; // mask off IB/IA/DB/DA bits

            val = 0; // clear ^ and ! bits

            reg1 = REG_Get(regs);
            if (reg1 > 15) reg1 = reg1 - 3; // SP LR PC -> R13 R14 R15
            if (REG_Check(reg1)) break;

            oldLine = linePtr;
            token = TOKEN_GetWord(word);
            if (token == '!')
            {
                val = 1 << 21;
            }
            else
            {
                linePtr = oldLine;
            }

            if (TOKEN_Comma()) break;

            if (ARM_GetMultiRegs(&regbits)) break;

            oldLine = linePtr;
            token = TOKEN_GetWord(word);
            if (token == '^')
            {
                if (parm && (!val || (regbits & 0x8000)))
                {
                    val |= (1 << 22);
                }
                else
                {
                    ASMX_IllegalOperand();
                    break;
                }
            }
            else
            {
                linePtr = oldLine;
            }

            INSTR_L(parm | 0x08000000 | (cond << 28) | (reg2 << 23) | (reg1 << 16) | val | regbits);

            break;

        case OP_LDR_STR:      // LDR/STR
            if ((reg2 = ARM_GetLDRType(word)) < 0) return 0;
            if (!parm && (reg2 == LDR_SB || reg2 == LDR_SH)) return 0; // STRSB and STRSH are invalid
            if (ARM_Cond(&cond, word)) return 0;

            reg1 = REG_Get(regs);
            if (reg1 > 15) reg1 = reg1 - 3; // SP LR PC -> R13 R14 R15
            if (REG_Check(reg1)) break;

            if (TOKEN_Comma()) break;

            if (reg2 == LDR_H || reg2 == LDR_SB || reg2 == LDR_SH)
            {
                val = ARM_AddrMode3(&mode);
            }
            else
            {
                val = ARM_AddrMode2(&mode, reg2 == LDR_BT || reg2 == LDR_T);
            }
            if (val) break;

            switch (reg2)
            {
                case LDR_none:
                    INSTR_L(parm | (cond << 28) | 0x04000000 | (reg1 << 12) | mode);
                    break;

                case LDR_B:
                    INSTR_L(parm | (cond << 28) | 0x04400000 | (reg1 << 12) | mode);
                    break;

                case LDR_BT:
                    INSTR_L(parm | (cond << 28) | 0x04600000 | (reg1 << 12) | mode);
                    break;

                case LDR_H:
                    INSTR_L(parm | (cond << 28) | 0x000000B0 | (reg1 << 12) | mode);
                    break;

                case LDR_SB:
                    INSTR_L(parm | (cond << 28) | 0x000000D0 | (reg1 << 12) | mode);
                    break;

                case LDR_SH:
                    INSTR_L(parm | (cond << 28) | 0x000000F0 | (reg1 << 12) | mode);
                    break;

                case LDR_T:
                    INSTR_L(parm | (cond << 28) | 0x04200000 | (reg1 << 12) | mode);
                    break;
            }
            break;

        case OP_MRS:          // MRS
            if (ARM_Cond(&cond, word)) return 0;

            reg1 = REG_Get(regs);
            if (reg1 > 15) reg1 = reg1 - 3; // SP LR PC -> R13 R14 R15
            if (REG_Check(reg1)) break;

            if (TOKEN_Comma()) break;

            reg2 = REG_Get("CPSR SPSR");
            if (REG_Check(reg2)) break;

            INSTR_L(parm | (cond << 28) | (reg2 << 22) | (reg1 << 12));
            break;

        case OP_MSR:          // MSR
            if (ARM_Cond(&cond, word)) return 0;

            if (!TOKEN_GetWord(word))
            {
                ASMX_MissingOperand();
                break;
            }
            reg1 = ARM_GetMSRReg(word);
            if (reg1 < 0)
            {
                ASMX_IllegalOperand();
                break;
            }

            if (TOKEN_Comma()) break;

            token = TOKEN_GetWord(word);
            if (token == '#')
            {
                val = EXPR_Eval();
                INSTR_L(parm | (cond << 28) | (1 << 25) | reg1 | ARM_Immed(val));
            }
            else
            {
                reg2 = REG_Find(word, regs);
                if (reg2 > 15) reg2 = reg2 - 3; // SP LR PC -> R13 R14 R15
                if (REG_Check(reg2)) break;

                INSTR_L(parm | (cond << 28) | reg1 | reg2);
            }

            break;

        case OP_MLA:          // MLA
            val = ARM_OpcodeFlag(word, 'S');
            if (ARM_Cond(&cond, word)) return 0;

            reg1 = REG_Get(regs);
            if (reg1 > 15) reg1 = reg1 - 3; // SP LR PC -> R13 R14 R15
            if (REG_Check(reg1)) break;

            if (TOKEN_Comma()) break;

            reg2 = REG_Get(regs);
            if (reg2 > 15) reg2 = reg2 - 3; // SP LR PC -> R13 R14 R15
            if (REG_Check(reg2)) break;

            if (TOKEN_Comma()) break;

            reg3 = REG_Get(regs);
            if (reg3 > 15) reg3 = reg3 - 3; // SP LR PC -> R13 R14 R15
            if (REG_Check(reg1)) break;

            if (TOKEN_Comma()) break;

            reg4 = REG_Get(regs);
            if (reg4 > 15) reg4 = reg4 - 3; // SP LR PC -> R13 R14 R15
            if (REG_Check(reg2)) break;

            INSTR_L(parm | (cond << 28) | (val << 20) | (reg1 << 16) | (reg4 << 12) | (reg3 << 8) | reg2);
            break;

        case OP_MLAL:         // MLAL
            val = ARM_OpcodeFlag(word, 'S');
            if (ARM_Cond(&cond, word)) return 0;

            reg1 = REG_Get(regs);
            if (reg1 > 15) reg1 = reg1 - 3; // SP LR PC -> R13 R14 R15
            if (REG_Check(reg1)) break;

            if (TOKEN_Comma()) break;

            reg2 = REG_Get(regs);
            if (reg2 > 15) reg2 = reg2 - 3; // SP LR PC -> R13 R14 R15
            if (REG_Check(reg2)) break;

            if (TOKEN_Comma()) break;

            reg3 = REG_Get(regs);
            if (reg3 > 15) reg3 = reg3 - 3; // SP LR PC -> R13 R14 R15
            if (REG_Check(reg1)) break;

            if (TOKEN_Comma()) break;

            reg4 = REG_Get(regs);
            if (reg4 > 15) reg4 = reg4 - 3; // SP LR PC -> R13 R14 R15
            if (REG_Check(reg2)) break;

            INSTR_L(parm | (cond << 28) | (val << 20) | (reg2 << 16) | (reg1 << 12) | (reg4 << 8) | reg3);
            break;

        case OP_CDP:          // CDP
            if (word[0] == '2' && !word[1])
            {
                cond = 15;
            }
            else
            {
                if (ARM_Cond(&cond, word)) return 0;
            }

            val = REG_Get(pregs);
            if (REG_Check(val)) break;
            mode = val << 8;

            if (TOKEN_Comma()) break;

            val2 = EXPR_Eval();
            if (val2 < 0 || val2 > 15)
            {
                ASMX_IllegalOperand();
            }
            mode |= (val2 & 15) << 20;

            if (TOKEN_Comma()) break;

            reg1 = REG_Get(cregs);
            if (reg1 > 15) reg1 = reg1 - 3; // SP LR PC -> R13 R14 R15
            if (REG_Check(reg1)) break;

            if (TOKEN_Comma()) break;

            reg2 = REG_Get(cregs);
            if (reg2 > 15) reg2 = reg2 - 3; // SP LR PC -> R13 R14 R15
            if (REG_Check(reg2)) break;

            if (TOKEN_Comma()) break;

            reg3 = REG_Get(cregs);
            if (reg3 > 15) reg3 = reg3 - 3; // SP LR PC -> R13 R14 R15
            if (REG_Check(reg1)) break;

            if (TOKEN_Comma()) break;

            val = EXPR_Eval();
            if (val < 0 || val > 7) ASMX_IllegalOperand();
            mode |= (val & 7) << 5;

            INSTR_L(parm | (cond << 28) | mode | (reg2 << 16) | (reg1 << 12) | reg3);
            break;

        case OP_MCR_MRC:      // MCR/MRC
            if (word[0] == '2' && !word[1])
            {
                cond = 15;
            }
            else
            {
                if (ARM_Cond(&cond, word)) return 0;
            }

            val = REG_Get(pregs);
            if (REG_Check(val)) break;
            mode = val << 8;

            if (TOKEN_Comma()) break;

            val2 = EXPR_Eval();
            if (val2 < 0 || val2 > 7) ASMX_IllegalOperand();
            mode |= (val2 & 7) << 21;

            if (TOKEN_Comma()) break;

            reg1 = REG_Get(regs);
            if (reg1 > 15) reg1 = reg1 - 3; // SP LR PC -> R13 R14 R15
            if (REG_Check(reg1)) break;

            if (TOKEN_Comma()) break;

            reg2 = REG_Get(cregs);
            if (reg2 > 15) reg2 = reg2 - 3; // SP LR PC -> R13 R14 R15
            if (REG_Check(reg2)) break;

            if (TOKEN_Comma()) break;

            reg3 = REG_Get(cregs);
            if (reg3 > 15) reg3 = reg3 - 3; // SP LR PC -> R13 R14 R15
            if (REG_Check(reg1)) break;

            oldLine = linePtr;
            token = TOKEN_GetWord(word);
            if (token == ',')
            {
                val = EXPR_Eval();
                if (val < 0 || val > 7) ASMX_IllegalOperand();
                mode |= (val & 7) << 5;
            }
            else
            {
                linePtr = oldLine;
                if (token && TOKEN_Comma()) break;
            }

            INSTR_L(parm | (cond << 28) | mode | (reg2 << 16) | (reg1 << 12) | reg3);
            break;

        case OP_LDC_STC:      // LDC/STC
            if (word[0] == '2')
            {
                val = 0;
                cond = 15;
                if (word[1] == 'L')
                {
                    val = 1;
                    if (word[2]) return 0;
                }
                else if (word[1])
                {
                    return 0;
                }
            }
            else
            {
                val = ARM_OpcodeFlag(word, 'L');
                if (ARM_Cond(&cond, word)) return 0;
            }

            reg1 = REG_Get(pregs);
            if (REG_Check(reg1)) break;

            if (TOKEN_Comma()) break;

            reg2 = REG_Get(cregs);
            if (reg2 > 15) reg2 = reg2 - 3; // SP LR PC -> R13 R14 R15
            if (REG_Check(reg2)) break;

            if (TOKEN_Comma()) break;

            if (ARM_AddrMode5(&mode)) break;

            INSTR_L(parm | (cond << 28) | (val << 22) | (reg2 << 12) | (reg1 << 8) | mode);

            break;

        case OP_ADR:          // ADR/ADRL
// ADRL Rn,address
            if (word[0] == 'L')
            {
                parm = 1;
                if (ARM_Cond(&cond, word+1)) return 0;
            }
            else
            {
                if (ARM_Cond(&cond, word)) return 0;
            }

            reg1 = REG_Get(regs);
            if (REG_Check(reg1)) break;

            if (TOKEN_Comma()) break;

            val = EXPR_EvalLBranch(8);

            i = parm + 1; // number of longwords

            if (evalKnown && parm)
            {
                // *** determine auto size for ADRL
                if (-0xFF <= val && val <= 0xFF)
                {
                    i = 1;
                }
            }

            switch (i)
            {
                case 4:
                    break;

                case 3:
                    break;

                case 2:
                    if (val < 0)
                    {
                        INSTR_LL(0x028F0000 | (cond << 28) | (reg1 << 12) | (-val & 0xFF),
                                0x02800400 | (cond << 28) | (reg1 << 16) | (reg1 << 12) | ((-val >> 8) & 0xFF));
                    }
                    else
                    {
                        INSTR_LL(0x024F0000 | (cond << 28) | (reg1 << 12) | (val & 0xFF),
                                0x02400000 | (cond << 28) | (reg1 << 16) | (reg1 << 12) | ((val >> 8) & 0xFF));
                    }
                    break;

                default:
                    if (val < 0)
                    {
                        INSTR_L(0x028F0000 | (cond << 28) | (reg1 << 12) | (-val & 0xFF));
                    }
                    else
                    {
                        INSTR_L(0x024F0000 | (cond << 28) | (reg1 << 12) | (val & 0xFF));
                    }
                    break;
            }

// gets complicated because of offsets

//   ADR     R1,.            ; E24F1008
//   ADRL    R1,.            ; E24F1008
//   ADRL    R1,.+$1000      ; E28F1FFE E2811B03
//   ADRL    R1,.+$100000    ; E28F1FFE E2811BFF E2811703
//   ADRL    R1,.+$10000000  ; E28F1FFE E2811BFF E28117FF E2811303

//if (pass == 2) {
//    printf("*** %.8X *** %s\n", val, line);
//}
//            INSTR_L(   (cond << 28) | (val & 0x0FFF));
            break;

        case OP_Implied:      // NOP
            INSTR_L(parm);
            break;

        default:
            return 0;
            break;
    }

    if (locPtr & 3)
    {
        ASMX_Error("Code at non-longword-aligned address");
        // deposit extra bytes to reset alignment and prevent further errors
        for (int i = locPtr & 3; i < 4; i++)
        {
            INSTR_AddB(0);
        }
        // note: Inserting bytes in front won't work because offsets have already been assembled.
        // The line could be re-assembled by recursively calling DoCPUOpcode, but then
        // the label before the instruction would still be at the odd address.
    }

    return 1;
}


void ARM_AsmInit(void)
{
    void *p = ASMX_AddAsm(versionName, &ARM_DoCPUOpcode, NULL, NULL);

    ASMX_AddCPU(p, "ARM",    0, END_LITTLE, ADDR_24, LIST_24, 8, 0, ARM_opcdTab);
    ASMX_AddCPU(p, "ARM_BE", 0, END_BIG,    ADDR_24, LIST_24, 8, 0, ARM_opcdTab);
    ASMX_AddCPU(p, "ARM_LE", 0, END_LITTLE, ADDR_24, LIST_24, 8, 0, ARM_opcdTab);
}
