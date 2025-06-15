// asmM68HC16.c

#define versionName "M68HC16 assembler"
#include "asmx.h"

enum
{
    OP_Inherent,     // implied instructions
    OP_Branch,       // short relative branch instructions
    OP_LBranch,      // long relative branch instructions
    o_ImmediateW,   // immediate word instructions (ORP and ANDP)
    o_AIX,          // AIX/AIY/AIZ/AIS instructions
    o_MAC,          // MAC/RMAC instructions
    o_PSHM,         // PSHM instruction
    o_PULM,         // PULM instruction
    o_MOVB,         // MOVB/MOVW instructions
    OP_Logical,      // byte-data logical instructions
    o_LogicalW,     // word-data logical instructions
    o_STE,          // STE instruction
    o_ADDE,         // ADDE instruction
    o_ArithE,       // arithmetic operations on register E
    o_BCLRW,        // BCLRW/BSETW instructions
    o_BCLR,         // BCLR/BSET instructions
    o_BRCLR,        // BRCLR/BRSET instructions
    o_STX,          // STX/STY/STZ/STS instructions
    o_LDX,          // LDX/LDY/LDZ/LDS instructions
    o_ArithX,       // arithmetic operations on X register
    OP_JMP,          // JMP/JSR instructions
    o_STD,          // STD instruction
    o_StoreAB,      // STAA/STAB instructions
    o_ADDD,         // ADDD instruction
    o_ArithW,       // word-data arithmetic instructions
    OP_Arith,        // byte-data arithmetic instructions

//  o_Foo = OP_LabelOp,
};

