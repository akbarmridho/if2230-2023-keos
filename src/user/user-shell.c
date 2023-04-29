#include "../lib-header/stdtype.h"
#include "../user/sys/sys.h"
#include "../lib-header/string.h"
#include "../lib-header/math.h"
#include "snake/snake.h"
#include "command.h"

#define BLOCK_COUNT 16

char currentdir[255] = "/";
uint8_t currentdirlen = 1;

uint32_t currentdirnode = 1;

/*
Change name of a file from oldname to newname
I.S. oldname is a valid file in current directory, newname not exist in current directory
F.S. oldname is renamed to newname
Return value:
  0: success
  1: file to be renamed not found
  2: file with new name already exist
  3: failed to delete old file
*/
int8_t rename_file(struct EXT2DriverRequest *request, char *oldname, uint8_t oldname_len, char *newname, uint8_t newname_len)
{
  request->name = oldname;
  request->name_len = oldname_len;
  request->inode = currentdirnode;
  request->buffer_size = BLOCK_SIZE * BLOCK_COUNT;
  request->inode_only = FALSE;
  int8_t retval = sys_read(request);
  if (retval != 0)
  {
    // file to be renamed not found
    return 1;
  }

  request->name = newname;
  request->name_len = newname_len;
  request->inode = currentdirnode;
  request->buffer_size = BLOCK_SIZE * BLOCK_COUNT;
  request->inode_only = FALSE;
  int8_t retval2 = sys_write(request);
  if (retval2 != 0)
  {
    // file with new name already exist
    return 2;
  }

  request->name = oldname;
  request->name_len = oldname_len;
  request->inode = currentdirnode;
  request->buffer_size = BLOCK_SIZE * BLOCK_COUNT;
  request->inode_only = FALSE;
  retval = sys_delete(request);
  if (retval != 0)
  {
    char ret[2];
    ret[0] = retval + '0';
    ret[1] = '\0';
    puts("\nretval: ");
    puts(ret);
    // failed to delete old file
    return 3;
  }
  return 0;
}

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
/*
Move file to a folder
I.S. filename any, dirname is a valid folder in current directory
F.S. filename is valid and moved to dirname
Return value:
  0: success
  1: file to be moved not found
  2: file with new name already exist
  3: parent folder invalid
  4: unknown error
  5: failed to delete old file
*/
int8_t move_file_to_folder(struct EXT2DriverRequest *request, char *filename, uint8_t name_len, char *dirname, uint8_t dirname_len)
{
  // get directory inode
  request->name = dirname;
  request->name_len = dirname_len;
  request->inode_only = TRUE;
  int8_t retval = sys_read_directory(request);

  // get directory inode
  uint32_t dir_inode = request->inode;

  // get file to buffer
  request->name = filename;
  request->name_len = name_len;
  request->inode = currentdirnode;
  request->buffer_size = BLOCK_SIZE * BLOCK_COUNT;
  request->inode_only = FALSE;
  retval = sys_read(request);
  if (retval != 0)
  {
    // file to be moved not found
    return 1;
  }

  // write file to new directory
  request->inode = dir_inode;
  retval = sys_write(request);
  if (retval != 0)
  {
    switch (retval)
    {
    case 1:
      // file with name filename already exist within directory dirname
      return 2;

    case 2:
      // parent folder invalid
      return 3;

    default:
      // unknown error
      return 4;
    }
  }

  // delete old file
  request->name = filename;
  request->name_len = name_len;
  request->inode = currentdirnode;
  request->buffer_size = BLOCK_SIZE * BLOCK_COUNT;
  request->inode_only = FALSE;
  retval = sys_delete(request);
  if (retval != 0)
  {
    // failed to delete old file
    return 5;
  }
  return 0;
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

void mv(struct EXT2DriverRequest *request, char *src, uint8_t src_len, char *dst, uint8_t dst_len)
{
  request->name = src;
  request->inode = currentdirnode;
  request->name_len = src_len;
  request->buffer_size = BLOCK_SIZE * BLOCK_COUNT;
  request->inode_only = FALSE;
  int8_t readretval = sys_read_directory(request);
  if (readretval != 0)
  {
    // src not a directory
    request->name = dst;
    request->name_len = dst_len;
    request->inode = currentdirnode;
    request->buffer_size = BLOCK_SIZE * BLOCK_COUNT;
    request->inode_only = FALSE;
    readretval = sys_read_directory(request);
    if (readretval != 0)
    {
      // dst not a directory
      int8_t rename_retval = rename_file(request, src, src_len, dst, dst_len);
      switch (rename_retval)
      {
        {
        case 0:
          puts("Rename successful\n");
          return;
        case 1:
          puts("File to be renamed not found\n");
          return;
        case 2:
          puts("File already exists\n");
          return;
        case 3:
          puts("Fail to delete old file\n");
          return;

        default:
          puts("Unknown error\n");
          return;
        }
      }
    }
    else
    {
      // dst a valid directory
      int8_t move_retval = move_file_to_folder(request, src, src_len, dst, dst_len);
      switch (move_retval)
      {
        {
        case 0:
          puts("Move successful\n");
          return;
        case 1:
          puts("File to be moved not found\n");
          return;
        case 2:
          puts("File already exists\n");
          return;
        case 3:
          puts("Parent folder not found\n");
          return;
        case 4:
          puts("Fail to delete old file\n");
          return;
        default:
          puts("Unknown error\n");
          return;
        }
      }
    }
  }
  else
  {
    // dst a valid directory
    request->name = src;
    request->inode = currentdirnode;
    request->name_len = src_len;
    request->buffer_size = BLOCK_SIZE * BLOCK_COUNT;
    request->inode_only = FALSE;

    struct EXT2DriverRequest dst_request;
    dst_request.name = dst;
    dst_request.inode = currentdirnode;
    dst_request.name_len = dst_len;
    dst_request.buffer_size = BLOCK_SIZE * BLOCK_COUNT;
    dst_request.inode_only = TRUE;

    int8_t move_retval = sys_move_dir(request, &dst_request);
    switch (move_retval)
    // Error code: 0 success - 1 source folder not found - 2 invalid parent folder - 3 invalid destination folder - 4 folder already exist - -1 not found any block
    {
      {
      case 0:
        puts("Move successful\n");
        return;
      case 1:
        puts("Folder to be moved not found\n");
        return;
      case 2:
        puts("Parent folder not found\n");
        return;
      case 3:
        puts("Destination folder not found\n");
        return;
      case 4:
        puts("File or Folder already exists\n");
        return;
      case -1:
        puts("Not found any block\n");
        return;
      }
    }
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

  request->inode_only = TRUE;
  retval = sys_read_directory(request);
  if (retval == 0)
  {
    puts("Folder with the same name already exists. nano is write only\n");
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
    puts(" successfully written\n");
  }
  else
  {
    puts("failed to write file\n");
  }
}

void whereis(struct EXT2DriverRequest *request, char *targetdir, char *targetname, char *current_path, bool *found)
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
      if (is_equal(filename, targetname))
      {
        char result[strlen(filename) + strlen(current_path) + 2]; // include path separator
        append_path(current_path, filename, result);
        puts(result);
        puts("\n");

        *found = TRUE;
      }
    }
    else
    {
      // cek jika sama
      // panggil whereis juga
      char *foldername = get_entry_name(entry);

      if (is_equal(foldername, targetname) || (is_equal(foldername, targetdir)))
      {
        char result[strlen(foldername) + strlen(current_path) + 2];
        append_path(current_path, foldername, result);
        puts(result);
        puts("\n");

        *found = TRUE;
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
      whereis(newReq, targetdir, targetname, new_current_path, found);

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
      dirname[name_len] = '\0';
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
        if (src_len == 0)
        {
          puts("Missing source file\n");
        }
        uint8_t dst_len;
        next_arg(&dst, &dst_len);
        if (dst_len == 0)
        {
          puts("Missing destination file\n");
          continue;
        }
        request->inode = currentdirnode;
        dst[dst_len] = '\0';
        cp(request, src, src_len, dst, dst_len, currentdirnode);
      }
      else
      {
        char *src = flag + flag_len + 1;
        uint8_t src_len;
        next_arg(&src, &src_len);
        char *dst = src + src_len + 1;
        if (src_len == 0)
        {
          puts("Missing source file\n");
        }
        uint8_t dst_len;
        next_arg(&dst, &dst_len);
        if (dst_len == 0)
        {
          puts("Missing destination file\n");
          continue;
        }
        dst[dst_len] = '\0';
        request->inode = currentdirnode;
        cpr(request, src, src_len, dst, dst_len, currentdirnode);
      }
    }
    else if (!strcmp(arg, "cat", len))
    {
      char *filename = arg + len;
      uint8_t name_len;
      next_arg(&filename, &name_len);
      if (name_len == 0)
      {
        puts("missing filename arg\n");
        continue;
      }
      request->buf = buffer;
      cat(request, filename, name_len);
    }
    else if (!strcmp(arg, "mv", len))
    {
      char *src = arg + len;
      uint8_t src_len;
      next_arg(&src, &src_len);
      if (src_len == 0)
      {
        puts("mv: missing file operand\n");
        continue;
      }
      char *dst = src + src_len + 1;
      uint8_t dst_len;
      next_arg(&dst, &dst_len);
      if (dst_len == 0)
      {
        puts("mv: missing destination file operand after '");
        puts(src);
        puts("'\n");
        continue;
      }
      char *next = dst + dst_len + 1;
      uint8_t next_len;
      next_arg(&next, &next_len);
      dst[dst_len] = '\0';
      if (next_len == 0)
      {
        request->buf = buffer;
        mv(request, src, src_len, dst, dst_len);
      }
      else
      {
        puts("not implemented, only takes two argument\n");
      }
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
      filename[name_len] = '\0';
      nano(request, filename, name_len);
    }
    else if (!strcmp(arg, "clear", len))
    {
      clear_screen();
    }
    else if (!strcmp(arg, "help", len))
    {
      puts("Documentation\n\n");
      help();
    }
    else if (!strcmp(arg, "time", len))
    {
      time();
    }
    else if (!strcmp(arg, "whereis", len))
    {
      char *filename = arg + len;
      uint8_t name_len;
      next_arg(&filename, &name_len);

      char dirname[name_len];
      strcpy(filename, dirname);
      bool found = FALSE;

      request->buf = buffer;
      request->name = ".";
      request->inode = 1; // start from root
      request->buffer_size = BLOCK_SIZE * BLOCK_COUNT;
      request->name_len = 1;
      request->inode_only = FALSE;
      whereis(request, dirname, filename, "", &found);

      if (!found)
      {
        puts("File/ folder not exist\n");
      }
    }
    else if (!strcmp(arg, "snake", len))
    {
      start_snake();
    }
    else if (!strcmp(arg, "rm", len))
    {
      char *flag = arg + len;
      uint8_t flag_len;
      next_arg(&flag, &flag_len);
      if (strcmp(flag, "-r", flag_len) != 0)
      {
        char *src = flag;
        uint8_t src_len = flag_len;
        if (src_len == 0)
        {
          puts("Missing source file\n");
        }

        request->inode = currentdirnode;
        rm(request, src, src_len, currentdirnode);
      }
      else
      {
        char *src = flag + flag_len + 1;
        uint8_t src_len;
        next_arg(&src, &src_len);
        if (src_len == 0)
        {
          puts("Missing source file\n");
        }

        request->inode = currentdirnode;
        rmr(request, src, src_len, currentdirnode);
      }
    }
    else
    {
      puts("Invalid command!\n\n");
      help();
    }
  }

  return 0;
}
