// asmx.c

#include "asmx.h"

#define VERSION_NAME "asmx multi-assembler"

//#define ENABLE_REP    // uncomment to enable REPEAT pseudo-op (still under development)
//#define DOLLAR_SYM    // allow symbols to start with '$' (incompatible with $ for hexadecimal constants!)
//#define TEMP_LBLAT    // enable use of '@' temporary labels (deprecated)
#define OCTAL_AT        // enable use of Motorola-style @nnnnnn octal constants
// NOTE: OCTAL_AT may cause problems with TEMP_LBLAT
//       if you use numeric temp labels like @1 etc.!

#ifndef VERSION // should be defined on the command line
#define VERSION "2.0"
#endif
#define COPYRIGHT "Copyright 1998-2020 Bruce Tomlin"

// --------------------------------------------------------------

const char      *progname;      // pointer to argv[0]

struct MacroLine
{
    struct MacroLine    *next;      // pointer to next macro line
    char                text[1];    // macro line, storage = 1 + length
};
typedef struct MacroLine MacroLine;

struct MacroParm
{
    struct MacroParm    *next;      // pointer to next macro parameter name
    char                name[1];    // macro parameter name, storage = 1 + length
};
typedef struct MacroParm MacroParm;

struct MacroRec
{
    struct MacroRec     *next;      // pointer to next macro
    bool                def;        // true after macro is defined in pass 2
    bool                toomany;    // true if too many parameters in definition
    MacroLine           *text;      // macro text
    MacroParm           *parms;     // macro parms
    int                 nparms;     // number of macro parameters
    char                name[1];    // macro name, storage = 1 + length
} *macroTab = NULL;             // pointer to first entry in macro table
typedef struct MacroRec MacroRec;

struct SegRec
{
    struct SegRec       *next;      // pointer to next segment
//  bool                gen;        // false to supress code output (not currently implemented)
    uint32_t            loc;        // locptr for this segment
    uint32_t            cod;        // codptr for this segment
    char                name[1];    // segment name, storage = 1 + length
} *segTab = NULL;               // pointer to first entry in macro table
typedef struct SegRec SegRec;

int             macroCondLevel;     // current IF nesting level inside a macro definition
int             macUniqueID;        // unique ID, incremented per macro invocation
int             macLevel;           // current macro nesting level
int             macCurrentID[MAX_MACRO]; // current unique ID
MacroRec        *macPtr[MAX_MACRO]; // current macro in use
MacroLine       *macLine[MAX_MACRO];// current macro text pointer
int             numMacParms[MAX_MACRO];  // number of macro parameters
Str255          macParmsLine[MAX_MACRO]; // text of current macro parameters
char            *macParms[MAXMACPARMS * MAX_MACRO]; // pointers to current macro parameters
#ifdef ENABLE_REP
int             macRepeat[MAX_MACRO]; // repeat count for REP pseudo-op
#endif

struct AsmRec
{
    struct AsmRec   *next;          // next AsmRec
    int             (*DoCPUOpcode) (int typ, int parm);
    int             (*DoCPULabelOp) (int typ, int parm, char *labl);
    void            (*PassInit) (void);
    char            name[1];        // name of this assembler
};
typedef struct AsmRec AsmRec;

struct CpuRec
{
    struct CpuRec   *next;          // next CpuRec
    AsmRec          *as;            // assembler for CPU type
    int             index;          // CPU type index for assembler
    int             endian;         // endianness for this CPU
    int             addrWid;        // address bus width, ADDR_16 or ADDR_32
    int             listWid;        // listing hex area width, LIST_16 or LIST_24
    int             wordSize;       // addressing word size in bits
    const OpcdRec   *opcdTab;       // opcdTab[] for this assembler
    int             opts;           // option flags
    char            name[1];        // all-uppercase name of CPU
};
typedef struct CpuRec CpuRec;

// --------------------------------------------------------------

SegRec         *curSeg;             // current segment
SegRec         *nullSeg;            // default null segment

uint32_t        locPtr;             // Current program address
uint32_t        codPtr;             // Current program "real" address
int             pass;               // Current assembler pass
bool            warnFlag;           // true if warning occurred this line
bool            errFlag;            // true if error occurred this line
int             errCount;           // Total number of errors

Str255          line;               // Current line from input file
char           *linePtr;            // pointer into current line
Str255          listLine;           // Current listing line
bool            listLineFF;         // true if an FF was in the current listing line
bool            listFlag;           // false to suppress listing source
bool            listThisLine;       // true to force listing this line
bool            sourceEnd;          // true when END pseudo encountered
Str255          lastLabl;           // last label for '@' temp labels
Str255          subrLabl;           // current SUBROUTINE label for '.' temp labels
bool            listMacFlag;        // false to suppress showing macro expansions
bool            macLineFlag;        // true if line came from a macro
int             linenum;            // line number in main source file
bool            expandHexFlag;      // true to expand long hex data to multiple listing lines
bool            symtabFlag;         // true to show symbol table in listing
bool            tempSymFlag;        // true to show temp symbols in symbol table listing
bool            exactFlag;          // true to disable assembler-specific optimizations

int             condLevel;          // current IF nesting level
char            condState[MAX_COND];// state of current nesting level
enum
{
    condELSE = 1, // ELSE has already been countered at this level
    condTRUE = 2, // condition is currently true
    condFAIL = 4  // condition has failed (to handle ELSE after ELSIF)
};

int             instrLen;           // Current instruction length (negative to display as long DB)
uint8_t         bytStr[MAX_BYTSTR]; // Current instruction / buffer for long DB statements
int             hexSpaces;          // flags for spaces in hex output for instructions
bool            showAddr;           // true to show LocPtr on listing
uint32_t        xferAddr;           // Transfer address from END pseudo
bool            xferFound;          // true if xfer addr defined w/ END

//  Command line parameters
Str255          cl_SrcName;         // Source file name
Str255          cl_ListName;        // Listing file name
Str255          cl_ObjName;         // Object file name
bool            cl_Err;             // true for errors to screen
bool            cl_Warn;            // true for warnings to screen
bool            cl_List;            // true to generate listing file
bool            cl_Obj;             // true to generate object file
uint8_t         cl_ObjType;         // type of object file to generate:
enum { OBJ_HEX, OBJ_S9, OBJ_BIN, OBJ_TRSDOS, OBJ_TRSCAS };  // values for cl_Obj
uint32_t        cl_Binbase;         // base address for OBJ_BIN
uint32_t        cl_Binend;          // end address for OBJ_BIN
int             cl_S9type;          // type of S9 file: 9, 19, 28, or 37
uint16_t        cl_trslen;          // TRSDOS binary block size (default 256)
bool            cl_Stdout;          // true to send object file to stdout
bool            cl_ListP1;          // true to show listing in first assembler pass
bool            cl_edtasm;          // true to show "classic EDTASM" pass/errors messages

FILE            *source;            // source input file
FILE            *object;            // object output file
FILE            *listing;           // listing output file
FILE            *incbin;            // binary include file
FILE            *(include[MAX_INCLUDE]);    // include files
Str255          incname[MAX_INCLUDE];       // include file names
int             incline[MAX_INCLUDE];       // include line number
int             nInclude;           // current include file index

bool            evalKnown;          // true if all operands in Eval were "known"

AsmRec          *asmTab;            // list of all assemblers
CpuRec          *cpuTab;            // list of all CPU types
AsmRec          *curAsm;            // current assembler
int             curCPU;             // current CPU index for current assembler

int             endian;             // CPU endian: UNKNOWN_END, LITTLE_END, BIG_END
int             addrWid;            // CPU address width: ADDR_16, ADDR_32
int             listWid;            // listing hex area width: LIST_16, LIST_24
int             opts;               // current CPU's option flags
int             wordSize;           // current CPU's addressing size in bits
int             wordDiv;            // scaling factor for current word size
int             addrMax;            // maximum addrWid used
const OpcdRec   *opcdTab;           // current CPU's opcode table
Str255          defCPU;             // default CPU name

// --------------------------------------------------------------

enum
{
//  0x00-0xFF = CPU-specific opcodes

//  OP_Illegal = 0x100,  // opcode not found in FindOpcode

    OP_DB = OP_Illegal + 1,       // DB pseudo-op
    OP_DW,       // DW pseudo-op
    OP_DL,       // DL pseudo-op
    OP_DWRE,     // reverse-endian DW
    OP_DS,       // DS pseudo-op
    OP_HEX,      // HEX pseudo-op
    OP_FCC,      // FCC pseudo-op
    OP_ZSCII,    // ZSCII pseudo-op
    OP_ASCIIC,   // counted DB pseudo-op
    OP_ASCIIZ,   // null-terminated DB pseudo-op
    OP_ALIGN,    // ALIGN pseudo-op
    OP_ALIGN_n,  // for EVEN pseudo-op

    OP_END,      // END pseudo-op
    OP_Include,  // INCLUDE pseudo-op

    OP_ENDM,     // ENDM pseudo-op
#ifdef ENABLE_REP
    o_REPEND,   // REPEND pseudo-op
#endif
    OP_MacName,  // Macro name
    OP_Processor,// CPU selection pseudo-op

    // the following pseudo-ops handle the label field specially

//  OP_LabelOp = 0x1000,   // flag to handle opcode in ASMX_DoLabelOp
//  0x1000-0x10FF = CPU-specific label-ops

//  o_EQU = o_LabelOp + 0x100,      // EQU and SET pseudo-ops
    OP_ORG = OP_EQU + 1,      // ORG pseudo-op
    OP_RORG,     // RORG pseudo-op
    OP_REND,     // REND pseudo-op
    OP_LIST,     // LIST pseudo-op
    OP_OPT,      // OPT pseudo-op
    OP_ERROR,    // ERROR pseudo-op
    OP_ASSERT,   // ASSERT pseudo-op
    OP_MACRO,    // MACRO pseudo-op
#ifdef ENABLE_REP
    o_REPEAT,   // REPEAT pseudo-op
#endif
    OP_Incbin,   // INCBIN pseudo-op
    OP_WORDSIZE, // WORDSIZE pseudo-op

    OP_SEG,      // SEG pseudo-op
    OP_SUBR,     // SUBROUTINE pseudo-op

    OP_IF,       // IF <expr> pseudo-op
    OP_ELSE,     // ELSE pseudo-op
    OP_ELSIF,    // ELSIF <expr> pseudo-op
    o_ENDIF     // ENDIF pseudo-op
};

static const struct OpcdRec opcdTab2[] =
{
    {"DB",        OP_DB,       0},
    {"FCB",       OP_DB,       0},
    {"BYTE",      OP_DB,       0},
    {"DC.B",      OP_DB,       0},
    {"DFB",       OP_DB,       0},
    {"DEFB",      OP_DB,       0},
    {"DEFM",      OP_DB,       0},

    {"DW",        OP_DW,       0},
    {"FDB",       OP_DW,       0},
    {"WORD",      OP_DW,       0},
    {"DC.W",      OP_DW,       0},
    {"DFW",       OP_DW,       0},
    {"DA",        OP_DW,       0},
    {"DEFW",      OP_DW,       0},
    {"DRW",       OP_DWRE,     0},

    {"LONG",      OP_DL,       0},
    {"DL",        OP_DL,       0},
    {"DC.L",      OP_DL,       0},

    {"DS",        OP_DS,       1},
    {"DS.B",      OP_DS,       1},
    {"RMB",       OP_DS,       1},
    {"BLKB",      OP_DS,       1},
    {"DEFS",      OP_DS,       1},
    {"DS.W",      OP_DS,       2},
    {"BLKW",      OP_DS,       2},
    {"DS.L",      OP_DS,       4},
    {"HEX",       OP_HEX,      0},
    {"FCC",       OP_FCC,      0},
    {"ZSCII",     OP_ZSCII,    0},
    {"ASCIIC",    OP_ASCIIC,   0},
    {"ASCIZ",     OP_ASCIIZ,   0},
    {"ASCIIZ",    OP_ASCIIZ,   0},
    {"END",       OP_END,      0},
    {"ENDM",      OP_ENDM,     0},
    {"ALIGN",     OP_ALIGN,    0},
    {"EVEN",      OP_ALIGN_n,  2},
#ifdef ENABLE_REP
    {"REPEND",    o_REPEND,   0},
#endif
    {"INCLUDE",   OP_Include,  0},
    {"INCBIN",    OP_Incbin,   0},
    {"PROCESSOR", OP_Processor,0},
    {"CPU",       OP_Processor,0},

    {"=",         OP_EQU,      0},
    {"EQU",       OP_EQU,      0},
    {":=",        OP_EQU,      1},
    {"SET",       OP_EQU,      1},
    {"DEFL",      OP_EQU,      1},
    {"ORG",       OP_ORG,      0},
    {"AORG",      OP_ORG,      0},
    {"RORG",      OP_RORG,     0},
    {"REND",      OP_REND,     0},
    {"LIST",      OP_LIST,     0},
    {"OPT",       OP_OPT,      0},
    {"ERROR",     OP_ERROR,    0},
    {"ASSERT",    OP_ASSERT,   0},
#ifdef ENABLE_REP
    {"REPEAT",    o_REPEAT,   0},
#endif
    {"MACRO",     OP_MACRO,    0},
    {"SEG",       OP_SEG,      1},
    {"RSEG",      OP_SEG,      1},
    {"SEG.U",     OP_SEG,      0},
    {"SUBR",      OP_SUBR,     0},
    {"SUBROUTINE",OP_SUBR,     0},
    {"IF",        OP_IF,       0},
    {"ELSE",      OP_ELSE,     0},
    {"ELSIF",     OP_ELSIF,    0},
    {"ENDIF",     o_ENDIF,    0},
    {"WORDSIZE",  OP_WORDSIZE, 0},

    {"",          OP_Illegal,  0}
};


// --------------------------------------------------------------

// multi-assembler call vectors

static int DoCPUOpcode(int typ, int parm)
{
    if (curAsm && curAsm -> DoCPUOpcode)
    {
        return curAsm -> DoCPUOpcode(typ, parm);
    }
    else
    {
        return 0;
    }
}


static int DoCPULabelOp(int typ, int parm, char *labl)
{
    if (curAsm && curAsm -> DoCPULabelOp)
    {
        return curAsm -> DoCPULabelOp(typ, parm, labl);
    }
    else
    {
        return 0;
    }
}


static void PassInit(void)
{
    AsmRec *p = asmTab;

    // for each assembler, call PassInit
    while (p)
    {
        if (p -> PassInit)
        {
            p -> PassInit();
        }
        p = p -> next;
    }
}


void *ASMX_AddAsm(const char *name, // assembler name
             int (*DoCPUOpcode) (int typ, int parm),
             int (*DoCPULabelOp) (int typ, int parm, char *labl),
             void (*PassInit) (void) )
{
    AsmRec *p = (AsmRec *) malloc(sizeof *p + strlen(name));

    strcpy(p -> name, name);
    p -> next           = asmTab;
    p -> DoCPUOpcode    = DoCPUOpcode;
    p -> DoCPULabelOp   = DoCPULabelOp;
    p -> PassInit       = PassInit;

    asmTab = p;

    return p;
}

void ASMX_AddCPU(void *as,           // assembler for this CPU
            const char *name,   // uppercase name of this CPU
            int index,          // index number for this CPU
            int endian,         // assembler endian
            int addrWid,        // assembler 32-bit
            int listWid,        // listing width
            int wordSize,       // addressing word size in bits
            int opts,           // option flags
            const struct OpcdRec opcdTab[])  // assembler opcode table
{
    CpuRec *p = (CpuRec *) malloc(sizeof *p + strlen(name));

    strcpy(p -> name, name);
    p -> next  = cpuTab;
    p -> as    = (AsmRec *) as;
    p -> index = index;
    p -> endian   = endian;
    p -> addrWid  = addrWid;
    p -> listWid  = listWid;
    p -> wordSize = wordSize;
    p -> opts     = opts;
    p -> opcdTab  = opcdTab;

    cpuTab = p;
}


static CpuRec *FindCPU(const char *cpuName)
{
    CpuRec *p = cpuTab;

    while (p)
    {
        if (strcmp(cpuName, p->name) == 0)
        {
            return p;
        }
        p = p -> next;
    }

    return NULL;
}


static void SetWordSize(int wordSize)
{
    wordDiv = (wordSize + 7) / 8;
}


void OBJF_CodeFlush(void); // forward declaration
// sets up curAsm and curCpu based on cpuName, returns non-zero if success
static bool SetCPU(const char *cpuName)
{
    CpuRec *p = FindCPU(cpuName);

    if (p)
    {
        curCPU   = p -> index;
        curAsm   = p -> as;
        endian   = p -> endian;
        addrWid  = p -> addrWid;
        listWid  = p -> listWid;
        wordSize = p -> wordSize;
        opcdTab  = p -> opcdTab;
        opts     = p -> opts;
        SetWordSize(wordSize);

        OBJF_CodeFlush();    // make a visual change in the hex object file

        return 1;
    }

    return 0;
}


int isalphanum(char c); // forward declaration
void Uprcase(char *s); // forward declaration


static void ASMX_AsmInit(void)
{
    // macro to call an assembler initialization
#define ASSEMBLER(name) extern void name##_AsmInit(void); name##_AsmInit();

    ASMX_AddCPU(ASMX_AddAsm("None", NULL, NULL, NULL),
           "NONE", 0, END_UNKNOWN, ADDR_32, LIST_24, 8, 0, NULL);

    ASSEMBLER(R1802);
    ASSEMBLER(M6502);
    ASSEMBLER(M68K);
    ASSEMBLER(M6805);
    ASSEMBLER(M6809);
    ASSEMBLER(M68XX);
    ASSEMBLER(M68HC16);
    ASSEMBLER(I8048);
    ASSEMBLER(I8051);
    ASSEMBLER(I8085);
    ASSEMBLER(I8008);
    ASSEMBLER(F8);
    ASSEMBLER(JAG);
    ASSEMBLER(Z8);
    ASSEMBLER(Z80);
    ASSEMBLER(THUMB);
    ASSEMBLER(ARM);

//  strcpy(defCPU, "Z80");      // hard-coded default for testing

    strcpy(line, progname);
    Uprcase(line);

    // try to find the CPU name in the executable's name
    char *p = line + strlen(line);    // start at end of executable name

    while (p > line && isalphanum(p[-1]))
    {
        p--;                    // skip back to last non alpha-numeric character
    }
    if (!isalphanum(*p)) p++;   // advance past last non alpha-numeric character

    if (p[0] == 'A' && p[1] == 'S' && p[2] == 'M')
    {
        p = p + 3;              // skip leading "ASM"
    }

    // for each substring, try to find a matching CPU name
    while (*p)
    {
        if (FindCPU(p))
        {
            strcpy(defCPU, p);
            return;
        }
        p++;
    }
}


// --------------------------------------------------------------
// error messages


/*
 *  Error
 */

void ASMX_Error(const char *message)
{
    errFlag = true;
    errCount++;

    char *name = cl_SrcName;
    int line = linenum;
    if (nInclude >= 0)
    {
        name = incname[nInclude];
        line = incline[nInclude];
    }

    if (pass == 2)
    {
        listThisLine = true;
        if (cl_List)    fprintf(listing, "%s:%d: *** Error:  %s ***\n", name, line, message);
        if (cl_Err)     fprintf(stderr,  "%s:%d: *** Error:  %s ***\n", name, line, message);
    }
}


/*
 *  ASMX_Warning
 */

