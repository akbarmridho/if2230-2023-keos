#include "../lib-header/stdtype.h"
#include "../filesystem/ext2.h"
#include "../user/sys/sys.h"
#include "../lib-header/string.h"
#include "command.h"

char currentdir[255] = "/";
uint8_t currentdirlen = 1;

uint32_t currentdirnode = 1;

void resolve_new_path(char *dirname, uint8_t name_len)
{
    if (name_len == 0)
        return;
    if (dirname[0] == '/')
    {
        currentdir[0] = '/';
        currentdirlen = 1;
        currentdir[1] = '\0';
        dirname++;
        resolve_new_path(dirname, name_len - 1);
        return;
    }

    uint32_t len = 0;
    while (len < name_len && dirname[len] != '/')
    {
        len++;
    }
    if (!strcmp(dirname, ".", len))
    {
        if (name_len == len)
        {
            return;
        }
        dirname += 2;
        resolve_new_path(dirname, name_len - 2);
        return;
    }

    if (!strcmp(dirname, "..", len))
    {
        if (currentdirlen == 1)
            // already in root dir
            return;
        uint8_t minus_len = 1;
        while (currentdir[currentdirlen - minus_len - 1] != '/')
            minus_len++;
        currentdirlen -= minus_len;
        if (currentdirlen > 1)
            currentdirlen--;
        currentdir[currentdirlen] = '\0';
        if (name_len == len)
            return;
        dirname += 3;
        resolve_new_path(dirname, name_len - 3);
        return;
    }
    if (currentdir[currentdirlen - 1] != '/')
    {
        currentdir[currentdirlen] = '/';
        currentdirlen++;
    }
    for (uint8_t i = 0; i < len; i++)
    {
        currentdir[currentdirlen] = dirname[i];
        currentdirlen++;
    }
    currentdir[currentdirlen] = '\0';

    if (name_len == len)
        return;
    dirname += len + 1;
    resolve_new_path(dirname, name_len - len);
}

int main(void)
{
    struct EXT2DriverRequest request;
    struct BlockBuffer buffer[4];
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

    char buf[255];
    buf[254] = '\0';
    while (TRUE)
    {
        command_input(buf, currentdir, 254);
        char *arg = buf;
        uint8_t len;
        next_arg(&arg, &len);
        if (!strcmp(arg, "cd", len))
        {
            char *dirname = arg + len;
            uint8_t name_len;
            next_arg(&dirname, &name_len);
            if (name_len == 0)
            {
                dirname[0] = '/';
                name_len = 1;
            }
            request.buf = buffer;
            request.name = dirname;
            request.inode = currentdirnode;
            request.buffer_size = 0;
            request.name_len = name_len;
            request.inode_only = FALSE;
            int8_t retval = sys_read_directory(&request);
            if (retval != 0)
            {
                puts(dirname);
                puts(": No such file or directory\n");
            }
            else
            {
                currentdirnode = request.inode;
                resolve_new_path(dirname, name_len);
            }
        }
        else if (!strcmp(arg, "mkdir", len))
        {
            char *dirname = arg + len;
            uint8_t name_len;
            next_arg(&dirname, &name_len);
            if (name_len == 0)
            {
                puts("missing filename arg\n");
                continue;
            }
            request.buf = buffer;
            request.name = dirname;
            request.inode = currentdirnode;
            request.buffer_size = 0;
            request.name_len = name_len;
            int8_t retval = sys_write(&request);
            if (retval == 0)
            {
                puts("Created directory ");
                puts(dirname);
                puts("\n");
            }
            else if (retval == 1)
            {
                puts("cannot create directory '");
                puts(dirname);
                puts("': Directory exists\n");
            }
        }
    }

    return 0;
}
