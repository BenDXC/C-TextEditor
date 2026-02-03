#ifndef EDITOR_H
#define EDITOR_H

/** OS Sources **/
#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

/*** Includes ***/
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

/** Defines **/
#define CTRL_KEY(k) ((k) & 0x1f)
#define TEXTEDITOR_VERSION "0.0.1"
#define TEXTEDITOR_TAB_STOP 8
#define TEXTEDITOR_QUIT_TIMES 3

enum editorKey {
    BACKSPACE = 127,
    ARROW_LEFT = 1000,
    ARROW_RIGHT,
    ARROW_UP,
    ARROW_DOWN,
    PAGE_UP,
    PAGE_DOWN,
    HOME_KEY,
    END_KEY,
    DEL_KEY,
};

/** Data **/
typedef struct erow {
    int size;
    int rsize;
    char *chars;
    char *render;
} erow;

struct editorConfig {
    int cx, cy;              /* Cursor positions */
    int rx;                   /* Render x position */
    int rowoff;               /* Row offset */
    int coloff;               /* Column offset */
    int screenrows;           /* Number of screen rows */
    int screencols;           /* Number of screen columns */
    int numrows;              /* Number of rows of text */
    erow *row;                /* Array of rows */
    int dirty;                /* Flag for unsaved changes */
    char *filename;           /* Name of the file */
    char statusmsg[80];       /* Status message */
    time_t statusmsg_time;    /* Time for status message */
    struct termios orig_termios;  /* Original terminal settings */
};

extern struct editorConfig E;  /* Global editor configuration */

/** Prototypes **/
void editorSetStatusMessage(const char *fmt, ...);
void editorRefreshScreen();
char *editorPrompt(char *prompt, void (*callback)(char *, int));

/** Terminal functions **/
void die(const char *s);
void disableRawMode();
void enableRawMode();
int editorReadKey();
int getCursorPosition(int *rows, int *cols);
int getWindowSize(int *rows, int *cols);

/*** Row Operations ***/
int editorRowCxToRx(erow *row, int cx);
int editorRowRxToCx(erow *row, int rx);
void editorUpdateRow(erow *row);
void editorInsertRow(int at, char *s, size_t len);
void editorAppendRow(char *s, size_t len);
void editorFreeRow(erow *row);
void editorRowDelChar(erow *row, int at);
void editorDelRow(int at);
void editorRowInsertChar(erow *row, int at, int c);

/*** Editor Operations ***/
void editorInsertChar(int c);
void editorInsertNewline();
void editorRowAppendString(erow *row, char *s, size_t len);
void editorDelChar();

/*** File I/O ***/
char *editorRowsToString(int *buflen);
void editorOpen(char *filename);
void editorSave();

/*** Find ***/
void editorFindCallback(char *query, int key);
void editorFind();

/*** Append Buffer ***/
struct abuf {
    char *b;
    int len;
};
#define ABUF_INIT {NULL, 0}
void abAppend(struct abuf *ab, const char *s, int len);
void abFree(struct abuf *ab);

/** Output **/
void editorScroll();
void editorDrawRows(struct abuf *ab);
void editorDrawStatusBar(struct abuf *ab);
void editorDrawMessageBar(struct abuf *ab);
void editorRefreshScreen();
void editorSetStatusMessage(const char *fmt, ...);

/*** Input ***/
char *editorPrompt(char *prompt, void (*callback)(char *, int));
void editorMoveCursor(int key);
void editorProcessKeypress();

/** Init System **/
void initEditor();

#endif /* EDITOR_H */