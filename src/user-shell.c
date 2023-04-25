#include "lib-header/stdtype.h"

int main(void)
{
    __asm__ volatile("mov %0, %%eax"
                     : /* <Empty> */
                     : "r"(0xDEADBEEF));
    while (TRUE)
        ;
    return 0;
}
