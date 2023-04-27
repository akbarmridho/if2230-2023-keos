#include "command.h"
#include "sys/sys.h"

void command_input(char *buf, char *currentdir, uint32_t size)
{
  puts_color("Keos@OS-IF2230", 0x2);
  puts_color(":", 0x7);
  puts_color(currentdir, 0x9);
  puts_color("$ ", 0x7);
  gets(buf, size);
}

void next_arg(char **pstr, uint8_t *len)
{
  *len = 0;
  while ((**pstr) == ' ')
  {
    if (**pstr == '\0')
      return;
    *pstr += 1;
  }
  while ((*pstr)[*len] != ' ' && (*pstr)[*len] != '\0')
  {
    *len += 1;
  }
}