void ASMX_Warning(const char *message)
{
    warnFlag = true;

    char *name = cl_SrcName;
    int line = linenum;
    if (nInclude >= 0)
    {
        name = incname[nInclude];
        line = incline[nInclude];
    }

    if (pass == 2 && cl_Warn)
    {
        listThisLine = true;
        if (cl_List)    fprintf(listing, "%s:%d: *** Warning:  %s ***\n", name, line, message);
        if (cl_Warn)    fprintf(stderr,  "%s:%d: *** Warning:  %s ***\n", name, line, message);
    }
}


// --------------------------------------------------------------
// string utilities


/*
 *  Debleft deblanks the string s from the left side
 */

void Debleft(char *s)
{
    char *p = s;

    while (*p == 9 || *p == ' ')
    {
        p++;
    }

    if (p != s)
    {
        while ((*s++ = *p++)) /* copy until end of string */;
    }
}


/*
 *  Debright deblanks the string s from the right side
 */

void Debright(char *s)
{
    char *p = s + strlen(s);

    while (p > s && *--p == ' ')
    {
        *p = 0;
    }
}


/*
 *  Deblank removes blanks from both ends of the string s
 */

void Deblank(char *s)
{
    Debleft(s);
    Debright(s);
}


/*
 *  Uprcase converts string s to upper case
 */

void Uprcase(char *s)
{
    char *p = s;

    while ((*p = toupper(*p)))
        p++;
}


int ishex(char c)
{
    c = toupper(c);
    return isdigit(c) || ('A' <= c && c <= 'F');
}


int isoctal(char c)
{
    return ('0' <= c && c <= '7');
}


int isalphaul(char c)
{
    c = toupper(c);
    return ('A' <= c && c <= 'Z') || c == '_';
}


int isalphanum(char c)
{
    c = toupper(c);
    return isdigit(c) || ('A' <= c && c <= 'Z') || c == '_';
}


static unsigned int EvalBin(const char *binStr)
{
    unsigned int binVal = 0;
    bool evalErr = false;

    char c;
    while ((c = *binStr++))
    {
        if (c < '0' || c > '1')
        {
            evalErr = true;
        }
        else
        {
            binVal = binVal * 2 + c - '0';
        }
    }

    if (evalErr)
    {
        binVal = 0;
        ASMX_Error("Invalid binary number");
    }

    return binVal;
}


static unsigned int EvalOct(const char *octStr)
{
    unsigned int octVal = 0;
    bool evalErr = false;

    char c;
    while ((c = *octStr++))
    {
        if (c < '0' || c > '7')
        {
            evalErr = true;
        }
        else
        {
            octVal = octVal * 8 + c - '0';
        }
    }

    if (evalErr)
    {
        octVal = 0;
        ASMX_Error("Invalid octal number");
    }

    return octVal;
}


static unsigned int EvalSplitOct(const char *octStr)
{
    unsigned int octVal = 0;
    int len = strlen(octStr);
    bool evalErr = len > 6;

    // xxxyyyA format was used by Heath to represent
    // octal as separate octal bytes xxx and yyy
    // as would be seen on an H8 front panel
    // 377377A represents 0xFFFF

    char c;
    int digit = 0;
    while ((c = *octStr++))
    {
        if (c < '0' || c > '7' || (c > '3' && (len - digit) % 3 == 0))
        {
            evalErr = true;
        }
        else
        {
            if (digit == len - 3)
            {
                octVal = octVal * 4 + c - '0';
            }
            else
            {
                octVal = octVal * 8 + c - '0';
            }
        }
        digit++;
    }

    if (evalErr)
    {
        octVal = 0;
        ASMX_Error("Invalid split-octal number");
    }

    return octVal;
}


static unsigned int EvalDec(const char *decStr)
{
    unsigned int decVal = 0;
    bool evalErr = false;

    char c;
    while ((c = *decStr++))
    {
        if (!isdigit(c))
        {
            evalErr = true;
        }
        else
        {
            decVal = decVal * 10 + c - '0';
        }
    }

    if (evalErr)
    {
        decVal = 0;
        ASMX_Error("Invalid decimal number");
    }

    return decVal;
}


static int Hex2Dec(int c)
{
    c = toupper(c);
    if (c > '9')
    {
        return c - 'A' + 10;
    }
    return c - '0';
}


static unsigned int EvalHex(const char *hexStr)
{
    unsigned int hexVal = 0;
    bool evalErr = false;

    char c;
    while ((c = *hexStr++))
    {
        if (!ishex(c))
        {
            evalErr = true;
        }
        else
        {
            hexVal = hexVal * 16 + Hex2Dec(c);
        }
    }

    if (evalErr)
    {
        hexVal = 0;
        ASMX_Error("Invalid hexadecimal number");
    }

    return hexVal;
}


static unsigned int EvalNum(char *word)
{
    // handle C-style 0xnnnn hexadecimal constants
    if(word[0] == '0')
    {
        if (toupper(word[1]) == 'X')
        {
            return EvalHex(word+2);
        }
        // return EvalOct(word);    // 0nnn octal constants are in less demand, though
    }

    unsigned int val = strlen(word) - 1;
    switch (word[val])
    {
        case 'A': // nnnnnnA format used by Heath
            word[val] = 0;
            val = EvalSplitOct(word);
            break;

        case 'B':
            word[val] = 0;
            val = EvalBin(word);
            break;

        case 'O':
        case 'Q':
            word[val] = 0;
            val = EvalOct(word);
            break;

        case 'D':
            word[val] = 0;
            val = EvalDec(word);
            break;

        case 'H':
            word[val] = 0;
            val = EvalHex(word);
            break;

        default:
            val = EvalDec(word);
            break;
    }

    return val;
}


// --------------------------------------------------------------
// listing hex output routines

char *LIST_Str(char *l, const char *s)
{
    // copy string to line
    while (*s) *l++ = *s++;

    return l;
}


char *LIST_Byte(char *p, uint8_t b)
{
    char s[16]; // with extra space for just in case

    sprintf(s, "%.2X", b);
    return LIST_Str(p, s);
}


char *LIST_Word(char *p, uint16_t w)
{
    char s[16]; // with extra space for just in case

    sprintf(s, "%.4X", w);
    return LIST_Str(p, s);
}


char *ListLong24(char *p, uint32_t l)
{
    char s[16]; // with extra space for just in case

    sprintf(s, "%.6X", l & 0xFFFFFF);
    return LIST_Str(p, s);
}


char *LIST_Long(char *p, uint32_t l)
{
    char s[16]; // with extra space for just in case

    sprintf(s, "%.8X", l);
    return LIST_Str(p, s);
}


char *LIST_Addr(char *p, uint32_t addr)
{
    switch (addrWid)
    {
        default:
        case ADDR_16:
            p = LIST_Word(p, addr);
            break;

        case ADDR_24:
            p = ListLong24(p, addr);
            break;

        case ADDR_32: // you need listWid == LIST_24 with this too
            p = LIST_Long(p, addr);
            break;
    };

    return p;
}

char *LIST_Loc(uint32_t addr)
{
    char *p = LIST_Addr(listLine, addr);

    *p++ = ' ';
    if (listWid == LIST_24 && addrWid == ADDR_16)
    {
        *p++ = ' ';
    }

    return p;
}

// --------------------------------------------------------------
// ZSCII conversion routines

uint8_t zStr[MAX_BYTSTR];   // output data buffer
int     zLen;               // length of output data
int     zOfs, zPos;         // current output offset (in bytes) and bit position
int     zShift;             // current shift lock status (0, 1, 2)
const char zSpecial[] = "0123456789.,!?_#'\"/\\<-:()"; // special chars table


static void InitZSCII(void)
{
    zOfs = 0;
    zPos = 0;
    zLen = 0;
    zShift = 0;
}


static void PutZSCII(char nib)
{
    nib = nib & 0x1F;

    // is it time to start a new word?
    if (zPos == 3)
    {
        // check for overflow
        if (zOfs >= MAX_BYTSTR)
        {
            if (!errFlag) ASMX_Error("ZSCII string length overflow");
            return;
        }
        zOfs = zOfs + 2;
        zPos = 0;
    }

    switch (zPos)
    {
        case 0:
            zStr[zOfs] = nib << 2;
            break;

        case 1:
            zStr[zOfs] |= nib >> 3;
            zStr[zOfs+1] = nib << 5;
            break;

        case 2:
            zStr[zOfs+1] |= nib;
            break;

        default:    // this shouldn't happen!
            break;
    }

    zPos++;
}


static void PutZSCIIShift(char shift, char nshift)
{
    int lock = 0;                   // only do a temp shift if next shift is different

    if (shift == nshift) lock = 2;  // generate shift lock if next shift is same

    switch ((shift - zShift + 3) % 3)
    {
        case 0: // no shift
            break;

        case 1: // shift forward
            PutZSCII(0x02 + lock);
            break;

        case 2: // shift backwards
            PutZSCII(0x03 + lock);
            break;
    }

    if (lock) zShift = shift;       // update shift lock state
}


static void EndZSCII(void)
{
    // pad final word with shift nibbles
    while (zPos != 3)
    {
        PutZSCII(0x05);
    }

    // set high bit in final word as end-of-text marker
    zStr[zOfs] |= 0x80;

    zOfs = zOfs + 2;
}


static int GetZSCIIShift(char ch)
{
    if (ch == ' ')  return -2;
    if (ch == '\n') return -1;
    if ('a' <= ch && ch <= 'z') return 0;
    if ('A' <= ch && ch <= 'Z') return 1;
    return 2;
}


static void ConvertZSCII(void)
{
    //  input data is in bytStr[]
    //  input data length is instrLen

    char    *p;                         // pointer for looking up special chars

    InitZSCII();

    int inpos = 0;                      // input position
    while (inpos < instrLen)
    {
        // get current char and shift
        char ch = bytStr[inpos];        // current input byte
        int shift = GetZSCIIShift(ch);  // current shift state
        int nshift = shift;             // next shift state

        // determine next char's shift (skipping blanks and newlines, stopping at end of data)
        int i = inpos + 1;
        while (i < instrLen && (nshift = GetZSCIIShift(bytStr[i])) < 0) i++;
        if (i >= instrLen) nshift = zShift;    // if end of data, use current shift as "next" shift

        switch (shift)
        {
            case 0: // alpha lower case
            case 1: // alpha upper case
                PutZSCIIShift(shift, nshift);
                PutZSCII(ch - 'A' + 6);
                break;

            case 2: // non-alpha
                if ((p = strchr(zSpecial, ch)))
                {
                    // numeric and special chars
                    PutZSCIIShift(shift, nshift);
                    PutZSCII(p - zSpecial + 7);
                }
                else
                {
                    // extended char
                    PutZSCIIShift(shift, nshift);
                    PutZSCII(0x06);
                    PutZSCII(ch >> 5);
                    PutZSCII(ch);
                }
                break;

            default: // blank and newline
                PutZSCII(shift + 2);
                break;
        }

        inpos++;
    }

    EndZSCII();

    memcpy(bytStr, zStr, zOfs);
    instrLen = zOfs;
}


// --------------------------------------------------------------
// token handling

// returns 0 for end-of-line, -1 for alpha-numeric, else char value for non-alphanumeric
// converts the word to uppercase, too
int TOKEN_GetWord(char *word)
{
    word[0] = 0;

    // skip initial whitespace
    char c = *linePtr;
    while (c == 12 || c == '\t' || c == ' ')
    {
        c = *++linePtr;
    }

    // skip comments
    if (c == ';')
    {
        while (c)
        {
            c = *++linePtr;
        }
    }

    // test for end of line
    if (c)
    {
        // test for alphanumeric token
#if 1
        if (isalphanum(c) ||
                (
                    (((opts & OPT_DOLLARSYM) && c == '$') || ((opts & OPT_ATSYM) && c == '@'))
                    && ((isalphanum(linePtr[1]) ||
                         linePtr[1]=='$' ||
                         ((opts & OPT_ATSYM) && linePtr[1]=='@'))
                       )
                ))

#else

#ifdef DOLLAR_SYM
        if (isalphanum(c) || (c == '$' && (isalphanum(linePtr[1]) || linePtr[1]=='$')))
#else
        if (isalphanum(c))
#endif

#endif
        {
            while (isalphanum(c) || c == '$' || ((opts & OPT_ATSYM) && c == '@'))
            {
                *word++ = toupper(c);
                c = *++linePtr;
            }
            *word = 0;
            return -1;
        }
        else
        {
            word[0] = c;
            word[1] = 0;
            linePtr++;
            return c;
        }
    }

    return 0;
}


// same as GetWord, except it allows '.' chars in alphanumerics and ":=" as a token
int GetOpcode(char *word)
{
    word[0] = 0;

    // skip initial whitespace
    char c = *linePtr;
    while (c == 12 || c == '\t' || c == ' ')
    {
        c = *++linePtr;
    }

    // skip comments
    if (c == ';')
    {
        while (c)
        {
            c = *++linePtr;
        }
    }

    // test for ":="
    if (c == ':' && linePtr[1] == '=')
    {
        word[0] = ':';
        word[1] = '=';
        word[2] = 0;
        linePtr = linePtr + 2;
        return -1;
    }
    else if (c)
    {
        // test for end of line
        // test for alphanumeric token
        if (isalphanum(c) || c=='.')
        {
            while (isalphanum(c) || c=='.')
            {
                *word++ = toupper(c);
                c = *++linePtr;
            }
            *word = 0;
            return -1;
        }
        else
        {
            word[0] = c;
            word[1] = 0;
            linePtr++;
            return c;
        }
    }

    return 0;
}


void GetFName(char *word)
{
    // skip leading whitespace
    while (*linePtr == ' ' || *linePtr == '\t')
    {
        linePtr++;
    }
    char *oldLine = word;

    // check for quote at start of file name
    char quote = 0;
    if (*linePtr == '"' || *linePtr == '\'')
    {
        quote = *linePtr++;
    }

    // continue reading until quote or whitespace or EOL
    while (*linePtr != 0 && *linePtr != quote &&
            (quote || (*linePtr != ' ' && *linePtr != '\t')))
    {
        char ch = *linePtr++;
        if (ch == '\\' && *linePtr != 0)
        {
            ch = *linePtr++;
        }
        *oldLine++ = ch;
    }
    *oldLine++ = 0;

    // if looking for quote, error on end quote
    if (quote)
    {
        if (*linePtr == quote)
        {
            linePtr++;
        }
        else
        {
            ASMX_Error("Missing close quote");
        }
    }
}


bool TOKEN_Expect(const char *expected)
{
    Str255 s;
    TOKEN_GetWord(s);
    if (strcmp(s, expected) != 0)
    {
        snprintf(s, sizeof s, "\"%s\" expected", expected);
        ASMX_Error(s);
        return 1;
    }
    return 0;
}


bool TOKEN_Comma()
{
    return TOKEN_Expect(",");
}


bool TOKEN_RParen()
{
    return TOKEN_Expect(")");
}


void TOKEN_EatIt()
{
    Str255  word;
    while (TOKEN_GetWord(word));      // eat junk at end of line
}


/*
 *  ASMX_IllegalOperand
 */

void ASMX_IllegalOperand()
{
    ASMX_Error("Illegal operand");
    TOKEN_EatIt();
}


/*
 *  ASMX_MissingOperand
 */

void ASMX_MissingOperand()
{
    ASMX_Error("Missing operand");
    TOKEN_EatIt();
}


/*
 *  TOKEN_BadMode
 */
void TOKEN_BadMode()
{
    ASMX_Error("Illegal addressing mode");
    TOKEN_EatIt();
}


// find a register name
// regList is a space-separated list of register names
// FindReg returns:
//       -2 (aka reg_EOL) if empty string
//      -1 (aka reg_None) if no register found
//      0 if regName is the first register in regList
//      1 if regName is the second register in regList
//      etc.
int REG_Find(const char *regName, const char *regList)
{
    if (!regName[0]) return reg_EOL;

    int i = 0;
    while (*regList)
    {
        const char *p = regName;
        // compare words
        while (*p && *p == *regList)
        {
            regList++;
            p++;
        }

        // if not match, skip rest of word
        if (*p || (*regList != 0 && *regList != ' '))
        {
            // skip to next whitespace
            while (*regList && *regList != ' ')
            {
                regList++;
            }
            // skip to next word
            while (*regList == ' ')
            {
                regList++;
            }
            i++;
        }
        else
        {
            return i;
        }
    }

    return reg_None;
}


// get a word and find a register name
// regList is a space-separated list of register names
// REG_Get returns:
//      -2 (aka reg_EOL) and reports a "missing operand" error if end of line
//      -1 (aka reg_None) if no register found
//      0 if regName is the first register in regList
//      1 if regName is the second register in regList
//      etc.
int REG_Get(const char *regList)
{
    Str255  word;

    char *oldLine = linePtr;
    if (!TOKEN_GetWord(word))
    {
        ASMX_MissingOperand();
        return reg_EOL;
    }

    int reg = REG_Find(word, regList);
    if (reg < 0)
    {
        linePtr = oldLine;
    }
    return reg;
}


// check if a register from FindReg/REG_Get is valid
int REG_Check(int reg) // may want to add a maxreg parameter
{
    if (reg == reg_EOL)
    {
        ASMX_MissingOperand();      // abort if not valid register
        return 1;
    }
//  if ((reg < 0) || (reg > maxReg))
    if (reg < 0)
    {
        ASMX_IllegalOperand();      // abort if not valid register
        return 1;
    }
    return 0;
}


static int GetBackslashChar(void)
{
    Str255      s;

    if (!*linePtr)
    {
        return -1;
    }

    unsigned char ch = *linePtr++;
    if (ch == '\\' && *linePtr != 0)   // backslash
    {
        ch = *linePtr++;
        switch (ch)
        {
            case 'r':
                ch = '\r';
                break;
            case 'n':
                ch = '\n';
                break;
            case 't':
                ch = '\t';
                break;
            case 'x':
                if (ishex(linePtr[0]) && ishex(linePtr[1]))
                {
                    s[0] = linePtr[0];
                    s[1] = linePtr[1];
                    s[2] = 0;
                    linePtr = linePtr + 2;
                    ch = EvalHex(s);
                }
                break;
            default:
                break;
        }
    }

    return ch;
}


// --------------------------------------------------------------
// macro handling


static MacroRec *FindMacro(const char *name)
{
    MacroRec *p = macroTab;
    bool found = false;

    while (p && !found)
    {
        found = (strcmp(p -> name, name) == 0);
        if (!found)
        {
            p = p -> next;
        }
    }

    return p;
}


static MacroRec *NewMacro(const char *name)
{
    MacroRec *p = (MacroRec *) malloc(sizeof *p + strlen(name));

    if (p)
    {
        strcpy(p -> name, name);
        p -> def     = false;
        p -> toomany = false;
        p -> text    = NULL;
        p -> next    = macroTab;
        p -> parms   = NULL;
        p -> nparms  = 0;
    }

    return p;
}


static MacroRec *AddMacro(const char *name)
{
    MacroRec *p = NewMacro(name);

    if (p)
    {
        macroTab = p;
    }

    return p;
}


static void AddMacroParm(MacroRec *macro, const char *name)
{
    MacroParm *parm = (MacroParm *) malloc(sizeof *parm + strlen(name));

    parm -> next = NULL;
    strcpy(parm -> name, name);
    macro -> nparms++;

    MacroParm *p = macro -> parms;
    if (p)
    {
        while (p -> next)
        {
            p = p -> next;
        }
        p -> next = parm;
    }
    else
    {
        macro -> parms = parm;
    }
}


static void AddMacroLine(MacroRec *macro, const char *line)
{
    MacroLine *m = (MacroLine *) malloc(sizeof *m + strlen(line));

    if (m)
    {
        m -> next = NULL;
        strcpy(m -> text, line);

        MacroLine *p = macro -> text;
        if (p)
        {
            while (p -> next)
            {
                p = p -> next;
            }
            p -> next = m;
        }
        else
        {
            macro -> text = m;
        }
    }
}


