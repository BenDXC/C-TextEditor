/** includes */
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
/** Defines */
#define CTRL_KEY(k) ((k) & 0x1f) /* Macro to map a character to its control key equivalent */
/** Data */
struct termios orig_termios;
/** Terminal */
void die(const char *s)
{
  perror(s);
  exit(1);
}
void disableRawMode()
{
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1 ? die("tcsetattr") : 0;
}
void enableRawMode()
{
  tcgetattr(STDIN_FILENO, &orig_termios) == -1 ? die("tcgetattr") : 0;
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
/*** Input */
void editorProcessKeypress()
{
  char c = editorReadKey();
  if (iscntrl(c))
  {
    printf("%d\r\n", c); /* Print control characters as their integer values */
  }
  else
  {
    printf("%d ('%c')\r\n", c, c); /* Print printable characters as their integer values and characters */
  }
}
/** Init System */
int main()
{
  enableRawMode();
  while (1)
  {
    editorProcessKeypress();
  }
  return 0;
}