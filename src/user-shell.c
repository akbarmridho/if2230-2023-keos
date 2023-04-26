#include "lib-header/stdtype.h"
// #include "filesystem/ext2.h"

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

int main(void)
{
    // struct BlockBuffer buffer[4];
    // struct EXT2DriverRequest request = {
    //     .buf = &buffer,
    //     .name = "ikanaide",
    //     .ext = "\0\0\0",
    //     .inode = 1,
    //     .buffer_size = BLOCK_SIZE * 4,
    //     .name_len = 9};
    // int32_t retcode;
    // syscall(0, (uint32_t)&request, (uint32_t)&retcode, 0);
    // if (retcode == 0)
    //     syscall(5, (uint32_t) "owo\n", 4, 0xF);
    syscall(5, (uint32_t) "owo", 3, 0xF);

    // char buf[16];
    // while (TRUE)
    // {
    //     syscall(4, (uint32_t)buf, 16, 0);
    //     syscall(5, (uint32_t)buf, 16, 0xF);
    // }

    return 0;
}
