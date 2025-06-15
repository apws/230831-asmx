// asM68XX.c

#define versionName "M68XX assembler"
#include "asmx.h"

enum
{
    OP_Inherent,     // implied instructions
    OP_Inherent_01,  // implied instructions, 6801/6803/6811
    OP_Inherent_03,  // implied instructions, 6803 only
    OP_Inherent_11,  // implied instructions, 6811 only
    OP_Relative,     // branch instructions
    OP_Bit_03,       // 6303 AIM OIM EIM TIM instructions
    OP_Bit,          // 6811 BSET/BCLR
    OP_BRelative,    // 6811 BRSET/BRCLR
    OP_Logical,      // instructions with multiple addressing modes
    OP_Arith,        // arithmetic instructions with multiple addressing modes
    OP_LArith,       // OP_Arith instructions with 16-bit immediate modes
    OP_LArith_01,    // OP_Arith instructions with 16-bit immediate modes, 6801/6803/6811
    OP_LArith_11,    // OP_Arith instructions with 16-bit immediate modes, 6811 only
    OP_PSHPUL_AB,    // PSH A / PUL A / PSH B / PUL B with a blank (6800 opcodes only!)
    OP_Arith_AB,     // opcd A / opcd B with a blank (6800 opcodes only!)

//  o_Foo = OP_LabelOp,
};

enum cputype
{
    CPU_6800, CPU_6801, CPU_68HC11, CPU_6303
};

