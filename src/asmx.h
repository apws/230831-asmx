// asmx.h

#ifndef _ASMX_H_
#define _ASMX_H_

enum
{
    IHEX_SIZE   = 32,       // max number of data bytes per line in hex object file
    MAXSYMLEN   = 32,       // max symbol length (only used in SYM_Dump())
    symTabCols  = 1,        // number of columns for symbol table dump
    MAXMACPARMS = 30,       // maximum macro parameters
    MAX_INCLUDE = 10,       // maximum INCLUDE nesting level
    MAX_BYTSTR  = 1024,     // size of bytStr[] (moved to asmx.h)
    MAX_COND    = 256,      // maximum nesting level of IF blocks
    MAX_MACRO   = 10,       // maximum nesting level of MACRO invocations
    TRS_BUF_MAX = 256,      // maximum buffer size for TRSDOS block
};

#include <stdio.h>
#include <sys/types.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>


#if defined(__clang__) // disable unwanted warnings for xcode
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#endif

#if 0
typedef unsigned char  bool;    // define a bool type
enum { false = 0, false = 1 };
#else
#include <stdbool.h>
#endif

typedef char Str255[256];       // generic string type

enum
{
    maxOpcdLen = 11,            // max opcode length (for building opcode table)
};
typedef char OpcdStr[maxOpcdLen+1];
struct OpcdRec
{
    OpcdStr         name;       // opcode name
    short           typ;        // opcode type
    uint32_t        parm;       // opcode parameter
};
typedef struct OpcdRec OpcdRec;

// CPU option flags
enum
{
    OPT_ATSYM     = 0x01,   // allow symbols to start with '@'
    OPT_DOLLARSYM = 0x02,   // allow symbols to start with '$'
};

void *ASMX_AddAsm(const char *name,  // assembler name
             int (*DoCPUOpcode) (int typ, int parm),
             int (*DoCPULabelOp) (int typ, int parm, char *labl),
             void (*PassInit) (void) );
void ASMX_AddCPU(void *as,           // assembler for this CPU
            const char *name,   // uppercase name of this CPU
            int index,          // index number for this CPU
            int endian,         // assembler endian
            int addrWid,        // assembler 32-bit
            int listWid,        // listing width
            int wordSize,       // addressing word size in bits
            int opts,           // option flags
            const struct OpcdRec opcdTab[]); // assembler opcode table

// assembler endian, address width, and listing hex width settings
// 0 = little endian, 1 = big endian, -1 = undefined endian
enum { END_UNKNOWN = -1, END_LITTLE, END_BIG };
enum { ADDR_16, ADDR_24, ADDR_32 };
enum { LIST_16, LIST_24 }; // Note: ADDR_24 and ADDR_32 should always use LIST_24

// special register numbers for FindReg/GetReg
enum
{
    reg_EOL = -2,   // -2
    reg_None,       // -1
};

// opcode constants
enum
{
    OP_Illegal = 0x0100,
    OP_LabelOp = 0x1000,
    OP_EQU     = OP_LabelOp + 0x100,
};

void ASMX_Error(const char *message);
void ASMX_Warning(const char *message);
void SYM_Def(const char *symName, uint32_t val, bool setSym, bool equSym);
int TOKEN_GetWord(char *word);
bool TOKEN_Expect(const char *expected);
bool TOKEN_Comma(void);
bool TOKEN_RParen(void);
void TOKEN_EatIt(void);
void ASMX_IllegalOperand(void);
void ASMX_MissingOperand(void);
void TOKEN_BadMode(void);
int REG_Find(const char *regName, const char *regList);
int REG_Get(const char *regList);
int REG_Check(int reg);
int EXPR_Eval(void);
void EXPR_CheckByte(int val);
void EXPR_CheckStrictByte(int val);
void EXPR_CheckWord(int val);
void EXPR_CheckStrictWord(int val);
int EXPR_EvalByte(void);
int EXPR_EvalBranch(int instrLen);
int EXPR_EvalWBranch(int instrLen);
int EXPR_EvalLBranch(int instrLen);
void ASMX_DoLabelOp(int typ, int parm, char *labl);

void INSTR_Clear(void);
void INSTR_AddB(uint8_t b);
void INSTR_AddX(uint32_t op);
void INSTR_AddW(uint16_t w);
void INSTR_Add3(uint32_t l);
void INSTR_AddL(uint32_t l);

