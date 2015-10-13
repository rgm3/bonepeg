#include <sys/ioctl.h>
#include <stdio.h>

int main(int argc, char **argv)
{
  struct winsize sz;

  ioctl(0, TIOCGWINSZ, &sz);
  printf("Screen width: %i  Screen height: %i\n", sz.ws_col, sz.ws_row);
  return 0;
} 
