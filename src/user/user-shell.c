#include "../lib-header/stdtype.h"
#include "../user/sys/sys.h"
#include "../lib-header/string.h"
#include "../lib-header/math.h"
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

void ls(struct EXT2DriverRequest *request, char *dirname, uint8_t name_len)
{
    request->name = dirname;
    request->inode = currentdirnode;
    request->buffer_size = BLOCK_SIZE * 4;
    request->name_len = name_len;
    request->inode_only = FALSE;
    int8_t retval = sys_read_directory(request);
    if (retval != 0)
    {
        puts(dirname);
        puts(": No such file or directory\n");
        return;
    }
    uint32_t offset = get_directory_first_child_offset(request->buf);
    struct EXT2DirectoryEntry *entry = get_directory_entry(request->buf, offset);
    while (entry->inode != 0)
    {
        if (entry->file_type == EXT2_FT_NEXT)
        {
            request->inode = entry->inode;
            sys_read_next_directory(request);
            offset = 0;
            entry = get_directory_entry(request->buf, offset);
            continue;
        }
        if (entry->file_type == EXT2_FT_REG_FILE)
        {
            puts(get_entry_name(entry));
            if (entry->ext[0] != '\0')
            {
                puts(".");
                puts(entry->ext);
            }
        }
        else
        {
            puts_color(get_entry_name(entry), 0x9);
        }
        puts("  ");
        offset += entry->rec_len;
        entry = get_directory_entry(request->buf, offset);
    }
    puts("\n");
}

void cat(struct EXT2DriverRequest *request, char *filename, uint8_t name_len)
{
    request->name = filename;
    request->inode = currentdirnode;
    request->buffer_size = BLOCK_SIZE * 4;
    request->name_len = name_len;
    request->inode_only = FALSE;
    int8_t retval = sys_read(request);
    if (retval != 0)
    {
        puts(filename);
        puts(": No such file\n");
        return;
    }
    puts(request->buf);
    puts("\n");
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
            request.inode_only = TRUE;
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
        else if (!strcmp(arg, "ls", len))
        {
            char *dirname = arg + len;
            uint8_t name_len;
            next_arg(&dirname, &name_len);
            if (name_len == 0)
            {
                dirname[0] = '.';
                name_len = 1;
            }
            request.buf = buffer;

            ls(&request, dirname, name_len);
        }
        else if (!strcmp(arg, "cp", len))
        {
            // cp {src} {dst}
            char *src = arg + len;
            uint8_t src_len;
            next_arg(&src, &src_len);
            char *dst = src + src_len + 1;
            if (separate_filename_extension(&src, &src_len, &request.ext) != 0)
            {
                continue;
            }
            if (src_len == 0)
            {
                puts("Missing source file\n");
            }
            char extdst[4];
            uint8_t dst_len;
            next_arg(&dst, &dst_len);
            if (separate_filename_extension(&dst, &dst_len, &extdst) != 0)
            {
                continue;
            }
            if (dst_len == 0)
            {
                puts("Missing destination file\n");
                continue;
            }
            request.name = src;
            request.inode = currentdirnode;
            request.name_len = src_len;
            request.buffer_size = BLOCK_SIZE * 4;
            request.inode_only = FALSE;
            int8_t readretval = sys_read(&request);
            if (readretval != 0)
            {
                puts("Fail to read file\n");
                readretval = sys_read_directory(&request);
                if (readretval != 0)
                {
                    puts("Fail to read folder\n");
                }
                else
                {
                    uint32_t offset = get_directory_first_child_offset(request.buf);
                    struct EXT2DirectoryEntry *entry = get_directory_entry(request.buf, offset);
                    if (entry->inode != 0)
                    {
                        puts("Folder is not empty, cannot copy\n");
                        continue;
                    }
                    continue;
                }
                continue;
            }

            request.name = dst;
            request.name_len = dst_len;
            strcpy(extdst, request.ext);
            int8_t writeretval = sys_write(&request);
            if (writeretval != 0)
            {
                puts("Fail to write file\n");
                continue;
            }
            else
            {
                puts("Copy successful\n");
            }
        }
        else if (!strcmp(arg, "cat", len))
        {
            char *filename = arg + len;
            uint8_t name_len;
            next_arg(&filename, &name_len);
            uint8_t sep_retval;
            sep_retval = separate_filename_extension(&filename, &name_len, &request.ext);
            if (sep_retval != 0)
            {
                continue;
            }
            if (name_len == 0)
            {
                puts("missing filename arg\n");
                continue;
            }
            request.buf = buffer;
            cat(&request, filename, name_len);
        }
    }

    return 0;
}
