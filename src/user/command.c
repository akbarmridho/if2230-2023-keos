#include "command.h"
#include "sys/sys.h"

void command_input(char *buf, uint32_t size)
{
  puts_color("Keos@OS-IF2230", 0x2);
  puts_color(":", 0x8);
  puts_color("/", 0x1);
  puts_color("$ ", 0x8);
  gets(buf, size);
}