static void GetMacParms(MacroRec *macro)
{
    macCurrentID[macLevel] = macUniqueID++;

    for (int i = 0; i < MAXMACPARMS; i++)
    {
        macParms[i + macLevel * MAXMACPARMS] = NULL;
    }

    // skip initial whitespace
    char c = *linePtr;
    while (c == 12 || c == '\t' || c == ' ')
    {
        c = *++linePtr;
    }

    // copy rest of line for safekeeping
    strcpy(macParmsLine[macLevel], linePtr);

    int n = 0;
    char *p = macParmsLine[macLevel];
    while (*p && *p != ';' && n < MAXMACPARMS)
    {
        // skip whitespace before current parameter
        c = *p;
        while (c == 12 || c == '\t' || c == ' ')
        {
            c = *++p;
        }

        // record start of parameter
        macParms[n + macLevel * MAXMACPARMS] = p;
        n++;

        char quote = 0;
        bool done = false;
        // skip to end of parameter
        while (!done)
        {
            c = *p++;
            switch (c)
            {
                case 0:
                    --p;
                    done = true;
                    break;

                case ';':
                    if (quote == 0)
                    {
                        *--p = 0;
                        done = true;
                    }
                    break;

                case ',':
                    if (quote == 0)
                    {
                        *(p-1) = 0;
                        done = true;
                    }
                    break;

                case 0x27:  // quote
                case '"':
                    if (quote == 0)
                    {
                        quote = c;
                    }
                    else if (quote == c)
                    {
                        quote = 0;
                    }
            }
        }
    }

    numMacParms[macLevel] = n;

    // terminate last parameter and point remaining parameters to null strings
    *p = 0;
    for (int i = n; i < MAXMACPARMS; i++)
    {
        macParms[i + macLevel * MAXMACPARMS] = p;
    }

    // remove whitespace from end of parameter
    for (int i = 0; i < MAXMACPARMS; i++)
    {
        if (macParms[i + macLevel * MAXMACPARMS])
        {
            p = macParms[i + macLevel * MAXMACPARMS] + strlen(macParms[i + macLevel * MAXMACPARMS]) - 1;
            while (p >= macParms[i + macLevel * MAXMACPARMS] && (*p == ' ' || *p == 9))
            {
                *p-- = 0;
            }
        }
    }

    if (n > macro -> nparms || n > MAXMACPARMS)
    {
        ASMX_Error("Too many macro parameters");
    }
}


static void DoMacParms()
{
    Str255      word, word2;

    // start at beginning of line
    linePtr = line;

    // skip initial whitespace
    char c = *linePtr;
    while (c == 12 || c == '\t' || c == ' ')
    {
        c = *++linePtr;
    }

    // while not end of line
    char *p = linePtr; // pointer to start of word
    int token = TOKEN_GetWord(word);
    while (token)
    {
        // if alphanumeric, search for macro parameter of the same name
        if (token == -1)
        {
            int i = 0;
            MacroParm *parm = macPtr[macLevel] -> parms;
            while (parm && strcmp(parm -> name, word))
            {
                parm = parm -> next;
                i++;
            }

            // if macro parameter found, replace parameter name with parameter value
            if (parm)
            {
                // copy from linePtr to temp string
                strcpy(word, linePtr);
                // copy from corresponding parameter to p
                strcpy(p, macParms[i + macLevel * MAXMACPARMS]);
                // point p to end of appended text
                p = p + strlen(macParms[i + macLevel * MAXMACPARMS]);
                // copy from temp to p
                strcpy(p, word);
                // update linePtr
                linePtr = p;
            }
        }
        else if (token == '#' && *linePtr == '#')
        {
            // handle '##' concatenation operator
            p = linePtr + 1;    // skip second '#'
            linePtr--;          // skip first '#'
            // skip whitespace to the left
            while (linePtr > line && linePtr[-1] == ' ')
            {
                linePtr--;
            }
            // skip whitespace to the right
            while (*p == ' ') p++;
            // copy right side of chopped zone
            strcpy(word, p);
            // paste it at new linePtr
            strcpy(linePtr, word);
            // and linePtr now even points to where it should
        }
        else if (token == '\\' && *linePtr == '0')
        {
            // handle '\0' number of parameters operator
            p = linePtr + 1;    // skip '0'
            linePtr--;          // skip '\'
            // make string of number of parameters
            sprintf(word2, "%d", numMacParms[macLevel]);
            // copy right side of chopped zone
            strcpy(word, p);
            // paste number
            strcpy(linePtr, word2);
            linePtr = linePtr + strlen(word2);
            // paste right side at new linePtr
            strcpy(linePtr, word);
        }
        else if (token == '\\' && '1' <= *linePtr && *linePtr <= '9')
        {
            // handle '\n' parameter operator
            int i = *linePtr - '1';
            p = linePtr + 1;    // skip 'n'
            linePtr--;          // skip '\'
            // copy right side of chopped zone
            strcpy(word, p);
            // paste parameter
            strcpy(linePtr, macParms[i + macLevel * MAXMACPARMS]);
            linePtr = linePtr + strlen(macParms[i + macLevel * MAXMACPARMS]);
            // paste right side at new linePtr
            strcpy(linePtr, word);
        }
        else if (token == '\\' && *linePtr == '?')
        {
            // handle '\?' unique ID operator
            p = linePtr + 1;    // skip '?'
            linePtr--;          // skip '\'
            // make string of number of parameters
            sprintf(word2, "%.5d", macCurrentID[macLevel]);
            // copy right side of chopped zone
            strcpy(word, p);
            // paste number
            strcpy(linePtr, word2);
            linePtr = linePtr + strlen(word2);
            // paste right side at new linePtr
            strcpy(linePtr, word);
        }
        /* just use "\##" instead to avoid any confusion with \\ inside of DB pseudo-op
                // handle '\\' escape
                else if (token == '\\' && *linePtr == '\\') {
                    p = linePtr + 1;    // skip second '\'
                    // copy right side of chopped zone
                    strcpy(word, p);
                    // paste right side at new linePtr
                    strcpy(linePtr, word);
                }
        */

        // skip initial whitespace
        c = *linePtr;
        while (c == 12 || c == '\t' || c == ' ')
        {
            c = *++linePtr;
        }

        p = linePtr;
        token = TOKEN_GetWord(word);
    }
}


static void DumpMacro(MacroRec *p)
{
    if (cl_List)
    {
        fprintf(listing, "--- Macro '%s' ---", p -> name);
        fprintf(listing, " def = %d, nparms = %d\n", p -> def, p -> nparms);

        // dump parms here
        fprintf(listing, "Parms:");
        for (MacroParm *parm = p->parms; parm; parm = parm->next)
        {
            fprintf(listing, " '%s'", parm->name);
        }
        fprintf(listing, "\n");

        // dump text here
        for (MacroLine *line = p->text; line; line = line->next)
        {
            fprintf(listing, " '%s'\n", line->text);
        }
    }
}


/*static*/ void DumpMacroTab(void)
{
    MacroRec *p = macroTab;

    while (p)
    {
        DumpMacro(p);
        p = p -> next;
    }
}


// --------------------------------------------------------------
// opcodes handling


/*
 *  FindOpcodeTab - finds an entry in an opcode table
 */

// special compare for opcodes to allow "*" wildcard
static int opcode_strcmp(const char *s1, const char *s2)
{
    while (*s1 == *s2++)
    {
        if (*s1++ == 0) return 0;
    }
    if (*s1 == '*') return 0; // this is the magic
    return (*s1 - *(s2 - 1));
}


static const OpcdRec *FindOpcodeTab(const OpcdRec *p, const char *name, int *typ, int *parm)
{
    bool found = false;

//  while (p -> typ != o_Illegal && !found)
    while (*(p -> name) && !found)
    {
        found = (opcode_strcmp(p -> name, name) == 0);
        if (!found)
        {
            p++;
        }
        else
        {
            *typ  = p -> typ;
            *parm = p -> parm;
        }
    }

    if (!found) p = NULL; // because this is an array, not a linked list
    return p;
}


/*
 *  FindOpcode - finds an opcode in either the generic or CPU-specific
 *               opcode tables, or as a macro name
 */

static const OpcdRec *GetFindOpcode(char *opcode, int *typ, int *parm, MacroRec **macro)
{
    *typ   = OP_Illegal;
    *parm  = 0;
    *macro = NULL;

    const OpcdRec *p = NULL;
    if (GetOpcode(opcode))
    {
        if (opcdTab) p = FindOpcodeTab(opcdTab,  opcode, typ, parm);
        if (!p)
        {
            if (opcode[0] == '.') opcode++; // allow pseudo-ops to be invoked as ".OP"
            p = FindOpcodeTab((OpcdRec *) &opcdTab2, opcode, typ, parm);
        }
        if (p)
        {
            // if wildcard was matched, back up linePtr
            // NOTE: if wildcard matched an empty string, linePtr will be
            //       unchanged and point to whitespace
            int len = strlen(p->name);
            if (len && (p->name[len-1] == '*'))
            {
                linePtr = linePtr - (strlen(opcode) - len + 1);
            }
        }
        else
        {
            if ((*macro = FindMacro(opcode)))
            {
                *typ = OP_MacName;
                p = opcdTab2; // return dummy non-null valid opcode pointer
            }
        }
    }

    return p;
}


// --------------------------------------------------------------
// symbol table


struct SymRec
{
    struct SymRec   *next;      // pointer to next symtab entry
    uint32_t        value;      // symbol value
    bool            defined;    // true if defined
    bool            multiDef;   // true if multiply defined
    bool            isSet;      // true if defined with SET pseudo
    bool            equ;        // true if defined with EQU pseudo
    bool            known;      // true if value is known
    char            name[1];    // symbol name, storage = 1 + length
} *symTab = NULL;           // pointer to first entry in symbol table
typedef struct SymRec SymRec;


/*
 *  SYM_Find
 */

static SymRec *SYM_Find(const char *symName)
{
    SymRec *p = symTab;
    bool found = false;

    while (p && !found)
    {
        found = (strcmp(p -> name, symName) == 0);
        if (!found)
        {
            p = p -> next;
        }
    }

    return p;
}


/*
 *  SYM_Add
 */

static SymRec *SYM_Add(const char *symName)
{
    SymRec *p = (SymRec *) malloc(sizeof *p + strlen(symName));

    strcpy(p -> name, symName);
    p -> value    = 0;
    p -> next     = symTab;
    p -> defined  = false;
    p -> multiDef = false;
    p -> isSet    = false;
    p -> equ      = false;
    p -> known    = false;

    symTab = p;

    return p;
}


/*
 *  SYM_Ref
 */

static int SYM_Ref(const char *symName, bool *known)
{
    SymRec *p;
    Str255 s;

    if ((p = SYM_Find(symName)))
    {
        if (!p -> defined)
        {
            snprintf(s, sizeof s, "Symbol '%s' undefined", symName);
            ASMX_Error(s);
        }
        switch (pass)
        {
            case 1:
                if (!p -> defined) *known = false;
                break;
            case 2:
                if (!p -> known) *known = false;
                break;
        }
#if 0 // FIXME: possible fix that may be needed for 16-bit address
        if (addrWid == ADDR_16)
        {
            return (short) p -> value;    // sign-extend from 16 bits
        }
#endif
        return p -> value;
    }

    // check for 'FFH' style constants here
    int i = strlen(symName) - 1;
    if (toupper(symName[i]) != 'H')
    {
        i = -1;
    }
    else
    {
        while (i > 0 && ishex(symName[i-1]))
        {
            i--;
        }
    }

    if (i == 0)
    {
        strncpy(s, symName, 255);
        s[strlen(s)-1] = 0;
        return EvalHex(s);
    }
    else
    {
        p = SYM_Add(symName);
        *known = false;
//      snprintf(s, sizeof s, "Symbol '%s' undefined", symName);
//      Error(s);
    }

    return 0;
}


/*
 *  SYM_Def
 */

void SYM_Def(const char *symName, uint32_t val, bool setSym, bool equSym)
{
    Str255 s;

    if (symName[0])   // ignore null string symName
    {
        SymRec *p = SYM_Find(symName);
        if (p == NULL)
        {
            p = SYM_Add(symName);
        }

        if (!p -> defined || (p -> isSet && setSym))
        {
            // symbol has not been defined yet
            // or a SET symbol is being re-defined
            p -> value = val;
            p -> defined = true;
            p -> isSet = setSym;
            p -> equ = equSym;
        }
        else if (p -> value != val)
        {
            // trying to re-define a non-SET symbol
            p -> multiDef = true;
            if (pass == 2 && !p -> known)
            {
                sprintf(s, "Phase error");
            }
            else
            {
                snprintf(s, sizeof s, "Symbol '%s' multiply defined", symName);
            }
            ASMX_Error(s);
        }

        if (pass == 0 || pass == 2) p -> known = true;
    }
}


static void SYM_Dump(SymRec *p, char *s, int *w)
{
    int n = 0;
    int max = MAXSYMLEN;

    char *s2 = p->name;
    int len = strlen(s2);

    *w = 1;
    while (max-1 < len)
    {
        *w = *w + 1;
        max = max + MAXSYMLEN + 8; // 8 = number of extra chars between symbol names
    }

    while (*s2 && n < max)
    {
        *s++ = *s2++;
        n++;
    }

    while (n < max)
    {
        *s++ = ' ';
        n++;
    }

    switch (addrMax)
    {
        default:
        case ADDR_16:
            sprintf(s, "%.4X ", p->value & 0xFFFF);
            break;

        case ADDR_24:
#if 0
            sprintf(s, "%.6X ", p->value & 0xFFFFFF);
            break;
#endif
        case ADDR_32:
            sprintf(s, "%.8X ", p->value);
            break;
    }

    s = s + strlen(s);

    n = 0;
    if (!p -> defined)
    {
        *s++ = 'U';    // Undefined
        n++;
    }
    if ( p -> multiDef)
    {
        *s++ = 'M';    // Multiply defined
        n++;
    }
    if ( p -> isSet)
    {
        *s++ = 'S';    // Set
        n++;
    }
    if ( p -> equ)
    {
        *s++ = 'E';    // Equ
        n++;
    }
    while (n < 3)
    {
        *s++ = ' ';
        n++;
    }
    *s = 0;
}


static void SYM_DumpTab(void)
{
    Str255  s;

    int i = 0;
    SymRec *p = symTab;
    while (p)
    {
#ifdef TEMP_LBLAT
        if (tempSymFlag || !(strchr(p->name, '.') || strchr(p->name, '@')))
#else
        if (tempSymFlag || !strchr(p->name, '.'))
#endif
        {
            int w;
            SYM_Dump(p, s, &w);
            p = p -> next;

            // force a newline if new symbol won't fit on current line
            if (i + w > symTabCols)
            {
                if (cl_List)
                {
                    fprintf(listing, "\n");
                }
                i = 0;
            }
            if (p == NULL || i + w >= symTabCols)
            {
                // if last symbol or if symbol fills line, deblank and print it
                Debright(s);
                if (cl_List)
                {
                    fprintf(listing, "%s\n", s);
                }
                i = 0;
            }
            else
            {
                // otherwise just print it and count its width
                if (cl_List)
                {
                    fprintf(listing, "%s", s);
                }
                i = i + w;
            }
        }
        else
        {
            p = p -> next;
        }
    }
}


static void SYM_SortTab()
{
    SymRec *i, *j;      // pointers to current elements
    SymRec *ip, *jp;    // pointers to previous elements
    SymRec *t;          // temp for swapping

    // yes, it's a linked-list bubble sort

    if (symTab != NULL)
    {
        ip = NULL;
        i = symTab;
        jp = i;
        j = i -> next;

        while (j != NULL)
        {
            while (j != NULL)
            {
                if (strcmp(i->name, j->name) > 0)
                {
                    // (i->name > j->name)
                    if (ip != NULL)
                    {
                        ip -> next = j;
                    }
                    else
                    {
                        symTab     = j;
                    }
                    if (i == jp)
                    {
                        i -> next = j -> next;
                        j -> next = i;
                    }
                    else
                    {
                        jp -> next = i;

                        t         = i -> next;
                        i -> next = j -> next;
                        j -> next = t;
                    }
                    t = i;
                    i = j;
                    j = t;
                }
                jp = j;
                j = j -> next;
            }
            ip = i;
            i = i -> next;
            jp = i;
            j = i -> next;
        }
    }
}


// --------------------------------------------------------------
// expression evaluation


static int EXPR_Eval0(void);        // forward declaration


