#include "../lib-header/stdtype.h"
#include "../filesystem/ext2.h"
#include "../user/sys/sys.h"
#include "command.h"

int main(void)
{
    // struct BlockBuffer buffer[4];
    // struct EXT2DriverRequest request = {
    //     .buf = &buffer,
    //     .name = "ikanaide",
    //     .ext = "\0\0\0",
    //     .inode = 1,
    //     .buffer_size = BLOCK_SIZE * 4,
    //     .name_len = 8};
    // int8_t retcode = sys_read(request);
    // if (retcode == 0)
    //     puts("owo\n");

    char buf[16];
    char endl[2] = "\n";
    while (TRUE)
    {
        command_input(buf, 16);
        puts(buf);
        puts(endl);
    }

    return 0;
}
