#ifndef SYS_H
#define SYS_H
#include "../../lib-header/stdtype.h"
#include "../../filesystem/ext2-api.h"
#include "../../lib-header/event.h"

void puts_color(const char *str, uint8_t color);
void puts(const char *str);

void syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx);

void gets(char *buf, uint16_t size);

int8_t sys_read(struct EXT2DriverRequest *request);
int8_t sys_read_directory(struct EXT2DriverRequest *request);

int8_t sys_write(struct EXT2DriverRequest *request);

int8_t sys_read_next_directory(struct EXT2DriverRequest *request);

void *malloc(uint32_t size);

void *realloc(void *ptr, uint32_t size);

bool free(void *ptr);

void get_text(char *buf, uint32_t size, uint32_t *result_size);

void clear_screen(void);

void get_keyboard_events(struct KeyboardEvents *events);

void reset_keyboard_events(void);

#endif