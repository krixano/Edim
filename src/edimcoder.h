#ifndef LINEEDITOR_H
#define LINEEDITOR_H

#ifndef __APPLE__
#include <malloc.h>
#endif

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>

#define true 1
#define false 0
#define bool char
#define forever for(;;)

#define internal static

void clrscr();

#ifdef _WIN32
#include <conio.h>
#define getch _getch
#define kbhit _kbhit
#else
#include <alloca.h>
#include <unistd.h>
#include <termios.h>
char getch();
char getch_nonblocking();
#endif

typedef enum State {
    KEEP, EXIT, FORCE_EXIT, QUIT, FORCE_QUIT
} State;

/* === editor.c === */
#define MAXLENGTH 900 /* 2000000 was too big on Windows */

State editorState_menu(void);
void editorState_editor(void);

void printText(int startLine);
void printLine(int line, char operation, int printNewLine);
void printFileInfo(void);

/* == Streatchy Buffers (by Sean Barratt) === */

#define MAX(x, y) ((x) >= (y) ? (x) : (y))
#define CLAMP_MAX(x, max) MIN(x, max)
#define CLAMP_MIN(x, min) MAX(x, min)
#define IS_POW2(x) (((x) != 0) && ((x) & ((x)-1)) == 0)

void *xcalloc(size_t num_elems, size_t elem_size);
void *xrealloc(void *prt, size_t num_bytes);
void *xmalloc(size_t num_bytes);
void fatal(const char *fmt, ...);

typedef struct BufHdr {
    size_t len;
    size_t cap;
    char buf[0]; // [0] new in c99
} BufHdr;

#define buf__hdr(b) ((BufHdr *) ((char *) (b)  - offsetof(BufHdr, buf)))
#define buf__fits(b, n) (buf_len(b) + (n) <= buf_cap(b))
#define buf__fit(b, n) (buf__fits((b), (n)) ? 0 : ((b) = buf__grow((b), buf_len(b) + (n), sizeof(*(b)))))

#define buf_len(b) ((b) ? buf__hdr(b)->len : 0)
#define buf_cap(b) ((b) ? buf__hdr(b)->cap : 0)
#define buf_push(b, x) (buf__fit((b), 1), (b)[buf__hdr(b)->len++] = (x))
#define buf_end(b) ((b) + buf_len(b))

#define buf_add(b, n) (buf__fit((b), n), buf__hdr(b)->len += n, &(b)[buf__hdr(b)->len - n]) // TODO: Not sure if I should be returning the address or not
#define buf_pop(b) (buf__hdr(b)->len--, &(b)[buf__hdr(b)->len + 1]) // TODO: Check that array exists and length doesn't go below 0
#define buf_pop_all(b) (buf__hdr(b)->len = 0)

#define buf_free(b) ((b) ? (free(buf__hdr(b)), (b) = NULL) : 0)

void *buf__grow(const void *buf, size_t new_len, size_t elem_size);

/* === Hash Map === */

uint64_t hash_uint64(uint64_t x);
uint64_t hash_ptr(const void *prt);
uint64_t hash_mix(uint64_t x, uint64_t y);
uint64_t hash_bytes(const void *ptr, size_t len);

typedef struct Map {
    uint64_t *keys;
    uint64_t *vals;
    size_t len;
    size_t cap;
} Map;

uint64_t map_get_uint64_from_uint64(Map *map, uint64_t key);
void map_put_uint64_from_uint64(Map *map, uint64_t key, uint64_t val);
void map_grow(Map *map, size_t new_cap);
void *map_get(Map *map, const void *key);
void map_put(Map *map, const void *key, void *val);
void *map_get_from_uint64(Map *map, uint64_t key);
void map_put_from_uint64(Map *map, uint64_t key, void *val);
uint64_t map_get_uint64(Map *map, void *key);
void map_put_uint64(Map *map, void *key, uint64_t val);
void map_test(void);

/* === buffer.c - Text Editing Data Structures === */

