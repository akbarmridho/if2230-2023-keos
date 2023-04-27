#include "sys.h"
#include "../../lib-header/string.h"

void syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx)
{
  __asm__ volatile("mov %0, %%ebx"
                   : /* <Empty> */
                   : "r"(ebx));
  __asm__ volatile("mov %0, %%ecx"
                   : /* <Empty> */
                   : "r"(ecx));
  __asm__ volatile("mov %0, %%edx"
                   : /* <Empty> */
                   : "r"(edx));
  __asm__ volatile("mov %0, %%eax"
                   : /* <Empty> */
                   : "r"(eax));
  // Note : gcc usually use %eax as intermediate register,
  //        so it need to be the last one to mov
  __asm__ volatile("int $0x30");
}

void puts_color(const char *str, uint8_t color)
{
  syscall(6, (uint32_t)str, strlen(str), color);
}

void puts(const char *str)
{
  puts_color(str, 0xF);
}

void gets(char *buf, uint16_t size)
{
  syscall(5, (uint32_t)buf, size, 0);
}

int8_t sys_read(struct EXT2DriverRequest *request)
{
  uint32_t ret;
  syscall(0, (uint32_t)request, (uint32_t)&ret, 0);
  return ret;
}

int8_t sys_write(struct EXT2DriverRequest *request)
{
  uint32_t ret;
  syscall(3, (uint32_t)request, (uint32_t)&ret, 0);
  return ret;
}

int8_t sys_read_directory(struct EXT2DriverRequest *request)
{
  uint32_t ret;
  syscall(1, (uint32_t)request, (uint32_t)&ret, 0);
  return ret;
}

int8_t sys_read_next_directory(struct EXT2DriverRequest *request)
{
  uint32_t ret;
  syscall(2, (uint32_t)request, (uint32_t)&ret, 0);
  return ret;
}