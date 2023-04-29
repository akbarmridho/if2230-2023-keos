#include "command.h"
#include "../lib-header/string.h"
#include "sys/sys.h"

void command_input(char *buf, char *currentdir, uint32_t size)
{
  puts_color("Keos@OS-IF2230", 0x2);
  puts_color(":", 0x7);
  puts_color(currentdir, 0x9);
  puts_color("$ ", 0x7);
  gets(buf, size);
}
/**
 * @brief Get the next arg object
 */
void next_arg(char **pstr, uint8_t *len)
{
  *len = 0;
  while ((**pstr) == ' ')
  {
    if (**pstr == '\0')
      return;
    *pstr += 1;
  }
  while ((*pstr)[*len] != ' ' && (*pstr)[*len] != '\0')
  {
    *len += 1;
  }
}

void cp(struct EXT2DriverRequest *request, char *src, uint8_t src_len, char *dst, uint8_t dst_len, char *extdst, uint32_t parent)
{
  request->name = src;
  request->inode = parent;
  request->name_len = src_len;
  request->buffer_size = BLOCK_SIZE * BLOCK_COUNT;
  request->inode_only = FALSE;
  int8_t readretval = sys_read(request);
  if (readretval != 0)
  {
    puts("Not a file\n");
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
        puts("Folder is not empty, did not specify -r (usage: cp -r {src} {dst})\n");
        return;
      }
      else
      {
        request->name = dst;
        request->inode = parent;
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

void cpr(struct EXT2DriverRequest *request, char *src, uint8_t src_len, char *dst, uint8_t dst_len, char *extdst, uint32_t parent)
{
  request->name = src;
  request->inode = parent;
  request->name_len = src_len;
  request->buffer_size = BLOCK_SIZE * BLOCK_COUNT;
  request->inode_only = FALSE;
  int8_t readretval = sys_read(request);
  if (readretval != 0)
  {
    puts("Not a file\n");
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
        // Folder is not empty, iterate every entry in the folder
        // Make the folder
        request->inode = parent;
        request->name = dst;
        request->name_len = dst_len;
        request->buffer_size = 0;
        int8_t writeretval = sys_write(request);
        if (writeretval != 0)
        {
          puts("Fail to write folder\n");
          return;
        }
        else
        {
          puts("Copy successful\n");
        }
        // Recursive call
        // Block habis, lanjut block berikutnya
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
          char *filename = get_entry_name(entry);
          uint8_t name_len = entry->name_len;
          char *ext = entry->ext;
          struct EXT2DriverRequest *newReq = malloc(sizeof(struct EXT2DriverRequest));
          struct BlockBuffer *buffer = malloc(sizeof(struct BlockBuffer) * BLOCK_COUNT);
          newReq->buf = buffer;
          newReq->buffer_size = BLOCK_COUNT * BLOCK_SIZE;
          newReq->inode_only = FALSE;
          newReq->inode = entry->inode;
          newReq->name = filename;
          newReq->name_len = name_len;

          cp(request, filename, name_len, filename, name_len, ext, request->inode);
          free(buffer);
          free(newReq);
        }
        else
        {
          char *foldername = get_entry_name(entry);
          uint8_t name_len = entry->name_len;
          // explore folder berikutnya
          struct EXT2DriverRequest *newReq = malloc(sizeof(struct EXT2DriverRequest));
          struct BlockBuffer *buffer = malloc(sizeof(struct BlockBuffer) * BLOCK_COUNT);
          newReq->buf = buffer;
          newReq->buffer_size = BLOCK_COUNT * BLOCK_SIZE;
          newReq->inode_only = FALSE;
          newReq->inode = request->inode;
          newReq->name = foldername;
          newReq->name_len = name_len;

          char *ext = "";
          cpr(newReq, foldername, name_len, foldername, name_len, ext, request->inode);
          free(buffer);
          free(newReq);
        }
        offset += entry->rec_len;
        entry = get_directory_entry(request->buf, offset);
      }
      return;
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

void time()
{
  uint16_t year;
  uint16_t month;
  uint16_t day;
  uint16_t hour;
  uint16_t minute;
  uint16_t second;
  read_rtc(&year, &month, &day, &hour, &minute, &second);

  char syear[10];
  char smonth[10];
  char sday[10];
  char shour[10];
  char sminute[10];
  char ssecond[10];
  itoa((int32_t)year, syear);
  itoa((int32_t)month, smonth);
  itoa((int32_t)day, sday);
  itoa((int32_t)hour, shour);
  itoa((int32_t)minute, sminute);
  itoa((int32_t)second, ssecond);

  puts(syear);
  puts("-");
  puts(smonth);
  puts("-");
  puts(sday);
  puts(" ");
  puts(shour);
  puts(":");
  puts(sminute);
  puts(":");
  puts(ssecond);

  puts(" UTC\n");
}