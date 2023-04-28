#include "lib-header/framebuffer.h"
#include "lib-header/stdtype.h"
#include "lib-header/stdmem.h"
#include "lib-header/portio.h"

struct FramebufferState framebuffer_state = {
    .col = 0,
    .row = 0};

void enable_cursor(uint8_t start, uint8_t end)
{
    out(0x3D4, 0x0A);
    out(0x3D5, (in(0x3D5) & 0xC0) | start);

    out(0x3D4, 0x0B);
    out(0x3D5, (in(0x3D5) & 0xE0) | end);
}

void disable_cursor()
{
    out(0x3D4, 0x0A);
    out(0x3D5, 0x20);
}
void framebuffer_set_cursor(uint8_t r, uint8_t c)
{
    disable_cursor();
    enable_cursor(14, 15);

    uint16_t pos = COLUMN * r + c;

    out(0x3D4, 0x0F);
    out(0x3D5, (uint8_t)(pos & 0xFF));
    out(0x3D4, 0x0E);
    out(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

void framebuffer_write(uint8_t row, uint8_t col, char c, uint8_t fg, uint8_t bg)
{
    uint8_t *position = MEMORY_FRAMEBUFFER + COLUMN * 2 * row + col * 2;
    uint16_t attributes = (bg << 4) | (fg & 0x0F);

    memset(position, c, 1);
    memset(position + 1, attributes, 1);
}

void framebuffer_clear(void)
{
    for (int i = 0; i < ROW; i++)
    {
        for (int j = 0; j < COLUMN; j++)
        {
            framebuffer_write(i, j, ' ', 0xFF, 0);
        }
    }
}

void clear_screen(void)
{
    framebuffer_clear();
    framebuffer_state.row = 0;
    framebuffer_state.col = 0;
}

void scroll_up()
{
    memcpy(MEMORY_FRAMEBUFFER, MEMORY_FRAMEBUFFER + COLUMN * 2, COLUMN * 2 * ROW - COLUMN * 2);
    framebuffer_state.start_row--;
    framebuffer_state.row--;
    for (int i = 0; i < COLUMN; i++)
    {
        framebuffer_write(framebuffer_state.row, i, ' ', 0xF, 0);
    }
}

void put_char(char c, uint32_t color)
{
    if (c != '\n')
        framebuffer_write(framebuffer_state.row, framebuffer_state.col, c, color, 0);
    if (framebuffer_state.col == COLUMN - 1 || c == '\n')
    {
        framebuffer_state.row++;
        framebuffer_state.col = 0;
        if (framebuffer_state.row == ROW)
            scroll_up();
    }
    else
    {
        framebuffer_state.col++;
    }
}

void puts(const char *str, uint32_t count, uint32_t color)
{
    for (uint32_t i = 0; i < count; i++)
    {
        if (str[i] == '\0')
            break;
        put_char(str[i], color);
    }
    framebuffer_set_cursor(framebuffer_state.row, framebuffer_state.col);
}