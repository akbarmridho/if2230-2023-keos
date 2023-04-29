#include "snake.h"
#include "../sys/sys.h"
#include "../utils.h"
#include "../../lib-header/string.h"

struct Snake snake;
char *chars;
uint8_t *fgs;
uint8_t *bgs;
int foodx;
int foody;
int32_t initial_wait_loop = 2;

int get_segment_pos(int x, int y)
{
  return (ROW_OFFSET + y) * COLUMN + x + COLUMN_OFFSET;
}

void update_framebuffer()
{
  for (uint32_t i = 0; i < ROW * COLUMN; i++)
  {
    fgs[i] = 0x0F;
    bgs[i] = 0;
  }
  int pos;
  // wall atas bawah
  for (uint32_t i = 0; i < SNAKE_COLUMN + 2; i++)
  {
    pos = COLUMN * (ROW_OFFSET - 1) + COLUMN_OFFSET - 1 + i;
    bgs[pos] = 0x06;
    pos += (SNAKE_ROW + 1) * COLUMN;
    bgs[pos] = 0x06;
  }
  // wall kiri kanan
  for (uint32_t i = 0; i < SNAKE_ROW; i++)
  {
    pos = COLUMN * (ROW_OFFSET + i) + COLUMN_OFFSET - 1;
    bgs[pos] = 0x06;
    pos += SNAKE_COLUMN + 1;
    bgs[pos] = 0x06;
  }
  pos = get_segment_pos(foodx, foody);
  bgs[pos] = 0x04;
  pos = get_segment_pos(snake.segments->x, snake.segments->y);
  bgs[pos] = 0x0A;
  for (uint32_t i = 1; i < snake.length; i++)
  {
    pos = get_segment_pos(snake.segments[i].x, snake.segments[i].y);
    bgs[pos] = 0x0F;
  }

  for (uint32_t i = 0; i < COLUMN; i++)
  {
    chars[COLUMN * 2 + i] = ' ';
  }
  strcpy("SCORE: ", chars + COLUMN_OFFSET + COLUMN * 2 + 1);
  itoa(snake.score, chars + COLUMN_OFFSET + COLUMN * 2 + 8);

  rewrite_framebuffer(chars, fgs, bgs);
}

void new_food()
{
  bool valid = FALSE;
  do
  {
    foodx = rand(0, SNAKE_COLUMN - 1);
    foody = rand(0, SNAKE_ROW - 1);
    valid = TRUE;
    for (uint32_t i = 0; i < snake.length; i++)
    {
      if (foodx == snake.segments[i].x && foody == snake.segments[i].y)
      {
        valid = FALSE;
        break;
      }
    }
  } while (!valid);
}

void check_collision()
{
  // collision wall
  if (snake.segments->x < 0 || snake.segments->x >= SNAKE_COLUMN || snake.segments->y < 0 || snake.segments->y >= SNAKE_ROW)
  {
    snake.alive = FALSE;
    return;
  }
  // collision self
  for (uint32_t i = 1; i < snake.length; i++)
  {
    if (snake.segments->x == snake.segments[i].x && snake.segments->y == snake.segments[i].y)
    {
      snake.alive = FALSE;
      return;
    }
  }
  // collision food
  if (snake.segments->x == foodx && snake.segments->y == foody)
  {
    if (snake.length == snake.capacity)
    {
      snake.capacity *= 2;
      snake.segments = realloc(snake.segments, sizeof(struct Segment) * snake.capacity);
    }
    snake.segments[snake.length] = snake.segments[snake.length - 1];
    snake.length++;
    snake.score++;
    if (snake.score % 2 == 0 && snake.score != 0)
    {
      if (initial_wait_loop > 2)
      {
        initial_wait_loop--;
      }
    }
    new_food();
  }
}

void walk()
{
  for (uint32_t i = snake.length - 1; i > 0; i--)
  {
    snake.segments[i] = snake.segments[i - 1];
  }
  snake.segments->x += snake.vel_x;
  snake.segments->y += snake.vel_y;
}

void change_dir(int8_t dir)
{
  // up right down left
  switch (dir)
  {
  case 1:
    if (snake.vel_y != 1)
    {
      snake.vel_y = -1;
      snake.vel_x = 0;
    }
    break;
  case 2:
    if (snake.vel_x != -1)
    {
      snake.vel_y = 0;
      snake.vel_x = 1;
    }
    break;
  case 3:
    if (snake.vel_y != -1)
    {
      snake.vel_y = 1;
      snake.vel_x = 0;
    }
    break;
  case 4:
    if (snake.vel_x != 1)
    {
      snake.vel_y = 0;
      snake.vel_x = -1;
    }
    break;
  default:
    break;
  }
}

void start_snake()
{
  srand(get_timestamp());

  puts("Loading ...\n");

  uint32_t starttime = get_timestamp();
  // loop n million time
  uint32_t n_million = 30;
  waitbusy(n_million);
  uint32_t endtime = get_timestamp();

  initial_wait_loop = (int32_t)(0.4 * n_million / ((float)endtime - (float)starttime));
  snake.alive = TRUE;
  snake.score = 0;
  chars = malloc(ROW * COLUMN);
  for (uint32_t i = 0; i < ROW * COLUMN; i++)
  {
    chars[i] = ' ';
  }
  fgs = malloc(ROW * COLUMN);
  bgs = malloc(ROW * COLUMN);
  int start = COLUMN / 2 - 5;
  strcpy("SNAKE GAME", chars + start + COLUMN);
  snake.capacity = 10;
  snake.length = 3;
  snake.vel_x = 1;
  snake.vel_y = 0;
  snake.segments = malloc(sizeof(struct Segment) * snake.capacity);
  for (uint8_t i = 0; i < 3; i++)
  {
    snake.segments[i].x = SNAKE_COLUMN / 2 - i;
    snake.segments[i].y = SNAKE_ROW / 2;
  }
  struct KeyboardEvents events;
  clear_screen();
  reset_keyboard_events();
  new_food();
  do
  {

    // waitsecond();
    waitbusy(initial_wait_loop);

    get_keyboard_events(&events);
    if (events.scancodes[KEY_UP_DOWN])
    {
      change_dir(1);
    }
    else if (events.scancodes[KEY_RIGHT_DOWN])
    {
      change_dir(2);
    }
    else if (events.scancodes[KEY_DOWN_DOWN])
    {
      change_dir(3);
    }
    else if (events.scancodes[KEY_LEFT_DOWN])
    {
      change_dir(4);
    }
    reset_keyboard_events();
    walk();
    check_collision();
    update_framebuffer();
  } while (snake.alive);
  start = COLUMN / 2 - 5;
  strcpy("GAME OVER", chars + start + COLUMN * 3);
  update_framebuffer();
  for (int i = 0; i < 5; i++)
  {
    waitsecond();
  }
  free(chars);
  free(fgs);
  free(bgs);
  free(snake.segments);
  clear_screen();
}