static int EXPR_Factor(void)
{
    Str255      word, s;
    char        *oldLine;

    int token = TOKEN_GetWord(word);
    int val = 0;

    switch (token)
    {
        case 0:
            ASMX_MissingOperand();
            break;

        case '%':
            TOKEN_GetWord(word);
            val = EvalBin(word);
            break;

#ifdef OCTAL_AT
        case '@': // Motorola-style octal constants
            if (isoctal(*linePtr))
            {
                TOKEN_GetWord(word);
                val = EvalOct(word);
                break;
            }
#ifdef TEMP_LBLAT
            // try it as a temp label
            goto lblat;
#else
            ASMX_MissingOperand(); // @ without a digit
#endif // TEMP_LBLAT
            break;
#endif // OCTAL_AT

        case '$':
            if (ishex(*linePtr))
            {
                TOKEN_GetWord(word);
                val = EvalHex(word);
                break;
            }
        // fall-through...
        case '*':
            val = locPtr;
#if 0 // FIXME: possible fix that may be needed for 16-bit address
            if (addrWid == ADDR_16)
            {
                val = (short) val;            // sign-extend from 16 bits
            }
#endif
            val = val / wordDiv;
            break;

        case '+':
            val = EXPR_Factor();
            break;

        case '-':
            val = -EXPR_Factor();
            break;

        case '~':
            val = ~EXPR_Factor();
            break;

        case '!':
            val = !EXPR_Factor();
            break;

        case '<':
            val = EXPR_Factor() & 0xFF;
            break;

        case '>':
            val = (EXPR_Factor() >> 8) & 0xFF;
            break;

        case '(':
            val = EXPR_Eval0();
            TOKEN_RParen();
            break;

        case '[':
            val = EXPR_Eval0();
            TOKEN_Expect("]");
            break;

        case 0x27:  // single quote
#if 1 // new flexible single-quote constants
            {
                // This first attempts to get a multi-byte 'xxxx' constant.
                // If no closing quote is found, it tries again for just a
                // single character (the old Motorola 8-bit style).
                // Then it checks to make sure the number of characters
                // is in the range of 1 to 4.
                // The resulting constant is a big-endian number with the
                // last character in the low byte. (The 4-byte version was
                // used extensively for 68K Macintosh assembly language.)
                // Note the false positive when a single-quote constant
                // is followed by a comment with an unclosed quote closely
                // enough to look like a multi-character quote: LDD #' ;'
                int len = 0;
                val = 0;
                oldLine = linePtr;
                // get bytes after the quote character
                while (*linePtr != 0x27 && *linePtr >= ' ')
                {
                    val = val * 256 + GetBackslashChar();
                    len++;
                }
                if (*linePtr == 0x27)   // add "len &&" to allow '' -> 0x27
                {
                    // close quote found!
                    linePtr++;
                }
                else
                {
                    // close quote not found, try again for just one char
                    linePtr = oldLine;
                    len = 0;
                    val = 0;
                    if (*linePtr >= ' ')
                    {
                        val = GetBackslashChar();
                        len++;
                    }
                }
                if (len < 1)
                {
                    ASMX_MissingOperand();
                }
                else if (len > 4)
                {
                    ASMX_Error("Quote characters too long");
                }
            }
#elif 0 // multi-char single-quote constants
            val = 0;
            while (*linePtr != 0x27 && *linePtr != 0)
            {
                val = val * 256 + GetBackslashChar();
            }
            if (*linePtr == 0x27)
            {
                linePtr++;
            }
            else
            {
                Error("Missing close quote");
            }
#else // single-char single-quote constants
            if ((val = GetBackslashChar()) >= 0)
            {
                if (*linePtr && *linePtr != 0x27)
                {
                    val = val * 256 + GetBackslashChar();
                }
                if (*linePtr == 0x27)
                {
                    linePtr++;
                }
                else
                {
                    Error("Missing close quote");
                }
            }
            else
            {
                ASMX_MissingOperand();
            }
#endif
            break;

        case '.':
            // check for ".."
            oldLine = linePtr;
            val = TOKEN_GetWord(word);
            if (val == '.')
            {
                TOKEN_GetWord(word);
                // check for "..DEF" operator
                if (strcmp(word, "DEF") == 0)
                {
                    val = 0;
                    if (TOKEN_GetWord(word) == -1)
                    {
                        SymRec *p = SYM_Find(word);
                        val = (p && (p -> known || pass == 1));
                    }
                    else
                    {
                        ASMX_IllegalOperand();
                    }
                    break;
                }

                // check for "..UNDEF" operator
                if (strcmp(word, "UNDEF") == 0)
                {
                    val = 0;
                    if (TOKEN_GetWord(word) == -1)
                    {
                        SymRec *p = SYM_Find(word);
                        val = !(p && (p -> known || pass == 1));
                    }
                    else
                    {
                        ASMX_IllegalOperand();
                    }
                    break;
                }

                // invalid ".." operator
                // rewind and return "current location"
                linePtr = oldLine;
                break;
            }
            else if (val != -1)
            {
                // check for '.' as "current location"
                linePtr = oldLine;
                val = locPtr;
#if 0 // FIXME: possible fix that may be needed for 16-bit address
                if (addrWid == ADDR_16)
                {
                    val = (short) val;    // sign-extend from 16 bits
                }
#endif
                val = val / wordDiv;
                break;
            }

            // now try it as a local ".symbol"
            linePtr = oldLine;
            // fall-through...
#ifdef TEMP_LBLAT
#ifdef OCTAL_AT
lblat:
#else
        case '@':
#endif // OCTAL_AT
#endif // TEMP_LBLAT
            TOKEN_GetWord(word);
            if (token == '.' && subrLabl[0])
            {
                strcpy(s, subrLabl);
            }
            else
            {
                strcpy(s, lastLabl);
            }
            s[strlen(s)+1] = 0;
            s[strlen(s)]   = token;
            strcat(s, word);
            val = SYM_Ref(s, &evalKnown);
            break;

        case -1:
            if ((word[0] == 'H' || word[0] == 'L') && word[1] == 0 && *linePtr == '(')
            {
                // handle H() and L() from vintage Atari 7800 source code
                // note: no whitespace allowed before the open paren!
                token = word[0];    // save 'H' or 'L'
                TOKEN_GetWord(word);      // eat left paren
                val = EXPR_Eval0();      // evaluate sub-expression
                TOKEN_RParen();           // check for right paren
                if (token == 'H') val = (val >> 8) & 0xFF;
                if (token == 'L') val = val & 0xFF;
                break;
            }
            if (isdigit(word[0]))
            {
                val = EvalNum(word);
            }
            else
            {
                val = SYM_Ref(word, &evalKnown);
            }
            break;

        default:
            ASMX_MissingOperand();
            break;
    }

    return val;
}


static int EXPR_Term(void)
{
    Str255  word;
    int     val2;

    int val = EXPR_Factor();

    char *oldLine = linePtr;
    int token = TOKEN_GetWord(word);
    while (token == '*' || token == '/' || token == '%')
    {
        switch (token)
        {
            case '*':
                val = val * EXPR_Factor();
                break;

            case '/':
                val2 = EXPR_Factor();
                if (val2)
                {
                    val = val / val2;
                }
                else
                {
                    ASMX_Warning("Division by zero");
                    val = 0;
                }
                break;

            case '%':
                val2 = EXPR_Factor();
                if (val2)
                {
                    val = val % val2;
                }
                else
                {
                    ASMX_Warning("Division by zero");
                    val = 0;
                }
                break;
        }
        oldLine = linePtr;
        token = TOKEN_GetWord(word);
    }
    linePtr = oldLine;

    return val;
}


static int EXPR_Eval2(void)
{
    Str255  word;

    int val = EXPR_Term();

    char *oldLine = linePtr;
    int token = TOKEN_GetWord(word);
    while (token == '+' || token == '-')
    {
        switch (token)
        {
            case '+':
                val = val + EXPR_Term();
                break;
            case '-':
                val = val - EXPR_Term();
                break;
        }
        oldLine = linePtr;
        token = TOKEN_GetWord(word);
    }
    linePtr = oldLine;

    return val;
}


static int EXPR_Eval1(void)
{
    Str255  word;

    int val = EXPR_Eval2();

    char *oldLine = linePtr;
    int token = TOKEN_GetWord(word);
    while ((token == '<' && *linePtr != token) ||
            (token == '>' && *linePtr != token) ||
            token == '=' ||
            (token == '!' && *linePtr == '='))
    {
        switch (token)
        {
            case '<':
                if (*linePtr == '=')
                {
                    linePtr++;
                    val = (val <= EXPR_Eval2());
                }
                else
                {
                    val = (val <  EXPR_Eval2());
                }
                break;

            case '>':
                if (*linePtr == '=')
                {
                    linePtr++;
                    val = (val >= EXPR_Eval2());
                }
                else
                {
                    val = (val >  EXPR_Eval2());
                }
                break;

            case '=':
                if (*linePtr == '=')
                {
                    linePtr++; // allow either one or two '=' signs
                }
                val = (val == EXPR_Eval2());
                break;

            case '!':
                linePtr++;
                val = (val != EXPR_Eval2());
                break;
        }
        oldLine = linePtr;
        token = TOKEN_GetWord(word);
    }
    linePtr = oldLine;

    return val;
}


static int EXPR_Eval0(void)
{
    Str255  word;

    int val = EXPR_Eval1();

    char *oldLine = linePtr;
    int token = TOKEN_GetWord(word);
    while (token == '&' || token == '|' || token == '^' ||
            (token == '<' && *linePtr == '<') ||
            (token == '>' && *linePtr == '>'))
    {
        switch (token)
        {
            case '&':
                if (*linePtr == '&')
                {
                    linePtr++;
                    val = ((val & EXPR_Eval1()) != 0);
                }
                else
                {
                    val =   val & EXPR_Eval1();
                }
                break;

            case '|':
                if (*linePtr == '|')
                {
                    linePtr++;
                    val = ((val | EXPR_Eval1()) != 0);
                }
                else
                {
                    val =   val | EXPR_Eval1();
                }
                break;

            case '^':
                val = val ^ EXPR_Eval1();
                break;

            case '<':
                linePtr++;
                val = val << EXPR_Eval1();
                break;

            case '>':
                linePtr++;
                val = val >> EXPR_Eval1();
                break;
        }
        oldLine = linePtr;
        token = TOKEN_GetWord(word);
    }
    linePtr = oldLine;

    return val;
}


int EXPR_Eval(void)
{
    evalKnown = true;

    return EXPR_Eval0();
}


void EXPR_CheckByte(int val)
{
    if (!errFlag && (val < -128 || val > 255))
    {
        ASMX_Warning("Byte out of range");
    }
}


void EXPR_CheckStrictByte(int val)
{
    if (!errFlag && (val < -128 || val > 127))
    {
        ASMX_Warning("Byte out of range");
    }
}


void EXPR_CheckWord(int val)
{
    if (!errFlag && (val < -32768 || val > 65535))
    {
        ASMX_Warning("Word out of range");
    }
}


void EXPR_CheckStrictWord(int val)
{
    if (!errFlag && (val < -32768 || val > 32767))
    {
        ASMX_Warning("Word out of range");
    }
}


int EXPR_EvalByte(void)
{
    long val = EXPR_Eval();
    EXPR_CheckByte(val);

    return val & 0xFF;
}


static long EXPR_AddrWrap(long addr)
{
    // handle wrap-around by sign-extending for smaller address bus sizes

    switch (addrWid)
    {
        case ADDR_16:
            if (addr & 0x8000)
            {
                return addr | 0xFFFF0000;
            }
            else
            {
                return addr & 0x0000FFFF;
            }
            break;

        case ADDR_24:
            if (addr & 0x8000)
            {
                return addr | 0xFF000000;
            }
            else
            {
                return addr & 0x00FFFFFF;
            }
            break;

        default:
        case ADDR_32:
            break;
    };

    return addr;
}


int EXPR_EvalBranch(int instrLen)
{
    int32_t val = EXPR_Eval();
    val = EXPR_AddrWrap(val - locPtr - instrLen);
    if (!errFlag && (val < -128 || val > 127))
    {
        ASMX_Error("Short branch out of range");
    }

    return val & 0xFF;
}


int EXPR_EvalWBranch(int instrLen)
{
    int32_t val = EXPR_Eval();
    val = EXPR_AddrWrap(val - locPtr - instrLen);
    if (!errFlag && (val < -32768 || val > 32767))
    {
        ASMX_Error("Word branch out of range");
    }
    return val;
}


int EXPR_EvalLBranch(int instrLen)
{
    int32_t val = EXPR_Eval();
    val = val - locPtr - instrLen;
    return val;
}


// --------------------------------------------------------------
// object file generation

// record types -- note that 0 and 1 are used directly for .hex
enum
{
    REC_DATA = 0,   // data record
    REC_XFER = 1,   // transfer address record
    REC_HEDR = 2,   // header record (must be sent before start of data)
#ifdef CODE_COMMENTS
    REC_CMNT = 3    // comment record
#endif // CODE_COMMENTS
};

uint8_t  hex_buf[IHEX_SIZE]; // buffer for current line of object data
uint32_t hex_len;            // current size of object data buffer
uint32_t hex_base;           // address of start of object data buffer
uint32_t hex_addr;           // address of next byte in object data buffer
uint16_t hex_page;           // high word of address for intel hex file
uint32_t bin_eof;            // current end of file when writing binary file

// Intel hex format:
//
// :aabbbbccdddd...ddee
//
// aa    = record data length (the number of dd bytes)
// bbbb  = address for this record
// cc    = record type
//       00 = data (data is in the dd bytes)
//       01 = end of file (bbbb is transfer address)
//       02 = extended segment address record
//            dddd (two bytes) represents the segment address
//       03 = Start segment address record
//            dddd (two bytes) represents the segment of the transfer address
//       04 = extended linear address record
//            dddd (two bytes) represents the high address word
//       05 = Start linear address record
//            dddd (two bytes) represents the high word of the transfer address
// dd... = data bytes if record type needs it
// ee    = checksum byte: add all bytes aa through dd
//                        and subtract from 256 (2's complement negate)

static void OBJF_write_ihex(uint32_t addr, uint8_t *buf, uint32_t len, int rectype)
{
    if (rectype > REC_XFER) return;

    // if transfer record with long address, write extended address record
    if (rectype == REC_XFER && (addr & 0xFFFF0000))
    {
        OBJF_write_ihex(addr >> 16, buf, 0, 5);
    }

    // if data record with long address, write extended address record
    if (rectype == REC_DATA && (addr >> 16) != hex_page)
    {
        OBJF_write_ihex(addr >> 16, buf, 0, 4);
        hex_page = addr >> 16;
    }

    // compute initial checksum from length, address, and record type
    int chksum = len + (addr >> 8) + addr + rectype;

    // print length, address, and record type
    fprintf(object, ":%.2X%.4X%.2X", len, addr & 0xFFFF, rectype);

    // print data while updating checksum
    for (uint32_t i = 0; i < len; i++)
    {
        fprintf(object, "%.2X", buf[i]);
        chksum = chksum + buf[i];
    }

    // print final checksum
    fprintf(object, "%.2X\n", (-chksum) & 0xFF);
}


// Motorola S-record format:
//
// Sabbcc...ccdd...ddee
//
// a     = record type
//         0 starting record (optional)
//         1 data record with 16-bit address
//         2 data record with 24-bit address
//         3 data record with 32-bit address
//         4 symbol record (LSI extension) - S4<length><address><name>,<checksum>
//         5 number of data records in preceeding block
//         6 unused
//         7 ending record for S3 records
//         8 ending record for S2 records
//         9 ending record for S1 records
// bb    = record data length (from bb through ee)
// cc... = address for this record, 2 bytes for S1/S9, 3 bytes for S2/S8, 4 bytes for S3/S7
// dd... = data bytes if record type needs it
// ee    = checksum byte: add all bytes bb through dd
//                        and subtract from 255 (1's complement)

void OBJF_write_srec(uint32_t addr, uint8_t *buf, uint32_t len, int rectype)
{
    if (rectype > REC_XFER) return; // should output S0 record?

    // start checksum with length and 16-bit address
    int chksum = len+3 + ((addr >> 8) & 0xFF) + (addr & 0xFF);

    // determine S9 record type
    int typ;
    if (rectype == REC_XFER)
    {
        typ = cl_S9type % 10;   // xfer record = S9/S8/S7
    }
    else
    {
        typ = cl_S9type / 10;   // code record = S1/S2/S3
    }

    // print length and address, and update checksum for long address
    switch (cl_S9type)
    {
        case 37:
            fprintf(object, "S%d%.2X%.8X", typ, len+5, addr);
            chksum = chksum + ((addr >> 24) & 0xFF) + ((addr >> 16) & 0xFF) + 2;
            break;

        case 28:
            fprintf(object, "S%d%.2X%.6X", typ, len+4, addr & 0xFFFFFF);
            chksum = chksum + ((addr >> 16) & 0xFF) + 1;
            break;

        default:
            if (typ == 0) typ = 1; // handle "-s9" option
            fprintf(object, "S%d%.2X%.4X", typ, len+3, addr & 0xFFFF);
            break;
    }

    // print data while updating checksum
    for (uint32_t i = 0; i < len; i++)
    {
        fprintf(object, "%.2X", buf[i]);
        chksum = chksum + buf[i];
    }

    // print final checksum
    fprintf(object, "%.2X\n", ~chksum & 0xFF);
}


void OBJF_write_bin(uint32_t addr, uint8_t *buf, uint32_t len, int rectype)
{
    if (rectype == REC_DATA)
    {
        // return if end of data less than base address
        if (addr + len <= cl_Binbase) return;

        // return if start of data greater than end address
        if (addr > cl_Binend) return;

        // if data crosses base address, adjust start of data
        if (addr < cl_Binbase)
        {
            buf = buf + cl_Binbase - addr;
            addr = cl_Binbase;
        }

        // if data crossses end address, adjust length of data
        if (addr+len-1 > cl_Binend)
        {
            len = cl_Binend - addr + 1;
        }

        // if addr is beyond current EOF, write (addr-bin_eof) bytes of 0xFF padding
        if (addr - cl_Binbase > bin_eof)
        {
            fseek(object, bin_eof, SEEK_SET);
            for (uint32_t i = 0; i < addr - cl_Binbase - bin_eof; i++)
            {
                fputc(0xFF, object);
            }
        }

        // seek to addr and write buf
        fseek(object, addr - cl_Binbase, SEEK_SET);
        fwrite(buf, 1, len, object);

        // update EOF of object file
        uint32_t i = ftell(object); //MIXWORX
        if (i > bin_eof)
        {
            bin_eof = i;
        }

        //fflush(object);
    }
}


uint8_t trs_buf[TRS_BUF_MAX]; // buffer for current object code data, used instead of hex_buf

void OBJF_write_trsdos(uint32_t addr, uint8_t *buf, uint32_t len, int rectype)
{
    switch (rectype)
    {
        case REC_DATA:  // write data record
            // 01 len+2 ll hh data...
            // NOTE: buf is ignored in favor of using trs_buf
            fputc(0x01, object);
            fputc((len+2) & 0xFF, object);
            fputc(addr & 0xFF, object);
            fputc((addr >> 8) & 0xFF, object);

            fwrite(trs_buf, len, 1, object);
            break;

        case REC_XFER:  // write transfer record
            // 02 02 ll hh
            fputc(0x02, object);
            fputc(0x02, object);
            fputc(addr & 0xFF, object);
            fputc((addr >> 8) & 0xFF, object);
            break;

        case REC_HEDR:  // write header record
            // 05 06 dd dd dd dd dd dd - dd = header data, padded with blanks

#if 1
            // Note: trimming to six chars uppercase for now only to keep with standard ASMX_usage
            fputc(0x05, object);
            fputc(0x06, object);

            for (int i = 0; i < 6; i++)
            {
                if (*buf == 0 || *buf == '.')
                {
                    fputc(' ', object);
                }
                else
                {
                    fputc(toupper(*buf++), object);
                }
            }
#else
            fputc(0x05, object);
            fputc(len, object);

            fwrite(buf, len, 1, object);
#endif
            break;

#ifdef CODE_COMMENTS
        case REC_CMNT:  // write copyright record
            // 1F len data

            fputc(0x1F, object);
            fputc(len,  object);

            for (int i = 0; i < len; i++)
            {
                fputc(*buf++, object);
            }
            break;
#endif // CODE_COMMENTS
    }
}


void OBJF_write_trscas(uint32_t addr, uint8_t *buf, uint32_t len, int rectype)
{
    uint8_t chksum;

    switch (rectype)
    {
        case REC_DATA:  // write data record
            // 3C len ll hh data... cs
            // NOTE: buf is ignored in favor of using trs_buf
            fputc(0x3C, object);
            fputc(len & 0xFF, object);
            fputc(addr & 0xFF, object);
            fputc((addr >> 8) & 0xFF, object);

            chksum = (addr & 0xFF) + ((addr >> 8) & 0xFF);
            for (unsigned int i = 0; i < len; i++)
            {
                chksum += trs_buf[i];
            }

            fwrite(trs_buf, len, 1, object);
            fwrite(&chksum, 1, 1, object);
            break;

        case REC_XFER:  // write transfer record
            // 78 ll hh
            fputc(0x78, object);
            fputc(addr & 0xFF, object);
            fputc((addr >> 8) & 0xFF, object);
            break;

        case REC_HEDR:  // write header record
            // [00 x 255] A5 = leader and sync
            for (int i = 0; i < 255; i++)
            {
                fputc(0x00, object);
            }
            fputc(0xA5, object);

            // 55 dd dd dd dd dd dd - dd = header data, padded with blanks

            fputc(0x55, object);

            for (int i = 0; i < 6; i++)
            {
                if (*buf == 0 || *buf == '.')
                {
                    fputc(' ', object);
                }
                else
                {
                    fputc(toupper(*buf++), object);
                }
            }
            break;

#ifdef CODE_COMMENTS
        case REC_CMNT:
            break;
#endif // CODE_COMMENTS
    }
}


// rectype 0 = code, rectype 1 = xfer
void OBJF_write_hex(uint32_t addr, uint8_t *buf, uint32_t len, int rectype)
{
    if (cl_Obj || cl_Stdout)
    {
        switch (cl_ObjType)
        {
            default:
            case OBJ_HEX:
                OBJF_write_ihex  (addr, buf, len, rectype);
                break;
            case OBJ_S9:
                OBJF_write_srec  (addr, buf, len, rectype);
                break;
            case OBJ_BIN:
                OBJF_write_bin   (addr, buf, len, rectype);
                break;
            case OBJ_TRSDOS:
                OBJF_write_trsdos(addr, buf, len, rectype);
                break;
            case OBJ_TRSCAS:
                OBJF_write_trscas(addr, buf, len, rectype);
                break;
        }
    }
}


