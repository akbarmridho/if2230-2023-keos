#include "./lib-header/math.h"

int divceil(int pembilang, int penyebut)
{
  int cmp = pembilang / penyebut;

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