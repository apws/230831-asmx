// asmM6502.c

#define versionName "M6502 assembler"
#include "asmx.h"

enum
{
    OP_Implied,          // implied instructions
    OP_Implied_65C02,    // implied instructions for 65C02/65C816
    OP_Implied_6502U,    // implied instructions for undocumented 6502 only
    OP_Branch,           // branch instructions
    OP_Branch_65C02,     // branch instructions for 65C02/65C816
    OP_Mode,             // instructions with multiple addressing modes
    OP_Mode_65C02,       // OP_Mode for 6502
    OP_Mode_6502U,       // OP_Mode for undocumented 6502
    OP_RSMB,             // RMBn/SMBn instructions for 65C02 only
    OP_BBRS,             // BBRn/BBSn instructions for 65C02 only
    // 65C816 instructions
    OP_Implied_65C816,   // implied instructions for 65C816 only
    OP_Mode_65C816,      // OP_Mode for 65C816
    OP_BranchW,          // 16-bit branch for 65C816
    OP_BlockMove,        // MVN,MVP for 65C816
    OP_COP,              // COP for 65C816

    OP_LONGA = OP_LabelOp,// .LONGA and .LONGI pseudo-ops for 65816
    OP_LONGI,
};

enum cputype
{
    CPU_6502, CPU_65C02, CPU_6502U, CPU_65C816
};

bool longa, longi;      // 65816 operand size flags

enum addrMode
{
    MADR_None = -1,// -1 invalid addressing mode
    MADR_Imm,      //  0 Immediate        #byte
    MADR_Abs,      //  1 Absolute         word
    MADR_Zpg,      //  2 Z-Page           byte
    MADR_Acc,      //  3 Accumulator      A (or no operand)
    MADR_Inx,      //  4 Indirect X       (byte,X)
    MADR_Iny,      //  5 Indirect Y       (byte),Y
    MADR_Zpx,      //  6 Z-Page X         byte,X
    MADR_Abx,      //  7 Absolute X       word,X
    MADR_Aby,      //  8 Absolute Y       word,Y
    MADR_Ind,      //  9 Indirect         (word)
    MADR_Zpy,      // 10 Z-Page Y         byte,Y
    MADR_Zpi,      // 11 Z-Page Indirect  (byte) 65C02/65C816 only
    // 65C816 modes
    MADR_AbL,      // 12 Absolute Long        al
    MADR_ALX,      // 13 Absolute Long X      al,x
    MADR_DIL,      // 14 Direct Indirect Long [d]
    MADR_DIY,      // 15 Direct Indirect Y    [d],Y
    MADR_Stk,      // 16 Stack Relative       d,S
    MADR_SIY,      // 17 Stack Indirect Y     (d,S),Y
    MADR_Max,      // 18
    MADR_Imm16,    // -- Immediate 16-bit, only generated in MADR_Imm
};

enum addrOps
{
    MOP_ORA,      //  0
    MOP_ASL,      //  1
    MOP_JSR,      //  2
    MOP_AND,      //  3
    MOP_BIT,      //  4
    MOP_ROL,      //  5
    MOP_EOR,      //  6
    MOP_LSR,      //  7
    MOP_JMP,      //  8
    MOP_ADC,      //  9
    MOP_ROR,      // 10
    MOP_STA,      // 11
    MOP_STY,      // 12
    MOP_STX,      // 13
    MOP_LDY,      // 14
    MOP_LDA,      // 15
    MOP_LDX,      // 16
    MOP_CPY,      // 17
    MOP_CMP,      // 18
    MOP_DEC,      // 19
    MOP_CPX,      // 20
    MOP_SBC,      // 21
    MOP_INC,      // 22

    MOP_Extra,    // 23

//  OP_Mode_6502U

    MOP_LAX = MOP_Extra, // 23
    MOP_DCP,
    MOP_ISB,
    MOP_RLA,
    MOP_RRA,
    MOP_SAX,
    MOP_SLO,
    MOP_SRE,
    MOP_ANC,
    MOP_ARR,
    MOP_ASR,
    MOP_SBX,

//  OP_Mode_65C02

    MOP_STZ = MOP_Extra, // 23
    MOP_TSB,      // 24
    MOP_TRB,      // 25

//  OP_Mode_65C816

    MOP_JSL,      // 26
    MOP_JML,      // 27
    MOP_REP,      // 28
    MOP_SEP,      // 29
    MOP_PEI,      // 30
    MOP_PEA,      // 31

    MOP_Max       // 32
};


