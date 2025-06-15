// asmM68K.c

#define versionName "M68K assembler"
#include "asmx.h"

// set false to allow ADD/AND/SUB/CMP #n to become ADDI etc.
#define exactArithI true

enum
{
    CPU_68000,
    CPU_68010
};

enum
{
    OP_Inherent,     // implied instructions
    OP_DBcc,         // DBcc instructions
    OP_Branch,       // relative branch instructions
    OP_TRAP,         // TRAP
    OP_BKPT,         // BKPT
    OP_Imm16,        // 16-bit immediate
    OP_LEA,          // LEA opcode
    OP_JMP,          // JMP/JSR/PEA (load-EA allowed except for immediate)
    OP_OneEA,        // single operand store-EA
    OP_DIVMUL,       // DIVS/DIVU/MULS/MULU
    OP_MOVE,         // MOVE opcode, two EAs
    OP_Bit,          // BCHG/BCLR/BSET/BTST
    OP_MOVEQ,        // MOVEQ
    OP_ArithA,       // MOVEA/ADDA/CMPA/SUBA
    OP_Arith,        // ADD/AND/CMP/EOR/OR/SUB
    OP_ArithI,       // ADDI/CMPI/SUBI
    OP_LogImm,       // ANDI/EORI/ORI
    OP_MOVEM,        // MOVEM
    OP_ADDQ,         // ADDQ/SUBQ
    OP_Shift,        // ASx/LSx/ROx/ROXx
    OP_CHK,          // CHK
    OP_CMPM,         // CMPM
    OP_MOVEP,        // MOVEP
    OP_EXG,          // EXG
    OP_ABCD,         // ABCD,SBCD
    OP_ADDX,         // ADDX,SUBX
    OP_LINK,         // LINK
    OP_OneA,         // UNLK
    OP_OneD,         // SWAP/EXT
    OP_MOVEC,        // 68010 MOVEC
    OP_MOVES,        // 68010 MOVES

//  o_Foo = OP_LabelOp,
};

enum
{
    WID_B, WID_W, WID_L, WID_X // byte, word, long, or auto
};

typedef struct EArec
{
    uint16_t        mode;       // 6 bits for the opcode
    uint16_t        len;        // number of extra words
    uint16_t        extra[5];   // storage for extra words
} EArec;

const char addr_regs[] = "A0 A1 A2 A3 A4 A5 A6 A7 SP";
const char data_regs[] = "D0 D1 D2 D3 D4 D5 D6 D7";
const char   DA_regs[] = "D0 D1 D2 D3 D4 D5 D6 D7 A0 A1 A2 A3 A4 A5 A6 A7 SP";
const char A_PC_regs[] = "A0 A1 A2 A3 A4 A5 A6 A7 SP PC";


