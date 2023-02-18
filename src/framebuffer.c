#include "lib-header/framebuffer.h"
#include "lib-header/stdtype.h"
#include "lib-header/stdmem.h"
#include "lib-header/portio.h"

void framebuffer_set_cursor(uint8_t r, uint8_t c)
{
    // TODO : Implement
}

void framebuffer_write(uint8_t row, uint8_t col, char c, uint8_t fg, uint8_t bg)
{
    uint8_t *position = MEMORY_FRAMEBUFFER + COLUMN * 2 * row + col * 2;
    uint16_t attributes = (bg << 4) | (fg & 0x0F);
    // uint16_t content = c | (attributes << 8);

    memset(position, c, 1);
    memset(position + 1, attributes, 1);
}

void framebuffer_clear(void)
{
    // TODO : Implement
}