static const uint8_t mode2op6502[] =   // [MOP_Max * MADR_Max] =
{
// Imm=0 Abs=1 Zpg=2 Acc=3 Inx=4 Iny=5 Zpx=6 Abx=7 Aby=8 Ind=9 Zpy=10 Zpi=11
    0x09, 0x0D, 0x05,    0, 0x01, 0x11, 0x15, 0x1D, 0x19,    0,    0,    0, 0,0,0,0,0,0, //  0 ORA
       0, 0x0E, 0x06, 0x0A,    0,    0, 0x16, 0x1E,    0,    0,    0,    0, 0,0,0,0,0,0, //  1 ASL
       0, 0x20,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 0,0,0,0,0,0, //  2 JSR
    0x29, 0x2D, 0x25,    0, 0x21, 0x31, 0x35, 0x3D, 0x39,    0,    0,    0, 0,0,0,0,0,0, //  3 AND
       0, 0x2C, 0x24,    0,    0,    0,    0,    0,    0,    0,    0,    0, 0,0,0,0,0,0, //  4 BIT
       0, 0x2E, 0x26, 0x2A,    0,    0, 0x36, 0x3E,    0,    0,    0,    0, 0,0,0,0,0,0, //  5 ROL
    0x49, 0x4D, 0x45,    0, 0x41, 0x51, 0x55, 0x5D, 0x59,    0,    0,    0, 0,0,0,0,0,0, //  6 EOR
       0, 0x4E, 0x46, 0x4A,    0,    0, 0x56, 0x5E,    0,    0,    0,    0, 0,0,0,0,0,0, //  7 LSR
       0, 0x4C,    0,    0,    0,    0,    0,    0,    0, 0x6C,    0,    0, 0,0,0,0,0,0, //  8 JMP
    0x69, 0x6D, 0x65,    0, 0x61, 0x71, 0x75, 0x7D, 0x79,    0,    0,    0, 0,0,0,0,0,0, //  9 ADC
       0, 0x6E, 0x66, 0x6A,    0,    0, 0x76, 0x7E,    0,    0,    0,    0, 0,0,0,0,0,0, // 10 ROR
       0, 0x8D, 0x85,    0, 0x81, 0x91, 0x95, 0x9D, 0x99,    0,    0,    0, 0,0,0,0,0,0, // 11 STA
       0, 0x8C, 0x84,    0,    0,    0, 0x94,    0,    0,    0,    0,    0, 0,0,0,0,0,0, // 12 STY
       0, 0x8E, 0x86,    0,    0,    0,    0,    0,    0,    0, 0x96,    0, 0,0,0,0,0,0, // 13 STX
    0xA0, 0xAC, 0xA4,    0,    0,    0, 0xB4, 0xBC,    0,    0,    0,    0, 0,0,0,0,0,0, // 14 LDY
    0xA9, 0xAD, 0xA5,    0, 0xA1, 0xB1, 0xB5, 0xBD, 0xB9,    0,    0,    0, 0,0,0,0,0,0, // 15 LDA
    0xA2, 0xAE, 0xA6,    0,    0,    0,    0,    0, 0xBE,    0, 0xB6,    0, 0,0,0,0,0,0, // 16 LDX
    0xC0, 0xCC, 0xC4,    0,    0,    0,    0,    0,    0,    0,    0,    0, 0,0,0,0,0,0, // 17 CPY
    0xC9, 0xCD, 0xC5,    0, 0xC1, 0xD1, 0xD5, 0xDD, 0xD9,    0,    0,    0, 0,0,0,0,0,0, // 18 CMP
       0, 0xCE, 0xC6,    0,    0,    0, 0xD6, 0xDE,    0,    0,    0,    0, 0,0,0,0,0,0, // 19 DEC
    0xE0, 0xEC, 0xE4,    0,    0,    0,    0,    0,    0,    0,    0,    0, 0,0,0,0,0,0, // 20 CPX
    0xE9, 0xED, 0xE5,    0, 0xE1, 0xF1, 0xF5, 0xFD, 0xF9,    0,    0,    0, 0,0,0,0,0,0, // 21 SBC
       0, 0xEE, 0xE6,    0,    0,    0, 0xF6, 0xFE,    0,    0,    0,    0, 0,0,0,0,0,0, // 22 INC
// undocumented 6502 instructions start here
// Imm=0 Abs=1 Zpg=2 Acc=3 Inx=4 Iny=5 Zpx=6 Abx=7 Aby=8 Ind=9 Zpy=10 Zpi=11
       0, 0xAF, 0xA7,    0, 0xA3, 0xB3,    0,    0, 0xBF,    0, 0xB7,    0, 0,0,0,0,0,0, // 23 LAX
       0, 0xCF, 0xC7,    0, 0xC3, 0xD3, 0xD7, 0xDF, 0xDB,    0,    0,    0, 0,0,0,0,0,0, // DCP
       0, 0xEF, 0xE7,    0, 0xE3, 0xF3, 0xF7, 0xFF, 0xFB,    0,    0,    0, 0,0,0,0,0,0, // ISB
       0, 0x2F, 0x27,    0, 0x23, 0x33, 0x37, 0x3F, 0x3B,    0,    0,    0, 0,0,0,0,0,0, // RLA
       0, 0x6F, 0x67,    0, 0x63, 0x73, 0x77, 0x7F, 0x7B,    0,    0,    0, 0,0,0,0,0,0, // RRA
       0, 0x8F, 0x87,    0, 0x83,    0,    0,    0,    0,    0, 0x97,    0, 0,0,0,0,0,0, // SAX
       0, 0x0F, 0x07,    0, 0x03, 0x13, 0x17, 0x1F, 0x1B,    0,    0,    0, 0,0,0,0,0,0, // SLO
       0, 0x4F, 0x47,    0, 0x43, 0x53, 0x57, 0x5F, 0x5B,    0,    0,    0, 0,0,0,0,0,0, // SRE
    0x2B,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 0,0,0,0,0,0, // ANC
    0x6B,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 0,0,0,0,0,0, // ARR
    0x4B,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 0,0,0,0,0,0, // ASR
    0xCB,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 0,0,0,0,0,0  // SBX
};


