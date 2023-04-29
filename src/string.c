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

int strlen(const char *str)
{
    int res = 0;
    while (str[res] != '\0')
    {
        res++;
    }
    return res;
}

bool is_equal(const char *stra, const char *strb)
{
    int lena = strlen(stra);
    int lenb = strlen(strb);
    if (lena != lenb)
    {
        return FALSE;
    }

    int i = 0;

    while (stra[i] == strb[i] && i < lena)
    {
        i++;
    }

    return i == lena;
}

void append(const char *stra, const char *strb, char *destination)
{
    int i = 0;

    for (int j = 0; j < strlen(stra); j++)
    {
        destination[i++] = stra[j];
    }

    for (int j = 0; j < strlen(strb); j++)
    {
        destination[i++] = strb[j];
    }

    destination[i] = '\0';
}

void append3(const char *stra, const char *strb, const char *strc, char *destination)
{
    int i = 0;

    for (int j = 0; j < strlen(stra); j++)
    {
        destination[i++] = stra[j];
    }

    for (int j = 0; j < strlen(strb); j++)
    {
        destination[i++] = strb[j];
    }

    for (int j = 0; j < strlen(strc); j++)
    {
        destination[i++] = strc[j];
    }

    destination[i] = '\0';
}

void append_path(const char *stra, const char *strb, char *destination)
{
    int i = 0;

    for (int j = 0; j < strlen(stra); j++)
    {
        destination[i++] = stra[j];
    }

    destination[i] = '/';
    i++;

    for (int j = 0; j < strlen(strb); j++)
    {
        destination[i++] = strb[j];
    }

    destination[i] = '\0';
}

void itoa(int32_t value, char *result)
{
    char *ptr = result, *ptr1 = result, tmp_char;
    int32_t tmp_value;

    do
    {
        tmp_value = value;
        value /= 10;
        *ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz"[35 + (tmp_value - value * 10)];
    } while (value);

    // Apply negative sign
    if (tmp_value < 0)
        *ptr++ = '-';
    *ptr-- = '\0';
    while (ptr1 < ptr)
    {
        tmp_char = *ptr;
        *ptr-- = *ptr1;
        *ptr1++ = tmp_char;
    }
}