static const struct OpcdRec M68HC16_opcdTab[] =
{
    {"ABA",   OP_Inherent, 0x370B},
    {"ABX",   OP_Inherent, 0x374F},
    {"ABY",   OP_Inherent, 0x375F},
    {"ABZ",   OP_Inherent, 0x376F},
    {"ACE",   OP_Inherent, 0x3722},
    {"ACED",  OP_Inherent, 0x3723},
    {"ADE",   OP_Inherent, 0x2778},
    {"ADX",   OP_Inherent, 0x37CD},
    {"ADY",   OP_Inherent, 0x37DD},
    {"ADZ",   OP_Inherent, 0x37ED},
    {"AEX",   OP_Inherent, 0x374D},
    {"AEY",   OP_Inherent, 0x375D},
    {"AEZ",   OP_Inherent, 0x376D},
    {"ASLA",  OP_Inherent, 0x3704},
    {"ASLB",  OP_Inherent, 0x3714},
    {"ASLD",  OP_Inherent, 0x27F4},
    {"ASLE",  OP_Inherent, 0x2774},
    {"ASLM",  OP_Inherent, 0x27B6},
    {"ASRA",  OP_Inherent, 0x370D},
    {"ASRB",  OP_Inherent, 0x371D},
    {"ASRD",  OP_Inherent, 0x27FD},
    {"ASRE",  OP_Inherent, 0x277D},
    {"ASRM",  OP_Inherent, 0x27BA},
    {"BGND",  OP_Inherent, 0x37A6},
    {"CBA",   OP_Inherent, 0x371B},
    {"CLRA",  OP_Inherent, 0x3705},
    {"CLRB",  OP_Inherent, 0x3715},
    {"CLRD",  OP_Inherent, 0x27F5},
    {"CLRE",  OP_Inherent, 0x2775},
    {"CLRM",  OP_Inherent, 0x27B7},
    {"COMA",  OP_Inherent, 0x3700},
    {"COMB",  OP_Inherent, 0x3710},
    {"COMD",  OP_Inherent, 0x27F0},
    {"COME",  OP_Inherent, 0x2770},
    {"DAA",   OP_Inherent, 0x3721},
    {"DECA",  OP_Inherent, 0x3701},
    {"DECB",  OP_Inherent, 0x3711},
    {"DECE",  OP_Inherent, 0x2771},
    {"EDIV",  OP_Inherent, 0x3728},
    {"EDIVS", OP_Inherent, 0x3729},
    {"EMUL",  OP_Inherent, 0x3725},
    {"EMULS", OP_Inherent, 0x3726},
    {"FDIV",  OP_Inherent, 0x372B},
    {"FMULS", OP_Inherent, 0x3727},
    {"IDIV",  OP_Inherent, 0x372A},
    {"INCA",  OP_Inherent, 0x3703},
    {"INCB",  OP_Inherent, 0x3713},
    {"INCE",  OP_Inherent, 0x2773},
    {"LDHI",  OP_Inherent, 0x27B0},
    {"LPSTOP",OP_Inherent, 0x27F1},
    {"LSRA",  OP_Inherent, 0x370F},
    {"LSRB",  OP_Inherent, 0x371F},
    {"LSRD",  OP_Inherent, 0x27FF},
    {"LSRE",  OP_Inherent, 0x277F},
    {"MUL",   OP_Inherent, 0x3724},
    {"NEGA",  OP_Inherent, 0x3702},
    {"NEGB",  OP_Inherent, 0x3712},
    {"NEGD",  OP_Inherent, 0x27F2},
    {"NEGE",  OP_Inherent, 0x2772},
    {"NOP",   OP_Inherent, 0x27CC},
    {"PSHA",  OP_Inherent, 0x3708},
    {"PSHB",  OP_Inherent, 0x3718},
    {"PSHMAC",OP_Inherent, 0x27B8},
    {"PULA",  OP_Inherent, 0x3709},
    {"PULB",  OP_Inherent, 0x3719},
    {"PULMAC",OP_Inherent, 0x27B9},
    {"ROLA",  OP_Inherent, 0x370C},
    {"ROLB",  OP_Inherent, 0x371C},
    {"ROLD",  OP_Inherent, 0x27FC},
    {"ROLE",  OP_Inherent, 0x277C},
    {"RORA",  OP_Inherent, 0x370E},
    {"RORB",  OP_Inherent, 0x371E},
    {"RORD",  OP_Inherent, 0x27FE},
    {"RORE",  OP_Inherent, 0x277E},
    {"RTI",   OP_Inherent, 0x2777},
    {"RTS",   OP_Inherent, 0x27F7},
    {"SBA",   OP_Inherent, 0x370A},
    {"SDE",   OP_Inherent, 0x2779},
    {"SWI",   OP_Inherent, 0x3720},
    {"SXT",   OP_Inherent, 0x27F8},
    {"TAB",   OP_Inherent, 0x3717},
    {"TAP",   OP_Inherent, 0x37FD},
    {"TBA",   OP_Inherent, 0x3707},
    {"TBEK",  OP_Inherent, 0x27FA},
    {"TBSK",  OP_Inherent, 0x379F},
    {"TBXK",  OP_Inherent, 0x379C},
    {"TBYK",  OP_Inherent, 0x379D},
    {"TBZK",  OP_Inherent, 0x379E},
    {"TDE",   OP_Inherent, 0x277B},
    {"TDMSK", OP_Inherent, 0x372F},
    {"TDP",   OP_Inherent, 0x372D},
    {"TED",   OP_Inherent, 0x27FB},
    {"TEDM",  OP_Inherent, 0x27B1},
    {"TEKB",  OP_Inherent, 0x27BB},
    {"TEM",   OP_Inherent, 0x27B2},
    {"TMER",  OP_Inherent, 0x27B4},
    {"TMET",  OP_Inherent, 0x27B5},
    {"TMXED", OP_Inherent, 0x27B3},
    {"TPA",   OP_Inherent, 0x37FC},
    {"TPD",   OP_Inherent, 0x372C},
    {"TSKB",  OP_Inherent, 0x37AF},
    {"TSTA",  OP_Inherent, 0x3706},
    {"TSTB",  OP_Inherent, 0x3716},
    {"TSTD",  OP_Inherent, 0x27F6},
    {"TSTE",  OP_Inherent, 0x2776},
    {"TSX",   OP_Inherent, 0x27CF},
    {"TSY",   OP_Inherent, 0x275F},
    {"TSZ",   OP_Inherent, 0x276F},
    {"TXKB",  OP_Inherent, 0x37AC},
    {"TXS",   OP_Inherent, 0x374E},
    {"TXY",   OP_Inherent, 0x275C},
    {"TXZ",   OP_Inherent, 0x276C},
    {"TYKB",  OP_Inherent, 0x37AD},
    {"TYS",   OP_Inherent, 0x375E},
    {"TYX",   OP_Inherent, 0x27CD},
    {"TYZ",   OP_Inherent, 0x276D},
    {"TZKB",  OP_Inherent, 0x37AE},
    {"TZS",   OP_Inherent, 0x376E},
    {"TZX",   OP_Inherent, 0x27CE},
    {"TZY",   OP_Inherent, 0x275E},
    {"WAI",   OP_Inherent, 0x27F3},
    {"XGAB",  OP_Inherent, 0x371A},
    {"XGDE",  OP_Inherent, 0x277A},
    {"XGDX",  OP_Inherent, 0x37CC},
    {"XGDY",  OP_Inherent, 0x37DC},
    {"XGDZ",  OP_Inherent, 0x37EC},
    {"XGEX",  OP_Inherent, 0x374C},
    {"XGEY",  OP_Inherent, 0x375C},
    {"XGEZ",  OP_Inherent, 0x376C},

    {"BCC",   OP_Branch,   0xB4}, // aka BHS
    {"BCS",   OP_Branch,   0xB5}, // aka BLO
    {"BEQ",   OP_Branch,   0xB7},
    {"BGE",   OP_Branch,   0xBC},
    {"BGT",   OP_Branch,   0xBE},
    {"BHI",   OP_Branch,   0xB2},
    {"BLE",   OP_Branch,   0xBF},
    {"BLS",   OP_Branch,   0xB3},
    {"BLT",   OP_Branch,   0xBD},
    {"BMI",   OP_Branch,   0xBB},
    {"BNE",   OP_Branch,   0xB6},
    {"BPL",   OP_Branch,   0xBA},
    {"BRA",   OP_Branch,   0xB0},
    {"BRN",   OP_Branch,   0xB1},
    {"BSR",   OP_Branch,   0x36},
    {"BVC",   OP_Branch,   0xB8},
    {"BVS",   OP_Branch,   0xB9},

    {"LBCC",  OP_LBranch,  0x3784},    // aka LBHS
    {"LBCS",  OP_LBranch,  0x3785},    // aka LBLO
    {"LBEQ",  OP_LBranch,  0x3787},
    {"LBEV",  OP_LBranch,  0x3791},
    {"LBGE",  OP_LBranch,  0x378C},
    {"LBGT",  OP_LBranch,  0x378E},
    {"LBHI",  OP_LBranch,  0x3782},
    {"LBLE",  OP_LBranch,  0x378F},
    {"LBLS",  OP_LBranch,  0x3783},
    {"LBLT",  OP_LBranch,  0x378D},
    {"LBMI",  OP_LBranch,  0x378B},
    {"LBMV",  OP_LBranch,  0x3790},
    {"LBNE",  OP_LBranch,  0x3786},
    {"LBPL",  OP_LBranch,  0x378A},
    {"LBRA",  OP_LBranch,  0x3780},
    {"LBRN",  OP_LBranch,  0x3781},
    {"LBSR",  OP_LBranch,  0x27F9},
    {"LBVC",  OP_LBranch,  0x3788},
    {"LBVS",  OP_LBranch,  0x3789},

    {"ANDP",  o_ImmediateW, 0x373A},
    {"ORP",   o_ImmediateW, 0x373B},

    {"AIS",   o_AIX,     0x3F},
    {"AIX",   o_AIX,     0x3C},
    {"AIY",   o_AIX,     0x3D},
    {"AIZ",   o_AIX,     0x3E},

    {"AIS",   o_AIX,     0x3F},
    {"AIX",   o_AIX,     0x3C},
    {"AIY",   o_AIX,     0x3D},
    {"AIZ",   o_AIX,     0x3E},

    {"MAC",   o_MAC,     0x7B},
    {"RMAC",  o_MAC,     0xFB},

    {"PSHM",  o_PSHM,    0x34},
    {"PULM",  o_PULM,    0x35},

    {"MOVB",  o_MOVB,    0x00},
    {"MOVW",  o_MOVB,    0x01},

    {"ASLW", o_LogicalW, 0x04},
    {"ASRW", o_LogicalW, 0x0D},
    {"CLRW", o_LogicalW, 0x05},
    {"COMW", o_LogicalW, 0x00},
    {"DECW", o_LogicalW, 0x01},
    {"INCW", o_LogicalW, 0x03},
    {"LSRW", o_LogicalW, 0x0F},
    {"NEGW", o_LogicalW, 0x02},
    {"ROLW", o_LogicalW, 0x0C},
    {"RORW", o_LogicalW, 0x0E},
    {"TSTW", o_LogicalW, 0x06},

    {"STE",  o_STE,      0x0A},
    {"ADDE", o_ADDE,     0x01},
    {"ADCE", o_ArithE,   0x03},
    {"ANDE", o_ArithE,   0x06},
    {"CPE",  o_ArithE,   0x08},
    {"EORE", o_ArithE,   0x04},
    {"LDE",  o_ArithE,   0x05},
    {"ORE",  o_ArithE,   0x07},
    {"SBCE", o_ArithE,   0x02},
    {"SUBE", o_ArithE,   0x00},

    {"ASL",  OP_Logical,  0x04},
    {"ASR",  OP_Logical,  0x0D},
    {"CLR",  OP_Logical,  0x05},
    {"COM",  OP_Logical,  0x00},
    {"DEC",  OP_Logical,  0x01},
    {"INC",  OP_Logical,  0x03},
    {"LSR",  OP_Logical,  0x0F},
    {"NEG",  OP_Logical,  0x02},
    {"ROL",  OP_Logical,  0x0C},
    {"ROR",  OP_Logical,  0x0E},
    {"TST",  OP_Logical,  0x06},

    {"BCLRW", o_BCLRW,   0x2708},
    {"BSETW", o_BCLRW,   0x2709},
    {"BCLR",  o_BCLR,    0x08},
    {"BSET",  o_BCLR,    0x09},

    {"BRCLR", o_BRCLR,   0x0A},
    {"BRSET", o_BRCLR,   0x0B},

    {"STS",   o_STX,     0x8F},
    {"STX",   o_STX,     0x8C},
    {"STY",   o_STX,     0x8D},
    {"STZ",   o_STX,     0x8E},
    {"LDX",   o_LDX,     0xCC},
    {"LDY",   o_LDX,     0xCD},
    {"LDZ",   o_LDX,     0xCE},
    {"CPS",   o_ArithX,  0x4F},
    {"CPX",   o_ArithX,  0x4C},
    {"CPY",   o_ArithX,  0x4D},
    {"CPZ",   o_ArithX,  0x4E},
    {"LDS",   o_ArithX,  0xCF},

    {"JMP",   OP_JMP,     0x4B},
    {"JSR",   OP_JMP,     0x89},

    {"STD",   o_STD,     0x378A},
    {"STAA",  o_StoreAB, 0x174A},
    {"STAB",  o_StoreAB, 0x17CA},
    {"ADDD",  o_ADDD,    0x3781},
    {"ADCD",  o_ArithW,  0x3783},
    {"ANDD",  o_ArithW,  0x3786},
    {"CPD",   o_ArithW,  0x3788},
    {"EORD",  o_ArithW,  0x3784},
    {"LDD",   o_ArithW,  0x3785},
    {"ORD",   o_ArithW,  0x3787},
    {"SBCD",  o_ArithW,  0x3782},
    {"SUBD",  o_ArithW,  0x3780},
    {"ADCA",  OP_Arith,   0x1743},
    {"ADCB",  OP_Arith,   0x17C3},
    {"ADDA",  OP_Arith,   0x1741},
    {"ADDB",  OP_Arith,   0x17C1},
    {"ANDA",  OP_Arith,   0x1746},
    {"ANDB",  OP_Arith,   0x17C6},
    {"BITA",  OP_Arith,   0x1749},
    {"BITB",  OP_Arith,   0x17C9},
    {"CMPA",  OP_Arith,   0x1748},
    {"CMPB",  OP_Arith,   0x17C8},
    {"EORA",  OP_Arith,   0x1744},
    {"EORB",  OP_Arith,   0x17C4},
    {"LDAA",  OP_Arith,   0x1745},
    {"LDAB",  OP_Arith,   0x17C5},
    {"ORAA",  OP_Arith,   0x1747},
    {"ORAB",  OP_Arith,   0x17C7},
    {"SBCA",  OP_Arith,   0x1742},
    {"SBCB",  OP_Arith,   0x17C2},
    {"SUBA",  OP_Arith,   0x1740},
    {"SUBB",  OP_Arith,   0x17C0},

    {"",     OP_Illegal,  0}
};