static const struct OpcdRec M68K_opcdTab[] =
{
    {"ILLEGAL", OP_Inherent, 0x4AFC},
    {"NOP",     OP_Inherent, 0x4E71},
    {"RESET",   OP_Inherent, 0x4E70},
    {"RTE",     OP_Inherent, 0x4E73},
    {"RTR",     OP_Inherent, 0x4E77},
    {"RTS",     OP_Inherent, 0x4E75},
    {"TRAPV",   OP_Inherent, 0x4E76},

    {"DBT",     OP_DBcc,     0x50C8},
    {"DBRA",    OP_DBcc,     0x51C8},
    {"DBF",     OP_DBcc,     0x51C8},
    {"DBHI",    OP_DBcc,     0x52C8},
    {"DBLS",    OP_DBcc,     0x53C8},
    {"DBCC",    OP_DBcc,     0x54C8},
    {"DBHS",    OP_DBcc,     0x54C8},
    {"DBCS",    OP_DBcc,     0x55C8},
    {"DBLO",    OP_DBcc,     0x55C8},
    {"DBNE",    OP_DBcc,     0x56C8},
    {"DBEQ",    OP_DBcc,     0x57C8},
    {"DBVC",    OP_DBcc,     0x58C8},
    {"DBVS",    OP_DBcc,     0x59C8},
    {"DBPL",    OP_DBcc,     0x5AC8},
    {"DBMI",    OP_DBcc,     0x5BC8},
    {"DBGE",    OP_DBcc,     0x5CC8},
    {"DBLT",    OP_DBcc,     0x5DC8},
    {"DBGT",    OP_DBcc,     0x5EC8},
    {"DBLE",    OP_DBcc,     0x5FC8},

    {"BRA",     OP_Branch,   0x6000 + WID_X},
    {"BRA.B",   OP_Branch,   0x6000 + WID_B},
    {"BRA.W",   OP_Branch,   0x6000 + WID_W},
    {"BSR",     OP_Branch,   0x6100 + WID_X},
    {"BSR.B",   OP_Branch,   0x6100 + WID_B},
    {"BSR.W",   OP_Branch,   0x6100 + WID_W},
    {"BHI",     OP_Branch,   0x6200 + WID_X},
    {"BHI.B",   OP_Branch,   0x6200 + WID_B},
    {"BHI.W",   OP_Branch,   0x6200 + WID_W},
    {"BLS",     OP_Branch,   0x6300 + WID_X},
    {"BLS.B",   OP_Branch,   0x6300 + WID_B},
    {"BLS.W",   OP_Branch,   0x6300 + WID_W},
    {"BCC",     OP_Branch,   0x6400 + WID_X},
    {"BCC.B",   OP_Branch,   0x6400 + WID_B},
    {"BCC.W",   OP_Branch,   0x6400 + WID_W},
    {"BHS",     OP_Branch,   0x6400 + WID_X},
    {"BHS.B",   OP_Branch,   0x6400 + WID_B},
    {"BHS.W",   OP_Branch,   0x6400 + WID_W},
    {"BCS",     OP_Branch,   0x6500 + WID_X},
    {"BCS.B",   OP_Branch,   0x6500 + WID_B},
    {"BCS.W",   OP_Branch,   0x6500 + WID_W},
    {"BLO",     OP_Branch,   0x6500 + WID_X},
    {"BLO.B",   OP_Branch,   0x6500 + WID_B},
    {"BLO.W",   OP_Branch,   0x6500 + WID_W},
    {"BNE",     OP_Branch,   0x6600 + WID_X},
    {"BNE.B",   OP_Branch,   0x6600 + WID_B},
    {"BNE.W",   OP_Branch,   0x6600 + WID_W},
    {"BEQ",     OP_Branch,   0x6700 + WID_X},
    {"BEQ.B",   OP_Branch,   0x6700 + WID_B},
    {"BEQ.W",   OP_Branch,   0x6700 + WID_W},
    {"BVC",     OP_Branch,   0x6800 + WID_X},
    {"BVC.B",   OP_Branch,   0x6800 + WID_B},
    {"BVC.W",   OP_Branch,   0x6800 + WID_W},
    {"BVS",     OP_Branch,   0x6900 + WID_X},
    {"BVS.B",   OP_Branch,   0x6900 + WID_B},
    {"BVS.W",   OP_Branch,   0x6900 + WID_W},
    {"BPL",     OP_Branch,   0x6A00 + WID_X},
    {"BPL.B",   OP_Branch,   0x6A00 + WID_B},
    {"BPL.W",   OP_Branch,   0x6A00 + WID_W},
    {"BMI",     OP_Branch,   0x6B00 + WID_X},
    {"BMI.B",   OP_Branch,   0x6B00 + WID_B},
    {"BMI.W",   OP_Branch,   0x6B00 + WID_W},
    {"BGE",     OP_Branch,   0x6C00 + WID_X},
    {"BGE.B",   OP_Branch,   0x6C00 + WID_B},
    {"BGE.W",   OP_Branch,   0x6C00 + WID_W},
    {"BLT",     OP_Branch,   0x6D00 + WID_X},
    {"BLT.B",   OP_Branch,   0x6D00 + WID_B},
    {"BLT.W",   OP_Branch,   0x6D00 + WID_W},
    {"BGT",     OP_Branch,   0x6E00 + WID_X},
    {"BGT.B",   OP_Branch,   0x6E00 + WID_B},
    {"BGT.W",   OP_Branch,   0x6E00 + WID_W},
    {"BLE",     OP_Branch,   0x6F00 + WID_X},
    {"BLE.B",   OP_Branch,   0x6F00 + WID_B},
    {"BLE.W",   OP_Branch,   0x6F00 + WID_W},
    {"BRA.S",   OP_Branch,   0x6000 + WID_B},
    {"BSR.S",   OP_Branch,   0x6100 + WID_B},
    {"BHI.S",   OP_Branch,   0x6200 + WID_B},
    {"BLS.S",   OP_Branch,   0x6300 + WID_B},
    {"BCC.S",   OP_Branch,   0x6400 + WID_B},
    {"BHS.S",   OP_Branch,   0x6400 + WID_B},
    {"BCS.S",   OP_Branch,   0x6500 + WID_B},
    {"BLO.S",   OP_Branch,   0x6500 + WID_B},
    {"BNE.S",   OP_Branch,   0x6600 + WID_B},
    {"BEQ.S",   OP_Branch,   0x6700 + WID_B},
    {"BVC.S",   OP_Branch,   0x6800 + WID_B},
    {"BVS.S",   OP_Branch,   0x6900 + WID_B},
    {"BPL.S",   OP_Branch,   0x6A00 + WID_B},
    {"BMI.S",   OP_Branch,   0x6B00 + WID_B},
    {"BGE.S",   OP_Branch,   0x6C00 + WID_B},
    {"BLT.S",   OP_Branch,   0x6D00 + WID_B},
    {"BGT.S",   OP_Branch,   0x6E00 + WID_B},
    {"BLE.S",   OP_Branch,   0x6F00 + WID_B},

    {"STOP",    OP_Imm16,    0x4E72},
    {"RTD",     OP_Imm16,    0x4E74}, // *** 68010 and up

    {"TRAP",    OP_TRAP,     0x4E40},
    {"BKPT",    OP_BKPT,     0x4848}, // *** 68010 and up

    {"LEA",     OP_LEA,      0x41C0}, // load-EA but no immediate
    {"LEA.L",   OP_LEA,      0x41C0}, // load-EA but no immediate

    {"JMP",     OP_JMP,      0x4EC0}, // load-EA but no immediate
    {"JSR",     OP_JMP,      0x4E80}, // load-EA but no immediate
    {"PEA",     OP_JMP,      0x4840}, // load-EA but no immediate
    {"PEA.L",   OP_JMP,      0x4840}, // load-EA but no immediate

    {"CLR",     OP_OneEA,    0x4240}, // store-EA
    {"CLR.B",   OP_OneEA,    0x4200},
    {"CLR.W",   OP_OneEA,    0x4240},
    {"CLR.L",   OP_OneEA,    0x4280},
    {"NBCD",    OP_OneEA,    0x4800},
    {"NBCD.B",  OP_OneEA,    0x4800},
    {"NEG",     OP_OneEA,    0x4440},
    {"NEG.B",   OP_OneEA,    0x4400},
    {"NEG.W",   OP_OneEA,    0x4440},
    {"NEG.L",   OP_OneEA,    0x4480},
    {"NEGX",    OP_OneEA,    0x4040},
    {"NEGX.B",  OP_OneEA,    0x4000},
    {"NEGX.W",  OP_OneEA,    0x4040},
    {"NEGX.L",  OP_OneEA,    0x4080},
    {"NOT",     OP_OneEA,    0x4640},
    {"NOT.B",   OP_OneEA,    0x4600},
    {"NOT.W",   OP_OneEA,    0x4640},
    {"NOT.L",   OP_OneEA,    0x4680},
    {"ST",      OP_OneEA,    0x50C0},
//  {"ST.B",    OP_OneEA,    0x50C0},
    {"SF",      OP_OneEA,    0x51C0},
//  {"SF.B",    OP_OneEA,    0x51C0},
    {"SHI",     OP_OneEA,    0x52C0},
//  {"SHI.B",   OP_OneEA,    0x52C0},
    {"SLS",     OP_OneEA,    0x53C0},
//  {"SLS.B",   OP_OneEA,    0x53C0},
    {"SCC",     OP_OneEA,    0x54C0},
//  {"SCC.B",   OP_OneEA,    0x54C0},
    {"SHS",     OP_OneEA,    0x54C0},
//  {"SHS.B",   OP_OneEA,    0x54C0},
    {"SCS",     OP_OneEA,    0x55C0},
//  {"SCS.B",   OP_OneEA,    0x55C0},
    {"SLO",     OP_OneEA,    0x55C0},
//  {"SLO.B",   OP_OneEA,    0x55C0},
    {"SNE",     OP_OneEA,    0x56C0},
//  {"SNE.B",   OP_OneEA,    0x56C0},
    {"SEQ",     OP_OneEA,    0x57C0},
//  {"SEQ.B",   OP_OneEA,    0x57C0},
    {"SVC",     OP_OneEA,    0x58C0},
//  {"SVC.B",   OP_OneEA,    0x58C0},
    {"SVS",     OP_OneEA,    0x59C0},
//  {"SVS.B",   OP_OneEA,    0x59C0},
    {"SPL",     OP_OneEA,    0x5AC0},
//  {"SPL.B",   OP_OneEA,    0x5AC0},
    {"SMI",     OP_OneEA,    0x5BC0},
//  {"SMI.B",   OP_OneEA,    0x5BC0},
    {"SGE",     OP_OneEA,    0x5CC0},
//  {"SGE.B",   OP_OneEA,    0x5CC0},
    {"SLT",     OP_OneEA,    0x5DC0},
//  {"SLT.B",   OP_OneEA,    0x5DC0},
    {"SGT",     OP_OneEA,    0x5EC0},
//  {"SGT.B",   OP_OneEA,    0x5EC0},
    {"SLE",     OP_OneEA,    0x5FC0},
//  {"SLE.B",   OP_OneEA,    0x5FC0},
    {"TAS",     OP_OneEA,    0x4AC0},
    {"TAS.B",   OP_OneEA,    0x4AC0},
    {"TST",     OP_OneEA,    0x4A40},
    {"TST.B",   OP_OneEA,    0x4A00},
    {"TST.W",   OP_OneEA,    0x4A40},
    {"TST.L",   OP_OneEA,    0x4A80},

    {"DIVS",    OP_DIVMUL,   0x81C0},
    {"DIVS.W",  OP_DIVMUL,   0x81C0},
    {"DIVU",    OP_DIVMUL,   0x80C0},
    {"DIVU.W",  OP_DIVMUL,   0x80C0},
    {"MULS",    OP_DIVMUL,   0xC1C0},
    {"MULS.W",  OP_DIVMUL,   0xC1C0},
    {"MULU",    OP_DIVMUL,   0xC0C0},
    {"MULU.W",  OP_DIVMUL,   0xC0C0},

    {"MOVE",    OP_MOVE,     0x3000 + WID_X},
    {"MOVE.B",  OP_MOVE,     0x1000 + WID_B},
    {"MOVE.W",  OP_MOVE,     0x3000 + WID_W},
    {"MOVE.L",  OP_MOVE,     0x2000 + WID_L},

    {"BCHG",    OP_Bit,      0x0040 + WID_X},
    {"BCHG.B",  OP_Bit,      0x0040 + WID_B},
    {"BCHG.L",  OP_Bit,      0x0040 + WID_L},
    {"BCLR",    OP_Bit,      0x0080 + WID_X},
    {"BCLR.B",  OP_Bit,      0x0080 + WID_B},
    {"BCLR.L",  OP_Bit,      0x0080 + WID_L},
    {"BSET",    OP_Bit,      0x00C0 + WID_X},
    {"BSET.B",  OP_Bit,      0x00C0 + WID_B},
    {"BSET.L",  OP_Bit,      0x00C0 + WID_L},
    {"BTST",    OP_Bit,      0x0000 + WID_X},
    {"BTST.B",  OP_Bit,      0x0000 + WID_B},
    {"BTST.L",  OP_Bit,      0x0000 + WID_L},

    {"MOVEQ",   OP_MOVEQ,    0},
    {"MOVEQ.L", OP_MOVEQ,    0},

    {"MOVEA",   OP_ArithA,   0x3040 + WID_W},
    {"MOVEA.W", OP_ArithA,   0x3040 + WID_W},
    {"MOVEA.L", OP_ArithA,   0x2040 + WID_L},
    {"ADDA",    OP_ArithA,   0xD0C0 + WID_W},
    {"ADDA.W",  OP_ArithA,   0xD0C0 + WID_W},
    {"ADDA.L",  OP_ArithA,   0xD1C0 + WID_L},
    {"CMPA",    OP_ArithA,   0xB0C0 + WID_W},
    {"CMPA.W",  OP_ArithA,   0xB0C0 + WID_W},
    {"CMPA.L",  OP_ArithA,   0xB1C0 + WID_L},
    {"SUBA",    OP_ArithA,   0x90C0 + WID_W},
    {"SUBA.W",  OP_ArithA,   0x90C0 + WID_W},
    {"SUBA.L",  OP_ArithA,   0x91C0 + WID_L},

    // a_Arith special parm bits:
    // 8000 = EOR
    // 0004 = CMP
    // 0008/0010/0020 = bits for immediate
    {"ADD",     OP_Arith,    0xD040 + WID_X + 0x18},
    {"ADD.B",   OP_Arith,    0xD000 + WID_B + 0x18},
    {"ADD.W",   OP_Arith,    0xD040 + WID_W + 0x18},
    {"ADD.L",   OP_Arith,    0xD080 + WID_L + 0x18},
    {"AND",     OP_Arith,    0xC040 + WID_X + 0x08},
    {"AND.B",   OP_Arith,    0xC000 + WID_B + 0x08},
    {"AND.W",   OP_Arith,    0xC040 + WID_W + 0x08},
    {"AND.L",   OP_Arith,    0xC080 + WID_L + 0x08},
    {"CMP",     OP_Arith,    0xB040 + WID_X + 0x30 + 4}, // EA,Dn only
    {"CMP.B",   OP_Arith,    0xB000 + WID_B + 0x30 + 4},
    {"CMP.W",   OP_Arith,    0xB040 + WID_W + 0x30 + 4},
    {"CMP.L",   OP_Arith,    0xB080 + WID_L + 0x30 + 4},
    {"EOR",     OP_Arith,    0xB040 + WID_X + 0x28 - 0x8000}, // Dn,EA only
    {"EOR.B",   OP_Arith,    0xB000 + WID_B + 0x28 - 0x8000},
    {"EOR.W",   OP_Arith,    0xB040 + WID_W + 0x28 - 0x8000},
    {"EOR.L",   OP_Arith,    0xB080 + WID_L + 0x28 - 0x8000},
    {"OR",      OP_Arith,    0x8040 + WID_X + 0x00},
    {"OR.B",    OP_Arith,    0x8000 + WID_B + 0x00},
    {"OR.W",    OP_Arith,    0x8040 + WID_W + 0x00},
    {"OR.L",    OP_Arith,    0x8080 + WID_L + 0x00},
    {"SUB",     OP_Arith,    0x9040 + WID_X + 0x10},
    {"SUB.B",   OP_Arith,    0x9000 + WID_B + 0x10},
    {"SUB.W",   OP_Arith,    0x9040 + WID_W + 0x10},
    {"SUB.L",   OP_Arith,    0x9080 + WID_L + 0x10},
    /*
    1100 rrrm mm00 0000 -> 0000 0010 SS00 0000 AND -> ANDI
    1011 rrr1 mm00 0000 -> 0000 1010 SS00 0000 EOR -> EORI
    1000 rrrm mm00 0000 -> 0000 0000 SS00 0000 OR  -> ORI

    1101 rrrm mm00 0000 -> 0000 0110 SS00 0000 ADD -> ADDI
    1011 rrr0 mm00 0000 -> 0000 1100 SS00 0000 CMP -> CMPI
    1001 rrrm mm00 0000 -> 0000 0100 SS00 0000 SUB -> SUBI
     ^^^                        ^^^
    */
    {"ADDI",    OP_ArithI,   0x0640 + WID_W},
    {"ADDI.B",  OP_ArithI,   0x0600 + WID_B},
    {"ADDI.W",  OP_ArithI,   0x0640 + WID_W},
    {"ADDI.L",  OP_ArithI,   0x0680 + WID_L},
    {"CMPI",    OP_ArithI,   0x0C40 + WID_W},
    {"CMPI.B",  OP_ArithI,   0x0C00 + WID_B},
    {"CMPI.W",  OP_ArithI,   0x0C40 + WID_W},
    {"CMPI.L",  OP_ArithI,   0x0C80 + WID_L},
    {"SUBI",    OP_ArithI,   0x0440 + WID_W},
    {"SUBI.B",  OP_ArithI,   0x0400 + WID_B},
    {"SUBI.W",  OP_ArithI,   0x0440 + WID_W},
    {"SUBI.L",  OP_ArithI,   0x0480 + WID_L},

    {"ANDI",    OP_LogImm,   0x0200 + WID_X},
    {"ANDI.B",  OP_LogImm,   0x0200 + WID_B},
    {"ANDI.W",  OP_LogImm,   0x0200 + WID_W},
    {"ANDI.L",  OP_LogImm,   0x0200 + WID_L},
    {"EORI",    OP_LogImm,   0x0A00 + WID_X},
    {"EORI.B",  OP_LogImm,   0x0A00 + WID_B},
    {"EORI.W",  OP_LogImm,   0x0A00 + WID_W},
    {"EORI.L",  OP_LogImm,   0x0A00 + WID_L},
    {"ORI",     OP_LogImm,   0x0000 + WID_X},
    {"ORI.B",   OP_LogImm,   0x0000 + WID_B},
    {"ORI.W",   OP_LogImm,   0x0000 + WID_W},
    {"ORI.L",   OP_LogImm,   0x0000 + WID_L},

    {"MOVEM",   OP_MOVEM,    0x4880 + WID_W},
    {"MOVEM.W", OP_MOVEM,    0x4880 + WID_W},
    {"MOVEM.L", OP_MOVEM,    0x48C0 + WID_L},

    {"ADDQ",    OP_ADDQ,     0x5040 + WID_W},
    {"ADDQ.B",  OP_ADDQ,     0x5000 + WID_B},
    {"ADDQ.W",  OP_ADDQ,     0x5040 + WID_W},
    {"ADDQ.L",  OP_ADDQ,     0x5080 + WID_L},
    {"SUBQ",    OP_ADDQ,     0x5140 + WID_W},
    {"SUBQ.B",  OP_ADDQ,     0x5100 + WID_B},
    {"SUBQ.W",  OP_ADDQ,     0x5140 + WID_W},
    {"SUBQ.L",  OP_ADDQ,     0x5180 + WID_L},

    {"ASL",     OP_Shift,    0xE100 + WID_X}, // E100 E1C0
    {"ASL.B",   OP_Shift,    0xE100 + WID_B},
    {"ASL.W",   OP_Shift,    0xE100 + WID_W},
    {"ASL.L",   OP_Shift,    0xE100 + WID_L},
    {"ASR",     OP_Shift,    0xE000 + WID_X}, // E000 E0C0
    {"ASR.B",   OP_Shift,    0xE000 + WID_B},
    {"ASR.W",   OP_Shift,    0xE000 + WID_W},
    {"ASR.L",   OP_Shift,    0xE000 + WID_L},
    {"LSL",     OP_Shift,    0xE108 + WID_X}, // E108 E3C0
    {"LSL.B",   OP_Shift,    0xE108 + WID_B},
    {"LSL.W",   OP_Shift,    0xE108 + WID_W},
    {"LSL.L",   OP_Shift,    0xE108 + WID_L},
    {"LSR",     OP_Shift,    0xE008 + WID_X}, // E008 E2C0
    {"LSR.B",   OP_Shift,    0xE008 + WID_B},
    {"LSR.W",   OP_Shift,    0xE008 + WID_W},
    {"LSR.L",   OP_Shift,    0xE008 + WID_L},
    {"ROL",     OP_Shift,    0xE118 + WID_X}, // E118 E7C0
    {"ROL.B",   OP_Shift,    0xE118 + WID_B},
    {"ROL.W",   OP_Shift,    0xE118 + WID_W},
    {"ROL.L",   OP_Shift,    0xE118 + WID_L},
    {"ROR",     OP_Shift,    0xE018 + WID_X}, // E018 E6C0
    {"ROR.B",   OP_Shift,    0xE018 + WID_B},
    {"ROR.W",   OP_Shift,    0xE018 + WID_W},
    {"ROR.L",   OP_Shift,    0xE018 + WID_L},
    {"ROXL",    OP_Shift,    0xE110 + WID_X}, // E110 E5C0
    {"ROXL.B",  OP_Shift,    0xE110 + WID_B},
    {"ROXL.W",  OP_Shift,    0xE110 + WID_W},
    {"ROXL.L",  OP_Shift,    0xE110 + WID_L},
    {"ROXR",    OP_Shift,    0xE010 + WID_X}, // E010 E4C0
    {"ROXR.B",  OP_Shift,    0xE010 + WID_B},
    {"ROXR.W",  OP_Shift,    0xE010 + WID_W},
    {"ROXR.L",  OP_Shift,    0xE010 + WID_L},

    {"CHK",     OP_CHK,      0x4180 + WID_W},
    {"CHK.W",   OP_CHK,      0x4180 + WID_W},

    {"CMPM",    OP_CMPM,     0xB148},
    {"CMPM.B",  OP_CMPM,     0xB108},
    {"CMPM.W",  OP_CMPM,     0xB148},
    {"CMPM.L",  OP_CMPM,     0xB188},

    {"MOVEP",   OP_MOVEP,    0x0108},
    {"MOVEP.W", OP_MOVEP,    0x0108},
    {"MOVEP.L", OP_MOVEP,    0x0148},

    {"EXG",     OP_EXG,      0},
    {"EXG.L",   OP_EXG,      0},

    {"ABCD",    OP_ABCD,     0xC100},
    {"ABCD.B",  OP_ABCD,     0xC100},
    {"SBCD",    OP_ABCD,     0x8100},
    {"SBCD.B",  OP_ABCD,     0x8100},

    {"ADDX",    OP_ADDX,     0xD140},
    {"ADDX.B",  OP_ADDX,     0xD100},
    {"ADDX.W",  OP_ADDX,     0xD140},
    {"ADDX.L",  OP_ADDX,     0xD180},
    {"SUBX",    OP_ADDX,     0x9140},
    {"SUBX.B",  OP_ADDX,     0x9100},
    {"SUBX.W",  OP_ADDX,     0x9140},
    {"SUBX.L",  OP_ADDX,     0x9180},

    {"LINK",    OP_LINK,     0x4E50},
    {"LINK.W",  OP_LINK,     0x4E50},

    {"UNLK",    OP_OneA,     0x4E58},

    {"SWAP",    OP_OneD,     0x4840},
    {"SWAP.W",  OP_OneD,     0x4840},
    {"EXT",     OP_OneD,     0x4880},
    {"EXT.W",   OP_OneD,     0x4880},
    {"EXT.L",   OP_OneD,     0x48C0},

    {"MOVEC",   OP_MOVEC,    0x4E7A},
    {"MOVES",   OP_MOVES,    0x0E00 + WID_W},
    {"MOVES.B", OP_MOVES,    0x0E00 + WID_B},
    {"MOVES.W", OP_MOVES,    0x0E00 + WID_W},
    {"MOVES.L", OP_MOVES,    0x0E00 + WID_L},

    {"",     OP_Illegal,     0}
};


