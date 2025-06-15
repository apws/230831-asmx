// asmJAG.c

#define versionName "Atari Jaguar GPU/DSP assembler"
#include "asmx.h"

enum instrType
{
    OP_None,     // no operands - NOP
    OP_One,      // one operand - Rd - ABS/NEG/etc
    OP_Two,      // two operands - Rs,Rd - most instructions
    OP_Num_15,   // numeric operand - n,Rd - n=-16..+15 - CMPQ
    OP_Num_31,   // numeric operand - n,Rd - n=0..31 - BCLR/BSET/BTST/MOVEQ
    OP_Num_32,   // numeric operand - n,Rd - n=1..32 - ADDQ/SUBQ
    OP_JR,       // jump relative - cc,n - n=-16..+15 words, reg2=cc
    OP_JUMP,     // jump absolute - cc,(Rs) - reg2=cc
    OP_MOVEI,    // move immediate - n,Rn - n in second word
    OP_MOVE,     // MOVE instruction - PC,Rn / Rn,Rn
    OP_LOAD,     // LOAD instruction - various forms
    OP_LOADn,    // LOADB/LOADP/LOADW - (Rs),Rd
    OP_STORE,    // STORE instruction - various forms
    OP_STOREn,   // STOREB/STOREP/STOREM - Rs,(Rd)

//  o_Foo = OP_LabelOp,
};

enum
{
    CPU_TOM,    // GPU
    CPU_JERRY   // DSP
};

enum
{
    SUB32   = 0x2000, // n = 32-n
    TOMONLY = 0x4000, // opcode is Tom-only
    JERONLY = 0x8000  // opcode is Jerry-only
};

static const struct OpcdRec JAG_opcdTab[] =
{
    {"ADD",     OP_Two,     0},
    {"ADDC",    OP_Two,     1},
    {"ADDQ",    OP_Num_32,  2},
    {"ADDQT",   OP_Num_32,  3},
    {"SUB",     OP_Two,     4},
    {"SUBC",    OP_Two,     5},
    {"SUBQ",    OP_Num_32,  6},
    {"SUBQT",   OP_Num_32,  7},
    {"NEG",     OP_One,     8},
    {"AND",     OP_Two,     9},
    {"OR",      OP_Two,    10},
    {"XOR",     OP_Two,    11},
    {"NOT",     OP_One,    12},
    {"BTST",    OP_Num_31, 13},
    {"BSET",    OP_Num_31, 14},
    {"BCLR",    OP_Num_31, 15},

    {"MULT",    OP_Two,    16},
    {"IMULT",   OP_Two,    17},
    {"IMULTN",  OP_Two,    18},
    {"RESMAC",  OP_One,    19},
    {"IMACN",   OP_Two,    20},
    {"DIV",     OP_Two,    21},
    {"ABS",     OP_One,    22},
    {"SH",      OP_Two,    23},
    {"SHLQ",    OP_Num_32, 24 + SUB32}, // encodes 32-n!
    {"SHRQ",    OP_Num_32, 25 + SUB32}, // encodes 32-n!
    {"SHA",     OP_Two,    26},
    {"SHARQ",   OP_Num_32, 27 + SUB32}, // encodes 32-n!
    {"ROR",     OP_Two,    28},
    {"RORQ",    OP_Num_32, 29},
    {"ROLQ",    OP_Num_32, 29 + SUB32}, // same as RORQ 32-n,Rn
    {"CMP",     OP_Two,    30},
    {"CMPQ",    OP_Num_15, 31},

    {"SAT8",    OP_One,    32 + TOMONLY},
    {"SUBQMOD", OP_Num_32, 32 + JERONLY},
    {"SAT16",   OP_One,    33 + TOMONLY},
    {"SAT16S",  OP_One,    33 + JERONLY},
//  {"MOVE",    OP_MOVE,   34},
    {"MOVEQ",   OP_Num_31, 35},
    {"MOVETA",  OP_Two,    36},
    {"MOVEFA",  OP_Two,    37},
    {"MOVEI",   OP_MOVEI,  38},
    {"LOADB",   OP_LOADn,  39},
    {"LOADW",   OP_LOADn,  40},
//  {"LOAD",    OP_LOAD,   41},
    {"LOADP",   OP_LOADn,  42 + TOMONLY},
    {"SAT32S",  OP_One,    42 + JERONLY},
//  {"LOAD",    OP_LOAD,   43}, // (R14+n),Rn
//  {"LOAD",    OP_LOAD,   44}, // (R15+n),Rn
    {"STOREB",  OP_STOREn, 45},
    {"STOREW",  OP_STOREn, 46},
//  {"STORE",   OP_STORE,  47},

    {"STOREP",  OP_STOREn, 48 + TOMONLY},
    {"MIRROR",  OP_One,    48 + JERONLY},
//  {"STORE",   OP_STORE,  49}, // Rn,(R14+n)
//  {"STORE",   OP_STORE,  50}, // Rn,(R15+n)
//  {"MOVE",    OP_MOVE,   51},
    {"JUMP",    OP_JUMP,   52},
    {"JR",      OP_JR,     53},
    {"MMULT",   OP_Two,    54},
    {"MTOI",    OP_Two,    55},
    {"NORMI",   OP_Two,    56},
    {"NOP",     OP_None,   57},
//  {"LOAD",    OP_LOAD,   58}, // (R14+Rn),Rn
//  {"LOAD",    OP_LOAD,   59}, // (R15+Rn),Rn
//  {"STORE",   OP_STORE,  60}, // Rn,(R14+Rn)
//  {"STORE",   OP_STORE,  61}, // Rn,(R15+Rn)
    {"SAT24",   OP_One,    62},
    {"UNPACK",  OP_One,    63 + TOMONLY},
    {"PACK",    OP_One,    63 + TOMONLY + (1 << 6)},
    {"ADDQMOD", OP_Num_32, 63 + JERONLY},

    {"MOVE",    OP_MOVE,    0},
    {"LOAD",    OP_LOAD,    0},
    {"STORE",   OP_STORE,   0},

    {"",        OP_Illegal, 0}
};


