#include "../lib-header/stdtype.h"
#include "../user/sys/sys.h"
#include "../lib-header/string.h"
#include "../lib-header/math.h"
#include "command.h"

#define BLOCK_COUNT 16

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
    request->buffer_size = BLOCK_SIZE * BLOCK_COUNT;
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
    request->buffer_size = BLOCK_SIZE * BLOCK_COUNT;
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

void cp(struct EXT2DriverRequest *request, char *src, uint8_t src_len, char *dst, uint8_t dst_len, char *extdst)
{
    request->name = src;
    request->inode = currentdirnode;
    request->name_len = src_len;
    request->buffer_size = BLOCK_SIZE * BLOCK_COUNT;
    request->inode_only = FALSE;
    int8_t readretval = sys_read(request);
    if (readretval != 0)
    {
        puts("Fail to read file\n");
        readretval = sys_read_directory(request);
        if (readretval != 0)
        {
            puts("Fail to read folder\n");
        }
        else
        {
            uint32_t offset = get_directory_first_child_offset(request->buf);
            struct EXT2DirectoryEntry *entry = get_directory_entry(request->buf, offset);
            if (entry->inode != 0)
            {
                // Folder is not empty
                puts("Folder is not empty, did not specify -r (usage: cp -r {src} {dst})");
                return;
            }
            else
            {
                request->name = dst;
                request->inode = currentdirnode;
                request->buffer_size = 0;
                request->name_len = dst_len;
                int8_t writefolderretval = sys_write(request);
                if (writefolderretval == 0)
                {
                    puts("Copy successful\n");
                    return;
                }
                else
                {
                    puts("Fail to write folder\n");
                    return;
                }
            }
        }
    }

    request->name = dst;
    request->name_len = dst_len;
    strcpy(extdst, request->ext);
    int8_t writeretval = sys_write(request);
    if (writeretval != 0)
    {
        puts("Fail to write file\n");
        return;
    }
    else
    {
        puts("Copy successful\n");
    }
}

void cpr(struct EXT2DriverRequest *request, char *src, uint8_t src_len, char *dst, uint8_t dst_len, char *extdst)
{
    request->name = src;
    request->inode = currentdirnode;
    request->name_len = src_len;
    request->buffer_size = BLOCK_SIZE * BLOCK_COUNT;
    request->inode_only = FALSE;
    int8_t readretval = sys_read(request);
    if (readretval != 0)
    {
        puts("Fail to read file\n");
        readretval = sys_read_directory(request);
        if (readretval != 0)
        {
            puts("Fail to read folder\n");
        }
        else
        {
            uint32_t offset = get_directory_first_child_offset(request->buf);
            struct EXT2DirectoryEntry *entry = get_directory_entry(request->buf, offset);
            while (entry->inode != 0)
            {
                // Folder is not empty
                // Recursive call
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
                return;
            }
            return;
        }
        return;
    }

    request->name = dst;
    request->name_len = dst_len;
    strcpy(extdst, request->ext);
    int8_t writeretval = sys_write(request);
    if (writeretval != 0)
    {
        puts("Fail to write file\n");
        return;
    }
    else
    {
        puts("Copy successful\n");
    }
}

void nano(struct EXT2DriverRequest *request, char *filename, uint8_t name_len)
{
    request->name = filename;
    request->inode = currentdirnode;
    request->buffer_size = 0;
    request->name_len = name_len;
    request->inode_only = FALSE;

    int8_t retval = sys_read(request);
    if (retval == 0 || retval == 2)
    {
        puts("file already exists. nano is write only\n");
        return;
    }
    uint32_t filesize;
    clear_screen();
    get_text(request->buf, BLOCK_SIZE * BLOCK_COUNT, &filesize);
    request->buffer_size = filesize;
    retval = sys_write(request);
    puts("\n");
    if (retval == 0)
    {
        puts(filename);
        if (request->ext[0] != '\0')
        {
            puts(".");
            puts(request->ext);
        }
        puts(" successfully written\n");
    }
    else
    {
        puts("failed to write file\n");
    }
}

