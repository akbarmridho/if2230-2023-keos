#include "stdtype.h"

int strcmp(const char *stra, const char *strb, int size);

char *strcpy(const char *src, char *dest);

int strlen(const char *str);

bool is_equal(const char *stra, const char *strb);

void append(const char *stra, const char *strb, char *destination);

void append3(const char *stra, const char *strb, const char *strc, char *destination);

void append_path(const char *stra, const char *strb, char *destination);

/**
 * C++ version 0.4 char* style "itoa":
 * Written by Lukás Chmela
 * Released under GPLv3.
 *
 * This is a modified version with base parameter removed
 * Modified by Akbar
 */
void itoa(int32_t value, char *result);