// --------------------------------------------------------------


// add extra words from an EA spec to the instruction
static void M68K_InstrAddE(EArec *ea)
{
    // detect longwords for proper hex spacing in listing
    if (ea->len == 2 && ((ea->mode & 0x38) == 0x28 || ea->mode == 0x39 ||
                         ea->mode == 0x3A || ea->mode == 0x3C))
    {
        INSTR_AddL(ea -> extra[0] * 65536 + ea -> extra[1]);
    }
    else
    {
        for (int i = 0; i < ea -> len; i++)
        {
            INSTR_AddW(ea -> extra[i]);
        }
    }
}


static void M68K_InstrWE(uint16_t op, EArec *ea)
{
    INSTR_Clear();

    INSTR_AddW((op & 0xFFC0) | ea -> mode);
    M68K_InstrAddE(ea);
}


static void M68K_InstrWWE(uint16_t op, uint16_t w1, EArec *ea)
{
    INSTR_Clear();

    INSTR_AddW((op & 0xFFC0) | ea -> mode);
    INSTR_AddW(w1);
    M68K_InstrAddE(ea);
}


static void M68K_InstrWLE(uint16_t op, uint32_t l1, EArec *ea)
{
    INSTR_Clear();

    INSTR_AddW((op & 0xFFC0) | ea -> mode);
#if 1 // zero to space out longwords as two words
    INSTR_AddL(l1);
#else
    INSTR_AddW(l1 >> 16);
    INSTR_AddW(l1);
#endif
    M68K_InstrAddE(ea);
}


