#include "../lib-header/stdtype.h"

/**
 * Wait until second changes
 * Time wait vary between 1000ms to 2000ms
 */
void waitsecond();

void waitbusy(uint32_t n);

int32_t rand(int32_t low, int32_t high);

void srand(uint32_t seed);