void OBJF_CodeInit(void)
{
    hex_len  = 0;
    hex_base = 0;
    hex_addr = 0;
    hex_page = 0;
    bin_eof  = 0;
}


void OBJF_CodeFlush(void)
{
    if (hex_len)
    {
        OBJF_write_hex(hex_base, hex_buf, hex_len, REC_DATA);
        hex_len  = 0;
        hex_base = hex_base + hex_len;
        hex_addr = hex_base;
    }
}


void OBJF_CodeOut(int byte)
{
    if (pass == 2)
    {
        if (codPtr != hex_addr)
        {
            OBJF_CodeFlush();
            hex_base = codPtr;
            hex_addr = codPtr;
        }

        switch (cl_ObjType)
        {
            case OBJ_TRSDOS:
            case OBJ_TRSCAS:
                trs_buf[hex_len++] = byte;
                hex_addr++;

                if (hex_len == cl_trslen)
                {
                    OBJF_CodeFlush();
                }
                break;

            default:
                hex_buf[hex_len++] = byte;
                hex_addr++;

                if (hex_len == IHEX_SIZE)
                {
                    OBJF_CodeFlush();
                }
                break;
        }
    }
    locPtr++;
    codPtr++;
}


void OBJF_CodeHeader(const char *s)
{
    OBJF_CodeFlush();

    OBJF_write_hex(0, (uint8_t *) s, strlen(s), REC_HEDR);
}


#ifdef CODE_COMMENTS
void CodeComment(const char *s)
{
    OBJF_CodeFlush();

    OBJF_write_hex(0, (uint8_t *) s, strlen(s), REC_CMNT);
}
#endif // CODE_COMMENTS


void OBJF_CodeEnd(void)
{
    OBJF_CodeFlush();

    if (pass == 2)
    {
        if (xferFound)
        {
            OBJF_write_hex(xferAddr, hex_buf, 0, REC_XFER);
        }
    }
}


void OBJF_CodeAbsOrg(int addr)
{
    codPtr = addr;
    locPtr = addr;
}


void OBJF_CodeRelOrg(int addr)
{
    locPtr = addr;
}


void OBJF_CodeXfer(int addr)
{
    xferAddr  = addr;
    xferFound = true;
}


void OBJF_AddLocPtr(int ofs)
{
    codPtr = codPtr + ofs;
    locPtr = locPtr + ofs;
}


// --------------------------------------------------------------
// instruction format calls

// clear the length of the instruction
void INSTR_Clear(void)
{
    instrLen = 0;
    hexSpaces = 0;
}


// add a byte to the instruction
void INSTR_AddB(uint8_t b)
{
    bytStr[instrLen++] = b;
    hexSpaces |= 1 << instrLen;
}


// add a byte/word opcode to the instruction
// a big-endian word is added if the opcode is > 255
// this is for opcodes with pre-bytes
void INSTR_AddX(uint32_t op)
{
//  if ((op & 0xFFFFFF00) == 0) hexSpaces |= 1; // to indent single-byte instructions
//  if (op & 0xFF000000) bytStr[instrLen++] = op >> 24;
//  if (op & 0xFFFF0000) bytStr[instrLen++] = op >> 16;
    if (op & 0xFFFFFF00) bytStr[instrLen++] = op >>  8;
    bytStr[instrLen++] = op & 255;
    hexSpaces |= 1 << instrLen;
}


// add a word to the instruction in the CPU's endianness
void INSTR_AddW(uint16_t w)
{
    if (endian == END_LITTLE)
    {
        bytStr[instrLen++] = w & 255;
        bytStr[instrLen++] = w >> 8;
    }
    else if (endian == END_BIG)
    {
        bytStr[instrLen++] = w >> 8;
        bytStr[instrLen++] = w & 255;
    }
    else
    {
        ASMX_Error("CPU endian not defined");
    }
    hexSpaces |= 1 << instrLen;
}


// add a 3-byte word to the instruction in the CPU's endianness
void INSTR_Add3(uint32_t l)
{
    switch (endian)
    {
        case END_LITTLE:
            bytStr[instrLen++] =  l        & 255;
            bytStr[instrLen++] = (l >>  8) & 255;
            bytStr[instrLen++] = (l >> 16) & 255;
            break;
        case END_BIG:
            bytStr[instrLen++] = (l >> 16) & 255;
            bytStr[instrLen++] = (l >>  8) & 255;
            bytStr[instrLen++] =  l        & 255;
            break;
        default:
            ASMX_Error("CPU endian not defined");
            break;
    }
    hexSpaces |= 1 << instrLen;
}


// add a longword to the instruction in the CPU's endianness
void INSTR_AddL(uint32_t l)
{
    switch (endian)
    {
        case END_LITTLE:
            bytStr[instrLen++] =  l        & 255;
            bytStr[instrLen++] = (l >>  8) & 255;
            bytStr[instrLen++] = (l >> 16) & 255;
            bytStr[instrLen++] = (l >> 24) & 255;
            break;
        case END_BIG:
            bytStr[instrLen++] = (l >> 24) & 255;
            bytStr[instrLen++] = (l >> 16) & 255;
            bytStr[instrLen++] = (l >>  8) & 255;
            bytStr[instrLen++] =  l        & 255;
            break;
        default:
            ASMX_Error("CPU endian not defined");
            break;
    }
    hexSpaces |= 1 << instrLen;
}


void INSTR_B(uint8_t b1)
{
    INSTR_Clear();
    INSTR_AddB(b1);
}


void INSTR_BB(uint8_t b1, uint8_t b2)
{
    INSTR_Clear();
    INSTR_AddB(b1);
    INSTR_AddB(b2);
}


void INSTR_BBB(uint8_t b1, uint8_t b2, uint8_t b3)
{
    INSTR_Clear();
    INSTR_AddB(b1);
    INSTR_AddB(b2);
    INSTR_AddB(b3);
}


void INSTR_BBBB(uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4)
{
    INSTR_Clear();
    INSTR_AddB(b1);
    INSTR_AddB(b2);
    INSTR_AddB(b3);
    INSTR_AddB(b4);
}


void INSTR_BBBBB(uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4, uint8_t b5)
{
    INSTR_Clear();
    INSTR_AddB(b1);
    INSTR_AddB(b2);
    INSTR_AddB(b3);
    INSTR_AddB(b4);
    INSTR_AddB(b5);
}


void INSTR_BW(uint8_t b1, uint16_t w1)
{
    INSTR_Clear();
    INSTR_AddB(b1);
    INSTR_AddW(w1);
}


void INSTR_BBW(uint8_t b1, uint8_t b2, uint16_t w1)
{
    INSTR_Clear();
    INSTR_AddB(b1);
    INSTR_AddB(b2);
    INSTR_AddW(w1);
}


void INSTR_BBBW(uint8_t b1, uint8_t b2, uint8_t b3, uint16_t w1)
{
    INSTR_Clear();
    INSTR_AddB(b1);
    INSTR_AddB(b2);
    INSTR_AddB(b3);
    INSTR_AddW(w1);
}


void INSTR_B3(uint8_t b, uint32_t l)
{
    INSTR_Clear();
    INSTR_AddB(b);
    INSTR_Add3(l);
}


void INSTR_X(uint32_t op)
{
    INSTR_Clear();
    INSTR_AddX(op);
}


void INSTR_XB(uint32_t op, uint8_t b1)
{
    INSTR_Clear();
    INSTR_AddX(op);
    INSTR_AddB(b1);
}


void INSTR_XBB(uint32_t op, uint8_t b1, uint8_t b2)
{
    INSTR_Clear();
    INSTR_AddX(op);
    INSTR_AddB(b1);
    INSTR_AddB(b2);
}


void INSTR_XBBB(uint32_t op, uint8_t b1, uint8_t b2, uint8_t b3)
{
    INSTR_Clear();
    INSTR_AddX(op);
    INSTR_AddB(b1);
    INSTR_AddB(b2);
    INSTR_AddB(b3);
}


void INSTR_XBBBB(uint32_t op, uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4)
{
    INSTR_Clear();
    INSTR_AddX(op);
    INSTR_AddB(b1);
    INSTR_AddB(b2);
    INSTR_AddB(b3);
    INSTR_AddB(b4);
}


void INSTR_XW(uint32_t op, uint16_t w1)
{
    INSTR_Clear();
    INSTR_AddX(op);
    INSTR_AddW(w1);
}


void INSTR_XBW(uint32_t op, uint8_t b1, uint16_t w1)
{
    INSTR_Clear();
    INSTR_AddX(op);
    INSTR_AddB(b1);
    INSTR_AddW(w1);
}


void INSTR_XBWB(uint32_t op, uint8_t b1, uint16_t w1, uint8_t b2)
{
    INSTR_Clear();
    INSTR_AddX(op);
    INSTR_AddB(b1);
    INSTR_AddW(w1);
    INSTR_AddB(b2);
}


void INSTR_XWW(uint32_t op, uint16_t w1, uint16_t w2)
{
    INSTR_Clear();
    INSTR_AddX(op);
    INSTR_AddW(w1);
    INSTR_AddW(w2);
}


void INSTR_X3(uint32_t op, uint32_t l1)
{
    INSTR_Clear();
    INSTR_AddX(op);
    INSTR_Add3(l1);
}


void INSTR_W(uint16_t w1)
{
    INSTR_Clear();
    INSTR_AddW(w1);
}

void INSTR_WW(uint16_t w1, uint16_t w2)
{
    INSTR_Clear();
    INSTR_AddW(w1);
    INSTR_AddW(w2);
}

void INSTR_WL(uint16_t w1, uint32_t l1)
{
    INSTR_Clear();
    INSTR_AddW(w1);
    INSTR_AddL(l1);
}


void INSTR_L(uint32_t l1)
{
    INSTR_Clear();
    INSTR_AddL(l1);
}


void INSTR_LL(uint32_t l1, uint32_t l2)
{
    INSTR_Clear();
    INSTR_AddL(l1);
    INSTR_AddL(l2);
}


// --------------------------------------------------------------
// segment handling


SegRec *SEG_Find(const char *name)
{
    SegRec *p = segTab;
    bool found = false;

    while (p && !found)
    {
        found = (strcmp(p -> name, name) == 0);
        if (!found)
        {
            p = p -> next;
        }
    }

    return p;
}


SegRec *SEG_Add(const char *name)
{
    SegRec *p = (SegRec *) malloc(sizeof *p + strlen(name));

    p -> next = segTab;
//  p -> gen = true;
    p -> loc = 0;
    p -> cod = 0;
    strcpy(p -> name, name);

    segTab = p;

    return p;
}


void SEG_Switch(SegRec *seg)
{
    OBJF_CodeFlush();
    curSeg -> cod = codPtr;
    curSeg -> loc = locPtr;

    curSeg = seg;
    codPtr = curSeg -> cod;
    locPtr = curSeg -> loc;
}


// --------------------------------------------------------------
// text I/O


int TEXT_OpenInclude(const char *fname)
{
    if (nInclude == MAX_INCLUDE - 1)
    {
        return -1;
    }

    nInclude++;
    include[nInclude] = NULL;
    incline[nInclude] = 0;
    strcpy(incname[nInclude], fname);
    include[nInclude] = fopen(fname, "r");
    if (include[nInclude])
    {
        return 1;
    }

    nInclude--;
    return 0;
}


void TEXT_CloseInclude(void)
{
    if (nInclude < 0)
    {
        return;
    }

    fclose(include[nInclude]);
    include[nInclude] = NULL;
    nInclude--;
}


int TEXT_ReadLine(FILE *file, char *line, int max)
{
    int c = 0;
    int len = 0;

    macLineFlag = true;

    // if at end of macro and inside a nested macro, pop the stack
    while (macLevel > 0 && macLine[macLevel] == NULL)
    {
#ifdef ENABLE_REP
        if (macRepeat[macLevel] > 0)
        {
            if (macRepeat[macLevel]--)
            {
                macLine = macPtr[macLevel] -> text;
            }
            else
            {
                free(macPtr[macLevel]);
                macLevel--;
            }
        }
        else
#endif
            macLevel--;
    }

    // if there is still another macro line to process, get it
    if (macLine[macLevel] != NULL)
    {
        strcpy(line, macLine[macLevel] -> text);
        macLine[macLevel] = macLine[macLevel] -> next;
        DoMacParms();
    }
    else
    {
        // else we weren't in a macro or we just ran out of macro
        macLineFlag = false;

        if (nInclude >= 0)
        {
            incline[nInclude]++;
        }
        else
        {
            linenum++;
        }

        macPtr[macLevel] = NULL;

        while (max > 1)
        {
            c = fgetc(file);
            *line = 0;
            switch (c)
            {
                case EOF:
                    if (len == 0) return 0;
                    FALLTHROUGH;
                case '\n':
                    return 1;
                case '\r':
                    c = fgetc(file);
                    if (c != '\n')
                    {
                        ungetc(c, file);
                        c = '\r';
                    }
                    return 1;
                default:
                    *line++ = c;
                    max--;
                    len++;
                    break;
            }
        }
        while (c != EOF && c != '\n')
        {
            c = fgetc(file);
        }
    }
    return 1;
}


int TEXT_ReadSourceLine(char *line, int max)
{
    while (nInclude >= 0)
    {
        int i = TEXT_ReadLine(include[nInclude], line, max);
        if (i) return i;

        TEXT_CloseInclude();
    }

    return TEXT_ReadLine(source, line, max);
}


void TEXT_ListOut(bool showStdErr)
{
#if 0 // uncomment this block if you want form feeds to be sent to the listing file
    if (listLineFF && cl_List)
    {
        fputc(12, listing);
    }
#endif
    Debright(listLine);

    if (cl_List)
    {
        fprintf(listing, "%s\n", listLine);
    }

    if (pass == 2 && showStdErr
            && ((errFlag && cl_Err) || (warnFlag && cl_Warn)))
    {
        fprintf(stderr, "%s\n", listLine);
    }
}


void TEXT_CopyListLine(void)
{
    int n;
    char c;

    char *p = listLine;
    char *q = line;
    listLineFF = false;

    // the old version
//  strcpy(listLine, "                ");       // 16 blanks
//  strncat(listLine, line, 255-16);

    if (listWid == LIST_24)
    {
        for (n = 24; n; n--) *p++ = ' ';  // 24 blanks at start of line
    }
    else
    {
        for (n = 16; n; n--) *p++ = ' ';  // 16 blanks at start of line
    }

//  n = 0 here
    while (n < 255-16 && (c = *q++))    // copy rest of line, stripping out form feeds
    {
        if (c == 12)
        {
            listLineFF = true; // if a form feed was found, remember it for later
        }
        else
        {
            *p++ = c;
            n++;
        }
    }
    *p = 0;   // string terminator
}


// --------------------------------------------------------------
// main assembler loops


