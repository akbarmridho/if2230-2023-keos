#include "./lib-header/string.h"

int strcmp(const char *stra, const char *strb, int size)
{
  int i;
  for (i = 0; i < size && stra[i] == strb[i]; i++)
  {
    if (stra[i] == '\0')
    {
      return 0;
    }
  }
  if (i == size)
  {
    return 0;
  }
  return stra[i] - strb[i];
}

char *strcpy(const char *src, char *dest)
{
  char *original_dest = dest; // original destination address

  while (*src != '\0')
  {
    *dest = *src;
    dest++;
    src++;
  }

  *dest = '\0'; // null terminator at the end

  return original_dest;
}