static const struct OpcdRec M68XX_opcdTab[] =
{
    {"TEST",  OP_Inherent_11, 0x00}, // 68HC11
    {"NOP",   OP_Inherent,    0x01},
    {"IDIV",  OP_Inherent_11, 0x02}, // 68HC11
    {"FDIV",  OP_Inherent_11, 0x03}, // 68HC11
    {"LSRD",  OP_Inherent_01, 0x04}, // 68HC11 6801 6303
    {"ASLD",  OP_Inherent_01, 0x05}, // 68HC11 6801 6303
    {"TAP",   OP_Inherent, 0x06},
    {"TPA",   OP_Inherent, 0x07},
    {"INX",   OP_Inherent, 0x08},
    {"DEX",   OP_Inherent, 0x09},
    {"CLV",   OP_Inherent, 0x0A},
    {"SEV",   OP_Inherent, 0x0B},
    {"CLC",   OP_Inherent, 0x0C},
    {"SEC",   OP_Inherent, 0x0D},
    {"CLI",   OP_Inherent, 0x0E},
    {"SEI",   OP_Inherent, 0x0F},
    {"SBA",   OP_Inherent, 0x10},
    {"CBA",   OP_Inherent, 0x11},
    {"TAB",   OP_Inherent, 0x16},
    {"TBA",   OP_Inherent, 0x17},
    {"DAA",   OP_Inherent, 0x19},
    {"ABA",   OP_Inherent, 0x1B},
    {"TSX",   OP_Inherent, 0x30},
    {"INS",   OP_Inherent, 0x31},
    {"PULA",  OP_Inherent, 0x32},
    {"PULB",  OP_Inherent, 0x33},
    {"DES",   OP_Inherent, 0x34},
    {"TXS",   OP_Inherent, 0x35},
    {"PSHA",  OP_Inherent, 0x36},
    {"PSHB",  OP_Inherent, 0x37},
    {"PULX",  OP_Inherent_01, 0x38}, // 68HC11 6801 6303
    {"RTS",   OP_Inherent,    0x39},
    {"ABX",   OP_Inherent_01, 0x3A}, // 68HC11 6801 6303
    {"RTI",   OP_Inherent,    0x3B},
    {"PSHX",  OP_Inherent_01, 0x3C}, // 68HC11 6801 6303
    {"MUL",   OP_Inherent_01, 0x3D}, // 68HC11 6801 6303
    {"WAI",   OP_Inherent, 0x3E},
    {"SWI",   OP_Inherent, 0x3F},
    {"NEGA",  OP_Inherent, 0x40},
    {"COMA",  OP_Inherent, 0x43},
    {"LSRA",  OP_Inherent, 0x44},
    {"RORA",  OP_Inherent, 0x46},
    {"ASRA",  OP_Inherent, 0x47},
    {"ASLA",  OP_Inherent, 0x48},
    {"ROLA",  OP_Inherent, 0x49},
    {"DECA",  OP_Inherent, 0x4A},
    {"INCA",  OP_Inherent, 0x4C},
    {"TSTA",  OP_Inherent, 0x4D},
    {"CLRA",  OP_Inherent, 0x4F},
    {"NEGB",  OP_Inherent, 0x50},
    {"COMB",  OP_Inherent, 0x53},
    {"LSRB",  OP_Inherent, 0x54},
    {"RORB",  OP_Inherent, 0x56},
    {"ASRB",  OP_Inherent, 0x57},
    {"ASLB",  OP_Inherent, 0x58},
    {"ROLB",  OP_Inherent, 0x59},
    {"DECB",  OP_Inherent, 0x5A},
    {"INCB",  OP_Inherent, 0x5C},
    {"TSTB",  OP_Inherent, 0x5D},
    {"CLRB",  OP_Inherent, 0x5F},
    {"XGDX",  OP_Inherent_11, 0x8F}, // 0x8F for 68HC11, 0x18 for 6303
    {"STOP",  OP_Inherent_11, 0xCF}, // 68HC11
    {"SLP",   OP_Inherent_03, 0x1A}, // 6303 only
    {"INY",   OP_Inherent_11, 0x1808}, // 68HC11
    {"DEY",   OP_Inherent_11, 0x1809}, // 68HC11
    {"TSY",   OP_Inherent_11, 0x1830}, // 68HC11
    {"TYS",   OP_Inherent_11, 0x1835}, // 68HC11
    {"PULY",  OP_Inherent_11, 0x1838}, // 68HC11
    {"ABY",   OP_Inherent_11, 0x183A}, // 68HC11
    {"PSHY",  OP_Inherent_11, 0x183C}, // 68HC11
    {"XGDY",  OP_Inherent_11, 0x188F}, // 68HC11

    {"BRA",   OP_Relative, 0x20},
    {"BRN",   OP_Relative, 0x21}, // 68HC11 6801 6303 (but probably works on 6800 anyhow)
    {"BHI",   OP_Relative, 0x22},
    {"BLS",   OP_Relative, 0x23},
    {"BCC",   OP_Relative, 0x24},
    {"BHS",   OP_Relative, 0x24},
    {"BCS",   OP_Relative, 0x25},
    {"BLO",   OP_Relative, 0x25},
    {"BNE",   OP_Relative, 0x26},
    {"BEQ",   OP_Relative, 0x27},
    {"BVC",   OP_Relative, 0x28},
    {"BVS",   OP_Relative, 0x29},
    {"BPL",   OP_Relative, 0x2A},
    {"BMI",   OP_Relative, 0x2B},
    {"BGE",   OP_Relative, 0x2C},
    {"BLT",   OP_Relative, 0x2D},
    {"BGT",   OP_Relative, 0x2E},
    {"BLE",   OP_Relative, 0x2F},
    {"BSR",   OP_Relative, 0x8D},

    {"NEG",   OP_Logical,  0x00}, // OP_Logical: indexed,X = $60; extended = $70; indexed,Y = $1860
    {"COM",   OP_Logical,  0x03},
    {"LSR",   OP_Logical,  0x04},
    {"ROR",   OP_Logical,  0x06},
    {"ASR",   OP_Logical,  0x07},
    {"ASL",   OP_Logical,  0x08},
    {"LSL",   OP_Logical,  0x08},
    {"ROL",   OP_Logical,  0x09},
    {"DEC",   OP_Logical,  0x0A},
    {"INC",   OP_Logical,  0x0C},
    {"TST",   OP_Logical,  0x0D},
    {"JMP",   OP_Logical,  0x0E},
    {"CLR",   OP_Logical,  0x0F},

    {"SUBA",  OP_Arith,    0x80}, // OP_Arith: immediate = $00, direct = $10, indexed,X = $20, indexed,Y = $1820
    {"CMPA",  OP_Arith,    0x81}, //           extended = $30
    {"SBCA",  OP_Arith,    0x82},
    {"SUBD",  OP_LArith_01,0x83}, // 68HC11 6801 6303
    {"ANDA",  OP_Arith,    0x84},
    {"BITA",  OP_Arith,    0x85},
    {"LDAA",  OP_Arith,    0x86},
    {"STAA",  OP_Arith,    0x97},
    {"EORA",  OP_Arith,    0x88},
    {"ADCA",  OP_Arith,    0x89},
    {"ORAA",  OP_Arith,    0x8A},
    {"ADDA",  OP_Arith,    0x8B},
    {"CPX",   OP_LArith,   0x8C},
    {"JSR",   OP_Arith,    0x9D}, // 9D is 68HC11 6801 6303 only
    {"LDS",   OP_LArith,   0x8E},
    {"STS",   OP_Arith,    0x9F},
    {"SUBB",  OP_Arith,    0xC0},
    {"CMPB",  OP_Arith,    0xC1},
    {"SBCB",  OP_Arith,    0xC2},
    {"ADDD",  OP_LArith_01,0xC3}, // 68HC11 6801 6303
    {"ANDB",  OP_Arith,    0xC4},
    {"BITB",  OP_Arith,    0xC5},
    {"LDAB",  OP_Arith,    0xC6},
    {"STAB",  OP_Arith,    0xD7},
    {"EORB",  OP_Arith,    0xC8},
    {"ADCB",  OP_Arith,    0xC9},
    {"ORAB",  OP_Arith,    0xCA},
    {"ADDB",  OP_Arith,    0xCB},
    {"LDD",   OP_LArith_01,0xCC}, // 68HC11 6801 6303
    {"STD",   OP_LArith_01,0xDD}, // 68HC11 6801 6303
    {"LDX",   OP_LArith,   0xCE},
    {"STX",   OP_Arith,    0xDF},
    {"CPD",   OP_LArith_11,0x1A83}, // 68HC11
    {"CPY",   OP_LArith_11,0x188C}, // 68HC11
    {"LDY",   OP_LArith_11,0x18CE}, // 68HC11
    {"STY",   OP_LArith_11,0x18DF}, // 68HC11

    {"BSET",  OP_Bit,      0},
    {"BCLR",  OP_Bit,      1},
    {"BRSET", OP_BRelative,0},
    {"BRCLR", OP_BRelative,1},

    {"AIM", OP_Bit_03, 0x61},
    {"OIM", OP_Bit_03, 0x62},
    {"EIM", OP_Bit_03, 0x65},
    {"TIM", OP_Bit_03, 0x6B},

    {"PSH", OP_PSHPUL_AB, 0x36},
    {"PUL", OP_PSHPUL_AB, 0x32},

    {"SUB",  OP_Arith_AB, 0x80},
    {"CMP",  OP_Arith_AB, 0x81},
    {"SBC",  OP_Arith_AB, 0x82},
    {"AND",  OP_Arith_AB, 0x84},
    {"BIT",  OP_Arith_AB, 0x85},
    {"LDA",  OP_Arith_AB, 0x86},
    {"STA",  OP_Arith_AB, 0x97},
    {"EOR",  OP_Arith_AB, 0x88},
    {"ADC",  OP_Arith_AB, 0x89},
    {"ORA",  OP_Arith_AB, 0x8A},
    {"ADD",  OP_Arith_AB, 0x8B},

    {"",     OP_Illegal, 0}
};


