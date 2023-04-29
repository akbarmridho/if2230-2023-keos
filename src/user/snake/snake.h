#ifndef SNAKE_H
#define SNAKE_H
#include "../../lib-header/stdtype.h"

#define COLUMN 80
#define ROW 25
#define KEY_LEFT_DOWN 0x4b
#define KEY_LEFT_UP (KEY_LEFT_DOWN + 0x80)
#define KEY_UP_UP 0x48
#define KEY_UP_DOWN (KEY_UP_UP + 0x80)
#define KEY_RIGHT_UP 0x4d
#define KEY_RIGHT_DOWN (KEY_RIGHT_UP + 0x80)
#define KEY_DOWN_UP 0x50
#define KEY_DOWN_DOWN (KEY_DOWN_UP + 0x80)
#define SNAKE_COLUMN 30
#define COLUMN_OFFSET ((COLUMN - SNAKE_COLUMN) / 2)
#define SNAKE_ROW 15
#define ROW_OFFSET ((ROW - SNAKE_ROW) / 2)

struct Snake
{
  struct Segment *segments;
  uint32_t length;
  uint32_t capacity;
  int8_t vel_x;
  int8_t vel_y;
  bool alive;
  uint32_t score;
} __attribute((packed));

struct Segment
{
  int x;
  int y;
};

void start_snake(void);

#endif