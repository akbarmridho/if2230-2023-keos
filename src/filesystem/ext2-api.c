#include "ext2-api.h"
#include "../lib-header/math.h"

char *get_entry_name(void *entry)
{
  return entry + sizeof(struct EXT2DirectoryEntry);
}

struct EXT2DirectoryEntry *get_directory_entry(void *ptr, uint32_t offset)
{
  struct EXT2DirectoryEntry *entry = (struct EXT2DirectoryEntry *)(ptr + offset);

  return entry;
}

struct EXT2DirectoryEntry *get_next_directory_entry(struct EXT2DirectoryEntry *entry)
{
  return get_directory_entry(entry, entry->rec_len);
}

uint16_t get_directory_record_length(uint8_t name_len)
{
  // directory entry record length is based by name length (+1 to also store null terminator)
  uint16_t len = sizeof(struct EXT2DirectoryEntry) + name_len + 1;
  return divceil(len, 4) * 4;
}

uint32_t get_directory_first_child_offset(void *ptr)
{
  uint32_t offset = 0;
  struct EXT2DirectoryEntry *entry = get_directory_entry(ptr, 0);
  offset += entry->rec_len;
  entry = get_next_directory_entry(entry);
  offset += entry->rec_len;
  return offset;
}

int8_t separate_filename_extension(char **filename, uint8_t *len_name, char (*ext)[4])
{
  char *ext_temp_ptr;
  for (int i = 0; i < *len_name; i++)
  {
    if ((*filename)[i] == '.')
    {
      (*filename)[i] = '\0';
      ext_temp_ptr = *filename + i + 1;
      int ext_len = *len_name - (i + 1);
      if (ext_len > 3 || ext_len == 0)
      {
        return 1;
      }
      if (ext_len <= 3 && ext_len > 0)
      {
        for (int j = 0; j < 4; j++)
        {
          if (j < ext_len)
          {
            (*ext)[j] = ext_temp_ptr[j];
          }
          else
          {
            (*ext)[j] = '\0';
          }
        }
      }
      *len_name -= ext_len + 1;
      return 0;
    }
  }
  (*ext)[0] = '\0';
  return 0;
}
