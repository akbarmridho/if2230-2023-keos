#ifndef SYS_H
#define SYS_H
#include "../../lib-header/stdtype.h"
#include "../../filesystem/ext2-api.h"

void puts_color(const char *str, uint8_t color);
void puts(const char *str);

void syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx);

void gets(char *buf, uint16_t size);

int8_t sys_read(struct EXT2DriverRequest *request);
int8_t sys_read_directory(struct EXT2DriverRequest *request);

int8_t sys_write(struct EXT2DriverRequest *request);

int8_t sys_read_next_directory(struct EXT2DriverRequest *request);
#endif