static void M68K_InstrWEE(uint16_t op, EArec *srcEA, EArec *dstEA)
{
    // OP dstEA srcEA
    // srcext - up to 2 words on 68000, 5 words on 68020
    // dstext - up to 2 words on 68000, 5 words on 68020

    INSTR_Clear();

    INSTR_AddW((op & 0xF000) | srcEA -> mode | (((dstEA -> mode) & 0x07) << 9) |
              (((dstEA -> mode) & 0x38) << 3));
    M68K_InstrAddE(srcEA);
    M68K_InstrAddE(dstEA);
}


static void M68K_CheckSize(int size, uint32_t val)
{
    switch (size)
    {
        case WID_B:
            EXPR_CheckByte(val);
            break;

        case WID_X:
        case WID_W:
            EXPR_CheckWord(val);
            break;

        case WID_L:
        default:
            break;
    }
}


static bool M68K_GetEA(bool store, int size, EArec *ea)
{
    Str255  word;
    int     val;
    int     width;
    int     reg1, reg2;

    char *oldLine = linePtr;
    char *oldLine0 = linePtr;
    int token = TOKEN_GetWord(word);

    ea -> mode = 0;
    ea -> len = 0;

    if (word[0] == 'D' && '0' <= word[1] && word[1] <= '7' && word[2] == 0)
    {
        // 000nnn = Dn
        ea -> mode = word[1] - '0';
        return true;

    }
    else if (word[0] == 'A' && '0' <= word[1] && word[1] <= '7' && word[2] == 0)
    {
        // 001nnn = An
        ea -> mode = 8 + word[1] - '0';
        return true;

    }
    else if (word[0] == 'S' && word[1] == 'P' && word[2] == 0)
    {
        // 001111 = SP
        ea -> mode = 0x0F;
        return true;

    }
    else if (token == '#')
    {
        // 111100 = #imm - load-EA only
        if (store || size < 0)
        {
            TOKEN_BadMode();
        }
        else
        {
            ea -> mode = 0x3C;
            val = EXPR_Eval();
            M68K_CheckSize(size, val);
            switch (size)
            {
                case WID_B:
                    val = val & 0xFF;
                    FALLTHROUGH;
                case WID_W:
                    ea -> len = 1;
                    ea -> extra[0] = val;
                    break;
                case WID_L:
                    ea -> len = 2;
                    ea -> extra[0] = val >> 16;
                    ea -> extra[1] = val;
                    break;
                default: // shouldn't get here
                    TOKEN_BadMode();
                    break;
            }
        }
        return true;

    }
    else if (token == '-')
    {
        // 100nnn -(An)
        if (TOKEN_GetWord(word) == '(')
        {
            reg1 = REG_Get(addr_regs);
            if (reg1 == 8) reg1 = 7;
            if (reg1 >= 0)
            {
                if (TOKEN_GetWord(word) == ')')
                {
                    ea -> mode = 0x20 + reg1;
                    return true;
                }
            }
        }

    }
    else if (token == '(')
    {
        // 010nnn (An)
        // 011nnn (An)+
        // 101nnn (d16,An)
        // 110nnn (d8,An,Xn)
        // 111010 (d16,PC) - load only
        // 111011 (d8,PC,Xn) - load only
        oldLine = linePtr; // in case this is just an expression in parens for absolute

        reg1 = REG_Get(A_PC_regs);
        if (reg1 == 8) reg1 = 7; // SP -> A7
        if (reg1 >= 0)
        {
            token = TOKEN_GetWord(word);
            switch (token)
            {
                case ')': // (An)
                    oldLine = linePtr;
                    if (TOKEN_GetWord(word) == '+')
                    {
                        // 011 (An)+
                        ea -> mode = 0x18 + reg1;
                        return true;
                    }
                    else
                    {
                        // 010 (An)
                        linePtr = oldLine;
                        ea -> mode = 0x10 + reg1;
                        return true;
                    }
                    break;

                case ',':
                    // 0(PC,Xn) 0(An,Xn)
                    // FIXME: reg1 can't be PC here?
                    val = 0;
                    reg2 = REG_Get(DA_regs);
                    if (reg2 == 16) reg2 = 15; // SP -> A7
                    if (reg2 >= 0)
                    {
                        // get Xn forced size if any
                        width = WID_W;
                        if (linePtr[0] == '.')
                        {
                            switch (toupper(linePtr[1]))
                            {
                                case 'L':
                                    width = WID_L;
                                // fall through...
                                case 'W':
                                    linePtr = linePtr + 2;
                                    break;
                            }
                        }

                        //CheckByte(val); // zero IS a byte
                        if (TOKEN_RParen()) return false;

                        if (reg1 == 9)   // PC
                        {
                            // (PC,Xn)
                            if (!store)
                            {
                                // val = val - locPtr - 2; // don't offset from PC
                                ea -> mode = 0x3B;
                                ea -> len = 1;
                                ea -> extra[0] = (reg2 << 12) + (val & 0xFF);
                                if (width == WID_L)
                                {
                                    ea -> extra[0] |= 0x0800;
                                }
                                return true;
                            }
                        }
                        else
                        {
                            // (An,Xn)
                            ea -> mode = 0x30 + reg1;
                            ea -> len = 1;
                            ea -> extra[0] = (reg2 << 12) + (val & 0xFF);
                            if (width == WID_L)
                            {
                                ea -> extra[0] |= 0x0800;
                            }
                            return true;
                        }
                    }
            }
        }
        else
        {
            // (value
            linePtr = oldLine;

            // look for "(ofs,reg"
            val = EXPR_Eval(); // get offset

            oldLine = linePtr;
            token = TOKEN_GetWord(word);
            if (token == ')')
            {
                // (expr) which may be followed by more expr: (foo)*(bar)
                oldLine = oldLine0; // completely back up to start of EA
            }
            else
            {
                if (token == '.')
                {
                    // abs.w/abs.l with forced size
                    width = WID_X;
                    switch (toupper(*linePtr))
                    {
                        case 'W':
                            width = WID_W;
                            linePtr = linePtr + 1;
                            break;
                        case 'L':
                            width = WID_L;
                            linePtr = linePtr + 1;
                            break;
                    }
                    if (TOKEN_RParen()) return false;

                    goto ABSOLUTE;
//              } else if (token == ')') {
//                  // abs
//                  width = WID_X;
//                  goto ABSOLUTE;
                }

                linePtr = oldLine;
                if (TOKEN_Comma()) return false;

                reg1 = REG_Get(A_PC_regs);
                if (reg1 == 8) reg1 = 7; // SP -> A7

                if (reg1 >= 0)
                {
                    // look for rparen or Xn
                    switch (TOKEN_GetWord(word))
                    {
                        case ')':
                            // (ofs,An) or (ofs,PC)
                            if (reg1 == 9)   // PC
                            {
                                // (d16,PC)
                                if (!store)
                                {
                                    val = val - locPtr - 2;
                                    if (!errFlag && (val < -128 || val > 127))
                                    {
                                        ASMX_Error("Offset out of range");
                                    }
                                    ea -> mode = 0x3A;
                                    ea -> len = 1;
                                    ea -> extra[0] = val;
                                    return true;
                                }
                            }
                            else
                            {
                                if (!exactFlag && evalKnown && val == 0)
                                {
                                    // 010 (An)
                                    ea -> mode = 0x10 + reg1;
                                    return true;
                                }
                                // (d16,An)
                                EXPR_CheckWord(val);
                                ea -> mode = 0x28 + reg1;
                                ea -> len = 1;
                                ea -> extra[0] = val;
                                return true;
                            }
                            break;

                        case ',': // (d8,An,Xn) or (d8,PC,Xn)
                            reg2 = REG_Get(DA_regs);
                            if (reg2 == 16) reg2 = 15; // SP -> A7
                            if (reg2 >= 0)
                            {
                                width = WID_W;
                                if (linePtr[0] == '.')
                                {
                                    switch (toupper(linePtr[1]))
                                    {
                                        case 'L':
                                            width = WID_L;
                                        // fall through...
                                        case 'W':
                                            linePtr = linePtr + 2;
                                            break;
                                    }
                                }

                                if (TOKEN_RParen()) break;

                                if (reg1 == 9)   // PC
                                {
                                    // (d8,PC,Xn)
                                    if (!store)
                                    {
                                        val = val - locPtr - 2;
                                        if (!errFlag && (val < -128 || val > 127))
                                        {
                                            ASMX_Error("Offset out of range");
                                        }
                                        ea -> mode = 0x3B;
                                        ea -> len = 1;
                                        ea -> extra[0] = (reg2 << 12) + (val & 0xFF);
                                        if (width == WID_L)
                                        {
                                            ea -> extra[0] |= 0x0800;
                                        }
                                        return true;
                                    }
                                }
                                else
                                {
                                    // (d8,An,Xn)
                                    EXPR_CheckByte(val);
                                    ea -> mode = 0x30 + reg1;
                                    ea -> len = 1;
                                    ea -> extra[0] = (reg2 << 12) + (val & 0xFF);
                                    if (width == WID_L)
                                    {
                                        ea -> extra[0] |= 0x0800;
                                    }
                                    return true;
                                }
                            }
                            break;
                    }
                }
            }
        }
    }

    linePtr = oldLine;
    token = 0;

    // 111000 abs.W
    // 111001 abs.L
    // 101nnn d16(An)
    // 110nnn d8(An,Xn)
    // 111010 d16(PC) - load only
    // 111011 d8(PC,Xn) - load only
    {
        // get address/offset and forced size if any
        val = EXPR_Eval();
        width = WID_X;
        if (linePtr[0] == '.')
        {
            switch (toupper(linePtr[1]))
            {
                case 'W':
                    width = WID_W;
                    linePtr = linePtr + 2;
                    break;
                case 'L':
                    width = WID_L;
                    linePtr = linePtr + 2;
                    break;
            }
        }

        oldLine = linePtr;
        switch (TOKEN_GetWord(word))
        {
            case '(':
                // offset indexed
                reg1 = REG_Get(A_PC_regs);
                if (reg1 == 8) reg1 = 7; // SP -> A7
                if (reg1 >= 0)
                {
                    // now look for ) or ,
                    switch (TOKEN_GetWord(word))
                    {
                        case ')':
                            if (reg1 == 9)   // PC
                            {
                                // d16(PC)
                                if (!store)
                                {
                                    val = val - locPtr - 2;
                                    if (!errFlag && (val < -32768 || val > 32767))
                                    {
                                        ASMX_Error("Offset out of range");
                                    }
                                    ea -> mode = 0x3A;
                                    ea -> len = 1;
                                    ea -> extra[0] = val;
                                    return true;
                                }
                            }
                            else
                            {
                                if (!exactFlag && evalKnown && val == 0)
                                {
                                    // 010 (An)
                                    ea -> mode = 0x10 + reg1;
                                    return true;
                                }

                                // d16(An)
                                EXPR_CheckWord(val);
                                ea -> mode = 0x28 + reg1;
                                ea -> len = 1;
                                ea -> extra[0] = val;
                                return true;
                            }
                            break;

                        case ',': // d8(An,Xn) or d8(PC,Xn)
                            reg2 = REG_Get(DA_regs);
                            if (reg2 == 16) reg2 = 15; // SP -> A7
                            if (reg2 >= 0)
                            {
                                // get Xn forced size if any
                                width = WID_W;
                                if (linePtr[0] == '.')
                                {
                                    switch (toupper(linePtr[1]))
                                    {
                                        case 'L':
                                            width = WID_L;
                                        // fall through
                                        case 'W':
                                            linePtr = linePtr + 2;
                                            break;
                                    }
                                }

                                if (TOKEN_RParen()) break;

                                if (reg1 == 9)   // PC
                                {
                                    // d8(PC,Xn)
                                    if (!store)
                                    {
                                        val = val - locPtr - 2;
                                        if (!errFlag && (val < -128 || val > 127))
                                        {
                                            ASMX_Error("Offset out of range");
                                        }
                                        ea -> mode = 0x3B;
                                        ea -> len = 1;
                                        ea -> extra[0] = (reg2 << 12) + (val & 0xFF);
                                        if (width == WID_L)
                                        {
                                            ea -> extra[0] |= 0x0800;
                                        }
                                        return true;
                                    }
                                }
                                else
                                {
                                    // d8(An,Xn)
                                    EXPR_CheckByte(val);
                                    ea -> mode = 0x30 + reg1;
                                    ea -> len = 1;
                                    ea -> extra[0] = (reg2 << 12) + (val & 0xFF);
                                    if (width == WID_L)
                                    {
                                        ea -> extra[0] |= 0x0800;
                                    }
                                    return true;
                                }
                            }
                            break;
                    }
                }
                break;

            default:
                // abs.W / abs.L
                linePtr = oldLine;
ABSOLUTE:
                reg2 = val & 0xFFFFFF; // 68000/68010: truncate to 24 bits for range checks
                if (reg2 & 0x800000) reg2 = reg2 | 0xFF000000; // 68000/68010: sign extend from 24 bits
                if ((evalKnown && width == WID_X && -0x8000 <= reg2 && reg2 <= 0x7FFF)
                        || width == WID_W)
                {
                    // abs.w
                    if (reg2 < -0x8000 || reg2 > 0x7FFF)
                    {
                        ASMX_Error("Absolute word addressing mode out of range");
                    }
                    ea -> mode = 0x38;
                    ea -> len = 1;
                    ea -> extra[0] = val;
                    return true;
                }
                else
                {
                    // abs.l
                    ea -> mode = 0x39;
                    ea -> len = 2;
                    ea -> extra[0] = val >> 16;
                    ea -> extra[1] = val;
                    return true;
                }
                break;

        }
    }

    TOKEN_BadMode();
    return false;

    // p2-2
    // D/A R-R-R W/L SC-ALE 0 dddddddd
    // D/A R-R-R W/L SC-ALE 1 BS IS BD-SIZE - I/-I-S + base disp + outer disp
}