static const uint8_t mode2op65C02[] =   // [MOP_Max * MADR_Max] =
{
// Imm=0 Abs=1 Zpg=2 Acc=3 Inx=4 Iny=5 Zpx=6 Abx=7 Aby=8 Ind=9 Zpy=10 Zpi=11
    0x09, 0x0D, 0x05,    0, 0x01, 0x11, 0x15, 0x1D, 0x19,    0,    0, 0x12, 0,0,0,0,0,0, //  0 ORA
       0, 0x0E, 0x06, 0x0A,    0,    0, 0x16, 0x1E,    0,    0,    0,    0, 0,0,0,0,0,0, //  1 ASL
       0, 0x20,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 0,0,0,0,0,0, //  2 JSR
    0x29, 0x2D, 0x25,    0, 0x21, 0x31, 0x35, 0x3D, 0x39,    0,    0, 0x32, 0,0,0,0,0,0, //  3 AND
    0x89, 0x2C, 0x24,    0,    0,    0, 0x34, 0x3C,    0,    0,    0,    0, 0,0,0,0,0,0, //  4 BIT
       0, 0x2E, 0x26, 0x2A,    0,    0, 0x36, 0x3E,    0,    0,    0,    0, 0,0,0,0,0,0, //  5 ROL
    0x49, 0x4D, 0x45,    0, 0x41, 0x51, 0x55, 0x5D, 0x59,    0,    0, 0x52, 0,0,0,0,0,0, //  6 EOR
       0, 0x4E, 0x46, 0x4A,    0,    0, 0x56, 0x5E,    0,    0,    0,    0, 0,0,0,0,0,0, //  7 LSR
       0, 0x4C,    0,    0, 0x7C,    0,    0,    0,    0, 0x6C,    0,    0, 0,0,0,0,0,0, //  8 JMP note: 7C is (abs,x)
    0x69, 0x6D, 0x65,    0, 0x61, 0x71, 0x75, 0x7D, 0x79,    0,    0, 0x72, 0,0,0,0,0,0, //  9 ADC
       0, 0x6E, 0x66, 0x6A,    0,    0, 0x76, 0x7E,    0,    0,    0,    0, 0,0,0,0,0,0, // 10 ROR
       0, 0x8D, 0x85,    0, 0x81, 0x91, 0x95, 0x9D, 0x99,    0,    0, 0x92, 0,0,0,0,0,0, // 11 STA
       0, 0x8C, 0x84,    0,    0,    0, 0x94,    0,    0,    0,    0,    0, 0,0,0,0,0,0, // 12 STY
       0, 0x8E, 0x86,    0,    0,    0,    0,    0,    0,    0, 0x96,    0, 0,0,0,0,0,0, // 13 STX
    0xA0, 0xAC, 0xA4,    0,    0,    0, 0xB4, 0xBC,    0,    0,    0,    0, 0,0,0,0,0,0, // 14 LDY
    0xA9, 0xAD, 0xA5,    0, 0xA1, 0xB1, 0xB5, 0xBD, 0xB9,    0,    0, 0xB2, 0,0,0,0,0,0, // 15 LDA
    0xA2, 0xAE, 0xA6,    0,    0,    0,    0,    0, 0xBE,    0, 0xB6,    0, 0,0,0,0,0,0, // 16 LDX
    0xC0, 0xCC, 0xC4,    0,    0,    0,    0,    0,    0,    0,    0,    0, 0,0,0,0,0,0, // 17 CPY
    0xC9, 0xCD, 0xC5,    0, 0xC1, 0xD1, 0xD5, 0xDD, 0xD9,    0,    0, 0xD2, 0,0,0,0,0,0, // 18 CMP
       0, 0xCE, 0xC6, 0x3A,    0,    0, 0xD6, 0xDE,    0,    0,    0,    0, 0,0,0,0,0,0, // 19 DEC
    0xE0, 0xEC, 0xE4,    0,    0,    0,    0,    0,    0,    0,    0,    0, 0,0,0,0,0,0, // 20 CPX
    0xE9, 0xED, 0xE5,    0, 0xE1, 0xF1, 0xF5, 0xFD, 0xF9,    0,    0, 0xF2, 0,0,0,0,0,0, // 21 SBC
       0, 0xEE, 0xE6, 0x1A,    0,    0, 0xF6, 0xFE,    0,    0,    0,    0, 0,0,0,0,0,0, // 22 INC
// 65C02-only instructions start here
       0, 0x9C, 0x64,    0,    0,    0, 0x74, 0x9E,    0,    0,    0,    0, 0,0,0,0,0,0, // 23 STZ
       0, 0x0C, 0x04,    0,    0,    0,    0,    0,    0,    0,    0,    0, 0,0,0,0,0,0, // 24 TSB
       0, 0x1C, 0x14,    0,    0,    0,    0,    0,    0,    0,    0,    0, 0,0,0,0,0,0  // 25 TRB
};