// --------------------------------------------------------------


static int M68XX_DoCPUOpcode(int typ, int parm)
{
    int     val, val2, val3;
    Str255  word;
    char    *oldLine;
    int     token;
    char    force;
    char    reg;

    switch (typ)
    {
        case OP_Inherent_01: // implied instructions, 6801/6803/6811
            if (typ == OP_Inherent_01 && curCPU == CPU_6800) return 0;
            FALLTHROUGH;
        case OP_Inherent_03: // implied instructions, 6803 only
            if (typ == OP_Inherent_03 && curCPU != CPU_6303) return 0;
            FALLTHROUGH;
        case OP_Inherent_11: // implied instructions, 6811 only
            if (parm == 0x8F && curCPU == CPU_6303)
            {
                parm = 0x18;    // 6303 XGDX
            }
            else if (typ == OP_Inherent_11 && curCPU != CPU_68HC11)
            {
                return 0;
            }
            FALLTHROUGH;
        case OP_Inherent:
            INSTR_X(parm);
            break;

        case OP_Relative:
            if (parm == 0x21 && curCPU == CPU_6800) return 0;  // BRN
            val = EXPR_EvalBranch(2);
            INSTR_XB(parm,val);
            break;

        case OP_Logical:
            // check for "opcd A" or "opcd B"
            oldLine = linePtr;
            TOKEN_GetWord(word);
            if (word[1] == 0 && (word[0] == 'A' || word[0] == 'B'))
            {
                reg = word[0] - 'A'; // 0=A 1=B
                INSTR_X(parm + 0x40 + 0x10*reg);
                break;
            }
            else
            {
                linePtr = oldLine;
            }

            force = 0;
            oldLine = linePtr;
            token = TOKEN_GetWord(word);

            if (token == '<' || token == '>')
            {
                // found < or > before operand
                force = token;
            }
            else if (token == 0)
            {
                // end of line
                ASMX_MissingOperand();
            }
            else if (token == ',')
            {
                // allow "JMP ,X" etc. instead of "JMP 0,X"
                switch (REG_Get("X Y"))
                {
                    case 0: // ,X
                        INSTR_XB(parm + 0x60, 0);
                        break;

                    case 1: // ,Y
                        if (curCPU == CPU_68HC11)
                        {
                            INSTR_XB(0x1800 + parm + 0x60, 0);
                            break;
                        }
                    // fall through

                    default:
                        TOKEN_BadMode();
                }
                break;
            }
            else if (word[0] == 'X' && word[1] == 0)
            {
                // for "JMP X" etc. in legacy 6800 syntax
                INSTR_XB(parm + 0x60, 0);
                break;
            }
            else
            {
                linePtr = oldLine;
            }

            val = EXPR_Eval();

            oldLine = linePtr;
            token = TOKEN_GetWord(word);
            switch (token)
            {
                case 0:
                    if (force == '<')
                    {
                        // 16-bit address only!
                        TOKEN_BadMode();
                        break;
                    }
                    INSTR_XW(parm + 0x70, val);
                    break;

                case ',':
                    if (force == '>')
                    {
                        // 8-bit offset only!
                        TOKEN_BadMode();
                        break;
                    }
                    switch (REG_Get("X Y"))
                    {
                        case 0: // ,X
                            INSTR_XB(parm + 0x60, val);
                            break;

                        case 1: // ,Y
                            if (curCPU == CPU_68HC11)
                            {
                                INSTR_XB(0x1800 + parm + 0x60, val);
                                break;
                            }
                        // fall through

                        default:
                            TOKEN_BadMode();
                            break;
                    }
                    break;

                default:
                    TOKEN_BadMode();
                    break;
            }
            break;

        case OP_LArith_01:
            if (typ == OP_LArith_01 && curCPU == CPU_6800) return 0;
            FALLTHROUGH;
        case OP_LArith_11:
            if (typ == OP_LArith_11 && curCPU != CPU_68HC11) return 0;
            FALLTHROUGH;
        case OP_Arith_AB:
            if (typ == OP_Arith_AB)
            {
                // check for "opcd A" or "opcd B"
                TOKEN_GetWord(word);
                if (word[1] == 0 && (word[0] == 'A' || word[0] == 'B'))
                {
                    reg = word[0] - 'A'; // 0=A 1=B
                    // change to OP_Arith with parm set for register A or B
                    typ = OP_Arith;
                    parm = parm + reg*0x40;
                }
                else if (word[0])
                {
                    ASMX_IllegalOperand();
                }
                else
                {
                   ASMX_MissingOperand();
                }
            }
            // abort if OP_Arith_AB failure
            if (typ == OP_Arith_AB)
            {
                break;
            }
            FALLTHROUGH;
        case OP_Arith:
        case OP_LArith:
            oldLine = linePtr;
            token = TOKEN_GetWord(word);
            if (token == '#')
            {
                if (parm & 0x10)    // immediate
                {
                    // don't allow STAA/STAB immediate
                    ASMX_Error("Invalid addressing mode");
                }
                else
                {
                    val = EXPR_Eval();
                    if (typ == OP_Arith)
                    {
                        INSTR_XB(parm & ~0x10, val);
                    }
                    else
                    {
                        INSTR_XW(parm & ~0x10, val);
                    }
                }
            }
            else
            {
                parm = parm & ~0x10;

                // allow "STAA ,X" etc. instead of "STAA 0,X"
                if (token == ',')
                {
                    switch (REG_Get("X Y"))
                    {
                        case 0: // ,X
                            if (parm >> 8 == 0x18)
                            {
                                INSTR_XB(0x1A00 + parm + 0x20, 0);
                            }
                            else
                            {
                                INSTR_XB(         parm + 0x20, 0);
                            }
                            break;

                        case 1: // ,Y
                            if (curCPU == CPU_68HC11)
                            {
                                if (parm == 0x8C || parm == 0xCE || parm == 0xCF
                                        || (parm >> 8 == 0x1A))
                                {
                                    INSTR_XB(0xCD00 + parm + 0x20, 0);
                                }
                                else
                                {
                                    INSTR_XB(0x1800 + parm + 0x20, 0);
                                }
                                break;
                            }
                        // fall through

                        default:
                            TOKEN_BadMode();
                    }
                    break;
                }

                if (word[0] == 'X' && word[1] == 0)
                {
                    // for "STAA X" etc. in legacy 6800 syntax
                    if (parm >> 8 == 0x18)
                    {
                        INSTR_XB(0x1A00 + parm + 0x20, 0);
                    }
                    else
                    {
                        INSTR_XB(         parm + 0x20, 0);
                    }
                    break;
                }

                force = 0;
                if (token == '<' || token == '>')
                {
                    force = token;
                }
                else
                {
                    linePtr = oldLine;
                }

                val = EXPR_Eval();

                oldLine = linePtr;
                token = TOKEN_GetWord(word);
                switch (token)
                {
                    case 0:
                        if (((force != '>' && evalKnown && (val & 0xFF00) >> 8 == 0) || force == '<')
                                && (curCPU != CPU_6800 || parm != 0x8D))
                        {
                            INSTR_XB(parm + 0x10, val);  // <$xx
                        }
                        else
                        {
                            INSTR_XW(parm + 0x30, val);  // >$xxxx
                        }
                        break;

                    case ',':
                        switch (REG_Get("X Y"))
                        {
                            case 0: // ,X
                                if (parm >> 8 == 0x18)
                                {
                                    INSTR_XB(0x1A00 + parm + 0x20, val);
                                }
                                else
                                {
                                    INSTR_XB(         parm + 0x20, val);
                                }
                                break;

                            case 1: // ,Y
                                if (curCPU == CPU_68HC11)
                                {
                                    if (parm == 0x8C || parm == 0xCE || parm == 0xCF
                                            || (parm >> 8 == 0x1A))
                                    {
                                        INSTR_XB(0xCD00 + parm + 0x20, val);
                                    }
                                    else
                                    {
                                        INSTR_XB(0x1800 + parm + 0x20, val);
                                    }
                                    break;
                                }
                            // fall through...

                            default:
                                TOKEN_BadMode();
                                break;
                        }
                        break;

                    default:
                        linePtr = oldLine;
                        TOKEN_Comma();
                        break;
                }
            }
            break;

        case OP_Bit_03:
            if (curCPU != CPU_6303) return 0;
            // AIM/OIM/EIM/TIM
            // opcode #,ofs,X (3 bytes)
            // opcode #,dir  (3 bytes)

            if (TOKEN_GetWord(word) != '#')
            {
                TOKEN_BadMode();
            }
            else
            {
                val = EXPR_Eval();   // get immediate value
                TOKEN_Comma();
                val2 = EXPR_Eval();  // get offset/address
                if (TOKEN_GetWord(word) == ',')
                {
                    if (REG_Get("X") == 0)
                    {
                        INSTR_XBB(parm, val, val2);
                    }
                    else
                    {
                        TOKEN_BadMode();
                    }
                }
                else     // direct mode = +0x10
                {
                    INSTR_XBB(parm + 0x10, val, val2);
                }
            }
            break;

        case OP_Bit:
            if (curCPU != CPU_68HC11) return 0;

            val = EXPR_Eval();   // direct page address or offset
            reg = 0;

            oldLine = linePtr;  // if comma present, may signal ,X or ,Y
            if (TOKEN_GetWord(word) == ',')
            {
                oldLine = linePtr;
                TOKEN_GetWord(word);
                if (word[1] == 0 && (word[0] == 'X' || word[0] == 'Y'))
                {
                    reg = word[0];
                    oldLine = linePtr;  // eat optional comma after ,X or ,Y
                    if (TOKEN_GetWord(word) != ',')
                    {
                        linePtr = oldLine;
                    }
                }
                else
                {
                    linePtr = oldLine; // not ,X or ,Y so reposition to after comma
                }
            }
            else
            {
                linePtr = oldLine; // optional comma not present, bit comes next
            }

            val2 = EXPR_Eval();  // mask

            switch (reg)
            {
                case 'X':
                    INSTR_XBB(parm +   0x1C, val, val2);
                    break;

                case 'Y':
                    INSTR_XBB(parm + 0x181C, val, val2);
                    break;

                default:
                    INSTR_XBB(parm +   0x14, val, val2);
                    break;
            }
            break;

        case OP_BRelative:
            if (curCPU != CPU_68HC11) return 0;

            val = EXPR_Eval();   // direct page address or offset
            reg = 0;

            oldLine = linePtr;  // if comma present, may signal ,X or ,Y
            if (TOKEN_GetWord(word) == ',')
            {
                oldLine = linePtr;
                TOKEN_GetWord(word);
                if (word[1] == 0 && (word[0] == 'X' || word[0] == 'Y'))
                {
                    reg = word[0];
                    oldLine = linePtr;  // eat optional comma after ,X or ,Y
                    if (TOKEN_GetWord(word) != ',')
                    {
                        linePtr = oldLine;
                    }
                }
                else
                {
                    linePtr = oldLine; // not ,X or ,Y so reposition to after comma
                }
            }
            else
            {
                linePtr = oldLine; // optional comma not present, bit comes next
            }

            val2 = EXPR_Eval();  // bit mask

            oldLine = linePtr;  // eat optional comma after bit mask
            if (TOKEN_GetWord(word) != ',')
            {
                linePtr = oldLine;
            }

            val3 = EXPR_EvalBranch(4 + (reg == 'Y'));  // offset

            switch (reg)
            {
                case 'X':
                    INSTR_XBBB(parm +   0x1E, val, val2, val3);
                    break;

                case 'Y':
                    INSTR_XBBB(parm + 0x181E, val, val2, val3);
                    break;

                default:
                    INSTR_XBBB(parm +   0x12, val, val2, val3);
                    break;
            }
            break;

        case OP_PSHPUL_AB:
            // check for "opcd A" or "opcd B"
            TOKEN_GetWord(word);
            if (word[1] == 0 && (word[0] == 'A' || word[0] == 'B'))
            {
                reg = word[0] - 'A'; // 0=A 1=B
                INSTR_X(parm + reg);
            }
            else if (word[0])
            {
                ASMX_IllegalOperand();
            }
            else
            {
                ASMX_MissingOperand();
            }
            break;

        default:
            return 0;
            break;
    }
    return 1;
}