static void M68K_SetMultiReg(int reg, int *regs, bool *warned)
{
    if (!*warned && *regs & (1 << reg))
    {
        ASMX_Warning("MOVEM register specified twice");
        *warned = true;
    }
    *regs |= 1 << reg;
}


static int M68K_GetMultiRegs(void)
{
    Str255  word;
    int     regs = 0;
    bool    warned = false;

    char *oldLine = linePtr;
    int token = '/';
    while (token == '/')
    {
        int reg1 = REG_Get(DA_regs);
        if (reg1 == 16) reg1 = 15; // SP -> A7
        if (reg1 < 0)
        {
            ASMX_IllegalOperand();      // abort if not valid register
            break;
        }

        // set single register bit
        M68K_SetMultiReg(reg1, &regs, &warned);

        // check for - or /
        oldLine = linePtr;
        token = TOKEN_GetWord(word);

        if (token == '-')       // register range
        {
            oldLine = linePtr;  // commit line position
            int reg2 = REG_Get(DA_regs); // check for second register
            if (reg2 == 16) reg2 = 15; // SP -> A7
            if (reg2 < 0)
            {
                ASMX_IllegalOperand();      // abort if not valid register
                break;
            }
            if (reg1 < reg2)
            {
                for (int i = reg1 + 1; i <= reg2; i++)
                {
                    M68K_SetMultiReg(i, &regs, &warned);
                }
            }
            else if (reg1 > reg2)
            {
                for (int i = reg1 - 1; i >= reg2; i--)
                {
                    M68K_SetMultiReg(i, &regs, &warned);
                }
            }
            oldLine = linePtr;  // commit line position
            token = TOKEN_GetWord(word);
        }
        if (token == '/')   // is there another register?
        {
            oldLine = linePtr;  // commit line position
        }
    }
    linePtr = oldLine;

    return regs;
}


// get a MOVEC register name
// M68K_GetMOVECreg returns:
//      -2 (aka reg_EOL) and reports a "missing operand" error if end of line
//      -1 (aka reg_None) if no register found
//      else the actual 12-bit register code
// 000 = SFC   68010+		800 = USP   68010+
// 001 = DFC   68010+		801 = VBR   68010+
// 002 = CACR  020/030/040	802 = CAAR  020/030 but NOT 040
// 003 = TC    040/LC040	803 = MSP   020/030/040
// 004 = ITT0  040/LC040	804 = ISP   020/030/040
// 005 = ITT1  040/LC040	805 = MMUSR 040/LC040
// 006 = DTT0  040/LC040	806 = URP   040/LC040
// 007 = DTT1  040/LC040	807 = SRP   040/LC040
// 004 = IACR0 EC040
// 005 = IACR1 EC040
// 006 = DACR0 EC040
// 007 = DACR1 EC040

static int M68K_GetMOVECreg(void)
{
    int reg = REG_Get("SFC DFC USP VBR"); // 68010 MOVEC registers

    switch (reg)
    {
        case 0: // SFC = 000
        case 1: // DFC = 001
            break;

        case 2:
            reg = 0x800; // USP
            break;

        case 3:
            reg = 0x801; // VBR
            break;

        default: // unknown or GetReg error
            break;
    }

    return reg;
}


