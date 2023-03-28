#include "./lib-header/math.h"

uint32_t divceil(uint32_t pembilang, uint32_t penyebut)
{
  uint32_t cmp = pembilang / penyebut;

  if (pembilang % penyebut == 0)
    return cmp;

  return cmp + 1;
}

int ceil(float a)
{
  if (a == (int)a)
  {
    return a;
  }
  else
  {
    return ((int)a) + 1;
  }
}