void ASMX_DoOpcode(int typ, int parm)
{
    int             val = 0;
    int             n;
    Str255          word, s;
    char            *oldLine;
    int             token;

    if (DoCPUOpcode(typ, parm)) return;

    switch (typ)
    {
        case OP_ZSCII:
        case OP_ASCIIC:
        case OP_ASCIIZ:
        case OP_DB:
            instrLen = 0;
            oldLine = linePtr;
            token = TOKEN_GetWord(word);

            if (token == 0)
            {
                // no operands at all?
                ASMX_MissingOperand();
            }

            if (typ == OP_ASCIIC)
            {
                // reserve space for ASCIIC count byte
                bytStr[instrLen++] = 0;
            }

            while (token)
            {
                // do another operand
                if ((token == '\'' && *linePtr && linePtr[1] != '\'') || token == '"')
                {
                    // found a single or double quote, so do a string
                    int quote = token;
                    int len = 0;
                    while (token == quote)
                    {
                        while (*linePtr >= ' ' && *linePtr != token)
                        {
                            int ch = GetBackslashChar();
                            if ((ch >= 0) && (instrLen < MAX_BYTSTR))
                            {
                                bytStr[instrLen++] = ch;
                                len++;
                            }
                        }
                        token = *linePtr;
                        if (token)
                        {
                            linePtr++;
                        }
                        else
                        {
                            if (len == 1 && quote == '\'')
                            {
                                // allow "FCB 'x" (with only one ' quote)
                            }
                            else
                            {
                                ASMX_Error("Missing close quote");
                            }
                        }
                        if (token == quote && *linePtr == quote)    // two quotes together
                        {
                            if (instrLen < MAX_BYTSTR)
                            {
                                bytStr[instrLen++] = *linePtr++;
                            }
                        }
                        else
                        {
                            token = *linePtr;
                        }
                    }
                }
                else
                {
                    // not a string, back up and get a value
                    linePtr = oldLine;
                    val = EXPR_EvalByte();
                    if (instrLen < MAX_BYTSTR)
                    {
                        bytStr[instrLen++] = val;
                    }
                }

                // get next thing after string or value
                token = TOKEN_GetWord(word);
                oldLine = linePtr;

                if (token == ',')
                {
                    // if comma, check for unexpected end of line
                    token = TOKEN_GetWord(word);
                    if (token == 0)
                    {
                        ASMX_MissingOperand();
                    }
                }
                else if (token)
                {
                    // if not comma or end of line, complain about missing comma
                    linePtr = oldLine;
                    TOKEN_Comma();
                    token = 0;
                }
                else if (errFlag)     // this is necessary to keep EXPR_EvalByte() errors
                {
                    token = 0;      // from causing phase errors
                }
            }

            if (instrLen >= MAX_BYTSTR || (typ == OP_ASCIIC && instrLen > 256))
            {
                ASMX_Error("String too long");
                instrLen = MAX_BYTSTR;
            }

            switch (typ)
            {
                case OP_ASCIIC:
                    // update length byte for ASCIIC
                    if (instrLen > 255)
                    {
                        bytStr[0] = 255;
                    }
                    else
                    {
                        bytStr[0] = instrLen-1;
                    }
                    break;

                case OP_ZSCII:
                    ConvertZSCII();
                    break;

                case OP_ASCIIZ:
                    // add zero byte for ASCIIZ
                    if (instrLen < MAX_BYTSTR)
                    {
                        bytStr[instrLen++] = 0;
                    }
                    break;

                default:
                    break;
            }

            instrLen = -instrLen;
            break;

        case OP_DW:
        case OP_DWRE:    // reverse-endian DW
            if (endian != END_LITTLE && endian != END_BIG)
            {
                ASMX_Error("CPU endian not defined");
                break;
            }

            instrLen = 0;
            oldLine = linePtr;
            token = TOKEN_GetWord(word);

            if (token == 0)
            {
                ASMX_MissingOperand();
            }

            while (token)
            {
#if 1 // enable padded string literals
                if ((token == '\'' && *linePtr && linePtr[1] != '\'') || token == '"')
                {
                    n = 0;
                    int quote = token;
                    while (token == quote)
                    {
                        while (*linePtr != 0 && *linePtr != token)
                        {
                            int ch = GetBackslashChar();
                            if ((ch >= 0) && (instrLen < MAX_BYTSTR))
                            {
                                bytStr[instrLen++] = ch;
                                n++;
                            }
                        }
                        token = *linePtr;
                        if (token)
                        {
                            linePtr++;
                        }
                        else
                        {
                            ASMX_Error("Missing close quote");
                        }
                        if (token == quote && *linePtr == quote)    // two quotes together
                        {
                            if (instrLen < MAX_BYTSTR)
                            {
                                bytStr[instrLen++] = *linePtr++;
                                n++;
                            }
                        }
                        else
                        {
                            token = *linePtr;
                        }
                    }
                    // add padding nulls here
                    if ((n & 1) && instrLen < MAX_BYTSTR)
                    {
                        bytStr[instrLen++] = 0;
                    }
                }
                else
#endif
                {
                    linePtr = oldLine;
                    val = EXPR_Eval();
                    if ((endian == END_LITTLE) ^ (typ == OP_DWRE))
                    {
                        // little endian
                        if (instrLen < MAX_BYTSTR)
                        {
                            bytStr[instrLen++] = val;
                        }
                        if (instrLen < MAX_BYTSTR)
                        {
                            bytStr[instrLen++] = val >> 8;
                        }
                    }
                    else
                    {
                        // big endian
                        if (instrLen < MAX_BYTSTR)
                        {
                            bytStr[instrLen++] = val >> 8;
                        }
                        if (instrLen < MAX_BYTSTR)
                        {
                            bytStr[instrLen++] = val;
                        }
                    }
                }

                token = TOKEN_GetWord(word);
                oldLine = linePtr;

                if (token == ',')
                {
                    token = TOKEN_GetWord(word);
                    if (token == 0)
                    {
                        ASMX_MissingOperand();
                    }
                }
                else if (token)
                {
                    linePtr = oldLine;
                    TOKEN_Comma();
                    token = 0;
                }
            }

            if (instrLen >= MAX_BYTSTR)
            {
                ASMX_Error("String too long");
                instrLen = MAX_BYTSTR;
            }
            instrLen = -instrLen;
            break;

        case OP_DL:
            if (endian != END_LITTLE && endian != END_BIG)
            {
                ASMX_Error("CPU endian not defined");
                break;
            }

            instrLen = 0;
            oldLine = linePtr;
            token = TOKEN_GetWord(word);

            if (token == 0)
            {
                ASMX_MissingOperand();
            }

            while (token)
            {
#if 1 // enable padded string literals
                if ((token == '\'' && *linePtr && linePtr[1] != '\'')
                        || token == '"')
                {
                    n = 0;
                    int quote = token;
                    while (token == quote)
                    {
                        while (*linePtr != 0 && *linePtr != token)
                        {
                            int ch = GetBackslashChar();
                            if ((ch >= 0) && (instrLen < MAX_BYTSTR))
                            {
                                bytStr[instrLen++] = ch;
                                n++;
                            }
                        }
                        token = *linePtr;
                        if (token)
                        {
                            linePtr++;
                        }
                        else
                        {
                            ASMX_Error("Missing close quote");
                        }
                        if (token == quote && *linePtr == quote)    // two quotes together
                        {
                            if (instrLen < MAX_BYTSTR)
                            {
                                bytStr[instrLen++] = *linePtr++;
                                n++;
                            }
                        }
                        else
                        {
                            token = *linePtr;
                        }
                    }
                    // add padding nulls here
                    while ((n & 3) && instrLen < MAX_BYTSTR)
                    {
                        bytStr[instrLen++] = 0;
                        n++;
                    }
                }
                else
#endif
                {
                    linePtr = oldLine;
                    val = EXPR_Eval();
                    if ((endian == END_LITTLE) ^ (typ == OP_DWRE))
                    {
                        // little endian
                        if (instrLen < MAX_BYTSTR)
                        {
                            bytStr[instrLen++] = val;
                        }
                        if (instrLen < MAX_BYTSTR)
                        {
                            bytStr[instrLen++] = val >> 8;
                        }
                        if (instrLen < MAX_BYTSTR)
                        {
                            bytStr[instrLen++] = val >> 16;
                        }
                        if (instrLen < MAX_BYTSTR)
                        {
                            bytStr[instrLen++] = val >> 24;
                        }
                    }
                    else
                    {
                        // big endian
                        if (instrLen < MAX_BYTSTR)
                        {
                            bytStr[instrLen++] = val >> 24;
                        }
                        if (instrLen < MAX_BYTSTR)
                        {
                            bytStr[instrLen++] = val >> 16;
                        }
                        if (instrLen < MAX_BYTSTR)
                        {
                            bytStr[instrLen++] = val >> 8;
                        }
                        if (instrLen < MAX_BYTSTR)
                        {
                            bytStr[instrLen++] = val;
                        }
                    }
                }

                token = TOKEN_GetWord(word);
                oldLine = linePtr;

                if (token == ',')
                {
                    token = TOKEN_GetWord(word);
                    if (token == 0)
                    {
                        ASMX_MissingOperand();
                    }
                }
                else if (token)
                {
                    linePtr = oldLine;
                    TOKEN_Comma();
                    token = 0;
                }
            }

            if (instrLen >= MAX_BYTSTR)
            {
                ASMX_Error("String too long");
                instrLen = MAX_BYTSTR;
            }
            instrLen = -instrLen;
            break;

        case OP_DS:
            val = EXPR_Eval();

            if (!evalKnown)
            {
                ASMX_Error("Can't use DS pseudo-op with forward-declared length");
                break;
            }

            oldLine = linePtr;
            token = TOKEN_GetWord(word);
            if (token == ',')
            {
                // DS len,data (with initialization)
                if (parm == 1)
                {
                    // data is a byte
                    n = EXPR_EvalByte();
                }
                else
                {
                    // data is bigger than a byte
                    n = EXPR_Eval();
                }

                // verify initialization data size
                if (val < 0)
                {
                    ASMX_Error("Invalid negative DS size");
                    break;
                }
                if (val*parm > MAX_BYTSTR)
                {
                    sprintf(s, "String too long (max %d bytes)", MAX_BYTSTR);
                    ASMX_Error(s);
                    break;
                }

                if (parm == 1)
                {
                    // DS.B
                    // copy the byte into every location
                    for (int i = 0; i < val; i++)
                    {
                        bytStr[i] = n;
                    }
                }
                else
                {
                    // DS.W / DS.L
                    if (endian != END_LITTLE && endian != END_BIG)
                    {
                        ASMX_Error("CPU endian not defined");
                        break;
                    }

                    // copy words/longwords, respecting current endianness
                    for (int i = 0; i < val*parm; i += parm)
                    {
                        if (endian == END_BIG)
                        {
                            if (parm == 4)
                            {
                                // DS.L
                                bytStr[i]   = n >> 24;
                                bytStr[i+1] = n >> 16;
                                bytStr[i+2] = n >> 8;
                                bytStr[i+3] = n;
                            }
                            else
                            {
                                // DS.W
                                bytStr[i]   = n >> 8;
                                bytStr[i+1] = n;
                            }
                        }
                        else
                        {
                            // DS.W / DS.L
                            bytStr[i]   = n;
                            bytStr[i+1] = n >> 8;
                            if (parm == 4)
                            {
                                // DS.L
                                bytStr[i+2] = n >> 16;
                                bytStr[i+3] = n >> 24;
                            }
                        }
                    }
                }

                // add size to the location pointer
                instrLen = -val * parm;
                break;
            }
            else if (token)
            {
                // report error "comma expected" if not EOL
                // but don't abort
                linePtr = oldLine;
                TOKEN_Comma();
                //token = 0;
            }

            // "DS len" without initialization
            if (pass == 2)
            {
                showAddr = false;

                // "XXXX  (XXXX)"
                char *p = LIST_Loc(locPtr);
                *p++ = ' ';
                *p++ = '(';
                p = LIST_Addr(p, val);
                *p++ = ')';
            }

            // add size to the location pointer
            OBJF_AddLocPtr(val*parm);
            break;

        case OP_HEX:
            instrLen = 0;
            while (!errFlag && TOKEN_GetWord(word))
            {
                n = strlen(word);
                for (int i = 0; i < n; i += 2)
                {
                    if (ishex(word[i]) && ishex(word[i+1]))
                    {
                        val = Hex2Dec(word[i]) * 16 + Hex2Dec(word[i+1]);
                        if (instrLen < MAX_BYTSTR)
                        {
                            bytStr[instrLen++] = val;
                        }
                    }
                    else
                    {
                        ASMX_Error("Invalid HEX string");
                        n = i;
                    }
                }
            }

            if (instrLen >= MAX_BYTSTR)
            {
                ASMX_Error("String too long");
                instrLen = MAX_BYTSTR;
            }
            instrLen = -instrLen;
            break;

        case OP_FCC:
            instrLen = 0;
            oldLine = linePtr;
            token = TOKEN_GetWord(word);

            if (token == 0)
            {
                // end of line at the start
                ASMX_MissingOperand();
            }
            else if (token == -1)
            {
                // starts with a word: FCC length,text
                // I'm not sure where I saw this used, but apparently
                // it was an actual thing somewhere.
                linePtr = oldLine;
                val = EXPR_Eval();
                token = TOKEN_GetWord(word);
                if (val >= MAX_BYTSTR)
                {
                    ASMX_Error("String too long");
                }
                if (!errFlag && (token == ','))
                {
                    while (*linePtr >= 0x20 && instrLen < val)
                    {
                        bytStr[instrLen++] = *linePtr++;
                    }
                    while (instrLen < val)
                    {
                        bytStr[instrLen++] = ' ';
                    }
                }
                else
                {
                    ASMX_IllegalOperand();
                }
            }
            else
            {
                // normal FCC handling, always starts with quote character
                int ch = token;
                while (token)
                {
                    // process next thing
                    if (token == ch)
                    {
                        // it's a string
                        while (token == ch)
                        {
                            // copy bytes until next quote
                            while (*linePtr != 0 && *linePtr != token)
                            {
                                if (instrLen < MAX_BYTSTR)
                                {
                                    bytStr[instrLen++] = *linePtr++;
                                }
                            }
                            if (*linePtr)
                            {
                                // advance past quote character
                                linePtr++;
                            }
                            else
                            {
                                // string ended without close quote
                                ASMX_Error("FCC not terminated properly");
                            }
                            if (*linePtr == token)
                            {
                                // if repeated quote, insert it
                                if (instrLen < MAX_BYTSTR)
                                {
                                    bytStr[instrLen++] = *linePtr++;
                                }
                            }
                            else
                            {
                                token = *linePtr;
                            }
                        }
                    }
                    else
                    {
                        // not quote character, back up and get a value
                        linePtr = oldLine;
                        val = EXPR_EvalByte();
                        if (instrLen < MAX_BYTSTR)
                        {
                            bytStr[instrLen++] = val;
                        }
                    }

                    // get next thing after string or value
                    token = TOKEN_GetWord(word);
                    oldLine = linePtr;

                    if (token == ',')
                    {
                        // if comma, check for unexpected end of line
                        token = TOKEN_GetWord(word);
                        if (token == 0)
                        {
                            ASMX_MissingOperand();
                        }
                    }
                    else if (token)
                    {
                        // if not comma or end of line, complain about missing comma
                        linePtr = oldLine;
                        TOKEN_Comma();
                        token = 0;
                    }
                    else if (errFlag)
                    {
                        // this is necessary to keep EXPR_EvalByte() errors
                        // from causing phase errors
                        token = 0;
                    }
                }
            }

            if (instrLen >= MAX_BYTSTR)
            {
                ASMX_Error("String too long");
                instrLen = MAX_BYTSTR;
            }
            instrLen = -instrLen;
            break;

        case OP_ALIGN_n:
            val = parm;
            FALLTHROUGH;
        case OP_ALIGN:
            if (typ == OP_ALIGN)
            {
                val = EXPR_Eval();
            }

            // val must be a power of two
            if (val <= 0 || val > 65535 || (val & (val - 1)) != 0)
            {
                ASMX_IllegalOperand();
                val = 0;
            }
            else
            {
                int i = locPtr & ~(val - 1);
                if (i != (signed) locPtr)
                {
                    val = val - (locPtr - i); // aka val = val + i - locPtr;
                }
                else
                {
                    val = 0;
                }
            }

            if (pass == 2)
            {
                showAddr = false;

                // "XXXX  (XXXX)"
                char *p = LIST_Loc(locPtr);
                *p++ = ' ';
                *p++ = '(';
                p = LIST_Addr(p, val);
                *p++ = ')';
            }

            OBJF_AddLocPtr(val);
            break;

        case OP_END:
            if (nInclude >= 0)
            {
                TEXT_CloseInclude();
            }
            else
            {
                oldLine = linePtr;
                if (TOKEN_GetWord(word))
                {
                    linePtr = oldLine;
                    val = EXPR_Eval();
                    OBJF_CodeXfer(val);

                    // "XXXX  (XXXX)"
                    char *p = LIST_Loc(locPtr);
                    *p++ = ' ';
                    *p++ = '(';
                    p = LIST_Addr(p, val);
                    *p++ = ')';
                }
                sourceEnd = true;
            }
            break;

        case OP_Include:
            GetFName(word);

            switch (TEXT_OpenInclude(word))
            {
                case -1:
                    ASMX_Error("Too many nested INCLUDEs");
                    break;
                case 0:
                    snprintf(s, sizeof s, "Unable to open INCLUDE file '%s'", word);
                    ASMX_Error(s);
                    break;
                default:
                    break;
            }
            break;

        case OP_ENDM:
            ASMX_Error("ENDM without MACRO");
            break;

#ifdef ENABLE_REP
        case o_REPEND:
            Error("REPEND without REPEAT");
            break;
#endif

        case OP_Processor:
            if (!TOKEN_GetWord(word))
            {
                ASMX_MissingOperand();
            }
            else
            {
                if (!SetCPU(word)) ASMX_IllegalOperand();
            }
            break;

        default:
            ASMX_Error("Unknown opcode");
            break;
    }
}