typedef enum FileType {
    FT_UNKNOWN, FT_TEXT, FT_MARKDOWN, FT_C, FT_CPP, FT_C_HEADER // TODO: Add Batch and Bash files
} FileType;

void getFileTypeExtension(FileType ft, char **ftExt);

typedef struct Line {
    char *chars;
} Line;

typedef enum OperationKind {
    Undo, InsertAfter, InsertBefore, AppendTo, PrependTo, ReplaceLine, ReplaceString, DeleteLine
} OperationKind;

typedef struct Operation {
    OperationKind kind;
    int *lines; // Line(s) that have been modified/added/deleted by the operation
    union {
        Line original; // The original line that was modified/deleted - TODO: Not very memory efficient
    };
} Operation;

// Levels:
//  0 for #
//  1 for ##
//  ...
typedef struct MarkdownOutlineNode {
    Line *line;
    int lineNum;
    int level;
} MarkdownOutlineNode;

typedef struct COutlineNode {
    Line *line;
    int lineNum;
} COutlineNode;

typedef struct Bookmark Bookmark;

typedef struct Buffer {
    char *openedFilename; // char Stretchy buffer for the currently opened filename
    FileType fileType;
    Line *lines;
    Operation lastOperation;
    Bookmark *bookmarks;
    // Used by default when no line passed into a command.
    // Commands that modify the file will change the currentLine to the last line it modified. Some commands, like 'c', don't modify the file based on the current line, but will change the current line to what it's modifying ('c' will change the current line to the last line in the file and start inserting from there).
    int currentLine;
    bool modified;
    union outline {
        void *nodes;
        MarkdownOutlineNode *markdown_nodes;
        COutlineNode *c_nodes;
    } outline;
} Buffer;

// Stretchy buffer of Buffers
Buffer *buffers;
Buffer *currentBuffer;

void buffer_initEmptyBuffer(Buffer *buffer);
int buffer_openFile(Buffer *buffer, char *filename);
void buffer_saveFile(Buffer *buffer, char *filename);
void buffer_close(Buffer *buffer);

int buffer_insertAfterLine(Buffer *buffer, int line, Line *lines);
int buffer_insertBeforeLine(Buffer *buffer, int line, Line *lines);
void buffer_appendToLine(Buffer *buffer, int line, char *chars);
void buffer_prependToLine(Buffer *buffer, int line, char *chars);
void buffer_replaceLine(Buffer *buffer, int line, char *chars);
// TODO: This will basically just delete the lines and then insert before the line number
// void buffer_replaceLines(Buffer *buffer, int lineStart, int lineEnd, Line *lines);
void buffer_replaceInLine(Buffer *buffer, int line, int startIndex, int endIndex, char *chars);
// TODO: Add the number to move up by
void buffer_moveLineUp(Buffer *buffer, int line);
// TODO: Add the number to move up by
void buffer_moveLineDown(Buffer *buffer, int line);
void buffer_deleteLine(Buffer *buffer, int line);

// TODO
// void buffer_deleteLines(Buffer *buffer, int lineStart, int lineEnd);
int buffer_findStringInLine(Buffer *buffer, int line, char *str, int strLength);
int buffer_findStringInFile(Buffer *buffer, char *str, int strLength, int *colIndex);


/* === parsing.c === */

typedef struct pString {
    char *start;
    char *end;
} pString;

typedef struct lineRange {
    int start;
    int end;
} lineRange;

typedef struct Bookmark {
    char *name; // Dynamic Array Buffer
    lineRange range;
} Bookmark;

// Function pointer to function that can run user-code on specific keypresses during input (with getInput). If null, the function is not called
// Return true if keypress should continue to use default action provided by getInput()
typedef bool (*inputKeyCallback)(char, bool isSpecial, char **, int *);

#define INPUT_ESC 27

