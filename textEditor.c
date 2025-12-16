/** Imports */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
/** Defines */
#define CTRL_KEY(k) ((k) & 0x1f)
#define TEXTEDITOR_VERSION "0.0.1"
enum editorKey
{
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
/** Data */
struct editorConfig
{
  int cx, cy;
  int screenrows;
  int screencols;
  struct termios orig_termios;
};
struct editorConfig E;
/** Terminal */
void die(const char *s)
{
  perror(s);
  exit(1);
}
void disableRawMode()
{
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios);
}
void enableRawMode()
{
  tcgetattr(STDIN_FILENO, &E.orig_termios);
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
int editorReadKey()
{
  int nread;
  char c;
  while ((nread = read(STDIN_FILENO, &c, 1)) != 1)
  {
    if (nread == -1 && errno != EAGAIN)
      die("read");
  }
  if (c == '\x1b')
  {
    char seq[3];
    if (read(STDIN_FILENO, &seq[0], 1) != 1)
      return '\x1b';
    if (read(STDIN_FILENO, &seq[1], 1) != 1)
      return '\x1b';
    if (seq[0] == '[')
    {
      if (seq[1] >= '0' && seq[1] <= '9')
      {
        if (read(STDIN_FILENO, &seq[2], 1) != 1)
          return '\x1b';
        if (seq[2] == '~')
        {
          switch (seq[1])
          {
          case '1':
            return HOME_KEY;
          case '3':
            return DEL_KEY;
          case '4':
            return END_KEY;
          case '5':
            return PAGE_UP;
          case '6':
            return PAGE_DOWN;
          case '7':
            return HOME_KEY;
          case '8':
            return END_KEY;
          }
        }
      }
      else
      {
        switch (seq[1])
        {
        case 'A':
          return ARROW_UP;
        case 'B':
          return ARROW_DOWN;
        case 'C':
          return ARROW_RIGHT;
        case 'D':
          return ARROW_LEFT;
        case 'H':
          return HOME_KEY;
        case 'F':
          return END_KEY;
        }
      }
    }
    else if (seq[0] == 'O')
    {
      switch (seq[1])
      {
      case 'H':
        return HOME_KEY;
      case 'F':
        return END_KEY;
      }
    }
    return '\x1b';
  }
  else
  {
    return c;
  }
}
int getCursorPosition(int *rows, int *cols)
{
  char buf[32];       /* Buffer to store the response */
  unsigned int i = 0; /* Index for the buffer */
  if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4)
    return -1;
  while (i < sizeof(buf) - 1)
  {
    if (read(STDIN_FILENO, &buf[i], 1) != 1)
      break;
    if (buf[i] == 'R')
      break;
    i++;
  }
  buf[i] = '\0';
  if (buf[0] != '\x1b' || buf[1] != '[')
    return -1;
  if (sscanf(&buf[2], "%d;%d", rows, cols) != 2)
    return -1;
  return 0;
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
/*** Append Buffer ***/
struct abuf
{
  char *b;
  int len;
};
#define ABUF_INIT {NULL, 0}
void abAppend(struct abuf *ab, const char *s, int len)
{
  char *new = realloc(ab->b, ab->len + len); /* Reallocate memory for the buffer */
  if (new == NULL)
    return;
  memcpy(&new[ab->len], s, len); /* Copy the new string to the end of the buffer */
  ab->b = new;
  ab->len += len; /* Update the length of the buffer */
}
void abFree(struct abuf *ab)
{
  free(ab->b); /* Free the allocated memory */
}
/** Output */
void editorDrawRows(struct abuf *ab)
{
  int y;
  for (y = 0; y < E.screenrows; y++)
  {
    if (y == E.screenrows / 3)
    {
      char welcome[80];
      int welcomelen = snprintf(welcome, sizeof(welcome),
                                "C Text Editor -- version %s", TEXTEDITOR_VERSION);
      if (welcomelen > E.screencols)
        welcomelen = E.screencols;
      int padding = (E.screencols - welcomelen) / 2;
      if (padding)
      {
        abAppend(ab, "~", 1);
        padding--;
      }
      while (padding--)
        abAppend(ab, " ", 1);
      abAppend(ab, welcome, welcomelen);
    }
    else
      abAppend(ab, "~", 1);
  }
  abAppend(ab, "\x1b[K", 3); /* Clear the line from cursor to the end */
  if (y < E.screenrows - 1)
    abAppend(ab, "\r\n", 2);
}
void editorRefreshScreen()
{
  struct abuf ab = ABUF_INIT;
  abAppend(&ab, "\x1b[?25l", 6); /* Hide the cursor */
  abAppend(&ab, "\x1b[H", 3);    /* Move the cursor to the top-left corner */

  editorDrawRows(&ab);

  char buf[32];
  snprintf(buf, sizeof(buf), "\x1b[%d;%dH", E.cy + 1, E.cx + 1);
  abAppend(&ab, buf, strlen(buf)); /* Move the cursor to its current position */

  abAppend(&ab, "\x1b[?25h", 6);      /* Show the cursor */
  write(STDOUT_FILENO, ab.b, ab.len); /* Write the buffer to the standard output */
  abFree(&ab);                        /* Free the allocated memory for the buffer */
}
/*** Input */
void editorMoveCursor(int key)
{
  switch (key)
  {
  case ARROW_LEFT:
    if (E.cx != 0)
      E.cx--;
    break;
  case ARROW_RIGHT:
    if (E.cx != E.screencols - 1)
      E.cx++;
    break;
  case ARROW_UP:
    if (E.cy != 0)
      E.cy--;
    break;
  case ARROW_DOWN:
    if (E.cy != E.screenrows - 1)
      E.cy++;
    break;
  }
}
void editorProcessKeypress()
{
  int c = editorReadKey();
  switch (c)
  {
  case CTRL_KEY('q'):
    write(STDOUT_FILENO, "\x1b[2J", 4); /* Clear the entire screen */
    write(STDOUT_FILENO, "\x1b[H", 3);  /* Move the cursor to the top-left corner */
    exit(0);
    break;

  case HOME_KEY:
    E.cx = 0;
    break;
  case END_KEY:
    E.cx = E.screencols - 1;
    break;

  case PAGE_UP:
  case PAGE_DOWN:
  {
    int times = E.screenrows;
    while (times--)
      editorMoveCursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
  }
  break;

  case ARROW_UP:
  case ARROW_DOWN:
  case ARROW_LEFT:
  case ARROW_RIGHT:
    editorMoveCursor(c);
    break;
  }
}
/** Init System */
void initEditor()
{
  E.cx = 0; // Initialize cursor x position
  E.cy = 0; // Initialize cursor y position
  if (getWindowSize(&E.screenrows, &E.screencols) == -1)
    die("getWindowSize");
}
int main()
{
  enableRawMode();

  while (1)
  {
    char c;
    read(STDIN_FILENO, &c, 1);
    if (iscntrl(c))
    {
      printf("%d\r\n", c); /* Print control characters as their integer values */
    }
    else
    {
      printf("%d ('%c')\r\n", c, c); /* Print printable characters as their integer values and characters */
    }
    if (c == 'q')
      break; /* Exit on 'q' */
  }
  return 0;
}