const char jag_regs[]    = "R0 R1 R2 R3 R4 R5 R6 R7 R8 R9 R10 R11 R12 R13 R14 R15 R16 "
                           "R17 R18 R19 R20 R21 R22 R23 R24 R25 R26 R27 R28 R29 R30 R31";
const char jag_regs_PC[] = "R0 R1 R2 R3 R4 R5 R6 R7 R8 R9 R10 R11 R12 R13 R14 R15 R16 "
                           "R17 R18 R19 R20 R21 R22 R23 R24 R25 R26 R27 R28 R29 R30 R31 PC";

const char jag_conds[] = "NZ Z NC NCNZ NCZ C CNZ CZ NN NNNZ NNZ N N_NZ N_Z "
                         "T A NE EQ CC HS HI CS LO PL MI F";
const char jag_cond[]  = { 1,2, 4,   5,  6,8,  9,10,20,  21, 22,24, 25, 26,
                           0,0, 1, 2, 4, 4, 5, 8, 8,20,24,31
                         };

// --------------------------------------------------------------


static void JAG_Instr(uint16_t parm, uint16_t reg1, uint16_t reg2)
{
    INSTR_W(((parm & 0x3F) << 10) + ((reg1 & 0x1F) << 5) + (reg2 & 0x1F));
}


