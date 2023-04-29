#ifndef COMMAND_H
#define COMMAND_H
#include "../lib-header/stdtype.h"
#include "../lib-header/string.h"
#include "../lib-header/math.h"
#include "../filesystem/ext2-api.h"

#define BLOCK_COUNT 16

void command_input(char *buf, char *currentdir, uint32_t size);

void next_arg(char **pstr, uint8_t *len);

void cp(struct EXT2DriverRequest *request, char *src, uint8_t src_len, char *dst, uint8_t dst_len, char *extdst, uint32_t parent);

void cpr(struct EXT2DriverRequest *request, char *src, uint8_t src_len, char *dst, uint8_t dst_len, char *extdst, uint32_t parent);

void time();

#endif