void whereis(struct EXT2DriverRequest *request, char *targetname, char targetext[4], char *current_path)
{
    int8_t retval = sys_read_directory(request);
    if (retval != 0)
    {
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
            // cek jika sama
            char *filename = get_entry_name(entry);

            // filename sama
            if (is_equal(filename, targetname) && is_equal(entry->ext, targetext))
            {
                // file tanpa ext
                if (strlen(targetext) == 0)
                {
                    char result[strlen(filename) + strlen(current_path) + 2]; // include path separator
                    append_path(current_path, filename, result);
                    puts(result);
                    puts("\n");
                }
                else // file dengan ext
                {
                    char file_wext[strlen(filename) + strlen(targetext) + 2]; // include dot
                    append3(filename, ".", targetext, file_wext);

                    char result[strlen(file_wext) + strlen(current_path) + 2];
                    append_path(current_path, file_wext, result);
                    puts(result);
                    puts("\n");
                }
            }
        }
        else
        {
            // cek jika sama
            // panggil whereis juga
            char *foldername = get_entry_name(entry);

            if (strlen(targetext) == 0 && is_equal(foldername, targetname))
            {
                char result[strlen(foldername) + strlen(current_path) + 2];
                append_path(current_path, foldername, result);
                puts(result);
                puts("\n");
            }

            // explore folder berikutnya
            struct EXT2DriverRequest *newReq = malloc(sizeof(struct EXT2DriverRequest));
            struct BlockBuffer *buffer = malloc(sizeof(struct BlockBuffer) * BLOCK_COUNT);
            newReq->buf = buffer;
            newReq->buffer_size = BLOCK_COUNT * BLOCK_SIZE;
            newReq->inode_only = FALSE;
            newReq->inode = request->inode;
            newReq->name = foldername;
            newReq->name_len = strlen(foldername);

            char *new_current_path = malloc(strlen(current_path) + strlen(foldername) + 2);
            append_path(current_path, foldername, new_current_path);
            whereis(newReq, targetname, targetext, new_current_path);

            free(new_current_path);
            free(buffer);
            free(newReq);
        }
        offset += entry->rec_len;
        entry = get_directory_entry(request->buf, offset);
    }
}

int main()
{
    struct EXT2DriverRequest *request = malloc(sizeof(struct EXT2DriverRequest));
    struct BlockBuffer *buffer = malloc(BLOCK_SIZE * BLOCK_COUNT);
    request->buf = buffer;
    // struct EXT2DriverRequest request = {
    //     .buf = &buffer,
    //     .name = "ikanaide",
    //     .ext = "\0\0\0",
    //     .inode = 1,
    //     .buffer_size = BLOCK_SIZE * BLOCK_COUNT,
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
            request->buf = buffer;
            request->name = dirname;
            request->inode = currentdirnode;
            request->buffer_size = 0;
            request->name_len = name_len;
            request->inode_only = TRUE;
            int8_t retval = sys_read_directory(request);
            if (retval != 0)
            {
                puts(dirname);
                puts(": No such file or directory\n");
            }
            else
            {
                currentdirnode = request->inode;
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
            request->buf = buffer;
            request->name = dirname;
            request->inode = currentdirnode;
            request->buffer_size = 0;
            request->name_len = name_len;
            int8_t retval = sys_write(request);
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
            request->buf = buffer;

            ls(request, dirname, name_len);
        }
        else if (!strcmp(arg, "cp", len))
        {
            // cp {src} {dst}
            char *flag = arg + len;
            uint8_t flag_len;
            next_arg(&flag, &flag_len);
            if (strcmp(flag, "-r", flag_len) != 0)
            {
                char *src = flag;
                uint8_t src_len = flag_len;
                char *dst = src + src_len + 1;
                if (separate_filename_extension(&src, &src_len, &request->ext) != 0)
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
                cp(request, src, src_len, dst, dst_len, extdst);
            }
            else
            {
                char *src = flag + flag_len + 1;
                uint8_t src_len;
                next_arg(&src, &src_len);
                char *dst = src + src_len + 1;
                if (separate_filename_extension(&src, &src_len, &request->ext) != 0)
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
                cp(request, src, src_len, dst, dst_len, extdst);
            }
        }
        else if (!strcmp(arg, "cat", len))
        {
            char *filename = arg + len;
            uint8_t name_len;
            next_arg(&filename, &name_len);
            uint8_t sep_retval;
            sep_retval = separate_filename_extension(&filename, &name_len, &request->ext);
            if (sep_retval != 0)
            {
                continue;
            }
            if (name_len == 0)
            {
                puts("missing filename arg\n");
                continue;
            }
            request->buf = buffer;
            cat(request, filename, name_len);
        }
        else if (!strcmp(arg, "nano", len))
        {
            char *filename = arg + len;
            uint8_t name_len;
            next_arg(&filename, &name_len);
            if (name_len == 0)
            {
                puts("missing filename arg\n");
                continue;
            }
            uint8_t sep_retval;
            sep_retval = separate_filename_extension(&filename, &name_len, &request->ext);
            if (sep_retval != 0)
            {
                puts("invalid filename\n");
                continue;
            }
            nano(request, filename, name_len);
        }
        else if (!strcmp(arg, "clear", len))
        {
            clear_screen();
        }
        else if (!strcmp(arg, "whereis", len))
        {
            char *filename = arg + len;
            uint8_t name_len;
            next_arg(&filename, &name_len);

            char ext[4];

            uint8_t sep_retval;
            sep_retval = separate_filename_extension(&filename, &name_len, &ext);
            if (sep_retval != 0)
            {
                continue;
            }

            request->buf = buffer;
            request->name = ".";
            request->inode = 1; // start from root
            request->buffer_size = BLOCK_SIZE * BLOCK_COUNT;
            request->name_len = 1;
            request->inode_only = FALSE;
            whereis(request, filename, ext, "");
        }
    }

    return 0;
}
