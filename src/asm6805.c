// asmM6805.c

#define versionName "M6805 assembler"
#include "asmx.h"

enum
{
    OP_Inherent,     // implied instructions
    OP_Relative,     // branch instructions
    OP_Bit,          // BCLR/BSET
    OP_BRelative,    // BRSET/BRCLR
    OP_Logical,      // instructions with multiple addressing modes
    OP_Arith,        // arithmetic instructions with multiple addressing modes
    OP_Store,        // STA/STX/JMP/JSR - same as OP_Arith except no immediate allowed

    OP_Inh08,        // 68HCS08 inherent instructions
    OP_Rel08,        // 68HCS08 branch instructions and DBNZA/DBNZX
    OP_AIS_AIX,      // 68HCS08 AIS and AIX instructions
    OP_LDHX,         // 68HCS08 LDHX, STHX, CPHX instructions
    OP_CBEQA,        // 68HCS08 CBEQA, CBEQX instructions
    OP_CBEQ,         // 68HCS08 CBEQ, DBNZ instructions
    OP_MOV,          // 68HCS08 MOV instruction

//  o_Foo = OP_LabelOp,
};

enum cputype
{
    CPU_6805,
    CPU_68HCS08
};

static const struct OpcdRec M6805_opcdTab[] =
{
    {"NEGA",  OP_Inherent, 0x40},
    {"COMA",  OP_Inherent, 0x43},
    {"LSRA",  OP_Inherent, 0x44},
    {"RORA",  OP_Inherent, 0x46},
    {"ASRA",  OP_Inherent, 0x47},
    {"ASLA",  OP_Inherent, 0x48},
    {"LSLA",  OP_Inherent, 0x48},
    {"ROLA",  OP_Inherent, 0x49},
    {"DECA",  OP_Inherent, 0x4A},
    {"INCA",  OP_Inherent, 0x4C},
    {"TSTA",  OP_Inherent, 0x4D},
    {"CLRA",  OP_Inherent, 0x4F},

    {"NEGX",  OP_Inherent, 0x50},
    {"COMX",  OP_Inherent, 0x53},
    {"LSRX",  OP_Inherent, 0x54},
    {"RORX",  OP_Inherent, 0x56},
    {"ASRX",  OP_Inherent, 0x57},
    {"ASLX",  OP_Inherent, 0x58},
    {"LSLX",  OP_Inherent, 0x58},
    {"ROLX",  OP_Inherent, 0x59},
    {"DECX",  OP_Inherent, 0x5A},
    {"INCX",  OP_Inherent, 0x5C},
    {"TSTX",  OP_Inherent, 0x5D},
    {"CLRX",  OP_Inherent, 0x5F},

    {"MUL",   OP_Inherent, 0x42},
    {"RTI",   OP_Inherent, 0x80},
    {"RTS",   OP_Inherent, 0x81},
    {"SWI",   OP_Inherent, 0x83},
    {"STOP",  OP_Inherent, 0x8E},
    {"WAIT",  OP_Inherent, 0x8F},
    {"TAX",   OP_Inherent, 0x97},
    {"CLC",   OP_Inherent, 0x98},
    {"SEC",   OP_Inherent, 0x99},
    {"CLI",   OP_Inherent, 0x9A},
    {"SEI",   OP_Inherent, 0x9B},
    {"RSP",   OP_Inherent, 0x9C},
    {"NOP",   OP_Inherent, 0x9D},
    {"TXA",   OP_Inherent, 0x9F},

    {"BRA",   OP_Relative, 0x20},
    {"BRN",   OP_Relative, 0x21},
    {"BHI",   OP_Relative, 0x22},
    {"BLS",   OP_Relative, 0x23},
    {"BCC",   OP_Relative, 0x24},
    {"BCS",   OP_Relative, 0x25},
    {"BHS",   OP_Relative, 0x24},
    {"BLO",   OP_Relative, 0x25},
    {"BNE",   OP_Relative, 0x26},
    {"BEQ",   OP_Relative, 0x27},
    {"BHCC",  OP_Relative, 0x28},
    {"BHCS",  OP_Relative, 0x29},
    {"BPL",   OP_Relative, 0x2A},
    {"BMI",   OP_Relative, 0x2B},
    {"BMC",   OP_Relative, 0x2C},
    {"BMS",   OP_Relative, 0x2D},
    {"BIL",   OP_Relative, 0x2E},
    {"BIH",   OP_Relative, 0x2F},

    {"BSR",   OP_Relative, 0xAD},

    {"NEG",   OP_Logical,  0x00}, // OP_Logical: direct = $30; ix1 = $60, idx = $70
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
    {"CLR",   OP_Logical,  0x0F},


    {"SUB",   OP_Arith,    0x00}, // OP_Arith: immediate = $80, direct = $90, ix2 = $A0
    {"CMP",   OP_Arith,    0x01}, //           ix1 = $E0, extended = $F0
    {"SBC",   OP_Arith,    0x02},
    {"CPX",   OP_Arith,    0x03},
    {"AND",   OP_Arith,    0x04},
    {"BIT",   OP_Arith,    0x05},
    {"LDA",   OP_Arith,    0x06},
    {"STA",   OP_Store,    0x07},
    {"EOR",   OP_Arith,    0x08},
    {"ADC",   OP_Arith,    0x09},
    {"ORA",   OP_Arith,    0x0A},
    {"ADD",   OP_Arith,    0x0B},
    {"JMP",   OP_Store,    0x0C},
    {"JSR",   OP_Store,    0x0D},
    {"LDX",   OP_Arith,    0x0E},
    {"STX",   OP_Store,    0x0F},

    {"BSET",  OP_Bit,      0x10},
    {"BCLR",  OP_Bit,      0x11},
    {"BRSET", OP_BRelative,0x00},
    {"BRCLR", OP_BRelative,0x01},

    // 68HCS08 opcodes

    {"DIV",   OP_Inh08,    0x52},
    {"NSA",   OP_Inh08,    0x62},
    {"DAA",   OP_Inh08,    0x72},
    {"BGND",  OP_Inh08,    0x82},
    {"TAP",   OP_Inh08,    0x84},
    {"TPA",   OP_Inh08,    0x85},
    {"PULA",  OP_Inh08,    0x86},
    {"PSHA",  OP_Inh08,    0x87},
    {"PULX",  OP_Inh08,    0x88},
    {"PSHX",  OP_Inh08,    0x89},
    {"PULH",  OP_Inh08,    0x8A},
    {"PSHH",  OP_Inh08,    0x8B},
    {"CLRH",  OP_Inh08,    0x8C},
    {"TXS",   OP_Inh08,    0x94},
    {"TSX",   OP_Inh08,    0x95},

    {"BGE",   OP_Rel08,    0x90},
    {"BLT",   OP_Rel08,    0x91},
    {"BGT",   OP_Rel08,    0x92},
    {"BLE",   OP_Rel08,    0x93},
    {"DBNZA", OP_Rel08,    0x4B},
    {"DBNZX", OP_Rel08,    0x5B},

    {"AIS",   OP_AIS_AIX,  0xA7},
    {"AIX",   OP_AIS_AIX,  0xAF},

    {"LDHX",  OP_LDHX,     0x0E},
    {"STHX",  OP_LDHX,     0x0F},
    {"CPHX",  OP_LDHX,     0x03},
    {"CBEQA", OP_CBEQA,    0x41},
    {"CBEQX", OP_CBEQA,    0x51},

    {"CBEQ",  OP_CBEQ,     0x01},
    {"DBNZ",  OP_CBEQ,     0x0B},

    {"MOV",   OP_MOV,         0},

    {"",      OP_Illegal,     0}
};