static const uint8_t mode2op65C816[] =   // [MOP_Max * MADR_Max] =
{
// Imm=0 Abs=1 Zpg=2 Acc=3 Inx=4 Iny=5 Zpx=6 Abx=7 Aby=8 Ind=9 Zpy10 Zpi11 AbL12 ALX13 DIL14 DIY15 Stk16 SIY17
    0x09, 0x0D, 0x05,    0, 0x01, 0x11, 0x15, 0x1D, 0x19,    0,    0, 0x12, 0x0F, 0x1F, 0x07, 0x17, 0x03, 0x13, //  0 ORA
       0, 0x0E, 0x06, 0x0A,    0,    0, 0x16, 0x1E,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, //  1 ASL
       0, 0x20,    0,    0, 0xFC,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, //  2 JSR note: FC is (abs,X)
    0x29, 0x2D, 0x25,    0, 0x21, 0x31, 0x35, 0x3D, 0x39,    0,    0, 0x32, 0x2F, 0x3F, 0x27, 0x37, 0x23, 0x33, //  3 AND
    0x89, 0x2C, 0x24,    0,    0,    0, 0x34, 0x3C,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, //  4 BIT
       0, 0x2E, 0x26, 0x2A,    0,    0, 0x36, 0x3E,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, //  5 ROL
    0x49, 0x4D, 0x45,    0, 0x41, 0x51, 0x55, 0x5D, 0x59,    0,    0, 0x52, 0x4F, 0x5F, 0x47, 0x57, 0x43, 0x53, //  6 EOR
       0, 0x4E, 0x46, 0x4A,    0,    0, 0x56, 0x5E,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, //  7 LSR
       0, 0x4C,    0,    0, 0x7C,    0,    0,    0,    0, 0x6C,    0,    0, 0x5C,    0,    0,    0,    0,    0, //  8 JMP note: 7C is (abs,x)
    0x69, 0x6D, 0x65,    0, 0x61, 0x71, 0x75, 0x7D, 0x79,    0,    0, 0x72, 0x6F, 0x7F, 0x67, 0x77, 0x63, 0x73, //  9 ADC
       0, 0x6E, 0x66, 0x6A,    0,    0, 0x76, 0x7E,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, // 10 ROR
       0, 0x8D, 0x85,    0, 0x81, 0x91, 0x95, 0x9D, 0x99,    0,    0, 0x92, 0x8F, 0x9F, 0x87, 0x97, 0x83, 0x93, // 11 STA
       0, 0x8C, 0x84,    0,    0,    0, 0x94,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, // 12 STY
       0, 0x8E, 0x86,    0,    0,    0,    0,    0,    0,    0, 0x96,    0,    0,    0,    0,    0,    0,    0, // 13 STX
    0xA0, 0xAC, 0xA4,    0,    0,    0, 0xB4, 0xBC,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, // 14 LDY
    0xA9, 0xAD, 0xA5,    0, 0xA1, 0xB1, 0xB5, 0xBD, 0xB9,    0,    0, 0xB2, 0xAF, 0xBF, 0xA7, 0xB7, 0xA3, 0xB3, // 15 LDA
    0xA2, 0xAE, 0xA6,    0,    0,    0,    0,    0, 0xBE,    0, 0xB6,    0,    0,    0,    0,    0,    0,    0, // 16 LDX
    0xC0, 0xCC, 0xC4,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, // 17 CPY
    0xC9, 0xCD, 0xC5,    0, 0xC1, 0xD1, 0xD5, 0xDD, 0xD9,    0,    0, 0xD2, 0xCF, 0xDF, 0xC7, 0xD7, 0xC3, 0xD3, // 18 CMP
       0, 0xCE, 0xC6, 0x3A,    0,    0, 0xD6, 0xDE,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, // 19 DEC
    0xE0, 0xEC, 0xE4,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, // 20 CPX
    0xE9, 0xED, 0xE5,    0, 0xE1, 0xF1, 0xF5, 0xFD, 0xF9,    0,    0, 0xF2, 0xEF, 0xFF, 0xE7, 0xF7, 0xE3, 0xF3, // 21 SBC
       0, 0xEE, 0xE6, 0x1A,    0,    0, 0xF6, 0xFE,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, // 22 INC
       0, 0x9C, 0x64,    0,    0,    0, 0x74, 0x9E,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, // 23 STZ
       0, 0x0C, 0x04,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, // 24 TSB
       0, 0x1C, 0x14,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, // 25 TRB
       0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 0x22,    0,    0,    0,    0,    0, // 26 JSL
       0,    0,    0,    0,    0,    0,    0,    0,    0, 0xDC,    0,    0,    0,    0,    0,    0,    0,    0, // 27 JML
    0xC2,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, // 28 REP
    0xE2,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, // 29 SEP
       0,    0, 0xD4,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, // 30 PEI
       0, 0xF4,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0  // 31 PEA
};

// 26 22 JSL abs-long
// 27 DC JML (ind)
// 28 C2 REP imm
// 29 E2 SEP imm
// 30 D4 PEI zp
// 31 F4 PEA absolute