static int JAG_DoCPUOpcode(int typ, int parm)
{
    int     val, reg1, reg2;
    Str255  word;
    char    *oldLine;
    int     token;

    if ((parm & TOMONLY) && curCPU == CPU_JERRY) return 0;
    if ((parm & JERONLY) && curCPU == CPU_TOM)   return 0;

    switch (typ)
    {
        case OP_None:
            JAG_Instr(parm, 0, 0);
            break;

        case OP_One:
            switch ((reg2 = REG_Get(jag_regs)))
            {
                case reg_None:
                    ASMX_IllegalOperand();
                    break;

                case reg_EOL:
                    break;

                default:
                    JAG_Instr(parm, parm >> 6, reg2);
                    break;
            }
            break;

        case OP_Two:
            switch ((reg1 = REG_Get(jag_regs)))
            {
                case reg_None:
                    ASMX_IllegalOperand();
                    break;

                case reg_EOL:
                    break;

                default:
                    if (TOKEN_Comma()) break;

                    switch ((reg2 = REG_Get(jag_regs)))
                    {
                        case reg_None:
                            ASMX_IllegalOperand();
                            break;

                        case reg_EOL:
                            break;

                        default:
                            JAG_Instr(parm, reg1, reg2);
                            break;
                    }
            }
            break;

        case OP_Num_15:
        case OP_Num_31:
        case OP_Num_32:
            switch (typ)
            {
                case OP_Num_15:
                    reg1 = -16;
                    reg2 = 15;
                    break;
                    FALLTHROUGH;
                default:
                case OP_Num_31:
                    reg1 =   0;
                    reg2 = 31;
                    break;
                case OP_Num_32:
                    reg1 =   1;
                    reg2 = 32;
                    break;
            }
            val = EXPR_Eval();
            if (val < reg1 || val > reg2)
            {
                ASMX_Error("Constant out of range");
            }
            if (parm & SUB32)
            {
                reg1 = 32 - val;
            }
            else
            {
                reg1 = val;
            }
            if (TOKEN_Comma()) break;
            switch ((reg2 = REG_Get(jag_regs)))
            {
                case reg_None:
                    ASMX_IllegalOperand();
                    break;

                case reg_EOL:
                    break;

                default:
                    JAG_Instr(parm, reg1, reg2);
                    break;
            }
            break;

        case OP_JR:
        case OP_JUMP:
            // get the condition code
            switch ((reg1 = REG_Get(jag_conds)))
            {
                case reg_EOL:
                    val = -1;
                    break;

                case reg_None:
                    val = EXPR_Eval();
                    if (val < 0 || val > 31)
                    {
                        ASMX_Error("Constant out of range");
                        val = 0;
                    }
                    break;

                default:
                    val = jag_cond[reg1];
                    break;
            }
            if (val < 0) break;

            reg1 = val;
            if (TOKEN_Comma()) break;
            if (typ == OP_JR)
            {
                // JR cc,n
                val = EXPR_Eval();
#if 0
                // the old way before WORDWIDTH
                reg2 = (val - locPtr - 2)/2;
#else
                // the new way after WORDWIDTH
                reg2 = val - (locPtr + 2)/2;
#endif
                if (!errFlag && (val < -16 || val > 15))
                {
                    ASMX_Error("Branch out of range");
                }
                JAG_Instr(parm, reg2, reg1);
            }
            else
            {
                // JUMP cc,(Rn)
                if (TOKEN_Expect("(")) break;
                switch ((reg2 = REG_Get(jag_regs)))
                {
                    case reg_None:
                        ASMX_IllegalOperand();
                        break;

                    case reg_EOL:
                        break;

                    default:
                        if (TOKEN_RParen()) break;
                        JAG_Instr(parm, reg2, reg1);
                        break;
                }
            }
            break;

        case OP_MOVEI:
            val = EXPR_Eval();
            if (TOKEN_Comma()) break;
            switch ((reg2 = REG_Get(jag_regs)))
            {
                case reg_None:
                    ASMX_IllegalOperand();
                    break;

                case reg_EOL:
                    break;

                default:
                    INSTR_WL(((parm & 0x3F) << 10) + reg2,
                            (val >> 16) | (val << 16));
                    break;
            }
            break;

        case OP_MOVE: // PC,Rd or Rs,Rd
            switch ((reg1 = REG_Get(jag_regs_PC)))
            {
                case reg_None:
                    ASMX_IllegalOperand();
                    break;

                case reg_EOL:
                    break;

                default:
                    if (TOKEN_Comma()) break;

                    switch ((reg2 = REG_Get(jag_regs)))
                    {
                        case reg_None:
                            ASMX_IllegalOperand();
                            break;

                        case reg_EOL:
                            break;

                        default:
                            parm = 34;
                            if (reg1 == 32)   // PC
                            {
                                parm = 51;
                            }
                            JAG_Instr(parm, reg1, reg2);
                            break;
                    }
            }
            break;

        case OP_LOAD: // (Rn),Rn = 41 / (R14/R15+n),Rn = 43/44 / (R14/R15+Rn),Rn = 58/59
            if (TOKEN_Expect("(")) break;
            switch ((reg1 = REG_Get(jag_regs)))
            {
                case reg_None:
                    ASMX_IllegalOperand();
                    break;

                case reg_EOL:
                    break;

                case 14:
                case 15:
                    oldLine = linePtr;
                    token = TOKEN_GetWord(word);
                    if (token == '+')
                    {
                        parm = reg1 - 14 + 58;
                        switch ((reg1 = REG_Get(jag_regs)))
                        {
                            case reg_EOL:
                                break;

                            case reg_None:
                                // (R14/R15 + n)
                                parm = parm - 58 + 43;
                                val = EXPR_Eval();
                                if (val < 1 || val > 32)
                                {
                                    ASMX_Error("Constant out of range");
                                }
                                reg1 = val;
                                FALLTHROUGH;

                            default:
                                if (TOKEN_RParen()) break;
                                if (TOKEN_Comma()) break;
                                switch ((reg2 = REG_Get(jag_regs)))
                                {
                                    case reg_None:
                                        ASMX_IllegalOperand();
                                        break;

                                    case reg_EOL:
                                        break;

                                    default:
                                        JAG_Instr(parm, reg1, reg2);
                                        break;
                                }
                                break;
                        }
                        break;
                    }
                    linePtr = oldLine;
                    FALLTHROUGH;

                default:
                    parm = 41;
                    if (TOKEN_RParen()) break;
                    if (TOKEN_Comma()) break;

                    switch ((reg2 = REG_Get(jag_regs)))
                    {
                        case reg_None:
                            ASMX_IllegalOperand();
                            break;

                        case reg_EOL:
                            break;

                        default:
                            JAG_Instr(parm, reg1, reg2);
                            break;
                    }
            }
            break;

        case OP_LOADn: // LOADB/LOADP/LOADW (Rn),Rn
            if (TOKEN_Expect("(")) break;
            switch ((reg1 = REG_Get(jag_regs)))
            {
                case reg_None:
                    ASMX_IllegalOperand();
                    break;

                case reg_EOL:
                    break;

                default:
                    if (TOKEN_RParen()) break;
                    if (TOKEN_Comma()) break;

                    switch ((reg2 = REG_Get(jag_regs)))
                    {
                        case reg_None:
                            ASMX_IllegalOperand();
                            break;

                        case reg_EOL:
                            break;

                        default:
                            JAG_Instr(parm, reg1, reg2);
                            break;
                    }
            }
            break;

        case OP_STORE: // Rn,(Rn) = 47 / Rn,(R14/R15+n) = 49/50 / Rn,(R14/R15+Rn) = 60/61
            switch ((reg1 = REG_Get(jag_regs)))
            {
                case reg_None:
                    ASMX_IllegalOperand();
                    break;

                case reg_EOL:
                    break;

                default:
                    if (TOKEN_Comma()) break;
                    if (TOKEN_Expect("(")) break;
                    switch ((reg2 = REG_Get(jag_regs)))
                    {
                        case reg_None:
                            ASMX_IllegalOperand();
                            break;

                        case reg_EOL:
                            break;

                        case 14:
                        case 15:
                            oldLine = linePtr;
                            token = TOKEN_GetWord(word);
                            if (token == '+')
                            {
                                parm = reg2 - 14 + 60;
                                switch ((reg2 = REG_Get(jag_regs)))
                                {
                                    case reg_EOL:
                                        break;

                                    case reg_None:
                                        // (R14/R15 + n)
                                        parm = parm - 60 + 49;
                                        val = EXPR_Eval();
                                        if (val < 1 || val > 32)
                                        {
                                            ASMX_Error("Constant out of range");
                                        }
                                        reg2 = val;
                                        FALLTHROUGH;

                                    default:
                                        if (TOKEN_RParen()) break;
                                        JAG_Instr(parm, reg1, reg2);
                                        break;
                                }
                                break;
                            }
                            linePtr = oldLine;
                            FALLTHROUGH;

                        default:
                            parm = 47;
                            if (TOKEN_RParen()) break;
                            JAG_Instr(parm, reg1, reg2);
                            break;
                    }
            }
            break;

        case OP_STOREn: // STOREB/STOREP/STOREW Rn,(Rn)
            switch ((reg1 = REG_Get(jag_regs)))
            {
                case reg_None:
                    ASMX_IllegalOperand();
                    break;

                case reg_EOL:
                    break;

                default:
                    if (TOKEN_Comma()) break;
                    if (TOKEN_Expect("(")) break;
                    switch ((reg2 = REG_Get(jag_regs)))
                    {
                        case reg_None:
                            ASMX_IllegalOperand();
                            break;

                        case reg_EOL:
                            break;

                        default:
                            if (TOKEN_RParen()) break;
                            JAG_Instr(parm, reg1, reg2);
                            break;
                    }
            }
            break;

        default:
            return 0;
            break;
    }
    return 1;
}


void JAG_AsmInit(void)
{
    void *p = ASMX_AddAsm(versionName, &JAG_DoCPUOpcode, NULL, NULL);

    ASMX_AddCPU(p, "TOM",   CPU_TOM,   END_BIG, ADDR_16, LIST_24, 16, 0, JAG_opcdTab);
    ASMX_AddCPU(p, "JERRY", CPU_JERRY, END_BIG, ADDR_16, LIST_24, 16, 0, JAG_opcdTab);
}
