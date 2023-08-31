// asm8008.c

#define versionName "8008 assembler"
#include "asmx.h"

enum {
    CPU_8008,
};

enum instrType {
    o_None,         // No operands

    o_Immediate,    // one-byte immediate operand
    o_LImmediate,   // two-byte immediate operand (mostly JMPs)
    o_MOV,          // MOV r,r opcode
    o_RST,          // RST instruction
    o_Arith,        // Arithmetic instructions
    o_MVI,          // MVI instruction
    o_LXI,          // LXI instruciton
    o_INR,          // INR/DCR instructions
    o_IN,           // IN instructions
    o_OUT,          // OUT instructions

//  o_Foo = o_LabelOp,
};

const char I8008_regs1[]      = "A B C D E H L M";
const char I8008_regs2[]      = "B D H";

static const struct OpcdRec I8008_opcdTab[] = {
    {"RLC", o_None, 0x02},
    {"RNC", o_None, 0x03}, // also RFC
    {"RET", o_None, 0x07},
    {"RRC", o_None, 0x0A},
    {"RNZ", o_None, 0x0B}, // also RFZ
    {"RAL", o_None, 0x12},
    {"RP",  o_None, 0x13}, // also RFS
    {"RPO", o_None, 0x1B}, // also RFP
    {"RAR", o_None, 0x1A},
    {"RC",  o_None, 0x23}, // also RTC
    {"RZ",  o_None, 0x2B}, // also RTZ
    {"RM",  o_None, 0x33}, // also RTS
    {"RPE", o_None, 0x3B}, // also RTP
    {"NOP", o_None, 0xC0},
    {"HLT", o_None, 0xFF},

    {"ADI", o_Immediate, 0x04},
    {"ACI", o_Immediate, 0x0C},
    {"SUI", o_Immediate, 0x14},
    {"SCI", o_Immediate, 0x1C}, // also SBI
    {"ANI", o_Immediate, 0x24}, // also NDI
    {"XRI", o_Immediate, 0x2C},
    {"ORI", o_Immediate, 0x34},
    {"CPI", o_Immediate, 0x3C},

    {"JNC", o_LImmediate, 0x40}, // also JFC
    {"CNC", o_LImmediate, 0x42}, // also CFC
    {"JMP", o_LImmediate, 0x44},
    {"CALL",o_LImmediate, 0x46}, // also CAL
    {"JNZ", o_LImmediate, 0x48}, // also JFZ
    {"CNZ", o_LImmediate, 0x4A}, // also CFZ
    {"JP",  o_LImmediate, 0x50}, // also JFS
    {"CP",  o_LImmediate, 0x52}, // also CFS
    {"JPO", o_LImmediate, 0x58}, // also JFP
    {"CPO", o_LImmediate, 0x5A}, // also CFP
    {"JC",  o_LImmediate, 0x60}, // also JTC
    {"CC",  o_LImmediate, 0x62}, // also CTC
    {"JZ",  o_LImmediate, 0x68}, // also JTZ
    {"CZ",  o_LImmediate, 0x6A}, // also CTZ
    {"JM",  o_LImmediate, 0x70}, // also JTS
    {"CM",  o_LImmediate, 0x72}, // also CTS
    {"JPE", o_LImmediate, 0x78}, // also JTP
    {"CPE", o_LImmediate, 0x7A}, // also CTP

    {"MOV", o_MOV,   0xC0},

    {"RST", o_RST,   0x05},

    {"ADD", o_Arith, 0x80}, // also ADA..ADM
    {"ADC", o_Arith, 0x88}, // also ACA..ACM
    {"SUB", o_Arith, 0x90}, // also SUA..SUM
    {"SBB", o_Arith, 0x98}, // also SBA..SBM
    {"ANA", o_Arith, 0xA0}, // also NDA..NDM
    {"XRA", o_Arith, 0xA8}, // also XRA..XRM
    {"ORA", o_Arith, 0xB0}, // also ORA..ORM
    {"CMP", o_Arith, 0xB8}, // also CRA..CRM

    {"MVI", o_MVI,   0x06},

    {"LXI", o_LXI,   0x16},

    {"INR", o_INR,   0x00}, // also INA..INM
    {"DCR", o_INR,   0x01}, // also DCA..DCM

    {"IN",  o_IN,    0x41}, // also INP

    {"OUT", o_OUT,   0x41},

    {"",    o_Illegal,  0}
};