static const struct OpcdRec M6502_opcdTab[] =
{
    {"BRK",  OP_Implied, 0x00},
    {"PHP",  OP_Implied, 0x08},
    {"CLC",  OP_Implied, 0x18},
    {"PLP",  OP_Implied, 0x28},
    {"SEC",  OP_Implied, 0x38},
    {"RTI",  OP_Implied, 0x40},
    {"PHA",  OP_Implied, 0x48},
    {"CLI",  OP_Implied, 0x58},
    {"RTS",  OP_Implied, 0x60},
    {"PLA",  OP_Implied, 0x68},
    {"SEI",  OP_Implied, 0x78},
    {"DEY",  OP_Implied, 0x88},
    {"TXA",  OP_Implied, 0x8A},
    {"TYA",  OP_Implied, 0x98},
    {"TXS",  OP_Implied, 0x9A},
    {"TAY",  OP_Implied, 0xA8},
    {"TAX",  OP_Implied, 0xAA},
    {"CLV",  OP_Implied, 0xB8},
    {"TSX",  OP_Implied, 0xBA},
    {"INY",  OP_Implied, 0xC8},
    {"DEX",  OP_Implied, 0xCA},
    {"CLD",  OP_Implied, 0xD8},
    {"INX",  OP_Implied, 0xE8},
    {"NOP",  OP_Implied, 0xEA},
    {"SED",  OP_Implied, 0xF8},

    {"ASLA", OP_Implied, 0x0A},
    {"ROLA", OP_Implied, 0x2A},
    {"LSRA", OP_Implied, 0x4A},
    {"RORA", OP_Implied, 0x6A},

    {"BPL",  OP_Branch, 0x10},
    {"BMI",  OP_Branch, 0x30},
    {"BVC",  OP_Branch, 0x50},
    {"BVS",  OP_Branch, 0x70},
    {"BCC",  OP_Branch, 0x90},
    {"BCS",  OP_Branch, 0xB0},
    {"BNE",  OP_Branch, 0xD0},
    {"BEQ",  OP_Branch, 0xF0},

    {"ORA",  OP_Mode, MOP_ORA},
    {"ASL",  OP_Mode, MOP_ASL},
    {"JSR",  OP_Mode, MOP_JSR},
    {"AND",  OP_Mode, MOP_AND},
    {"BIT",  OP_Mode, MOP_BIT},
    {"ROL",  OP_Mode, MOP_ROL},
    {"EOR",  OP_Mode, MOP_EOR},
    {"LSR",  OP_Mode, MOP_LSR},
    {"JMP",  OP_Mode, MOP_JMP},
    {"ADC",  OP_Mode, MOP_ADC},
    {"ROR",  OP_Mode, MOP_ROR},
    {"STA",  OP_Mode, MOP_STA},
    {"STY",  OP_Mode, MOP_STY},
    {"STX",  OP_Mode, MOP_STX},
    {"LDY",  OP_Mode, MOP_LDY},
    {"LDA",  OP_Mode, MOP_LDA},
    {"LDX",  OP_Mode, MOP_LDX},
    {"CPY",  OP_Mode, MOP_CPY},
    {"CMP",  OP_Mode, MOP_CMP},
    {"DEC",  OP_Mode, MOP_DEC},
    {"CPX",  OP_Mode, MOP_CPX},
    {"SBC",  OP_Mode, MOP_SBC},
    {"INC",  OP_Mode, MOP_INC},

    // 65C02 opcodes

    {"INCA", OP_Implied_65C02, 0x1A},
    {"INA",  OP_Implied_65C02, 0x1A},
    {"DECA", OP_Implied_65C02, 0x3A},
    {"DEA",  OP_Implied_65C02, 0x3A},
    {"PHY",  OP_Implied_65C02, 0x5A},
    {"PLY",  OP_Implied_65C02, 0x7A},
    {"PHX",  OP_Implied_65C02, 0xDA},
    {"PLX",  OP_Implied_65C02, 0xFA},

    {"BRA",  OP_Branch_65C02, 0x80},

    {"STZ",  OP_Mode_65C02, MOP_STZ},
    {"TSB",  OP_Mode_65C02, MOP_TSB},
    {"TRB",  OP_Mode_65C02, MOP_TRB},

    {"RMB0", OP_RSMB, 0x07},
    {"RMB1", OP_RSMB, 0x17},
    {"RMB2", OP_RSMB, 0x27},
    {"RMB3", OP_RSMB, 0x37},
    {"RMB4", OP_RSMB, 0x47},
    {"RMB5", OP_RSMB, 0x57},
    {"RMB6", OP_RSMB, 0x67},
    {"RMB7", OP_RSMB, 0x77},

    {"SMB0", OP_RSMB, 0x87},
    {"SMB1", OP_RSMB, 0x97},
    {"SMB2", OP_RSMB, 0xA7},
    {"SMB3", OP_RSMB, 0xB7},
    {"SMB4", OP_RSMB, 0xC7},
    {"SMB5", OP_RSMB, 0xD7},
    {"SMB6", OP_RSMB, 0xE7},
    {"SMB7", OP_RSMB, 0xF7},

    {"BBR0", OP_BBRS, 0x0F},
    {"BBR1", OP_BBRS, 0x1F},
    {"BBR2", OP_BBRS, 0x2F},
    {"BBR3", OP_BBRS, 0x3F},
    {"BBR4", OP_BBRS, 0x4F},
    {"BBR5", OP_BBRS, 0x5F},
    {"BBR6", OP_BBRS, 0x6F},
    {"BBR7", OP_BBRS, 0x7F},

    {"BBS0", OP_BBRS, 0x8F},
    {"BBS1", OP_BBRS, 0x9F},
    {"BBS2", OP_BBRS, 0xAF},
    {"BBS3", OP_BBRS, 0xBF},
    {"BBS4", OP_BBRS, 0xCF},
    {"BBS5", OP_BBRS, 0xDF},
    {"BBS6", OP_BBRS, 0xEF},
    {"BBS7", OP_BBRS, 0xFF},

    // 65C816 opcodes

    {"PHD",  OP_Implied_65C816, 0x0B},
    {"TCS",  OP_Implied_65C816, 0x1B},
    {"PLD",  OP_Implied_65C816, 0x2B},
    {"TSC",  OP_Implied_65C816, 0x3B},
    {"WDM",  OP_Implied_65C816, 0x42},
    {"PHK",  OP_Implied_65C816, 0x4B},
    {"TCD",  OP_Implied_65C816, 0x5B},
    {"RTL",  OP_Implied_65C816, 0x6B},
    {"TDC",  OP_Implied_65C816, 0x7B},
    {"PHB",  OP_Implied_65C816, 0x8B},
    {"TXY",  OP_Implied_65C816, 0x9B},
    {"PLB",  OP_Implied_65C816, 0xAB},
    {"TYX",  OP_Implied_65C816, 0xBB},
    {"WAI",  OP_Implied_65C816, 0xCB},
    {"STP",  OP_Implied_65C816, 0xDB},
    {"XBA",  OP_Implied_65C816, 0xEB},
    {"XCE",  OP_Implied_65C816, 0xFB},
    {"PER",  OP_BranchW,        0x62},
    {"BRL",  OP_BranchW,        0x82},
    {"MVP",  OP_BlockMove,      0x44},
    {"MVN",  OP_BlockMove,      0x54},
    {"COP",  OP_COP,            0x02},
    {"JSL",  OP_Mode_65C816,    MOP_JSL},
    {"REP",  OP_Mode_65C816,    MOP_REP},
    {"PEI",  OP_Mode_65C816,    MOP_PEI},
    {"JML",  OP_Mode_65C816,    MOP_JML},
    {"SEP",  OP_Mode_65C816,    MOP_SEP},
    {"PEA",  OP_Mode_65C816,    MOP_PEA},

    // These set the hint for whether A / X / Y immediate operands
    // They are OFF for 8-bit or ON for 16-bit
    {".LONGA", OP_LONGA,  0}, // for A (and C)
    {".LONGI", OP_LONGI,  0}, // for X and Y

//  undocumented instructions for original 6502 only
//  see http://www.s-direktnet.de/homepages/k_nadj/opcodes.html
    {"NOP3", OP_Implied_6502U, 0x0400},  // 3-cycle NOP
    {"LAX",  OP_Mode_6502U,    MOP_LAX},   // LDA + LDX
    {"DCP",  OP_Mode_6502U,    MOP_DCP},   // DEC + CMP (also called DCM)
    {"ISB",  OP_Mode_6502U,    MOP_ISB},   // INC + SBC (also called INS and ISC)
    {"RLA",  OP_Mode_6502U,    MOP_RLA},   // ROL + AND
    {"RRA",  OP_Mode_6502U,    MOP_RRA},   // ROR + ADC
    {"SAX",  OP_Mode_6502U,    MOP_SAX},   // store (A & X) (also called AXS)
    {"SLO",  OP_Mode_6502U,    MOP_SLO},   // ASL + ORA (also called ASO)
    {"SRE",  OP_Mode_6502U,    MOP_SRE},   // LSR + EOR (also called LSE)
    {"ANC",  OP_Mode_6502U,    MOP_ANC},   // AND# with bit 7 copied to C flag (also called AAC)
    {"ARR",  OP_Mode_6502U,    MOP_ARR},   // AND# + RORA with strange flag results
    {"ASR",  OP_Mode_6502U,    MOP_ASR},   // AND# + LSRA (also called ALR)
    {"SBX",  OP_Mode_6502U,    MOP_SBX},   // X = (A & X) - #nn (also called ASX and SAX)

    {"",     OP_Illegal, 0}
};


