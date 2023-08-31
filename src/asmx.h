// asmx.h

#ifndef _ASMX_H_
#define _ASMX_H_

enum {
    MAX_BYTSTR = 1024,          // size of bytStr[]
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

enum {
    maxOpcdLen = 11,            // max opcode length (for building opcode table)
};
typedef char OpcdStr[maxOpcdLen+1];
struct OpcdRec {
    OpcdStr         name;       // opcode name
    short           typ;        // opcode type
    uint32_t        parm;       // opcode parameter
};
typedef struct OpcdRec OpcdRec;

// CPU option flags
enum {
    OPT_ATSYM     = 0x01,   // allow symbols to start with '@'
    OPT_DOLLARSYM = 0x02,   // allow symbols to start with '$'
};

void *AddAsm(const char *name,  // assembler name
              int (*DoCPUOpcode) (int typ, int parm),
              int (*DoCPULabelOp) (int typ, int parm, char *labl),
              void (*PassInit) (void) );
void AddCPU(void *as,           // assembler for this CPU
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
enum { UNKNOWN_END = -1, LITTLE_END, BIG_END };
enum { ADDR_16, ADDR_24, ADDR_32 };
enum { LIST_16, LIST_24 }; // Note: ADDR_24 and ADDR_32 should always use LIST_24

// special register numbers for FindReg/GetReg
enum {
    reg_EOL = -2,   // -2
    reg_None,       // -1
};

// opcode constants
enum {
    o_Illegal = 0x0100,
    o_LabelOp = 0x1000,
    o_EQU     = o_LabelOp + 0x100,
};

void Error(const char *message);
void Warning(const char *message);
void DefSym(const char *symName, uint32_t val, bool setSym, bool equSym);
int GetWord(char *word);
bool Expect(const char *expected);
bool Comma(void);
bool RParen(void);
void EatIt(void);
void IllegalOperand(void);
void MissingOperand(void);
void BadMode(void);
int FindReg(const char *regName, const char *regList);
int GetReg(const char *regList);
int CheckReg(int reg);
int Eval(void);
void CheckByte(int val);
void CheckStrictByte(int val);
void CheckWord(int val);
void CheckStrictWord(int val);
int EvalByte(void);
int EvalBranch(int instrLen);
int EvalWBranch(int instrLen);
int EvalLBranch(int instrLen);
void DoLabelOp(int typ, int parm, char *labl);

void InstrClear(void);
void InstrAddB(uint8_t b);
void InstrAddX(uint32_t op);
void InstrAddW(uint16_t w);
void InstrAdd3(uint32_t l);
void InstrAddL(uint32_t l);

void InstrB(uint8_t b1);
void InstrBB(uint8_t b1, uint8_t b2);
void InstrBBB(uint8_t b1, uint8_t b2, uint8_t b3);
void InstrBBBB(uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4);
void InstrBBBBB(uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4, uint8_t b5);
void InstrBW(uint8_t b1, uint16_t w1);
void InstrBBW(uint8_t b1, uint8_t b2, uint16_t w1);
void InstrBBBW(uint8_t b1, uint8_t b2, uint8_t b3, uint16_t w1);
void InstrB3(uint8_t b, uint32_t l);
void InstrX(uint32_t op);
void InstrXB(uint32_t op, uint8_t b1);
void InstrXBB(uint32_t op, uint8_t b1, uint8_t b2);
void InstrXBBB(uint32_t op, uint8_t b1, uint8_t b2, uint8_t b3);
void InstrXBBBB(uint32_t op, uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4);
void InstrXW(uint32_t op, uint16_t w1);
void InstrXBW(uint32_t op, uint8_t b1, uint16_t w1);
void InstrXBWB(uint32_t op, uint8_t b1, uint16_t w1, uint8_t b2);
void InstrXWW(uint32_t op, uint16_t w1, uint16_t w2);
void InstrX3(uint32_t op, uint32_t l1);
void InstrW(uint16_t w1);
void InstrWW(uint16_t w1, uint16_t w2);
void InstrWL(uint16_t w1, uint32_t l1);
void InstrL(uint32_t l1);
void InstrLL(uint32_t l1, uint32_t l2);

char *ListStr(char *l, const char *s);
char *ListByte(char *p, uint8_t b);
char *ListWord(char *p, uint16_t w);
char *ListLong(char *p, uint32_t l);
char *ListAddr(char *p,uint32_t addr);
char *ListLoc(uint32_t addr);

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
