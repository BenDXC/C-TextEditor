#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
struct termios orig_termios;
void disableRawMode()
{
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}
void enableRawMode()
{
  tcgetattr(STDIN_FILENO, &orig_termios);
  atexit(disableRawMode);
  struct termios raw = orig_termios;
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
  for (int y = 0; y < E.screenrows; y++)
  {
    abAppend(ab, "~", 1);
    if (y < E.screenrows - 1)
      abAppend(ab, "\r\n", 2);
  }
}
void editorRefreshScreen()
{
  struct abuf ab = ABUF_INIT;
  abAppend(&ab, "\x1b[?25l", 6); /* Hide the cursor */
  abAppend(&ab, "\xlb[2J", 4);   /* Clear the entire screen */
  abAppend(&ab, "\x1b[H", 3);    /* Move the cursor to the top-left corner */

  editorDrawRows(&ab);
  abAppend(&ab, "\x1b[?25h", 6); /* Show the cursor */
  abAppend(&ab, "\x1b[H", 3);    /* Move the cursor to the top-left corner */

  write(STDOUT_FILENO, ab.b, ab.len); /* Write the buffer to the standard output */
  abFree(&ab);                        /* Free the allocated memory for the buffer */
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