// ANSI Control Characters
#define INPUT_CTRL_L 12 // Clear Scrren
#define INPUT_CTRL_X 24 // Cancel
#define INPUT_CTRL_C 3 // Currently: Exit Program, TODO: Exit or Copy?
#define INPUT_CTRL_O 15

// Special Keys
#ifdef _WIN32
#define INPUT_SPECIAL1 -32
#define INPUT_SPECIAL2 224 // TODO
#define INPUT_LEFT 75
#define INPUT_RIGHT 77
#define INPUT_DELETE 83
#define INPUT_END 79
#define INPUT_HOME 71
#define INPUT_ENDINPUT 26 // CTRL-Z
#define INPUT_BACKSPACE '\b'
#else
#define INPUT_SPECIAL1 27
#define INPUT_SPECIAL2 91
#define INPUT_LEFT 68
#define INPUT_RIGHT 67
#define INPUT_DELETE1 51
#define INPUT_DELETE2 126
#define INPUT_END 70
#define INPUT_HOME 72
#define INPUT_ENDINPUT 4 // CTRL-D
#define INPUT_BACKSPACE 127
#endif

char *skipWhitespace(char *start, char *endBound);
char *skipWord(char *start, char *endBound, bool includeNumbers, bool includeSymbols);
char *skipNumbers(char *start, char *endBound);
char *skipLineNumber(char *start, char *endBound);

long parseLineNumber(Buffer *buffer, char *start, char *endBound);

char *getInput(bool *canceled, char *inputBuffer, inputKeyCallback callback);
int parsing_getLine(char *line, int max, int trimSpace);
int parsing_getLine_dynamic(char **chars, int trimSpace);

void createOutline(void);
void recreateOutline(void);
void showOutline(void);

void createMarkdownOutline(void);
void createCOutline(void);
void showMarkdownOutline(void);
void showCOutline(void);

// Returns if bookmark was found with result_bookmark changed to a pointer
// to it.
bool get_bookmark(Buffer *buffer, pString name, Bookmark **result_bookmark);
// Returns true if updated existing bookmark, false otherwise
bool add_bookmark(Buffer *buffer, pString name, lineRange range);

/* === Colors === */

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define FOREGROUND_YELLOW FOREGROUND_RED|FOREGROUND_GREEN
#define FOREGROUND_CYAN FOREGROUND_GREEN|FOREGROUND_BLUE
#define FOREGROUND_MAGENTA FOREGROUND_RED|FOREGROUND_BLUE
#define FOREGROUND_WHITE FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE

#define BACKGROUND_YELLOW BACKGROUND_RED|BACKGROUND_GREEN
#define BACKGROUND_CYAN BACKGROUND_GREEN|BACKGROUND_BLUE
#define BACKGROUND_MAGENTA BACKGROUND_RED|BACKGROUND_BLUE
#define BACKGROUND_WHITE BACKGROUND_RED|BACKGROUND_GREEN|BACKGROUND_BLUE

#endif

#define COL_RED "\x1b[31m" // Error
#define COL_GREEN "\x1b[32m" // Prompt, Success?
#define COL_YELLOW "\x1b[33m" // Line Numbers
#define COL_BLUE "\x1b[34m"
#define COL_MAGENTA "\x1b[35m"
#define COL_CYAN "\x1b[36m" // Information
#define COL_RESET "\x1b[0m" // Input

typedef enum COLOR
{
    COLOR_RED,
    COLOR_GREEN,
    COLOR_BLUE,
    COLOR_YELLOW,
    COLOR_CYAN,
    COLOR_MAGENTA,
    COLOR_WHITE,
    COLOR_BLACK
} COLOR;

#ifdef _WIN32
HANDLE hConsole; // Used for coloring output on Windows
#endif

void setColor(COLOR foreground);
void resetColor(void);
void colors_printf(COLOR foreground, const char *fmt, ...);
void colors_puts(COLOR foreground, const char *fmt, ...);
void printError(const char *fmt, ...);
void printPrompt(const char *fmt, ...);
void printLineNumber(const char *fmt, ...);

#endif