static int M68K_DoCPUOpcode(int typ, int parm)
{
    int     val, reg1, reg2;
    EArec   ea1, ea2;
    int     size;
    Str255  word;
    char    *oldLine;
//  int     token;
    bool    skipArithI = false;

    switch (typ)
    {
        case OP_Inherent:
            INSTR_W(parm);
            break;

        case OP_DBcc:
            reg1 = REG_Get(data_regs);
            if (reg1 >= 0)
            {
                if (TOKEN_Comma()) break;
                val = EXPR_EvalWBranch(2);
                INSTR_WW(parm + reg1, val);
            }
            else
            {
                ASMX_IllegalOperand();
            }
            break;

        case OP_Branch:
            switch (parm & 3)
            {
                case WID_B:
                    val = EXPR_EvalBranch(2);
                    if (val == 0)
                    {
                        ASMX_Error("Short branch can not have zero offset!");
                        INSTR_W(0x4E71); // assemble a NOP instead of a zero branch
                    }
                    else
                    {
                        INSTR_W((parm & 0xFF00) + (val & 0x00FF));
                    }
                    break;

                case WID_X:
#if 1 // to enable optimized branches
#if 0 // long branch for 68020+
                    // if not a forward reference, can choose L, W, or B branch
                    val = EXPR_EvalLBranch(2);
                    if (evalKnown && -128 <= val && val <= 127 && val != 0 && val != 0xFF)
                    {
                        INSTR_W((parm & 0xFF00) + (val & 0x00FF));
                    }
                    else if (evalKnown && -32768 <= val && val <= 32767)
                    {
                        INSTR_WW(parm & 0xFF00, val);
                    }
                    else
                    {
                        INSTR_WL(parm & 0xFF00 + 0xFF, val);
                    }
#else
                    // if not a forward reference, can choose W or B branch
                    val = EXPR_EvalWBranch(2);
                    if (evalKnown && -128 <= val && val <= 127 && val != 0 && val != 0xFF)
                    {
                        INSTR_W((parm & 0xFF00) + (val & 0x00FF));
                    }
                    else
                    {
                        if (val != 0 && -128 <= val && val <= 129 && !exactFlag)
                        {
                            // max is +129 because short branch saves 2 bytes
                            ASMX_Warning("Short branch could be used here");
                        }
                        INSTR_WW(parm & 0xFF00, val);
                    }
#endif
                    break;
#endif
                case WID_W:
                    val = EXPR_EvalWBranch(2);
                    if (val != 0 && -128 <= val && val <= 129 && !exactFlag)
                    {
                        // max is +129 because short branch saves 2 bytes
                        ASMX_Warning("Short branch could be used here");
                    }
                    INSTR_WW(parm & 0xFF00, val);
                    break;

                case WID_L:
#if 0 // long branch for 68020+
                    val = EXPR_EvalLBranch(2);
                    if (val != 0 && -128 <= val && val <= 131 && !exactFlag)
                    {
                        // max is +131 because short branch saves 4 bytes
                        ASMX_Warning("Short branch could be used here");
                    }
                    else    if (-32768 <= val && val <= 32769 && !exactFlag)
                    {
                        // max is +32769 because word branch saves 2 bytes
                        ASMX_Warning("Word branch could be used here");
                    }
                    INSTR_WL(parm & 0xFF00 + 0xFF, val);
                    break;
#endif
                default:
                    TOKEN_BadMode();
                    break;
            }
            break;

        case OP_Imm16:
            if (curCPU == CPU_68000 && parm == 0x4E74) return 0; // RTD

            if (TOKEN_Expect("#")) break;
            val = EXPR_Eval();
            INSTR_WW(parm, val);
            break;

        case OP_TRAP:
        case OP_BKPT:
            if (TOKEN_Expect("#")) break;
            val = EXPR_Eval();
            if (val < 0 || val > 15 || (typ == OP_BKPT && val > 7))
            {
                ASMX_IllegalOperand();
                INSTR_W(parm); // to avoid potential phase errors
            }
            else
            {
                INSTR_W(parm + val);
            }
            break;

        case OP_LEA:
            if (M68K_GetEA(false, -1, &ea1))
            {
                reg1 = ea1.mode & 0x38; // don't allow Dn An (An)+ or -(An)
                if (reg1 == 0x00 || reg1 == 0x08 || reg1 == 0x18 || reg1 == 0x20)
                {
                    TOKEN_BadMode();
                }
                else
                {
                    if (TOKEN_Comma()) break;
                    reg1 = REG_Get(addr_regs);
                    if (reg1 == 8) reg1 = 7;
                    if (reg1 >= 0)
                    {
                        M68K_InstrWE(parm + (reg1 << 9), &ea1);
                    }
                    else
                    {
                        ASMX_IllegalOperand();
                    }
                }
            }
            break;

        case OP_JMP:
            if (M68K_GetEA(false, -1, &ea1))
            {
                reg1 = ea1.mode & 0x38; // don't allow Dn An (An)+ or -(An)
                if (reg1 == 0x00 || reg1 == 0x08 || reg1 == 0x18 || reg1 == 0x20)
                {
                    TOKEN_BadMode();
                }
                else
                {
                    M68K_InstrWE(parm, &ea1);
                }
            }
            break;

        case OP_OneEA:
            if (M68K_GetEA(true, -1, &ea1))
            {
                M68K_InstrWE(parm, &ea1);
            }
            break;

        case OP_DIVMUL:
            if (M68K_GetEA(false, WID_W, &ea1))
            {
                if (TOKEN_Comma()) break;
                reg1 = REG_Get(data_regs);
                if (0 <= reg1 && reg1 <= 7)
                {
                    M68K_InstrWE(parm + (reg1 << 9), &ea1);
                }
                else
                {
                    ASMX_IllegalOperand();
                }
            }
            break;

        case OP_MOVE:
            size = parm & 3;
            reg1 = REG_Get("CCR SR USP");
            if (reg1 == reg_EOL) break;
            if (reg1 >= 0)
            {
                if (TOKEN_Comma()) break;
                if (M68K_GetEA(true, size, &ea1))
                {
                    switch (reg1)
                    {
                        case 0: // from CCR
                            if (curCPU == CPU_68010)
                            {
                                if (size == WID_B || size == WID_L)
                                {
                                    TOKEN_BadMode();
                                }
                                else
                                {
                                    M68K_InstrWE(0x42C0, &ea1);
                                }
                            }
                            else
                            {
                                TOKEN_BadMode();
                            }
                            break;

                        case 1: // from SR
                            if (size == WID_B || size == WID_L)
                            {
                                TOKEN_BadMode();
                            }
                            else
                            {
                                M68K_InstrWE(0x40C0, &ea1);
                            }
                            break;

                        case 2: // from USP
                            if ((ea1.mode & 0x38) != 0x08 || // An
                                    size == WID_B || size == WID_W)
                            {
                                TOKEN_BadMode();
                            }
                            else
                            {
                                INSTR_W(0x4E68 + (ea1.mode & 0x0007));
                            }
                            break;
                    }
                }
            }
            else
            {
                reg1 = size;
                if (reg1 == WID_X) reg1 = WID_W;
                if (M68K_GetEA(false, reg1, &ea1))
                {
                    if (TOKEN_Comma()) break;
                    switch (REG_Get("CCR SR USP"))
                    {
                        case 0: // CCR
                            if (size == WID_B || size == WID_L)
                            {
                                TOKEN_BadMode();
                            }
                            else
                            {
                                M68K_InstrWE(0x44C0, &ea1);
                            }
                            break;

                        case 1: // SR
                            if (size == WID_B || size == WID_L)
                            {
                                TOKEN_BadMode();
                            }
                            else
                            {
                                M68K_InstrWE(0x46C0, &ea1);
                            }
                            break;

                        case 2: // USP
                            if ((ea1.mode & 0x38) != 0x08 || // An
                                    size == WID_B || size == WID_W)
                            {
                                TOKEN_BadMode();
                            }
                            else
                            {
                                INSTR_W(0x4E60 + (ea1.mode & 0x0007));
                            }
                            break;

                        case reg_EOL:
                            break;

                        default:
                            if (size == WID_X) size = WID_W;
                            if (M68K_GetEA(true, size, &ea2))
                            {
                                M68K_InstrWEE(parm, &ea1, &ea2);
                            }
                            break;
                    }
                }
            }
            break;

        case OP_Bit:
            size = parm & 3;
            parm = parm & 0xFFFC;

            reg1 = REG_Get("D0 D1 D2 D3 D4 D5 D6 D7 #");
            if (reg1 >= 0)
            {
                val = 0; // to avoid unitialized use
                if (reg1 == 8)   // immediate
                {
                    val = EXPR_Eval();
                }
                if (TOKEN_Comma()) break;
                if (M68K_GetEA(true, -1, &ea1))
                {
                    reg2 = (ea1.mode & 0x38);
                    if ((reg2 == 0x08)                  ||  // An not allowed
                            (reg2 == 0x00 && size == WID_B) ||  // Bxxx.B Dn
                            (reg2 >= 0x10 && size == WID_L))    // Bxxx.L everything else
                    {
                        TOKEN_BadMode();
                        break;
                    }
                    // mask out 0x07 immediate if non-data reg EA
                    if (reg1 == 8 && reg2 != 0) val = val & 0x07;
                    if (reg1 == 8)
                    {
                        M68K_InstrWWE(parm + 0x0800, val & 0x1F, &ea1);
                    }
                    else
                    {
                        M68K_InstrWE (parm + 0x0100 + (reg1 << 9), &ea1);
                    }
                }
            }
            else
            {
                ASMX_IllegalOperand();
            }
            break;

        case OP_MOVEQ:
            if (TOKEN_Expect("#")) break;
            val = EXPR_EvalByte();
            if (TOKEN_Comma()) break;
            reg1 = REG_Get(data_regs);
            if (0 <= reg1 && reg1 <= 7)
            {
                INSTR_W(0x7000 + (reg1 << 9) + val);
            }
            else
            {
                ASMX_IllegalOperand();
            }
            break;

        case OP_ArithA:
            size = parm & 3;
            parm = parm & 0xFFFC;
            if (M68K_GetEA(false, size, &ea1))
            {
                if (TOKEN_Comma()) break;
                reg1 = REG_Get(addr_regs);
                if (reg1 == 8) reg1 = 7;
                if (reg1 >= 0)
                {
                    M68K_InstrWE(parm + (reg1 << 9), &ea1);
                }
                else
                {
                    ASMX_IllegalOperand();
                }
            }
            break;

        case OP_Arith:
            size = parm & 3;
            reg1 = parm & 0x0038; // used to build parm for immediate opcode
            reg2 = parm & 0x8004; // 4 = EA,Dn only flag, high bit low = Dn,EA only flag
            val  = parm >> 12;    // save high nibble to filter specific instructions
            parm = (parm & 0xFFC0) | 0x8000; // rebuild base opcode word
            oldLine = linePtr;
            if (((TOKEN_GetWord(word) != '#' )  // let immediate fall through if allowed
                    ||  val == 0xD // ADD
                    ||  val == 0xC // AND
                    || (val == 0xB && reg2 == 0x8004) // CMP but not EOR
                    ||  val == 0x9 // SUB
                ) && (exactFlag || exactArithI)) // ...and if exact is not set
            {
                // check for Dn
                linePtr = oldLine;
                if (size == WID_X) size = WID_W;
                reg1 = REG_Get(data_regs);
                if (reg1 >= 0)
                {
                    // Dn,EA
                    if (TOKEN_Comma()) break;
                    if (M68K_GetEA(true, size, &ea1))
                    {
                        if ((ea1.mode & 0x38) == 8)   // Dn,An
                        {
                            // An can only be a destination register with ADDA / SUBA / CMPA
                            if (size == WID_B)
                            {
                                // must be word or long
                                ASMX_IllegalOperand();
                                break;
                            }
                            switch (parm & 0xF000)
                            {
                                case 0xB000: // CMP
                                    if ((reg2 & 4) == 0)   // EOR not allowed here
                                    {
                                        TOKEN_BadMode();
                                        break;
                                    }
                                    FALLTHROUGH;

                                case 0xD000: // ADD
                                case 0x9000: // SUB
                                    if (size != WID_B)
                                    {
                                        // bits 6-8 mode .W = 011 / .L = 111
                                        parm = (parm & 0xF000) | (size << 7) | 0xC0;
                                        // generate ANDA / SUBA / CMPA instruction
                                        INSTR_W(parm + (((ea1.mode & 7) << 9) + reg1));
                                        break;
                                    }
                                    FALLTHROUGH;

                                default:
                                    TOKEN_BadMode();
                                    break;
                            }
                            break;
                        }
                        if (reg2 & 4)   // check if Dn,EA is not allowed
                        {
                            // CMP Dn,EA is invalid unless EA is Dn
                            // dest must be a data register
                            if ((ea1.mode & 0x38) != 0)
                            {
                                TOKEN_BadMode();
                                break;
                            }
                        }
                        if ((ea1.mode & 0x38) == 0 && (reg2 & 0x8000))
                        {
                            // Dn,Dn, except for EOR
                            // Dn,Dn needs the dest to be a register for CMP
                            INSTR_W(parm + (ea1.mode << 9) + reg1);
                            break;
                        }
                        M68K_InstrWE(parm + (reg1 << 9) + 0x0100, &ea1);
                    }
                }
                else
                {
                    // EA,
                    if (M68K_GetEA(false, size, &ea1))
                    {
                        if (TOKEN_Comma()) break;
                        reg1 = REG_Get(DA_regs);
                        if (reg1 == 16) reg1 = 15; // SP -> A7
                        if (0 <= reg1 && reg1 <= 7)   // EA,Dn
                        {
                            if (reg2 == 0)
                            {
                                // EOR EA,Dn is invalid
                                TOKEN_BadMode();
                                break;
                            }
                            if (/*(reg2 & 4) &&*/ (ea1.mode & 0x38) == 8 && size == WID_B)
                            {
                                // all op.B An,Dn is invalid (orignally only tested for CMP.B)
                                TOKEN_BadMode();
                                break;
                            }
                            M68K_InstrWE(parm + (reg1 << 9), &ea1);
                        }
                        else if (8 <= reg1 && reg1 <= 15 && size != WID_B)     // EA,An
                        {
                            // EA,An allowed for ADD/SUB/CMP only!
                            reg1 = reg1 - 8; // reg1 = An
                            switch (parm & 0xF000)
                            {
                                case 0xD000: // AND
                                case 0x9000: // SUB
                                case 0xB000: // CMP/EOR
                                    if (reg2 & 0x8000)   // EOR not allowed here
                                    {
                                        // bits 6-8 mode .W = 011 / .L = 111
                                        parm = (parm & 0xF000) | (size << 7) | 0xC0;
                                        // generate ANDA / SUBA / CMPA instruction
                                        M68K_InstrWE(parm + (reg1 << 9), &ea1);
                                        break;
                                    }
                                    FALLTHROUGH;
                                default:
                                    ASMX_IllegalOperand();
                                    break;
                            }
                        }
                        else
                        {
                            ASMX_IllegalOperand();
                        }
                    }
                }
                break;
            }
            // first operand is immediate, rewind back to '#'
            linePtr = oldLine;

            // set up parm for OP_ArithI or OP_LogImm
            if (!(reg1 & 0x0100))
            {
                skipArithI = true; // do OP_LogImm
                parm = (reg1 << 6) + size;
            }
            else
            {
                if (size == WID_X) size = WID_W;
                parm = (reg1 << 6) + (size << 6) + size;
            }
            // fall through...
            FALLTHROUGH;

        case OP_ArithI:
            if (!skipArithI)
            {
                size = parm & 3;
                parm = parm & 0xFFFC;
                if (TOKEN_Expect("#")) break;
                val = EXPR_Eval();
                M68K_CheckSize(size, val);
                if (TOKEN_Comma()) break;
                if (M68K_GetEA(true, -1, &ea1))
                {
                    if ((ea1.mode & 0x38) == 0x08)
                    {
                        // arith immediate does not support An as dest
                        if ((parm & 0x0400) && size != WID_B)
                        {
                            // SUBI/ADDI/CMPI = 04xx/06xx/0Cxx
                            switch (parm & 0x0F00)
                            {
                                default:
                                case 0x0400:
                                    parm = 0x9000;
                                    break; // SUBI
                                case 0x0600:
                                    parm = 0xD000;
                                    break; // ADDI
                                case 0x0C00:
                                    parm = 0xB000;
                                    break; // CMPI
                            }
                            if (size == WID_L)
                            {
                                INSTR_WL(parm + ((ea1.mode & 7) << 9) + (size << 7) + 0x007C, val);
                            }
                            else
                            {
                                INSTR_WW(parm + ((ea1.mode & 7) << 9) + (size << 7) + 0x007C, val);
                            }
                            break;
                        }
                        TOKEN_BadMode();
                    }
                    else if (size == WID_L)
                    {
                        M68K_InstrWLE(parm, val, &ea1);
                    }
                    else
                    {
                        M68K_InstrWWE(parm, val, &ea1);
                    }
                }
                break;
            }
            // fall through for OP_Arith...
            FALLTHROUGH;

        case OP_LogImm:
            size = parm & 3;
            parm = parm & 0xFFFC;
            if (TOKEN_GetWord(word) != '#')
            {
                TOKEN_BadMode();
            }
            else
            {
                val = EXPR_Eval();
                if (TOKEN_Comma()) break;
                switch ((reg1 = REG_Get("CCR SR")))
                {
                    case 0: // CCR
                        EXPR_CheckByte(val);
                        if (size != WID_X && size != WID_B)
                        {
                            TOKEN_BadMode();
                        }
                        else
                        {
                            INSTR_WW((parm & 0xFF00) + 0x003C, val & 0xFF);
                        }
                        break;

                    case 1: // SR
                        M68K_CheckSize(size, val);
                        if (size != WID_X && size != WID_W)
                        {
                            TOKEN_BadMode();
                        }
                        else
                        {
                            INSTR_WW((parm & 0xFF00) + 0x007C, val);
                        }
                        break;

                    default:
                        if (size == WID_X) size = WID_W;
                        M68K_CheckSize(size, val);
                        if (M68K_GetEA(true, -1, &ea1))
                        {
                            if ((ea1.mode & 0x38) == 0x08)
                            {
                                // logical immediate does not support An as dest
                                TOKEN_BadMode();
                            }
                            else
                            {
                                if (size == WID_L)
                                {
                                    M68K_InstrWLE(parm + (size << 6), val, &ea1);
                                }
                                else
                                {
                                    M68K_InstrWWE(parm + (size << 6), val, &ea1);
                                }
                            }
                        }
                }
            }
            break;

        case OP_MOVEM:
            size = (parm & 3) >> 1;
            parm = parm & 0xFFFC;

            oldLine = linePtr;
            reg1 = REG_Get(DA_regs);
            if (reg1 == 16) reg1 = 15; // SP -> A7
            linePtr = oldLine;

            if (reg1 >= 0)   // register-to-memory
            {
                reg2 = M68K_GetMultiRegs();
                if (TOKEN_Comma()) break;
                if (M68K_GetEA(true, -1, &ea1))
                {
                    val = ea1.mode & 0x38;
                    if (val == 0x00 || val == 0x08 || val == 0x18)
                    {
                        TOKEN_BadMode();
                    }
                    else if (val == 0x20)
                    {
                        // reverse bits of reg2
                        reg1 = 0;
                        for (int i = 0; i <= 15; i++)
                        {
                            reg1 = (reg1 << 1) + (reg2 & 1);
                            reg2 = reg2 >> 1;
                        }
                        reg2 = reg1;
                    }
                    M68K_InstrWWE(parm, reg2, &ea1);
                }
            }
            else     // memory-to-register
            {
                if (M68K_GetEA(true, -1, &ea1))
                {
                    if (TOKEN_Comma()) break;
                    reg2 = M68K_GetMultiRegs();
                    val = ea1.mode & 0x38;
                    if (val == 0x00 || val == 0x08 || val == 0x20)
                    {
                        TOKEN_BadMode();
                    }
                    else
                    {
                        M68K_InstrWWE(parm + 0x0400, reg2, &ea1);
                    }
                }
            }
            break;

        case OP_ADDQ:
            size = parm & 3;
            parm = parm & 0xFFFC;

            if (TOKEN_GetWord(word) == '#')
            {
                val = EXPR_Eval();
                if (TOKEN_Comma()) break;
                if (M68K_GetEA(true, -1, &ea1))
                {
                    if ((ea1.mode & 0x38) == 0x08 && size == WID_B)
                    {
                        // byte add to An is invalid
                        TOKEN_BadMode();
                        break;
                    }
                    if (val < 1 || val > 8) ASMX_IllegalOperand();
                    val = val & 7;
                    M68K_InstrWE(parm + (val << 9), &ea1);
                }
            }
            else
            {
                TOKEN_BadMode();
            }
            break;

        case OP_Shift: // bits 3,4 -> bits 9,10 for EA
            size = parm & 3;
            parm = parm & 0xFFFC;

            oldLine = linePtr;
            if (TOKEN_GetWord(word) == '#')
            {
                // #data,Dy
                val = EXPR_Eval();
                if (TOKEN_Comma()) break;
                reg1 = REG_Get(data_regs);
                if (reg1 >= 0)
                {
                    if (size == WID_X) size = WID_W;
                    if (val < 1 || val > 8) ASMX_IllegalOperand();
                    val = val & 7;
                    INSTR_W(parm + (val << 9) + (size << 6) + reg1);
                }
                else
                {
                    ASMX_IllegalOperand();
                }
            }
            else
            {
                linePtr = oldLine;
                if (M68K_GetEA(true, -1, &ea1))
                {
                    val = ea1.mode & 0x38;
                    switch (val)
                    {
                        case 0x00: // Dx,Dy
                            if (TOKEN_Comma()) break;
                            if (size == WID_X) size = WID_W;
                            reg1 = REG_Get(data_regs);
                            if (reg1 >= 0)
                            {
                                INSTR_W(parm + (ea1.mode << 9) + (size << 6) + reg1 + 0x20);
                            }
                            else
                            {
                                ASMX_IllegalOperand();
                            }
                            break;

                        case 0x08: // Ax
                            TOKEN_BadMode();
                            break;

                        default: // EA
                            if (size != WID_W && size != WID_X)
                            {
                                // memory shifts can't be .B or .L!
                                ASMX_Error("Invalid size in opcode");
                            }
                            else
                            {
                                M68K_InstrWE((parm & 0xF100) + ((parm & 0x18) << 6) + 0xC0, &ea1);
                            }
                            break;
                    }
                }
            }
            break;

        case OP_CHK:
            size = parm & 3;
            parm = parm & 0xFFFC;

            if (M68K_GetEA(false, size, &ea1))
            {
                if ((ea1.mode & 0x38) == 0x08)
                {
                    TOKEN_BadMode(); // address register not allowed
                }
                else
                {
                    if (TOKEN_Comma()) break;
                    reg1 = REG_Get(data_regs);
                    if (reg1 >= 0)
                    {
                        M68K_InstrWE(parm + (reg1 << 9), &ea1);
                    }
                }
            }
            break;

        case OP_CMPM:
            if (M68K_GetEA(true, -1, &ea1))
            {
                if (TOKEN_Comma()) break;
                if (M68K_GetEA(true, -1, &ea2))
                {
                    reg1 = ea1.mode & 7;
                    reg2 = ea2.mode & 7;
                    if ((ea1.mode & 0x38) != 0x18 || (ea2.mode & 0x38) != 0x18)
                    {
                        TOKEN_BadMode();
                    }
                    else
                    {
                        INSTR_W(parm + (reg2 << 9) + reg1);
                    }
                }
            }
            break;

        case OP_MOVEP:
            if (M68K_GetEA(true, -1, &ea1))
            {
                if (TOKEN_Comma()) break;
                if (M68K_GetEA(true, -1, &ea2))
                {
                    reg1 = ea1.mode & 7;
                    reg2 = ea2.mode & 7;
                    switch ((ea1.mode & 0x38)*256 + (ea2.mode & 0x38))
                    {
                        case 0x0010: // Dx,(Ay)
                            ea2.extra[0] = 0;
                        // fall through...
                        case 0x0028: // Dx,(d16,Ay) or Dx,d16(Ay)
                            INSTR_WW(parm + (reg1 << 9) + reg2 + 0x0080, ea2.extra[0]);
                            break;

                        case 0x1000: // (Ay),Dx
                            ea1.extra[0] = 0;
                        // fall through...
                        case 0x2800: // (d16,Ay),Dx or d16(Ay),Dx
                            INSTR_WW(parm + (reg2 << 9) + reg1, ea1.extra[0]);
                            break;

                        default:
                            TOKEN_BadMode();
                            break;
                    }
                }
            }
            break;

        case OP_EXG:
            reg1 = REG_Get(DA_regs);
            if (reg1 == 16) reg1 = 15; // SP -> A7
            if (reg1 >= 0)
            {
                if (TOKEN_Comma()) break;
                reg2 = REG_Get(DA_regs);
                if (reg2 == 16) reg2 = 15; // SP -> A7
                if (reg2 >= 0)
                {
                    if (reg1 <= 7)   // Dn
                    {
                        if (reg2 <= 7)
                        {
                            // Dn,Dn
                            INSTR_W(0xC140 + (reg1 << 9) + reg2);
                        }
                        else
                        {
                            // Dn,An
                            INSTR_W(0xC188 + (reg1 << 9) + (reg2 & 7));
                        }
                    }
                    else     // An
                    {
                        reg1 = reg1 & 7;
                        if (reg2 <= 7)
                        {
                            // An,Dn
                            INSTR_W(0xC188 + (reg2 << 9) + reg1);
                        }
                        else
                        {
                            // An,An
                            INSTR_W(0xC148 + (reg1 << 9) + (reg2 & 7));
                        }
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

        case OP_ABCD:
        case OP_ADDX:
            if (M68K_GetEA(true, -1, &ea1))
            {
                if (TOKEN_Comma()) break;
                if (M68K_GetEA(true, -1, &ea2))
                {
                    reg1 = ea1.mode & 7;
                    reg2 = ea2.mode & 7;
                    switch ((ea1.mode & 0x38)*256 + (ea2.mode & 0x38))
                    {
                        case 0x0000:
                            INSTR_W(parm + (reg2 << 9) + reg1); // Dy,Dx
                            break;

                        case 0x2020:
                            INSTR_W(parm + (reg2 << 9) + reg1 + 8); // -(Ay),-(Ax)
                            break;

                        default:
                            TOKEN_BadMode();
                            break;
                    }
                }
            }
            break;

        case OP_LINK:
            reg1 = REG_Get(addr_regs);
            if (reg1 == 8) reg1 = 7;
            if (reg1 >= 0)
            {
                if (TOKEN_Comma()) break;
                if (TOKEN_Expect("#")) break;
                val = EXPR_Eval();
                if (val > 0)
                {
                    ASMX_Warning("LINK opcode with positive displacement");
                }
                EXPR_CheckWord(val);
                INSTR_WW(parm + reg1, val);
            }
            else
            {
                ASMX_IllegalOperand();
            }
            break;


        case OP_OneA:
            reg1 = REG_Get(addr_regs);
            if (reg1 == 8) reg1 = 7;
            if (reg1 >= 0)
            {
                INSTR_W(parm + reg1);
            }
            else
            {
                ASMX_IllegalOperand();
            }
            break;

        case OP_OneD:
            reg1 = REG_Get(data_regs);
            if (reg1 == 8) reg1 = 7;
            if (reg1 >= 0)
            {
                INSTR_W(parm + reg1);
            }
            else
            {
                ASMX_IllegalOperand();
            }
            break;

        case OP_MOVEC:
            if (curCPU == CPU_68000) return 0;

            reg2 = M68K_GetMOVECreg();
            if (reg2 >= 0)
            {
                // Rc,Rn
                if (TOKEN_Comma()) break;
                reg1 = REG_Get(DA_regs);
                if (reg1 == 16) reg1 = 15; // SP -> A7
                if (reg1 < 0)
                {
                    ASMX_IllegalOperand();
                    break;
                }
                val = 0;
            }
            else
            {
                // Rn,Rc
                reg1 = REG_Get(DA_regs);
                if (reg1 == 16) reg1 = 15; // SP -> A7
                if (reg1 < 0)
                {
                    ASMX_IllegalOperand();
                    break;
                }
                if (TOKEN_Comma()) break;
                reg2 = M68K_GetMOVECreg();
                if (reg2 < 0)
                {
                    ASMX_IllegalOperand();
                    break;
                }
                val = 1;
            }
            INSTR_WW(parm + val,(reg1 << 12) + reg2);
            break;

        case OP_MOVES:
            if (curCPU == CPU_68000) return 0;

            size = parm & 3;
            parm = parm & 0xFFFC;
            reg1 = REG_Get(DA_regs);
            if (reg1 == 16) reg1 = 15; // SP -> A7
            if (reg1 >= 0)
            {
                // Rn,ea
                reg2 = 0x0800;  // direction = general register to ea
                if (TOKEN_Comma()) break;
                if (!M68K_GetEA(true, size, &ea1)) break;
                if ((ea1.mode & 0x30) == 0)
                {
                    TOKEN_BadMode();
                    break;
                }
            }
            else
            {
                // ea,Rn
                reg2 = 0x0000;  // direction = ea to general register
                if (!M68K_GetEA(true, size, &ea1)) break;
                if (TOKEN_Comma()) break;
                if ((ea1.mode & 0x30) == 0)
                {
                    TOKEN_BadMode();
                    break;
                }
                reg1 = REG_Get(DA_regs);
                if (reg1 == 16) reg1 = 15; // SP -> A7
                if (reg1 < 0) break;
            }
            M68K_InstrWWE(parm + (size << 6), (reg1 << 12) + reg2, &ea1);
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


void M68K_AsmInit(void)
{
    void *p = ASMX_AddAsm(versionName, &M68K_DoCPUOpcode, NULL, NULL);

    ASMX_AddCPU(p, "68K",    CPU_68000, END_BIG, ADDR_24, LIST_24, 8, 0, M68K_opcdTab);
    ASMX_AddCPU(p, "68000",  CPU_68000, END_BIG, ADDR_24, LIST_24, 8, 0, M68K_opcdTab);
    ASMX_AddCPU(p, "68010",  CPU_68010, END_BIG, ADDR_24, LIST_24, 8, 0, M68K_opcdTab);
}