void ASMX_DoLabelOp(int typ, int parm, char *labl)
{
    int         val, i, n;
    Str255      word, s;
    int         token;
    Str255      opcode;
    MacroRec    *macro;
    MacroRec    *xmacro;
    int         nparms;
    char        *oldLine;
//  MacroRec    repList;        // repeat text list
//  MacroLine *rep, *rep2;   // pointer into repeat text list
    char        *p;

    if (DoCPULabelOp(typ, parm, labl)) return;

    switch (typ)
    {
        case OP_EQU:
            if (labl[0] == 0)
            {
                ASMX_Error("Missing label");
            }
            else
            {
                val = EXPR_Eval();

                // "XXXX  (XXXX)"
                p = listLine;
                switch (addrWid)
                {
                    default:
                    case ADDR_16:
                        if (listWid == LIST_24)
                        {
                            n = 5;
                        }
                        else
                        {
                            n = 4;
                        }
                        break;

                    case ADDR_24:
                        n = 6;
                        break;

                    case ADDR_32:
                        n = 8;
                        break;
                }
                for (int i = 0; i <= n; i++)
                {
                    *p++ = ' ';
                }
                *p++ = '=';
                *p++ = ' ';
                p = LIST_Addr(p, val);
                SYM_Def(labl, val, /*setSym*/ parm == 1, /*equSym*/ parm == 0);
            }
            break;

        case OP_ORG:
            OBJF_CodeAbsOrg(EXPR_Eval());
            if (!evalKnown)
            {
                ASMX_Warning("Undefined label used in ORG statement");
            }
            SYM_Def(labl, locPtr, false, false);
            showAddr = true;
            break;

        case OP_RORG:
            val = EXPR_Eval();
            OBJF_CodeRelOrg(val);
            SYM_Def(labl, codPtr, false, false);

            if (pass == 2)
            {
                // "XXXX = XXXX"
                p = LIST_Loc(codPtr);
                *p++ = '=';
                *p++ = ' ';
                p = LIST_Addr(p, val);
            }
            break;

        case OP_SEG:
            token = TOKEN_GetWord(word);  // get seg name
            if (token == 0 || token == -1)   // must be null or alphanumeric
            {
                SegRec *seg = SEG_Find(word); // find segment storage and create if necessary
                if (!seg) seg = SEG_Add(word);
//              seg -> gen = parm;      // copy gen flag from parameter
                SEG_Switch(seg);
                SYM_Def(labl, locPtr, false, false);
                showAddr = true;
            }
            break;

        case OP_SUBR:
            token = TOKEN_GetWord(word);  // get subroutine name
            strcpy(subrLabl, word);
            break;

        case OP_REND:
            if (pass == 2)
            {
                // "XXXX = XXXX"
                p = LIST_Loc(locPtr);
            }

            SYM_Def(labl, locPtr, false, false);

            OBJF_CodeAbsOrg(codPtr);

            break;

        case OP_LIST:
            listThisLine = true;    // always list this line

            if (labl[0])
            {
                ASMX_Error("Label not allowed");
            }

            TOKEN_GetWord(word);

            if (strcmp(word, "ON") == 0)       listFlag = true;
            else if (strcmp(word, "OFF") == 0)      listFlag = false;
            else if (strcmp(word, "MACRO") == 0)    listMacFlag = true;
            else if (strcmp(word, "NOMACRO") == 0)  listMacFlag = false;
            else if (strcmp(word, "EXPAND") == 0)   expandHexFlag = true;
            else if (strcmp(word, "NOEXPAND") == 0) expandHexFlag = false;
            else if (strcmp(word, "SYM") == 0)      symtabFlag = true;
            else if (strcmp(word, "NOSYM") == 0)    symtabFlag = false;
            else if (strcmp(word, "TEMP") == 0)     tempSymFlag = true;
            else if (strcmp(word, "NOTEMP") == 0)   tempSymFlag = false;
            else                                    ASMX_IllegalOperand();

            break;
        /*
                case o_PAGE:
                    listThisLine = true;    // always list this line

                    if (labl[0]) {
                        Error("Label not allowed");
                    }

                    listLineFF = true;
                    break;
        */
        case OP_OPT:
            listThisLine = true;    // always list this line

            if (labl[0])
            {
                ASMX_Error("Label not allowed");
            }

            TOKEN_GetWord(word);

            if (strcmp(word, "LIST") == 0)     listFlag = true;
            else if (strcmp(word, "NOLIST") == 0)   listFlag = false;
            else if (strcmp(word, "MACRO") == 0)    listMacFlag = true;
            else if (strcmp(word, "NOMACRO") == 0)  listMacFlag = false;
            else if (strcmp(word, "EXPAND") == 0)   expandHexFlag = true;
            else if (strcmp(word, "NOEXPAND") == 0) expandHexFlag = false;
            else if (strcmp(word, "SYM") == 0)      symtabFlag = true;
            else if (strcmp(word, "NOSYM") == 0)    symtabFlag = false;
            else if (strcmp(word, "TEMP") == 0)     tempSymFlag = true;
            else if (strcmp(word, "NOTEMP") == 0)   tempSymFlag = false;
            else if (strcmp(word, "EXACT") == 0)    exactFlag = true;
            else if (strcmp(word, "NOEXACT") == 0)  exactFlag = false;
            else if (strcmp(word, "NOOPT") == 0)    exactFlag = true;
            else if (strcmp(word, "OPT") == 0)      exactFlag = false;
            else                                    ASMX_Error("Illegal option");

            break;

        case OP_ERROR:
            if (labl[0])
            {
                ASMX_Error("Label not allowed");
            }
            while (*linePtr == ' ' || *linePtr == '\t')
            {
                linePtr++;
            }
            ASMX_Error(linePtr);
            break;

        case OP_ASSERT:
            if (labl[0])
            {
                ASMX_Error("Label not allowed");
            }
            val = EXPR_Eval();
            if (!val)
            {
                ASMX_Error("Assertion failed");
            }
            break;

        case OP_MACRO:
            // see if label already provided
            if (labl[0] == 0)
            {
                // get next word on line for macro name
                if (TOKEN_GetWord(labl) != -1)
                {
                    ASMX_Error("Macro name requried");
                    break;
                }
                // optional comma after macro name
                oldLine = linePtr;
                if (TOKEN_GetWord(word) != ',')
                {
                    linePtr = oldLine;
                }
            }

            // don't allow temporary labels as macro names
#ifdef TEMP_LBLAT
            if (strchr(labl, '.') || (!(opts & OPT_ATSYM) && strchr(labl, '@')))
#else
            if (strchr(labl, '.'))
#endif
            {
                ASMX_Error("Can not use temporary labels as macro names");
                break;
            }

            macro = FindMacro(labl);
            if (macro && macro -> def)
            {
                ASMX_Error("Macro multiply defined");
            }
            else
            {
                if (macro == NULL)
                {
                    macro = AddMacro(labl);
                    nparms = 0;

                    token = TOKEN_GetWord(word);
                    while (token == -1)
                    {
                        nparms++;
                        if (nparms > MAXMACPARMS)
                        {
                            ASMX_Error("Too many macro parameters");
                            macro -> toomany = true;
                            break;
                        }
                        AddMacroParm(macro, word);
                        token = TOKEN_GetWord(word);
                        if (token == ',')
                        {
                            token = TOKEN_GetWord(word);
                        }
                    }

                    if (word[0] && !errFlag)
                    {
                        ASMX_Error("Illegal operand");
                    }
                }

                if (pass == 2)
                {
                    macro -> def = true;
                    if (macro -> toomany)
                    {
                        ASMX_Error("Too many macro parameters");
                    }
                }

                macroCondLevel = 0;
                i = TEXT_ReadSourceLine(line, sizeof(line));
                while (i && typ != OP_ENDM)
                {
                    if ((pass == 2 || cl_ListP1) && (listFlag || errFlag))
                    {
                        TEXT_ListOut(true);
                    }
                    TEXT_CopyListLine();

                    // skip initial formfeeds
                    linePtr = line;
                    while (*linePtr == 12)
                    {
                        linePtr++;
                    }

                    // get label
                    labl[0] = 0;
#ifdef TEMP_LBLAT
                    if (isalphanum(*linePtr) || *linePtr == '.' || *linePtr == '@')
#else
                    if (isalphanum(*linePtr) || *linePtr == '.' || ((opts & OPT_ATSYM) && *linePtr == '@'))
#endif
                    {
                        token = TOKEN_GetWord(labl);
                        if (token)
                        {
                            showAddr = true;
                        }
                        while (*linePtr == ' ' || *linePtr == '\t')
                        {
                            linePtr++;
                        }

                        if (labl[0])
                        {
#ifdef TEMP_LBLAT
                            if (token == '.' || token == '@')
#else
                            if (token == '.')
#endif
                            {
                                // make labl = lastLabl + "." + word;
                                TOKEN_GetWord(word);
                                if (token == '.' && subrLabl[0])
                                {
                                    strcpy(labl, subrLabl);
                                }
                                else
                                {
                                    strcpy(labl, lastLabl);
                                }
                                labl[strlen(labl)+1] = 0;
                                labl[strlen(labl)]   = token;
                                strcat(labl, word);
                            }
                            else
                            {
                                strcpy(lastLabl, labl);
                            }
                        }

                        if (*linePtr == ':' && linePtr[1] != '=')
                        {
                            linePtr++;
                        }
                    }

                    typ = 0;
                    GetFindOpcode(opcode, &typ, &parm, &xmacro);

                    switch (typ)
                    {
                        case OP_IF:
                            if (pass == 1)
                            {
                                AddMacroLine(macro, line);
                            }
                            macroCondLevel++;
                            break;

                        case o_ENDIF:
                            if (pass == 1)
                            {
                                AddMacroLine(macro, line);
                            }
                            if (macroCondLevel)
                            {
                                macroCondLevel--;
                            }
                            else
                            {
                                ASMX_Error("ENDIF without IF in macro definition");
                            }
                            break;

                        case OP_END:
                            ASMX_Error("END not allowed inside a macro");
                            break;

                        case OP_ENDM:
                            if (pass == 1 && labl[0])
                            {
                                AddMacroLine(macro, labl);
                            }
                            break;

                        default:
                            if (pass == 1)
                            {
                                AddMacroLine(macro, line);
                            }
                            break;
                    }
                    if (typ != OP_ENDM)
                    {
                        i = TEXT_ReadSourceLine(line, sizeof(line));
                    }
                }

                if (macroCondLevel)
                {
                    ASMX_Error("IF block without ENDIF in macro definition");
                }

                if (typ != OP_ENDM)
                {
                    ASMX_Error("Missing ENDM");
                }
            }
            break;

        case OP_IF:
            if (labl[0])
            {
                ASMX_Error("Label not allowed");
            }

            if (condLevel >= MAX_COND)
            {
                ASMX_Error("IF statements nested too deeply");
            }
            else
            {
                condLevel++;
                condState[condLevel] = 0; // this block false but level not permanently failed

                val = EXPR_Eval();
                if (!errFlag && val != 0)
                {
                    condState[condLevel] = condTRUE; // this block true
                }
            }
            break;

        case OP_ELSE:    // previous IF was true, so this section stays off
            if (labl[0])
            {
                ASMX_Error("Label not allowed");
            }

            if (condLevel == 0)
            {
                ASMX_Error("ELSE outside of IF block");
            }
            else
            {
                if (condLevel < MAX_COND && (condState[condLevel] & condELSE))
                {
                    ASMX_Error("Multiple ELSE statements in an IF block");
                }
                else
                {
                    condState[condLevel] = condELSE | condFAIL; // ELSE encountered, permanent fail
//                  condState[condLevel] |= condELSE; // ELSE encountered
//                  condState[condLevel] |= condFAIL; // this level permanently failed
//                  condState[condLevel] &= ~condTRUE; // this block false
                }
            }
            break;

        case OP_ELSIF:   // previous IF was true, so this section stays off
            if (labl[0])
            {
                ASMX_Error("Label not allowed");
            }

            if (condLevel == 0)
            {
                ASMX_Error("ELSIF outside of IF block");
            }
            else
            {
                if (condLevel < MAX_COND && (condState[condLevel] & condELSE))
                {
                    ASMX_Error("Multiple ELSE statements in an IF block");
                }
                else
                {
                    // i = Eval(); // evaluate condition and ignore result
                    TOKEN_EatIt(); // just ignore the conditional expression

                    condState[condLevel] |= condFAIL; // this level permanently failed
                    condState[condLevel] &= ~condTRUE; // this block false
                }
            }
            break;

        case o_ENDIF:   // previous ELSE was true, now exiting it
            if (labl[0])
            {
                ASMX_Error("Label not allowed");
            }

            if (condLevel == 0)
            {
                ASMX_Error("ENDIF outside of IF block");
            }
            else
            {
                condLevel--;
            }
            break;

#ifdef ENABLE_REP
// still under construction
// notes:
//      REPEAT pseudo-op should act like an inline macro
//      1) collect all lines into new macro level
//      2) copy parameters from previous macro level (if any)
//      3) set repeat count for this macro level (repeat count is set to 0 for plain macro)
//      4) when macro ends, decrement repeat count and start over if count > 0
//      5) don't forget to dispose of the temp macro and its lines when done!
        case o_REPEAT:
            if (labl[0])
            {
                SYM_Def(labl, locPtr, false, false);
                showAddr = true;
            }

            // get repeat count
            val = Eval();
            if (!errFlag)
            {
                if (val < 0) ASMX_IllegalOperand();
            }

            if (!errFlag)
            {
                repList -> text = NULL;

// *** read line
// *** while line not REPEND
// ***      add line to repeat buffer
                macroCondLevel = 0;
                i = TEXT_ReadSourceLine(line, sizeof(line));
                while (i && typ != o_REPEND)
                {
                    if ((pass == 2 || cl_ListP1) && (listFlag || errFlag))
                    {
                        TEXT_ListOut(true);
                    }
                    TEXT_CopyListLine();

                    // skip initial formfeeds
                    linePtr = line;
                    while (*linePtr == 12)
                    {
                        linePtr++;
                    }

                    // get label
                    labl[0] = 0;
#ifdef TEMP_LBLAT
                    if (isalphanum(*linePtr) || *linePtr == '.')
#else
                    if (isalphanum(*linePtr) || *linePtr == '.' || ((opts & OPT_ATSYM) && *linePtr == '@'))
#endif
                    {
                        token = GetWord(labl);
                        if (token)
                        {
                            showAddr = true;
                        }
                        while (*linePtr == ' ' || *linePtr == '\t')
                        {
                            linePtr++;
                        }

                        if (labl[0])
                        {
#ifdef TEMP_LBLAT
                            if (token == '.' || token == '@'))
#else
                            if (token == '.')
#endif
                            {
                                // make labl = lastLabl + "." + word;
                                GetWord(word);
                                if (token == '.' && subrLabl[0])
                                {
                                    strcpy(labl, subrLabl);
                                }
                                else
                                {
                                    strcpy(labl, lastLabl);
                                }
                                labl[strlen(labl)+1] = 0;
                                labl[strlen(labl)]   = token;
                                strcat(labl, word);
                            }
                            else
                            {
                                strcpy(lastLabl, labl);
                            }
                        }

                        if (*linePtr == ':' && linePtr[1] != '=')
                        {
                            linePtr++;
                        }
                    }

                    typ = 0;
                    GetFindOpcode(opcode, &typ, &parm, &xmacro);

                    switch (typ)
                    {
                        case o_IF:
                            if (pass == 1)
                            {
                                AddMacroLine(&replist, line);
                            }
                            macroCondLevel++;
                            break;

                        case o_ENDIF:
                            if (pass == 1)
                            {
                                AddMacroLine(&replist, line);
                            }
                            if (macroCondLevel)
                            {
                                macroCondLevel--;
                            }
                            else
                            {
                                Error("ENDIF without IF in REPEAT block");
                            }
                            break;

                        case o_END:
                            Error("END not allowed inside REPEAT block");
                            break;

                        case o_ENDM:
                            if (pass == 1 && labl[0])
                            {
                                AddMacroLine(&replist, labl);
                            }
                            break;

                        default:
                            if (pass == 1)
                            {
                                AddMacroLine(&replist, line);
                            }
                            break;
                    }
                    if (typ != o_ENDM)
                    {
                        i = TEXT_ReadSourceLine(line, sizeof(line));
                    }
                }

                if (macroCondLevel)
                {
                    Error("IF block without ENDIF in REPEAT block");
                }

                if (typ != o_REPEND)
                {
                    Error("Missing REPEND");
                }

                if (!errFlag)
                {
// *** while (val--)
// ***      for each line
// ***            doline()
                }

                // free repeat line buffer
                while (replist)
                {
                    rep = replist->next;
                    free(replist);
                    replist = rep;
                }
            }
            break;
#endif

        case OP_Incbin:
            SYM_Def(labl, locPtr, false, false);

            GetFName(word);

            val = 0;

            // open binary file
            incbin = fopen(word, "rb");

            if (incbin)
            {
                // while not EOF
                do
                {
                    //   n = count of read up to MAX_BYTSTR bytes into bytStr
                    n = fread(bytStr, 1, MAX_BYTSTR, incbin);
                    if (n > 0)
                    {
                        // write data out to the object file
                        for (int i = 0; i < n; i++)
                        {
                            OBJF_CodeOut(bytStr[i]);
                        }
                        val = val + n;
                    }
                }
                while (n > 0);
                if (n < 0)
                {
                    snprintf(s, sizeof s, "Error reading INCBIN file '%s'", word);
                }

                if (pass == 2)
                {
                    // "XXXX  (XXXX)"
                    p = LIST_Loc(locPtr-val);
                    *p++ = ' ';
                    *p++ = '(';
                    p = LIST_Addr(p, val);
                    *p++ = ')';
                }

            }
            else
            {
                snprintf(s, sizeof s, "Unable to open INCBIN file '%s'", word);
                ASMX_Error(s);
            }

            // close binary file
            if (incbin)
            {
                fclose(incbin);
            }
            incbin = NULL;

            break;

        case OP_WORDSIZE:
            if (labl[0])
            {
                ASMX_Error("Label not allowed");
            }

            val = EXPR_Eval();
            if (evalKnown)
            {
                ASMX_Error("Forward reference not allowed in WORDSIZE");
            }
            else if (!errFlag)
            {
                if (val == 0)
                {
                    SetWordSize(wordSize);
                }
                else if (val < 1 || val > 64)
                {
                    ASMX_Error("WORDSIZE must be in the range of 0..64");
                }
                else
                {
                    SetWordSize(val);
                }
            }
            break;

        default:
            ASMX_Error("Unknown opcode");
            break;
    }
}


static void ASMX_DoLine()
{
    Str255      labl;
    Str255      opcode;
    int         typ;
    int         parm;
    int         i;
    Str255      word;
    int         token;
    MacroRec    *macro;
    char        *oldLine;
    char        *p;
    int         numhex;
    bool        firstLine;

    errFlag      = false;
    warnFlag     = false;
    instrLen     = 0;
    showAddr     = false;
    listThisLine = listFlag;
    firstLine    = true;
    TEXT_CopyListLine();

    // skip initial formfeeds
    linePtr = line;
    while (*linePtr == 12)
    {
        linePtr++;
    }

    // handle Motorola-style comments with "*" in column 1
    if (*linePtr == '*')
    {
        linePtr = "";
    }

    // look for label at beginning of line
    labl[0] = 0;
#ifdef TEMP_LBLAT
    if (isalphaul(*linePtr) || *linePtr == '$' || *linePtr == '.' || *linePtr == '@')
#else
    if (isalphaul(*linePtr) || *linePtr == '$' || *linePtr == '.' || ((opts & OPT_ATSYM) && *linePtr == '@'))
#endif
    {
        token = TOKEN_GetWord(labl);
        oldLine = linePtr;
        if (token)
        {
            showAddr = true;
        }
        while (*linePtr == ' ' || *linePtr == '\t')
        {
            linePtr++;
        }

        if (labl[0])
        {
#ifdef TEMP_LBLAT
            if (token == '.' || token == '@')
#else
            if (token == '.')
#endif
            {
                TOKEN_GetWord(word);
                if (token == '.' && FindOpcodeTab((OpcdRec* ) &opcdTab2, word, &typ, &parm) )
                {
                    // ".pseudo-op" in column 1
                    linePtr = oldLine;
                }
                else if (token == '.' && *linePtr != ':' && FindCPU(word))
                {
                    // ".cputype" in column 1
                    // This must NOT be followed by a colon, to prevent temporary labels
                    // like ".1802:" and ".6800:" from being parsed as a CPU type!
                    linePtr = line;
                }
                else
                {
                    // make labl = lastLabl + "." + word;
                    if (token == '.' && subrLabl[0])
                    {
                        strcpy(labl, subrLabl);
                    }
                    else
                    {
                        strcpy(labl, lastLabl);
                    }
                    labl[strlen(labl)+1] = 0;
                    labl[strlen(labl)]   = token;
                    strcat(labl, word);
                }
            }
            else
            {
                strcpy(lastLabl, labl);
            }
        }

        if (*linePtr == ':' && linePtr[1] != '=')
        {
            linePtr++;
        }
    }

    if (!(condState[condLevel] & condTRUE))
    {
        listThisLine = false;

        // inside failed IF statement
        if (GetFindOpcode(opcode, &typ, &parm, &macro))
        {
            switch (typ)
            {
                case OP_IF: // nested IF inside failed IF should stay failed
                    if (condState[condLevel-1] & condTRUE)
                    {
                        listThisLine = listFlag;
                    }

                    if (condLevel >= MAX_COND)
                    {
                        ASMX_Error("IF statements nested too deeply");
                    }
                    else
                    {
                        condLevel++;
                        condState[condLevel] = condFAIL; // this level false and permanently failed
                    }
                    break;

                case OP_ELSE:
                    if (condState[condLevel-1] & condTRUE)
                    {
                        listThisLine = listFlag;
                    }

                    if (!(condState[condLevel] & (condTRUE | condFAIL)))
                    {
                        // previous IF was false
                        listThisLine = listFlag;

                        if (condLevel < MAX_COND && (condState[condLevel] & condELSE))
                        {
                            ASMX_Error("Multiple ELSE statements in an IF block");
                        }
                        else
                        {
                            condState[condLevel] |= condTRUE;
                        }
                    }
                    condState[condLevel] |= condELSE;
                    break;

                case OP_ELSIF:
                    if (condState[condLevel-1] & condTRUE)
                    {
                        listThisLine = listFlag;
                    }

                    if (!(condState[condLevel] & (condTRUE | condFAIL)))
                    {
                        // previous IF was false
                        listThisLine = listFlag;

                        if (condLevel < MAX_COND && (condState[condLevel] & condELSE))
                        {
                            ASMX_Error("Multiple ELSE statements in an IF block");
                        }
                        else
                        {
                            i = EXPR_Eval();
                            if (!errFlag && i != 0)
                            {
                                condState[condLevel] |= condTRUE;
                            }
                        }
                    }
                    break;

                case o_ENDIF:
                    if (condLevel == 0)
                    {
                        ASMX_Error("ENDIF outside of IF block");
                    }
                    else
                    {
                        condLevel--;
                        if (condState[condLevel] & condTRUE)
                        {
                            listThisLine = listFlag;
                        }
                    }
                    break;

                default:    // ignore any other lines
                    break;
            }
        }

        if ((pass == 2 || cl_ListP1) && listThisLine
                && (errFlag || listMacFlag || !macLineFlag))
        {
            TEXT_ListOut(true);
        }
    }
    else
    {
        if (!GetFindOpcode(opcode, &typ, &parm, &macro) && !opcode[0])
        {
            // line with label only
            SYM_Def(labl, locPtr / wordDiv, false, false);
        }
        else
        {
            if (typ == OP_Illegal)
            {
                if (opcode[0] == '.' && SetCPU(opcode+1))
                {
                    /* successfully changed CPU type */;
                }
                else
                {
                    if (labl[0])
                    {
                        // always define a label even if the opcode is bad
                        SYM_Def(labl, locPtr / wordDiv, false, false);
                    }
                    snprintf(word, sizeof word, "Illegal opcode '%s'", opcode);
                    ASMX_Error(word);
                }
            }
            else if (typ == OP_MacName)
            {
                if (macPtr[macLevel] && macLevel >= MAX_MACRO)
                {
                    ASMX_Error("Macros nested too deeply");
#if 1
                }
                else if (pass == 2 && !macro -> def)
                {
                    ASMX_Error("Macro has not been defined yet");
#endif
                }
                else
                {
                    if (macPtr[macLevel])
                    {
                        macLevel++;
                    }

                    macPtr [macLevel] = macro;
                    macLine[macLevel] = macro -> text;
#ifdef ENABLE_REP
                    macRepeat[macLevel] = 0;
#endif

                    GetMacParms(macro);

                    showAddr = true;
                    SYM_Def(labl, locPtr, false, false);
                }
            }
            else if (typ >= OP_LabelOp)
            {
                showAddr = false;
                ASMX_DoLabelOp(typ, parm, labl);
            }
            else
            {
                showAddr = true;
                SYM_Def(labl, locPtr, false, false);
                ASMX_DoOpcode(typ, parm);
            }

            if (typ != OP_Illegal && typ != OP_MacName)
            {
                if (!errFlag && TOKEN_GetWord(word))
                {
                    ASMX_Error("Too many operands");
                }
            }
        }

        if (pass == 1 && !cl_ListP1)
        {
            OBJF_AddLocPtr(abs(instrLen));
        }
        else
        {
            p = listLine;
            if (showAddr)
            {
                p = LIST_Loc(locPtr);
            }
            else
            {
                switch (addrWid)
                {
                    default:
                    case ADDR_16:
                        p = listLine + 5;
                        break;

                    case ADDR_24:
                        p = listLine + 7;
                        break;

                    case ADDR_32:
                        p = listLine + 9;
                        break;
                }
            }

            // determine width of hex data area
            if (listWid == LIST_16)
            {
                numhex = 5;
            }
            else
            {
                // listWid == LIST_24
                switch (addrWid)
                {
                    default:
                    case ADDR_16:
                    case ADDR_24:
                        numhex = 8;
                        break;

                    case ADDR_32:
                        numhex = 6;
                        break;
                }
            }

            if (instrLen > 0)   // positive instrLen for CPU instruction formatting
            {
                // determine start of hex data area
                switch (addrWid)
                {
                    default:
                    case ADDR_16:
                        if (listWid == LIST_24)
                        {
                            p = listLine + 6;
                        }
                        else
                        {
                            p = listLine + 5;
                        }
                        break;

                    case ADDR_24:
                        p = listLine + 7;
                        break;

                    case ADDR_32:
                        p = listLine + 9;
                        break;
                }

                // special case because 24-bit address usually can't fit
                // 8 bytes of code with operand spacing
                if (addrWid == ADDR_24 && listWid == LIST_24)
                {
                    numhex = 6;
                }

                if (hexSpaces & 1)
                {
                    *p++ = ' ';
                }
                for (int i = 0; i < instrLen; i++)
                {
                    if (listWid == LIST_24)
                    {
                        if (hexSpaces & (1 << i)) *p++ = ' ';
                    }

                    if (i < numhex || expandHexFlag)
                    {
                        if (i > 0 && i % numhex == 0)
                        {
                            if (listThisLine && (errFlag || listMacFlag || !macLineFlag))
                            {
                                TEXT_ListOut(firstLine);
                                firstLine = false;
                            }
                            if (listWid == LIST_24)
                            {
                                strcpy(listLine, "                        ");   // 24 blanks
                            }
                            else
                            {
                                strcpy(listLine, "                ");           // 16 blanks
                            }
                            p = LIST_Loc(locPtr);
                        }

                        p = LIST_Byte(p, bytStr[i]);
                    }
                    OBJF_CodeOut(bytStr[i]);
                }
            }
            else if (instrLen < 0)     // negative instrLen for generic data formatting
            {
                instrLen = abs(instrLen);
                for (int i = 0; i < instrLen; i++)
                {
                    if (i < numhex || expandHexFlag)
                    {
                        if (i > 0 && i % numhex == 0)
                        {
                            if (listThisLine && (errFlag || listMacFlag || !macLineFlag))
                            {
                                TEXT_ListOut(firstLine);
                                firstLine = false;
                            }
                            if (listWid == LIST_24)
                            {
                                strcpy(listLine, "                        ");   // 24 blanks
                            }
                            else
                            {
                                strcpy(listLine, "                ");           // 16 blanks
                            }
                            p = LIST_Loc(locPtr);
                        }
                        if (numhex == 6 && (i%numhex) == 2) *p++ = ' ';
                        if (numhex == 6 && (i%numhex) == 4) *p++ = ' ';
                        if (numhex == 8 && (i%numhex) == 4 && addrWid != ADDR_24) *p++ = ' ';
                        p = LIST_Byte(p, bytStr[i]);
                        if (i >= numhex) *p = 0;
                    }
                    OBJF_CodeOut(bytStr[i]);
                }
            }

            if (listThisLine && (errFlag || listMacFlag || !macLineFlag))
            {
                TEXT_ListOut(firstLine);
            }
        }
    }
}


