#ifndef COMMAND_H
#define COMMAND_H
#include "../lib-header/stdtype.h"

void command_input(char *buf, char *currentdir, uint32_t size);

void next_arg(char **pstr, uint8_t *len);

#endif