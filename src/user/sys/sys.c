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

int8_t sys_delete(struct EXT2DriverRequest *request)
{
  uint32_t ret;
  syscall(4, (uint32_t)request, (uint32_t)&ret, 0);
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

void *malloc(uint32_t size)
{
  uint32_t retval;

  syscall(7, (uint32_t)&retval, size, 0);

  return (void *)retval;
}

void *realloc(void *ptr, uint32_t size)
{
  uint32_t retval;

  syscall(13, (uint32_t)&retval, (uint32_t)ptr, size);

  return (void *)retval;
}

bool free(void *ptr)
{
  bool retval;

  syscall(8, (uint32_t)&retval, (uint32_t)ptr, 0);

  return retval;
}

void get_text(char *buf, uint32_t size, uint32_t *result_size)
{
  syscall(9, (uint32_t)buf, size, (uint32_t)result_size);
}

void clear_screen(void)
{
  syscall(10, 0, 0, 0);
}

void get_keyboard_events(struct KeyboardEvents *events)
{
  syscall(11, (uint32_t)events, 0, 0);
}

void reset_keyboard_events(void)
{
  syscall(12, 0, 0, 0);
}