// --------------------------------------------------------------

static int M68HC16_GetIndex(void)
{
    Str255  word;

    char *oldLine = linePtr;
    int token = TOKEN_GetWord(word);
    if (token == ',')
    {
        token = REG_Get("X Y Z");
        if (token >= 0) return token;
    }
    linePtr = oldLine;
    return -1;
}


static int M68HC16_DoCPUOpcode(int typ, int parm)
{
    int     val, val2, val3;
    Str255  word;
    char    *oldLine;
    int     token;
//  char    force;
    char    reg;
    bool    known;

    switch (typ)
    {
        case OP_Inherent:
            INSTR_X(parm);
            break;

        case OP_Branch:
            val = EXPR_EvalBranch(2);
            INSTR_XB(parm, val);
            break;

        case OP_LBranch:
            if (parm < 256)
            {
                val = EXPR_EvalLBranch(3);
            }
            else
            {
                val = EXPR_EvalLBranch(4);
            }
            INSTR_XW(parm, val);
            break;

        case o_ImmediateW:
            TOKEN_Expect("#");
            val = EXPR_Eval();
            INSTR_XW(parm, val);
            break;

        case o_AIX:
            TOKEN_Expect("#");
            val = EXPR_Eval();
            if (evalKnown && -128 <= val && val <= 127)
            {
                INSTR_XB(parm, val);
            }
            else
            {
                INSTR_XW(parm + 0x3700, val);
            }
            break;

        case o_MAC:
            oldLine = linePtr;
            token = TOKEN_GetWord(word);
            if (token == '#')
            {
                val = EXPR_Eval();
                INSTR_XB(parm, val);
            }
            else
            {
                linePtr = oldLine;
                val = EXPR_Eval();
                TOKEN_Comma();
                val2 = EXPR_Eval();
                if (val < 0 || val > 15 || val2 < 0 || val2 > 15)
                {
                    ASMX_IllegalOperand();
                }
                else
                {
                    INSTR_XB(parm, val * 16 + val2);
                }
            }
            break;

        case o_PSHM:
        case o_PULM:
            val = 0;
            token = TOKEN_GetWord(word);
            while (token)
            {
                reg = REG_Find(word, "D E X Y Z K CCR");
                if (reg < 0)
                {
                    ASMX_IllegalOperand();
                }
                else
                {
                    if (typ == o_PULM)
                    {
                        reg = 6 - reg;
                    }
                    reg = 1 << reg;
                    if (val & reg)
                    {
                        ASMX_Error("PSHM/PULM register used twice");
                    }
                    else
                    {
                        val = val | reg;
                    }
                }

                token = TOKEN_GetWord(word);
                if (token == ',')
                {
                    val2 = TOKEN_GetWord(word);
                    if (val2 == 0)
                    {
                        ASMX_MissingOperand();
                        break;
                    }
                }
            }

            if (val == 0)
            {
                ASMX_Warning("PSHM/PULM with no registers");
            }
            else
            {
                INSTR_XB(parm, val);
            }

            break;

        case o_MOVB:
            // $xxxx,$yyyy = parm+$37FE
            // $xxxx,$yy,X = parm+$32
            // $xx,X,$yyyy = parm+$30

            val = EXPR_Eval();
            reg = M68HC16_GetIndex();
            if (reg == 0)   // $xx,X,$yyyy
            {
                EXPR_CheckByte(val);
                TOKEN_Comma();
                val2 = EXPR_Eval();
                INSTR_XBW(parm + 0x30, val, val2);
            }
            else if (reg < 0)
            {
                TOKEN_Comma();
                val2 = EXPR_Eval();
                reg = M68HC16_GetIndex();
                if (reg == 0)   // $xxxx,$yy,X
                {
                    EXPR_CheckByte(val2);
                    INSTR_XBW(parm + 0x32, val2, val);
                }
                else if (reg < 0)     // $xxxx,$yyyy
                {
                    INSTR_XWW(parm + 0x37FE, val, val2);
                    if (listWid == LIST_24)
                    {
                        hexSpaces = 0x14;
                    }
                    else
                    {
                        instrLen = -instrLen; // more than 5 bytes, so use long DB listing format
                    }
                }
                else
                {
                    ASMX_IllegalOperand();
                }
            }
            else
            {
                ASMX_IllegalOperand();
            }
            break;

        case o_LogicalW:
            // $xxxx,X = parm+$2700
            // $xxxx,Y = parm+$2710
            // $xxxx,Z = parm+$2720
            // $xxxx   = parm+$2730

            val = EXPR_Eval();
            reg = M68HC16_GetIndex();
            if (reg < 0)
            {
                // no index register
                INSTR_XW(parm + 0x2730, val);
            }
            else
            {
                INSTR_XW(parm + 0x2700 + reg*16, val);
            }
            break;

        case OP_Logical:
            // $xx,X   = parm+$00
            // $xx,Y   = parm+$10
            // $xx,Z   = parm+$20
            // $xxxx,X = parm+$1700
            // $xxxx,Y = parm+$1710
            // $xxxx,Z = parm+$1720
            // $xxxx   = parm+$1730
            // A       = parm+$3700
            // B       = parm+$3710
            // D       = parm+$27F0
            // E       = parm+$2770
            // M: ASLM=27B6 ASRM=27BA CLRM=27B7

            val = EXPR_Eval();
            reg = M68HC16_GetIndex();
            if (reg < 0)
            {
                // no index register
                INSTR_XW(parm + 0x1730, val);
            }
            else
            {
                if (evalKnown && 0 <= val && val <= 255)
                {
                    INSTR_XB(parm +          reg*16, val);
                }
                else
                {
                    INSTR_XW(parm + 0x1700 + reg*16, val);
                }
            }
            break;

        case o_STE:
        case o_ADDE:
        case o_ArithE:
            // #$xx    = 7C xx (only on o_ADDE)
            // #$xxxx  = parm+$3730 (not on o_StoreE)
            // $xxxx,X = parm+$3740
            // $xxxx,Y = parm+$3750
            // $xxxx,Z = parm+$3760
            // $xxxx   = parm+$3770

            oldLine = linePtr;
            token = TOKEN_GetWord(word);
            if (token == '#' && typ != o_STE)
            {
                val = EXPR_Eval();
                if (evalKnown && typ == o_ADDE && -128 <= val && val <= 127)
                {
                    INSTR_XB(         0x7C, val);
                }
                else
                {
                    INSTR_XW(parm + 0x3730, val);
                }
            }
            else
            {
                linePtr = oldLine;
                val = EXPR_Eval();
                reg = M68HC16_GetIndex();
                if (reg < 0)
                {
                    // no index register
                    INSTR_XW(parm + 0x3770, val);
                }
                else
                {
                    if (evalKnown && 0 <= val && val <= 255)
                    {
                        INSTR_XB(parm +          reg*16, val);
                    }
                    else
                    {
                        INSTR_XW(parm + 0x3740 + reg*16, val);
                    }
                }
            }
            break;

        case o_BCLRW:
        case o_BCLR:
        // $xxxx,X,#$mm = parm+$00
        // $xxxx,Y,#$mm = parm+$10
        // $xxxx,Z,#$mm = parm+$20
        // $xxxx,#$mm   = parm+$30
        // $xx,X,#$mm   = parm+$1700 (o_BCLR only)
        // $xx,Y,#$mm   = parm+$1710 (o_BCLR only)
        // $xx,Z,#$mm   = parm+$1720 (o_BCLR only)

        case o_BRCLR:
            // $xxxx,X,#$mm = parm+$00
            // $xxxx,Y,#$mm = parm+$10
            // $xxxx,Z,#$mm = parm+$20
            // $xxxx,#$mm   = parm+$30
            // $xx,X,#$mm   = (~parm & 0x01)*$40 + $8B
            // $xx,Y,#$mm   = (~parm & 0x01)*$40 + $9B
            // $xx,Z,#$mm   = (~parm & 0x01)*$40 + $AB

            val = EXPR_Eval();
            known = evalKnown;
            reg = M68HC16_GetIndex();
            TOKEN_Comma();
            TOKEN_Expect("#");
            val2 = EXPR_EvalByte();
            if (typ == o_BRCLR)
            {
                TOKEN_Comma();
                if (known && 0 <= val && val <= 255)
                {
                    val3 = EXPR_EvalBranch(4);
                    if (reg < 0)
                    {
                        reg = 3;
                    }
                    INSTR_XBBB((~parm & 0x01)*0x40 + 0x8B + reg*16, val2, val, val3);
                }
                else
                {
                    val3 = EXPR_EvalBranch(5);
                    if (reg < 0)
                    {
                        reg = 3;
                    }
                    INSTR_XBWB((parm&0xFF) + reg*16, val2, val, val3);
                }
            }
            else if (reg < 0)
            {
                INSTR_XBW(parm + 0x30, val2, val);
            }
            else
            {
                if (known && typ == o_BCLR && 0 <= val && val <= 255)
                {
                    INSTR_XBB(parm + 0x1700 + reg*16, val2, val);
                }
                else
                {
                    INSTR_XBW(parm +          reg*16, val2, val);
                }
            }
            break;

        case o_STX:
        case o_LDX:
        case o_ArithX:
            // $xx,X   = parm+$00
            // $xx,Y   = parm+$10
            // $xx,Z   = parm+$20
            // $xxxx,X = parm+$1700
            // $xxxx,Y = parm+$1710
            // $xxxx,Z = parm+$1720
            // $xxxx   = parm+$1730
            // #$xxxx  = parm+$3730 (except o_STX o_LDX)
            // #$xxxx  = parm+$3730-$40 (o_LDX only)

            oldLine = linePtr;
            token = TOKEN_GetWord(word);
            if (token == '#' && typ != o_STX)
            {
                val = EXPR_Eval();
                if (typ == o_LDX)
                {
                    INSTR_XW(parm + 0x3730 - 0x40, val);
                }
                else
                {
                    INSTR_XW(parm + 0x3730, val);
                }
            }
            else
            {
                linePtr = oldLine;
                val = EXPR_Eval();
                reg = M68HC16_GetIndex();
                if (reg < 0)
                {
                    // no index register
                    INSTR_XW(parm + 0x1730, val);
                }
                else
                {
                    if (evalKnown && 0 <= val && val <= 255)
                    {
                        INSTR_XB(parm +          reg*16, val);
                    }
                    else
                    {
                        INSTR_XW(parm + 0x1700 + reg*16, val);
                    }
                }
            }
            break;

        case OP_JMP:
            // JMP=$4B, JSR=$89
            // $xxxx    = $7A+(parm & $80)
            // $xxxxx,X = parm+$00
            // $xxxxx,Y = parm+$10
            // $xxxxx,Z = parm+$20

            val = EXPR_Eval();
            reg = M68HC16_GetIndex();
            if (reg < 0)
            {
                // no index register
                INSTR_XW(0x7A + (parm & 0x80), val);
            }
            else
            {
                INSTR_X3(parm + reg*16, val & 0xFFFFF); // 20-bit address
            }
            break;

        case o_STD:
        case o_StoreAB:
        case o_ADDD:
        case o_ArithW:
        case OP_Arith:
            // #$xx     = $FC (ADDD only)
            // #$xxxx   = parm+$30 (o_ArithW only)
            // $xxxx,X  = parm+$00 (parm+$40 o_ArithW,o_ADDD,o_STD)
            // $xxxx,Y  = parm+$10 (parm+$50 o_ArithW,o_ADDD,o_STD)
            // $xxxx,Z  = parm+$20 (parm+$60 o_ArithW,o_ADDD,o_STD)
            // $xxxx    = parm+$30 (parm+$70 o_ArithW,o_ADDD,o_STD)
            // $xx,X    = (parm & 0xFF)+$00
            // $xx,Y    = (parm & 0xFF)+$10
            // $xx,Z    = (parm & 0xFF)+$20
            // #$xx     = (parm & 0xFF)+$30 (OP_Arith only)
            // E,X      = (parm & 0xFF)+$2700
            // E,Y      = (parm & 0xFF)+$2710
            // E,Z      = (parm & 0xFF)+$2720

            oldLine = linePtr;
            token = TOKEN_GetWord(word);
            if (token == '#' && typ != o_StoreAB && typ != o_STD)
            {
                val = EXPR_Eval();
                if        (evalKnown && typ == o_ADDD  && -128 <= val && val <= 127)
                {
                    INSTR_XB(                0xFC, val);
                }
                else if (evalKnown && typ == OP_Arith && -128 <= val && val <= 127)
                {
                    INSTR_XB((parm & 0xFF) + 0x30, val);
                }
                else
                {
                    INSTR_XW( parm         + 0x30, val);
                }
            }
            else if (REG_Find(word, "E") == 0)
            {
                reg = M68HC16_GetIndex();
                if (reg < 0)
                {
                    TOKEN_BadMode();
                }
                else
                {
                    INSTR_X((parm & 0xFF) + 0x2700 + reg*16);
                }
            }
            else
            {
                linePtr = oldLine;
                val = EXPR_Eval();
                reg = M68HC16_GetIndex();
                if (reg < 0)
                {
                    // no index register
                    if (typ == o_ArithW || typ == o_ADDD || typ == o_STD)
                    {
                        INSTR_XW(parm + 0x70, val);
                    }
                    else
                    {
                        INSTR_XW(parm + 0x30, val);
                    }
                }
                else
                {
                    if (evalKnown && 0 <= val && val <= 255)
                    {
                        INSTR_XB((parm & 0xFF) + reg*16, val);
                    }
                    else if (typ == o_ArithW || typ == o_ADDD || typ == o_STD)
                    {
                        INSTR_XW( parm + 0x40  + reg*16, val);
                    }
                    else
                    {
                        INSTR_XW( parm +         reg*16, val);
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


void M68HC16_AsmInit(void)
{
    void *p = ASMX_AddAsm(versionName, &M68HC16_DoCPUOpcode, NULL, NULL);

    ASMX_AddCPU(p, "68HC16", 0, END_BIG, ADDR_16, LIST_24, 8, 0, M68HC16_opcdTab);
}