// --------------------------------------------------------------


static int M6502_DoCPUOpcode(int typ, int parm)
{
    int     val, i;
    Str255  word;
    char    *oldLine;
    int     token;
    int     opcode;
    int     mode;
    const   uint8_t *modes;     // pointer to current OP_Mode instruction's opcodes

    switch (typ)
    {
        case OP_Implied_65C816:
            if (curCPU != CPU_65C816) return 0;
            FALLTHROUGH;
        case OP_Implied_65C02:
            if (curCPU != CPU_65C02 && curCPU != CPU_65C816) return 0;
            FALLTHROUGH;
        case OP_Implied_6502U:
            if (typ == OP_Implied_6502U && curCPU != CPU_6502U) return 0;
            FALLTHROUGH;
        case OP_Implied:
            if (parm > 256)
            {
                INSTR_BB(parm >> 8, parm & 255);
            }
            else
            {
                INSTR_B (parm);
            }
            break;

        case OP_Branch_65C02:
            if (curCPU == CPU_6502) return 0;
            FALLTHROUGH;
        case OP_Branch:
            val = EXPR_EvalBranch(2);
            INSTR_BB(parm, val);
            break;

        case OP_Mode_65C816:
            if (curCPU != CPU_65C816) return 0;
            FALLTHROUGH;
        case OP_Mode_65C02:
            if (curCPU != CPU_65C02 && curCPU != CPU_65C816) return 0;
            FALLTHROUGH;
        case OP_Mode_6502U:
            if (typ == OP_Mode_6502U && curCPU != CPU_6502U) return 0;
            FALLTHROUGH;
        case OP_Mode:
            instrLen = 0;
            oldLine = linePtr;
            token = TOKEN_GetWord(word);

            if      (curCPU == CPU_65C02)  modes = mode2op65C02;
            else if (curCPU == CPU_65C816) modes = mode2op65C816;
            else                           modes = mode2op6502;
            modes = modes + parm * MADR_Max;

            opcode = 0;
            mode = MADR_None;
            val  = 0;

            if (!token)
            {
                mode = MADR_Acc;    // accumulator
            }
            else
            {
                switch (token)
                {
                    case '#':   // immediate
                        mode = MADR_Imm;
                        opcode = modes[mode];
                        // check for 65C816 16-bit immediate
                        if (curCPU == CPU_65C816)
                        {
                            // check if opcode can do 16-bit immediate
                            // longa: 09 29 49 69 89 A9 C9 E9
                            // longi: A0 A2 C0 E0
                            bool can_longa = ((opcode & 0x1F) == 0x09);
                            bool can_longi = (opcode == 0xA0 || opcode == 0xA2 || opcode == 0xC0 || opcode == 0xE0);

                            // peek at next token and check for '<' or '>' address force
                            oldLine = linePtr;
                            token = TOKEN_GetWord(word);
                            if ((can_longa || can_longi) && (token == '<' || token == '>'))
                            {
                                // use 16-bit mode if '>' forced
                                if (token == '>')
                                {
                                    mode = MADR_Imm16;
                                }
                            }
                            else
                            {
                                linePtr = oldLine;
                                // if not forced, check for longa/longi flags
                                // and generate 16-bit immediate if set
                                if ((longa && can_longa) || (longi && can_longi))
                                {
                                    mode = MADR_Imm16;
                                }
                            }
                        }
                        val = EXPR_Eval();
                        break;

                    case '(':   // indirect X,Y
                        val   = EXPR_Eval();
                        token = TOKEN_GetWord(word);
                        switch (token)
                        {
                            case ',':   // (val,X)
                                switch (REG_Get("X S"))
                                {
                                    case 0: // ,X
                                        TOKEN_RParen();
                                        mode = MADR_Inx;
                                        break;

                                    case 1: // ,S
                                        if (curCPU == CPU_65C816)
                                        {
                                            if (TOKEN_RParen()) break;
                                            if (TOKEN_Comma()) break;
                                            if (TOKEN_Expect("Y")) break;
                                            mode = MADR_SIY;
                                        }
                                    default:
                                        ; // fall through to report bad mode
                                }
                                break;

                            case ')':   // (val) -and- (val),Y
                                token = TOKEN_GetWord(word);
                                switch (token)
                                {
                                    case 0:
                                        mode = MADR_Ind;
                                        if (val >= 0 && val < 256 && evalKnown &&
                                                (modes[MADR_Zpi] != 0))
                                        {
                                            mode = MADR_Zpi;
                                        }
                                        else
                                        {
                                            mode = MADR_Ind;
                                        }
                                        break;
                                    case ',':
                                        TOKEN_Expect("Y");
                                        mode = MADR_Iny;
                                        break;
                                }
                                break;

                            default:
                                mode = MADR_None;
                                break;
                        }
                        break;

                    case '[':   // MADR_DIL [d] and MADR_DIY [d],Y
                        if (curCPU == CPU_65C816)
                        {
                            val = EXPR_Eval();
                            TOKEN_Expect("]");
                            mode = MADR_DIL;
                            oldLine = linePtr;
                            switch (TOKEN_GetWord(word))
                            {
                                default:
                                    linePtr = oldLine;
                                    // fall through to let garbage be handled as too many operands
                                    FALLTHROUGH;
                                case 0:
                                    break;

                                case ',':
                                    if (REG_Get("Y") == 0)
                                    {
                                        mode = MADR_DIY;
                                    }
                                    else
                                    {
                                        mode = MADR_None;
                                    }
                            }
                            break;
                        }
                        // else fall through letting eval() treat the '[' as an open paren
                        FALLTHROUGH;

                    default:    // everything else
                        if (REG_Find(word, "A") == 0)
                        {
                            // accumulator
                            token = TOKEN_GetWord(word);
                            if (token == 0)
                            {
                                // mustn't have anything after the 'A'
                                mode = MADR_Acc;
                            }
                        }
                        else
                        {
                            // absolute and absolute-indexed
                            // allow '>' in front of address to force absolute addressing
                            // modes instead of zero-page addressing modes
                            bool forceabs = false;  // true = force a mode larger than 8 bits
                            bool force24 = false;   // true = force a mode larger than 16 bits
                            if (token == '>')
                            {
                                forceabs = true;

                                // check for a second '>' to force 24-bit mode
                                // this will cause no difference if only 16-bit mode is available
                                oldLine = linePtr;
                                token = TOKEN_GetWord(word);
                                if (token == '>')
                                {
                                    force24 = true;
                                }
                                else
                                {
                                    linePtr = oldLine;
                                }

                            }
                            else
                            {
                                linePtr = oldLine;
                            }

                            val   = EXPR_Eval();
                            token = TOKEN_GetWord(word);

                            switch (token)
                            {
                                case 0:     // abs or zpg
                                    if (val >= 0 && val < 256 && !forceabs &&
                                            evalKnown && (modes[MADR_Zpg] != 0))
                                    {
                                        mode = MADR_Zpg;
                                    }
                                    else
                                    {
                                        mode = MADR_Abs;
                                    }
                                    if (evalKnown && modes[MADR_AbL] != 0 && ((val & 0xFF0000) != 0 || force24))
                                    {
                                        mode = MADR_AbL;
                                    }
                                    break;

                                case ',':   // indexed ,x or ,y or ,s
                                    switch (REG_Get("X Y S"))
                                    {
                                        case 0: // ,X
                                            if (val >= 0 && val < 256 && !forceabs &&
                                                    (evalKnown || modes[MADR_Abx] == 0))
                                            {
                                                mode = MADR_Zpx;
                                            }
                                            else
                                            {
                                                mode = MADR_Abx;
                                            }
                                            if (evalKnown && modes[MADR_ALX] != 0 && ((val & 0xFF0000) != 0 || force24))
                                            {
                                                mode = MADR_ALX;
                                            }
                                            break;

                                        case 1: // ,Y
                                            if (val >= 0 && val < 256 && !forceabs &&
                                                    (evalKnown || modes[MADR_Aby] == 0)
                                                    && modes[MADR_Zpy] != 0)
                                            {
                                                mode = MADR_Zpy;
                                            }
                                            else
                                            {
                                                mode = MADR_Aby;
                                            }
                                            break;

                                        case 2: // ,S
                                            if (curCPU == CPU_65C816)
                                            {
                                                if (forceabs)
                                                {
                                                    TOKEN_BadMode();
                                                }
                                                else
                                                {
                                                    mode = MADR_Stk;
                                                }
                                            }
                                    }
                                    break;
                            }
                        }
                        break;
                }
            }

            // determine opcode if it is isn't known yet
            if (!opcode && mode != MADR_None)
            {
                opcode = modes[mode];
                if (opcode == 0)
                {
                    mode = MADR_None;
                }
            }

            instrLen = 0;
            switch (mode)
            {
                case MADR_None:
                    ASMX_Error("Invalid addressing mode");
                    break;

                case MADR_Acc:
                    INSTR_B(opcode);
                    break;

                case MADR_Inx:
                    if (opcode == 0x7C || opcode == 0xFC)
                    {
                        // 65C02 JMP (abs,X) / 65C816 JSR (abs,X)
                        INSTR_BW(opcode, val);
                        break;
                    }
                // else fall through

                case MADR_Zpg:
                case MADR_Iny:
                case MADR_Zpx:
                case MADR_Zpy:
                case MADR_Zpi:
                case MADR_Stk:
                case MADR_DIL:
                case MADR_DIY:
                case MADR_SIY:
                    val = (short) val;
                    if (!errFlag && (val < 0 || val > 255))
                    {
                        ASMX_Error("Byte out of range");
                    }
                    INSTR_BB(opcode, val & 0xFF);
                    break;

                case MADR_Imm:
                    val = (short) val;
                    if (!errFlag && (val < -128 || val > 255))
                    {
                        ASMX_Error("Byte out of range");
                    }
                    INSTR_BB(opcode, val & 0xFF);
                    break;

                case MADR_Imm16:
                    INSTR_BW(opcode, val & 0xFFFF);
                    break;

                case MADR_Abs:
                case MADR_Abx:
                case MADR_Aby:
                case MADR_Ind:
                    INSTR_BW(opcode, val);
                    break;

                case MADR_AbL:
                case MADR_ALX:
                    INSTR_B3(opcode, val);
                    break;
            }
            break;

        case OP_RSMB:        //    RMBn/SMBn instructions
            if (curCPU != CPU_65C02) return 0;

            // RMB/SMB zpg
            val = EXPR_Eval();
            INSTR_BB(parm, val);
            break;

        case OP_BBRS:        //    BBRn/BBSn instructions
            if (curCPU != CPU_65C02) return 0;

            i = EXPR_Eval();
            TOKEN_Expect(",");
            val = EXPR_EvalBranch(3);
            INSTR_BBB(parm, i, val);
            break;

        case OP_BranchW:
            if (curCPU != CPU_65C816) return 0;

            val = EXPR_EvalWBranch(3);
            INSTR_BW(parm, val);
            break;

        case OP_BlockMove:
            if (curCPU != CPU_65C816) return 0;

            val = EXPR_Eval();
            if (!errFlag && (val < 0 || val > 255))
            {
                ASMX_Error("Byte out of range");
            }

            if (TOKEN_Comma()) break;

            i = EXPR_Eval();
            if (!errFlag && (val < 0 || val > 255))
            {
                ASMX_Error("Byte out of range");
            }

            INSTR_BBB(parm, i, val);
            break;

        case OP_COP:
            if (curCPU != CPU_65C816) return 0;

            TOKEN_Expect("#");
            val = EXPR_Eval();
            if (!errFlag && (val < 0 || val > 255))
            {
                ASMX_Error("Byte out of range");
            }

            INSTR_BB(parm, val);
            break;

        default:
            return 0;
            break;
    }
    return 1;
}


