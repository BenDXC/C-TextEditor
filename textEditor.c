#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
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
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
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