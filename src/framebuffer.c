#include "lib-header/framebuffer.h"
#include "lib-header/stdtype.h"
#include "lib-header/stdmem.h"
#include "lib-header/portio.h"

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
    memset(MEMORY_FRAMEBUFFER, 0, COLUMN * 2 * ROW);
}