// --------------------------------------------------------------


static int M6805_DoCPUOpcode(int typ, int parm)
{
    int     val, val2, val3;
    Str255  word;
    char    *oldLine;
    int     token;
    char    force;
    char    reg;

    switch (typ)
    {
        case OP_Inh08:       // 68HCS08 inherent instructions
            if (curCPU != CPU_68HCS08) return 0;
            FALLTHROUGH;
        case OP_Inherent:
            INSTR_B(parm);
            break;

        case OP_Rel08:       // 68HCS08 branch instructions and DBNZA/DBNZX
            if (curCPU != CPU_68HCS08) return 0;
            FALLTHROUGH;
        case OP_Relative:
            val = EXPR_EvalBranch(2);
            INSTR_BB(parm, val);
            break;

        case OP_Logical:
            oldLine = linePtr;
            token = TOKEN_GetWord(word);  // look for ",X"
            if (token == ',')
            {
                if (TOKEN_Expect("X")) break;
                INSTR_B(parm + 0x70);
                break;
            }

            linePtr = oldLine;
            val = EXPR_Eval();

            oldLine = linePtr;
            token = TOKEN_GetWord(word);  // look for ",X"
            switch (token)
            {
                case 0:
                    INSTR_BB(parm + 0x30, val);  // no ",X", so must be direct
                    break;

                case ',':
                    switch ((reg = REG_Get("X SP")))
                    {
                        default:
                        case reg_None:
                            ASMX_IllegalOperand();
                            break;

                        case reg_EOL: // EOL
                            break;

                        case 1: // X
                            parm = parm + 0x9E00;
                            FALLTHROUGH;
                        case 0: // SP
                            if (evalKnown && val == 0 && parm < 256)
                            {
                                INSTR_B(parm + 0x70); // 0,X
                            }
                            else
                            {
                                INSTR_XB(parm + 0x60, val); // ix1,X / sp1,SP
                            }
                    }
                    break;

                default:
                    linePtr = oldLine;
                    TOKEN_Comma();
                    break;
            }
            break;

        case OP_Arith:
        case OP_Store:
            oldLine = linePtr;
            token = TOKEN_GetWord(word);
            switch (token)
            {
                case '#': // immediate
                    if (typ == OP_Store)
                    {
                        ASMX_Error("Invalid addressing mode");
                    }
                    else
                    {
                        val = EXPR_Eval();
                        INSTR_BB(parm + 0xA0, val);
                    }
                    break;

                case ',': // ,X
                    if (TOKEN_Expect("X")) break;
                    INSTR_B(parm + 0xF0);
                    break;

                default:
                    force = 0;
                    parm = parm & ~0x10;

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
                        case 0: // dir or ext
                            if ((force != '>' && evalKnown && (val & 0xFF00) >> 8 == 0)
                                    || force == '<')
                            {
                                EXPR_CheckByte(val);
                                INSTR_BB(parm + 0xB0, val);  // <$xx
                            }
                            else
                            {
                                INSTR_BW(parm + 0xC0, val);   // >$xxxx
                            }
                            break;

                        case ',': // ix1,X or ix2,X
                            switch ((reg = REG_Get("X SP")))
                            {
                                default:
                                case reg_None:
                                    ASMX_IllegalOperand();
                                    break;

                                case reg_EOL: // EOL
                                    break;

                                case 1:
                                    if (parm == 0x0C || parm == 0x0D)   // JMP / JSR
                                    {
                                        TOKEN_BadMode();
                                        break;
                                    }
                                    parm = parm + 0x9E00;
                                    FALLTHROUGH;
                                case 0:
                                    if (evalKnown && val == 0 && parm < 256)   // 0,X
                                    {
                                        INSTR_B(parm + 0xF0);
                                    }
                                    else if ((force != '>' && evalKnown &&
                                              (val & 0xFF00) >> 8 == 0)
                                             || force == '<')
                                    {
                                        EXPR_CheckByte(val);
                                        INSTR_XB(parm + 0xE0, val); // ix1,X / sp1,SP
                                    }
                                    else
                                    {
                                        INSTR_XW(parm + 0xD0, val); // ix2,X / sp2,SP
                                    }
                            }
                            break;

                        default:
                            linePtr = oldLine;
                            TOKEN_Comma();
                            break;
                    }
            }
            break;

        case OP_Bit:
            val = EXPR_Eval();   // bit number
            if (val < 0 || val > 7)
            {
                ASMX_Error("Bit number must be 0 - 7");
            }

            oldLine = linePtr;  // eat optional comma after bit number
            if (TOKEN_GetWord(word) != ',')
            {
                linePtr = oldLine;
            }

            val2 = EXPR_Eval();      // direct page address or offset
            reg = 0;

            INSTR_BB(parm + val*2, val2);
            break;

        case OP_BRelative:
            val = EXPR_Eval();   // bit number
            if (val < 0 || val > 7)
            {
                ASMX_Error("Bit number must be 0 - 7");
            }

            oldLine = linePtr;  // eat optional comma after bit number
            if (TOKEN_GetWord(word) != ',')
            {
                linePtr = oldLine;
            }

            val2 = EXPR_Eval();      // direct page address or offset
            reg = 0;

            oldLine = linePtr;  // eat optional comma after direct page address
            if (TOKEN_GetWord(word) != ',')
            {
                linePtr = oldLine;
            }

            val3 = EXPR_EvalBranch(3);  // offset

            INSTR_BBB(parm + val*2, val2, val3);
            break;

        case OP_AIS_AIX:      // 68HCS08 AIS and AIX instructions
            if (curCPU != CPU_68HCS08) return 0;
            if (TOKEN_Expect("#")) break; // immediate only
            val = EXPR_Eval();
            EXPR_CheckStrictByte(val);
            INSTR_BB(parm, val);
            break;

        case OP_LDHX:        // 68HCS08 LDHX, STHX, CPHX instructions
            if (curCPU != CPU_68HCS08) return 0;
            oldLine = linePtr;
            token = TOKEN_GetWord(word);
            switch (token)
            {
                case '#': // 45 LDHX #imm / 65 CPHX #imm
                    if (parm == 0x0F)
                    {
                        TOKEN_BadMode();
                    }
                    else
                    {
                        val = EXPR_Eval();
                        INSTR_BW(0x45 + (parm & 0x01)*0x20, val);
                        break;
                    }
                    break;

                case ',': // 9EAE LDHX ,X
                    if (parm != 0x0E)
                    {
                        TOKEN_BadMode();
                    }
                    else
                    {
                        if (TOKEN_Expect("X")) break;
                        INSTR_X(0x9EAE);
                        break;
                    }
                    break;

                default:
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
                    if (token == 0)   // dir or ext
                    {
                        if ((force != '>' && evalKnown && (val & 0xFF00) >> 8 == 0)
                                || force == '<')
                        {
                            EXPR_CheckByte(val);
                            switch (parm)
                            {
                                case 0x0E:
                                    parm = 0x55;
                                    break; // LDHX
                                case 0x0F:
                                    parm = 0x35;
                                    break; // STHX
                                default:
                                    parm = 0x75;
                                    break; // CPHX
                            }
                            INSTR_BB(parm, val); // ix1,X / sp1,SP
                        }
                        else
                        {
                            switch (parm)
                            {
                                case 0x0E:
                                    parm = 0x32;
                                    break; // LDHX
                                case 0x0F:
                                    parm = 0x96;
                                    break; // STHX
                                default:
                                    parm = 0x3E;
                                    break; // CPHX
                            }
                            INSTR_BW(parm, val); // ix2,X / sp2,SP
                        }
                    }
                    else     // ofs,reg
                    {
                        linePtr = oldLine;
                        if (TOKEN_Comma()) break;
                        switch ((reg = REG_Get("X SP")))
                        {
                            default:
                            case reg_None:
                                ASMX_IllegalOperand();
                                break;

                            case reg_EOL: // EOL
                                break;

                            case 0: // X
                                if (parm != 0x0E)   // LDHX
                                {
                                    TOKEN_BadMode();
                                    break;
                                }
                                if ((force != '>' && evalKnown && (val & 0xFF00) >> 8 == 0)
                                        || force == '<')
                                {
                                    EXPR_CheckByte(val);
                                    INSTR_XB(parm + 0x9EC0, val); // ix1,X
                                }
                                else
                                {
                                    INSTR_XW(parm + 0x9EB0, val); // ix2,X
                                }
                                break;

                            case 1: // SP
                                EXPR_CheckByte(val);
                                INSTR_XB(parm + 0x9EF0, val); // sp1,SP
                                break;
                        }
                    }
                    break;
            }
            break;

        case OP_CBEQA:       // 68HCS08 CBEQA, CBEQX instructions
            if (curCPU != CPU_68HCS08) return 0;
            if (TOKEN_Expect("#")) break; // immediate only
            val = EXPR_EvalByte();
            if (TOKEN_Comma()) break;
            val2 = EXPR_EvalBranch(3);
            INSTR_BBB(parm, val, val2);
            break;

        case OP_CBEQ:        // 68HCS08 CBEQ, DBNZ instructions
            if (curCPU != CPU_68HCS08) return 0;
            oldLine = linePtr;
            if (TOKEN_GetWord(word) == ',')
            {
                if (TOKEN_Expect("X")) break;
                if (parm == 0x01 && TOKEN_Expect("+")) break;
                if (TOKEN_Comma()) break;
                val = EXPR_EvalBranch(2);
                INSTR_BB(parm + 0x70, val);
            }
            else
            {
                linePtr = oldLine;
                val = EXPR_EvalByte();
                if (TOKEN_Comma()) break;
                switch (reg = (REG_Get("X SP")))
                {
                    case 0: // ,X or ,X+
                        oldLine = linePtr;
                        if (parm == 0x01 && TOKEN_GetWord(word) != '+')
                        {
                            linePtr = oldLine;
                            FALLTHROUGH;
                        case reg_None:
                            val2 = EXPR_EvalBranch(3);
                            INSTR_BBB(parm + 0x30, val, val2);
                            break;
                        }
                        FALLTHROUGH;
                    case 1: // ,SP
                        if (reg == 1) parm = parm + 0x9E00;
                        if (TOKEN_Comma()) break;
                        if (reg == 1)
                        {
                            val2 = EXPR_EvalBranch(4);
                        }
                        else
                        {
                            val2 = EXPR_EvalBranch(3);
                        }
                        INSTR_XBB(parm + 0x60, val, val2);
                        break;

                    default:
                        ASMX_IllegalOperand();
                        break;

                    case reg_EOL: // EOL
                        break;
                }
            }
            break;

        case OP_MOV:         // 68HCS08 MOV instruction
            if (curCPU != CPU_68HCS08) return 0;
            oldLine = linePtr;
            switch (TOKEN_GetWord(word))
            {
                case ',':
                    if (TOKEN_Expect("X")) break;
                    if (TOKEN_Expect("+")) break;
                    if (TOKEN_Comma()) break;
                    val = EXPR_EvalByte();
                    INSTR_BB(0x7E, val); // MOV ,X+,dir
                    break;

                case '#':
                    val = EXPR_EvalByte();
                    if (TOKEN_Comma()) break;
                    val2 = EXPR_EvalByte();
                    INSTR_BBB(0x6E, val, val2); // MOV imm,dir
                    break;

                default:
                    linePtr = oldLine;
                    val = EXPR_EvalByte();
                    if (TOKEN_Comma()) break;
                    oldLine = linePtr;
                    if (REG_Get("X") == 0 && (REG_Get("+") == 0))
                    {
                        INSTR_BB(0x5E, val); // 5E MOV dir,X+
                    }
                    else
                    {
                        linePtr = oldLine;
                        val2 = EXPR_EvalByte();
                        INSTR_BBB(0x4E, val, val2); // 4E MOV dir,dir
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


void M6805_AsmInit(void)
{
    void *p = ASMX_AddAsm(versionName, &M6805_DoCPUOpcode, NULL, NULL);

    ASMX_AddCPU(p, "6805",    CPU_6805,    END_BIG, ADDR_16, LIST_24, 8, 0, M6805_opcdTab);
    ASMX_AddCPU(p, "68HC05",  CPU_6805,    END_BIG, ADDR_16, LIST_24, 8, 0, M6805_opcdTab);
    ASMX_AddCPU(p, "68HCS08", CPU_68HCS08, END_BIG, ADDR_16, LIST_24, 8, 0, M6805_opcdTab);
}