void INSTR_B(uint8_t b1);
void INSTR_BB(uint8_t b1, uint8_t b2);
void INSTR_BBB(uint8_t b1, uint8_t b2, uint8_t b3);
void INSTR_BBBB(uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4);
void INSTR_BBBBB(uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4, uint8_t b5);
void INSTR_BW(uint8_t b1, uint16_t w1);
void INSTR_BBW(uint8_t b1, uint8_t b2, uint16_t w1);
void INSTR_BBBW(uint8_t b1, uint8_t b2, uint8_t b3, uint16_t w1);
void INSTR_B3(uint8_t b, uint32_t l);
void INSTR_X(uint32_t op);
void INSTR_XB(uint32_t op, uint8_t b1);
void INSTR_XBB(uint32_t op, uint8_t b1, uint8_t b2);
void INSTR_XBBB(uint32_t op, uint8_t b1, uint8_t b2, uint8_t b3);
void INSTR_XBBBB(uint32_t op, uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4);
void INSTR_XW(uint32_t op, uint16_t w1);
void INSTR_XBW(uint32_t op, uint8_t b1, uint16_t w1);
void INSTR_XBWB(uint32_t op, uint8_t b1, uint16_t w1, uint8_t b2);
void INSTR_XWW(uint32_t op, uint16_t w1, uint16_t w2);
void INSTR_X3(uint32_t op, uint32_t l1);
void INSTR_W(uint16_t w1);
void INSTR_WW(uint16_t w1, uint16_t w2);
void INSTR_WL(uint16_t w1, uint32_t l1);
void INSTR_L(uint32_t l1);
void INSTR_LL(uint32_t l1, uint32_t l2);

char *LIST_Str(char *l, const char *s);
char *LIST_Byte(char *p, uint8_t b);
char *LIST_Word(char *p, uint16_t w);
char *LIST_Long(char *p, uint32_t l);
char *LIST_Addr(char *p,uint32_t addr);
char *LIST_Loc(uint32_t addr);

// various internal variables used by the assemblers
extern  bool            errFlag;            // true if error occurred this line
extern  int             pass;               // Current assembler pass
extern  char           *linePtr;            // pointer into current line
extern  int             instrLen;           // Current instruction length (negative to display as long DB)
extern  Str255          line;               // Current line from input file
extern  uint32_t        locPtr;             // Current program address
extern  int             instrLen;           // Current instruction length (negative to display as long DB)
extern  uint8_t         bytStr[MAX_BYTSTR]; // Current instruction / buffer for long DB statements
extern  bool            showAddr;           // true to show LocPtr on listing
extern  int             endian;             // 0 = little endian, 1 = big endian, -1 = undefined endian
extern  bool            evalKnown;          // true if all operands in Eval were "known"
extern  int             curCPU;             // current CPU index for current assembler
extern  Str255          listLine;           // Current listing line
extern  int             hexSpaces;          // flags for spaces in hex output for instructions
extern  int             listWid;            // listing width: LIST_16, LIST_24
extern  bool            exactFlag;          // true to disable assembler-specific optimizations

// fallthrough annotation to prevent warnings
#if defined(__clang__) && __cplusplus >= 201103L
#define FALLTHROUGH [[clang::fallthrough]]
#elif defined(_MSC_VER)
#include <sal.h>
#define FALLTHROUGH __fallthrough
#elif defined(__GNUC__) && __GNUC__ >= 7
#define FALLTHROUGH __attribute__ ((fallthrough))
#elif defined (__has_cpp_attribute)
#if __has_cpp_attribute(fallthrough)
#define FALLTHROUGH [[fallthrough]]
#else // default version
#define FALLTHROUGH ((void)0)
#endif
#else // default version
#define FALLTHROUGH ((void)0)
#endif /* __GNUC__ >= 7 */

// disable GCC format truncation detection for snprintf
// and strncpy trunction detection
#if defined(__GNUC__) && __GNUC__ >= 7
#pragma GCC diagnostic ignored "-Wformat-truncation"
#pragma GCC diagnostic ignored "-Wstringop-truncation"
#endif /* __GNUC__ >= 7 */

#endif // _ASMX_H_