static int M6502_DoCPULabelOp(int typ, int parm, char *labl)
{
    Str255  word;

    (void) parm; // unused parameter

    // ignore .LONGA and .LONGI if CPU is not 65C816
    if (curCPU != CPU_65C816) return 0;

    switch (typ)
    {
        case OP_LONGA:
            if (labl[0])
            {
                ASMX_Error("Label not allowed");
            }
            TOKEN_GetWord(word);
            if (strcmp(word, "ON") == 0)       longa = true;
            else if (strcmp(word, "OFF") == 0)      longa = false;
            else                                    ASMX_IllegalOperand();

            break;

        case OP_LONGI:
            if (labl[0])
            {
                ASMX_Error("Label not allowed");
            }
            TOKEN_GetWord(word);
            if (strcmp(word, "ON") == 0)       longi = true;
            else if (strcmp(word, "OFF") == 0)      longi = false;
            else                                    ASMX_IllegalOperand();

            break;

        default:
            return 0;
            break;
    }
    return 1;
}


static void M6502_PassInit(void)
{
    longa = false;
    longi = false;
}


void M6502_AsmInit(void)
{
    void *p = ASMX_AddAsm(versionName, &M6502_DoCPUOpcode, &M6502_DoCPULabelOp, &M6502_PassInit);

    ASMX_AddCPU(p, "6502",   CPU_6502,   END_LITTLE, ADDR_16, LIST_24, 8, 0, M6502_opcdTab);
    ASMX_AddCPU(p, "65C02",  CPU_65C02,  END_LITTLE, ADDR_16, LIST_24, 8, 0, M6502_opcdTab);
    ASMX_AddCPU(p, "6502U",  CPU_6502U,  END_LITTLE, ADDR_16, LIST_24, 8, 0, M6502_opcdTab);
    ASMX_AddCPU(p, "65816",  CPU_65C816, END_LITTLE, ADDR_24, LIST_24, 8, 0, M6502_opcdTab);
    ASMX_AddCPU(p, "65C816", CPU_65C816, END_LITTLE, ADDR_24, LIST_24, 8, 0, M6502_opcdTab);
}