static void ASMX_DoPass()
{
    Str255      opcode;
    int         typ;
    int         parm;

    fseek(source, 0, SEEK_SET); // rewind source file
    sourceEnd = false;
    lastLabl[0] = 0;
    subrLabl[0] = 0;

    if (cl_edtasm)
    {
        fprintf(stderr, "Pass %d\n", pass);

        if (cl_ListP1)
        {
            fprintf(listing, "Pass %d\n", pass);
        }
    }

    errCount      = 0;
    condLevel     = 0;
    condState[condLevel] = condTRUE; // top level always true
    listFlag      = true;
    listMacFlag   = false;
    expandHexFlag = true;
    symtabFlag    = true;
    tempSymFlag   = true;
    exactFlag     = false;
    linenum       = 0;
    macLevel      = 0;
    macUniqueID   = 0;
    macCurrentID[0] = 0;
    curAsm        = NULL;
    endian        = END_UNKNOWN;
    opcdTab       = NULL;
    listWid       = LIST_24;
    addrWid       = ADDR_32;
    wordSize      = 8;
    opts          = 0;
    SetWordSize(wordSize);
    SetCPU(defCPU);
    addrMax       = addrWid;

    // reset all code pointers
    OBJF_CodeAbsOrg(0);
    SegRec *seg = segTab;
    while (seg)
    {
        seg -> cod = 0;
        seg -> loc = 0;
        seg = seg -> next;
    }
    curSeg = nullSeg;

    if (pass == 2) OBJF_CodeHeader(cl_SrcName);

    PassInit();
    int i = TEXT_ReadSourceLine(line, sizeof(line));
    while (i && !sourceEnd)
    {
        ASMX_DoLine();
        i = TEXT_ReadSourceLine(line, sizeof(line));
    }

    if (condLevel != 0)
    {
        ASMX_Error("IF block without ENDIF");
    }

    if (pass == 2) OBJF_CodeEnd();

    // Put the lines after the END statement into the listing file
    // while still checking for listing control statements.  Ignore
    // any lines which have invalid syntax, etc., because whatever
    // is found after an END statement should esentially be ignored.

    if (pass == 2 || cl_ListP1)
    {
        while (i)
        {
            listThisLine = listFlag;
            TEXT_CopyListLine();

            if (line[0]==' ' || line[0]=='\t')          // ignore labels (this isn't the right way)
            {
                MacroRec    *macro;
                GetFindOpcode(opcode, &typ, &parm, &macro);
                if (typ == OP_LIST || typ == OP_OPT)
                {
                    ASMX_DoLabelOp(typ, parm, "");
                }
            }

            if (listThisLine)
            {
                TEXT_ListOut(true);
            }

            i = TEXT_ReadSourceLine(line, sizeof(line));
        }
    }
}

// --------------------------------------------------------------
// initialization and parameters


static void ASMX_stdversion(void)
{
    fprintf(stderr, "%s version %s\n", VERSION_NAME, VERSION);
    fprintf(stderr, "%s\n", COPYRIGHT);
}


static void ASMX_usage(void)
{
    ASMX_stdversion();
    fprintf(stderr, "\n");
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "    %s [options] srcfile\n", progname);
    fprintf(stderr, "\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "    --                  end of options\n");
    fprintf(stderr, "    -e                  show errors to screen\n");
    fprintf(stderr, "    -w                  show warnings to screen\n");
//  fprintf(stderr, "    -1                  output listing during first pass\n");
    fprintf(stderr, "    -l [filename]       make a listing file, default is srcfile.lst\n");
    fprintf(stderr, "    -o [filename]       make an object file, default is srcfile.hex or srcfile.s9\n");
    fprintf(stderr, "    -d label[[:]=value] define a label, and assign an optional value\n");
//  fprintf(stderr, "    -9                  output object file in Motorola S9 format (16-bit address)\n");
    fprintf(stderr, "    -s9                 output object file in Motorola S9 format (16-bit address)\n");
    fprintf(stderr, "    -s19                output object file in Motorola S9 format (16-bit address)\n");
    fprintf(stderr, "    -s28                output object file in Motorola S9 format (24-bit address)\n");
    fprintf(stderr, "    -s37                output object file in Motorola S9 format (32-bit address)\n");
    fprintf(stderr, "    -b [base[-end]]     output object file as binary with optional base/end addresses\n");
    fprintf(stderr, "    -t [reclen]         output object file in TRSDOS (implies -C Z80)\n");
    fprintf(stderr, "    -T [reclen]         output object file as TRS-80 cassette file (implies -C Z80)\n");
    fprintf(stderr, "    -c                  send object code to stdout\n");
    fprintf(stderr, "    -C cputype          specify default CPU type (currently ");
    if (defCPU[0])
    {
        fprintf(stderr, "%s", defCPU);
    }
    else
    {
        fprintf(stderr, "no default");
    }
    fprintf(stderr, ")\n");
    exit(1);
}


static void getopts(int argc, char * const argv[])
{
    int     ch;
    int     val;
    Str255  labl, word;
    bool    setSym;
    int     token;
    int     neg;

    while ((ch = getopt(argc, argv, "ew19t:T:b:cd:l:o:s:C:@?")) != -1)
    {
        errFlag = false;
        switch (ch)
        {
            case 'e':
                cl_Err = true;
                break;

            case 'w':
                cl_Warn = true;
                break;

            case '1':
                cl_ListP1 = true;
                break;

            case '9': // -9 option is deprecated
                cl_S9type  = 9;
                cl_ObjType = OBJ_S9;
                break;

            case 't':
                cl_ObjType = OBJ_TRSDOS;
                strcpy(defCPU, "Z80");

                if (!isdigit(optarg[0]))
                {
                    // -t with no parameter
                    optarg = "";
                    optind--;
                }
                else if (*optarg)
                {
                    // -t recsize
                    val = EvalNum(optarg);
                    if (errFlag)
                    {
                        fprintf(stderr, "%s: Invalid number '%s' in -t option\n", progname, word);
                        ASMX_usage();
                    }
                    if (1 <= val && val <= TRS_BUF_MAX)
                        cl_trslen = val;
                }
                break;

            case 'T':
                cl_ObjType = OBJ_TRSCAS;
                strcpy(defCPU, "Z80");

                if (!isdigit(optarg[0]))
                {
                    // -t with no parameter
                    optarg = "";
                    optind--;
                }
                else if (*optarg)
                {
                    // -t recsize
                    val = EvalNum(optarg);
                    if (errFlag)
                    {
                        fprintf(stderr, "%s: Invalid number '%s' in -t option\n", progname, word);
                        ASMX_usage();
                    }
                    if (1 <= val && val <= TRS_BUF_MAX)
                        cl_trslen = val;
                }
                break;

            case 's':
                if (optarg[0] == '9' && optarg[1] == 0)
                {
                    cl_S9type = 9;
                }
                else if (optarg[0] == '1' && optarg[1] == '9' && optarg[2] == 0)
                {
                    cl_S9type = 19;
                }
                else if (optarg[0] == '2' && optarg[1] == '8' && optarg[2] == 0)
                {
                    cl_S9type = 28;
                }
                else if (optarg[0] == '3' && optarg[1] == '7' && optarg[2] == 0)
                {
                    cl_S9type = 37;
                }
                else
                {
                    fprintf(stderr, "%s: Invalid S-record type '%s'\n", progname, optarg);
                    ASMX_usage();
                }
                cl_ObjType = OBJ_S9;
                break;

            case '@':
                cl_edtasm = true;
                break;

            case 'b':
                cl_ObjType = OBJ_BIN;
                cl_Binbase = 0;
                cl_Binend = 0xFFFFFFFF;

                if (!isdigit(optarg[0]))
                {
                    // -b with no parameter
                    optarg = "";
                    optind--;
                }
                else if (*optarg)
                {
                    // - b with parameter
                    strncpy(line, optarg, 255);
                    linePtr = line;

                    // get start parameter
                    if (TOKEN_GetWord(word) != -1)
                    {
                        fprintf(stderr, "%s: Invalid start argument '%s' for -b\n", progname, word);
                        ASMX_usage();
                    }
                    cl_Binbase = EvalNum(word);
                    if (errFlag)
                    {
                        fprintf(stderr, "%s: Invalid number '%s' in -b option\n", progname, word);
                        ASMX_usage();
                    }

                    // get optional end parameter
                    token = TOKEN_GetWord(word);
                    if (token)
                    {
                        if (token != '-')
                        {
                            fprintf(stderr, "%s: Invalid end argument '%s' for -b\n", progname, word);
                            ASMX_usage();
                        }

                        if (TOKEN_GetWord(word) != -1)
                        {
                            fprintf(stderr, "%s: Invalid end argument '%s' for -b\n", progname, word);
                            ASMX_usage();
                        }
                        cl_Binend = EvalNum(word);
                        if (errFlag)
                        {
                            fprintf(stderr, "%s: Invalid number '%s' in -b option\n", progname, word);
                            ASMX_usage();
                        }
                    }
                }
                break;

            case 'c':
                if (cl_Obj)
                {
                    fprintf(stderr, "%s: Conflicting options: -c can not be used with -o\n", progname);
                    ASMX_usage();
                }
                cl_Stdout = true;
                break;

            case 'd':
                strncpy(line, optarg, 255);
                linePtr = line;
                TOKEN_GetWord(labl);
                val = 0;
                setSym = false;
                token = TOKEN_GetWord(word);
                if (token == ':')
                {
                    // allow ":="
                    setSym = true;
                    token = TOKEN_GetWord(word);
                }
                if (token == '=')
                {
                    neg = 1;
                    if (TOKEN_GetWord(word) == '-')
                    {
                        neg = -1;
                        TOKEN_GetWord(word);
                    }
                    val = neg * EvalNum(word);
                    if (errFlag)
                    {
                        fprintf(stderr, "%s: Invalid number '%s' in -d option\n", progname, word);
                        ASMX_usage();
                    }
                }
                SYM_Def(labl, val, setSym, !setSym);
                break;

            case 'l':
                cl_List = true;
                if (optarg[0] == '-')
                {
                    optarg = "";
                    optind--;
                }
                strncpy(cl_ListName, optarg, 255);
                break;

            case 'o':
                if (cl_Stdout)
                {
                    fprintf(stderr, "%s: Conflicting options: -o can not be used with -c\n", progname);
                    ASMX_usage();
                }
                cl_Obj = true;
                if (optarg[0] == '-')
                {
                    optarg = "";
                    optind--;
                }
                strncpy(cl_ObjName, optarg, 255);
                break;

            case 'C':
                strncpy(word, optarg, 255);
                Uprcase(word);
                if (!FindCPU(word))
                {
                    fprintf(stderr, "%s: CPU type '%s' unknown\n", progname, word);
                    ASMX_usage();
                }
                strcpy(defCPU, word);
                break;

            case '?':
            default:
                ASMX_usage();
                break;
        }
    }
    argc -= optind;
    argv += optind;

    if (cl_Stdout && cl_ObjType == OBJ_BIN)
    {
        fprintf(stderr, "%s: Conflicting options: -b can not be used with -c\n", progname);
        ASMX_usage();
    }

#if 1
    // -b or -9 or -t must force -o!
    if (cl_ObjType != OBJ_HEX && !cl_Stdout && !cl_Obj)
    {
        cl_Obj = true;
    }
#endif

    // now argc is the number of remaining arguments
    // and argv[0] is the first remaining argument

    // error unless exactly one parameter left (for filename)
    if (argc != 1)
    {
        if (argc == 0)
        {
            fprintf(stderr, "%s: No filename found\n", progname);
        }
        else
        {
            fprintf(stderr, "%s: Unexpected argument '%s'\n", progname, argv[0]);
        }
        ASMX_usage();
    }

    strncpy(cl_SrcName, argv[0], 255);

    // print help if filename is '?'
    // note: this won't work if there's a single-char filename in the current directory!
    if (cl_SrcName[0] == '?' && cl_SrcName[1] == 0)
    {
        ASMX_usage();
    }

    if (cl_List && cl_ListName[0] == 0)
    {
        strncpy(cl_ListName, cl_SrcName, 255-4);
        strcat (cl_ListName, ".lst");
    }

    if (cl_Obj  && cl_ObjName [0] == 0)
    {
        switch (cl_ObjType)
        {
            case OBJ_S9:
                strncpy(cl_ObjName, cl_SrcName, 255-3);
                sprintf(word, ".s%d", cl_S9type);
                strcat (cl_ObjName, word);
                break;

            case OBJ_BIN:
                strncpy(cl_ObjName, cl_SrcName, 255-4);
                strcat (cl_ObjName, ".bin");
                break;

            case OBJ_TRSDOS:
                strncpy(cl_ObjName, cl_SrcName, 255-4);
                strcat (cl_ObjName, ".cmd");
                break;

            case OBJ_TRSCAS:
                strncpy(cl_ObjName, cl_SrcName, 255-4);
                strcat (cl_ObjName, ".cas");
                break;

            default:
            case OBJ_HEX:
                strncpy(cl_ObjName, cl_SrcName, 255-4);
                strcat (cl_ObjName, ".hex");
                break;
        }
    }
}


int main(int argc, char * const argv[])
{
    // initialize and get parms

    progname   = argv[0];
    pass       = 0;
    symTab     = NULL;
    xferAddr   = 0;
    xferFound  = false;

    macroTab   = NULL;
    macPtr[0]  = NULL;
    macLine[0] = NULL;
    segTab     = NULL;
    nullSeg    = SEG_Add("");
    curSeg     = nullSeg;

    cl_Err     = false;
    cl_Warn    = false;
    cl_List    = false;
    cl_Obj     = false;
    cl_ObjType = OBJ_HEX;
    cl_ListP1  = false;
    cl_trslen  = TRS_BUF_MAX;
    cl_edtasm  = false;

    asmTab     = NULL;
    cpuTab     = NULL;
    defCPU[0]  = 0;

    nInclude  = -1;
    for (int i = 0; i < MAX_INCLUDE; i++)
    {
        include[i] = NULL;
    }

    cl_SrcName [0] = 0;
    source  = NULL;
    cl_ListName[0] = 0;
    listing = NULL;
    cl_ObjName [0] = 0;
    object  = NULL;
    incbin = NULL;

    ASMX_AsmInit();

    getopts(argc, argv);

    // open files

    source = fopen(cl_SrcName, "r");
    if (source == NULL)
    {
        fprintf(stderr, "Unable to open source input file '%s'!\n", cl_SrcName);
        exit(1);
    }

    if (cl_List)
    {
        listing = fopen(cl_ListName, "w");
        if (listing == NULL)
        {
            fprintf(stderr, "Unable to create listing output file '%s'!\n", cl_ListName);
            if (source)
            {
                fclose(source);
            }
            exit(1);
        }
    }

    if (cl_Stdout)
    {
        object = stdout;
    }
    else if (cl_Obj)
    {
        if (cl_ObjType == OBJ_BIN || cl_ObjType == OBJ_TRSDOS)
        {
            object = fopen(cl_ObjName, "wb");
        }
        else
        {
            object = fopen(cl_ObjName, "w");
        }
        if (object == NULL)
        {
            fprintf(stderr, "Unable to create object output file '%s'!\n", cl_ObjName);
            if (source)
            {
                fclose(source);
            }
            if (listing)
            {
                fclose(listing);
            }
            exit(1);
        }
    }

    OBJF_CodeInit();

    pass = 1;
    ASMX_DoPass();

    pass = 2;
    ASMX_DoPass();

    if (cl_edtasm)
    {
        if (cl_List)    fprintf(listing, "\n%.5d Total Error(s)\n\n", errCount);
        if (cl_Err)     fprintf(stderr,  "\n%.5d Total Error(s)\n\n", errCount);
    }

    if (symtabFlag)
    {
        if (cl_List)    fprintf(listing, "\n");
        SYM_SortTab();
        SYM_DumpTab();
    }
//  DumpMacroTab();

    if (source)
    {
        fclose(source);
    }
    if (listing)
    {
        fclose(listing);
    }
    if (object && object != stdout)
    {
        fclose(object);
    }

    return (errCount != 0);
}