// --------------------------------------------------------------


static int I8008_DoCPUOpcode(int typ, int parm)
{
    int     val, reg1, reg2;
    Str255  word;
    char    *oldLine;
//  int     token;

    switch (typ) {
        case o_None:
            InstrB(parm & 255);
            break;

        case o_Immediate:
            val = Eval();
            InstrBB(parm & 0xFF, val);
            break;

        case o_LImmediate:
            val = Eval();
            InstrBW(parm, val);
            break;

        case o_MOV:
            GetWord(word);
            reg1 = FindReg(word, I8008_regs1);
            if (reg1 < 0) {
                IllegalOperand();
            } else {
                oldLine = linePtr;
                if (GetWord(word) != ',') {
                    linePtr = oldLine;
                    Comma();
                } else {
                    GetWord(word);
                    reg2 = FindReg(word, I8008_regs1);
                    if (reg2 < 0 || (reg1 == 7 && reg2 == 7)) {
                        IllegalOperand();
                    } else {
                        InstrB(parm + (reg1 << 3) + reg2);
                    }
                }
            }
            break;

        case o_RST:
            val = Eval();
            if (0 <= val && val <= 7) {
                InstrB(parm + val*8);
            } else {
                IllegalOperand();
            }
            break;

        case o_Arith:
            GetWord(word);
            reg1 = FindReg(word, I8008_regs1);
            if (reg1 < 0) {
                IllegalOperand();
            } else {
                InstrB(parm + reg1);
            }
            break;

        case o_MVI:
            GetWord(word);
            reg1 = FindReg(word, I8008_regs1);
            if (reg1 < 0) {
                IllegalOperand();
            } else {
                oldLine = linePtr;
                if (GetWord(word) != ',') {
                    linePtr = oldLine;
                    Comma();
                } else {
                    val = Eval();
                    InstrBB(parm + (reg1 << 3), val);
                }
            }
            break;

        case o_LXI:
            GetWord(word);
            reg1 = FindReg(word, I8008_regs2);
            if (reg1 < 0) {
                IllegalOperand();
            } else {
                oldLine = linePtr;
                if (GetWord(word) != ',') {
                    linePtr = oldLine;
                    Comma();
                } else {
                    val = Eval();
                    InstrBBBB(parm + (reg1 << 4), val & 0xFF, parm + (reg1 << 4) - 8, val >> 8);
                }
            }
            break;

        case o_INR:
            GetWord(word);
            reg1 = FindReg(word, I8008_regs1);
            if ((reg1 < 0) || (reg1 == 0) || (reg1 == 7)) {
                IllegalOperand();
            } else {
                InstrB(parm + (reg1 << 3));
            }
            break;

        case o_IN:
            val = Eval();
            if ((val < 0) || (val > 7)) {
                IllegalOperand();
            } else {
                InstrB(parm + val*2);
            }
            break;

        case o_OUT:
            val = Eval();
            if ((val < 8) || (val > 31)) {
                IllegalOperand();
            } else {
                InstrB(parm + val*2);
            }
            break;

        default:
            return 0;
            break;
    }
    return 1;
}


void Asm8008Init(void)
{
    void *p = AddAsm(versionName, &I8008_DoCPUOpcode, NULL, NULL);

    AddCPU(p, "8008",  CPU_8008,  LITTLE_END, ADDR_16, LIST_24, 8, 0, I8008_opcdTab);
}
