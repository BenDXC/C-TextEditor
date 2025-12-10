/** includes */
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
/** Defines */
#define CTRL_KEY(k) ((k) & 0x1f) /* Macro to map a character to its control key equivalent */
/** Data */
struct editorConfig
{
  int screenrows;
  int screencols;
  struct termios orig_termios;
};
struct editorConfig E;       /* Global editor configuration */
struct termios orig_termios; /* To store original terminal attributes */
/** Terminal */
void die(const char *s)
{
  perror(s);
  exit(1);
}
void disableRawMode()
{
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1 ? die("tcsetattr") : 0;
}
void enableRawMode()
{
  tcgetattr(STDIN_FILENO, &E.orig_termios) == -1 ? die("tcgetattr") : 0;
  atexit(disableRawMode);
  struct termios raw = E.orig_termios;
  raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);          /* Disable echoing, canonical mode, signals, and extended input processing*/
  raw.c_iflag &= ~(IXON | ICRNL | BRKINT | INPCK | ISTRIP); /* Disable software flow control, carriage return to newline translation, and other input flags*/
  raw.c_cflag |= (CS8);                                     /* Set character size to 8 bits per byte*/
  raw.c_cc[VMIN] = 0;                                       /* Minimum number of bytes of input needed before read() can return*/
  raw.c_cc[VTIME] = 1;                                      /* Maximum time to wait before read() returns*/
  raw.c_oflag &= ~(OPOST);                                  /* Disable all output processing*/
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1 ? die("tcsetattr") : 0;
}
char editorReadKey()
{
  int nread;
  char c;
  while ((nread = read(STDIN_FILENO, &c, 1)) != 1)
  {
    if (nread == -1 && errno != EAGAIN)
      die("read");
  }
  return c;
}
int getCursorPosition(int *rows, int *cols)
{
  if (write(STDOUT_FILENO, "\X1B[6n", 4) != 4)
    return -1;
  printf("\r\n");
  char c;
  while (read(STDIN_FILENO, &C, 1) == 1)
  {
    if (iscntrl(c))
    {
      printf("%d\r\n", c);
    }
    else
    {
      printf("%d ('%c')\r\n", c, c);
    }
  }
  editorReadKey();
  return -1;
}
int getWindowSize(int *rows, int *cols)
{
  struct winsize ws;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0)
  {
    if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12)
      return -1;
    return getCursorPosition(rows, cols);
  }
  else
  {
    *cols = ws.ws_col;
    *rows = ws.ws_row;
    return 0;
  }
}
/** Output */
void editorDrawRows()
{
  for (int y = 0; y < E.screenrows; y++)
  {
    write(STDOUT_FILENO, "~\r\n", 3);
  }
}
void editorRefreshScreen()
{
  write(STDOUT_FILENO, "\x1b[2J", 4); /* Clear the entire screen */
  write(STDOUT_FILENO, "\x1b[H", 3);  /* Move the cursor to the top-left corner */
  editorDrawRows();
  write(STDOUT_FILENO, "\x1b[H", 3); /* Move the cursor to the top-left corner */
}
/*** Input */
void editorProcessKeypress()
{
  char c = editorReadKey();
  switch (c)
  {
  case CTRL_KEY('q'):
    write(STDOUT_FILENO, "\x1b[2J", 4); /* Clear the entire screen */
    write(STDOUT_FILENO, "\x1b[H", 3);  /* Move the cursor to the top-left corner */
    exit(0);
    break;
  }
}
/** Init System */
void initEditor()
{
  if (getWindowSize(&E.screenrows, &E.screencols) == -1)
    die("getWindowSize");
}
int main()
{
  enableRawMode();
  initEditor();
  while (1)
  {
    editorRefreshScreen();
    editorProcessKeypress();
  }
  return 0;
}