void M68XX_AsmInit(void)
{
    void *p = ASMX_AddAsm(versionName, &M68XX_DoCPUOpcode, NULL, NULL);

    ASMX_AddCPU(p, "6800",    CPU_6800,   END_BIG, ADDR_16, LIST_24, 8, 0, M68XX_opcdTab);
    ASMX_AddCPU(p, "6801",    CPU_6801,   END_BIG, ADDR_16, LIST_24, 8, 0, M68XX_opcdTab);
    ASMX_AddCPU(p, "6802",    CPU_6800,   END_BIG, ADDR_16, LIST_24, 8, 0, M68XX_opcdTab);
    ASMX_AddCPU(p, "6803",    CPU_6801,   END_BIG, ADDR_16, LIST_24, 8, 0, M68XX_opcdTab);
    ASMX_AddCPU(p, "6808",    CPU_6801,   END_BIG, ADDR_16, LIST_24, 8, 0, M68XX_opcdTab);
    ASMX_AddCPU(p, "6303",    CPU_6303,   END_BIG, ADDR_16, LIST_24, 8, 0, M68XX_opcdTab);
    ASMX_AddCPU(p, "6811",    CPU_68HC11, END_BIG, ADDR_16, LIST_24, 8, 0, M68XX_opcdTab);
    ASMX_AddCPU(p, "68HC11",  CPU_68HC11, END_BIG, ADDR_16, LIST_24, 8, 0, M68XX_opcdTab);
    ASMX_AddCPU(p, "68HC711", CPU_68HC11, END_BIG, ADDR_16, LIST_24, 8, 0, M68XX_opcdTab);
    ASMX_AddCPU(p, "68HC811", CPU_68HC11, END_BIG, ADDR_16, LIST_24, 8, 0, M68XX_opcdTab);
    ASMX_AddCPU(p, "68HC99",  CPU_68HC11, END_BIG, ADDR_16, LIST_24, 8, 0, M68XX_opcdTab);
}
