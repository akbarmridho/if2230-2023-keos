#include "lib-header/keyboard.h"
#include "lib-header/portio.h"
#include "lib-header/framebuffer.h"
#include "lib-header/stdmem.h"

const char keyboard_scancode_1_to_ascii_map[256] = {
    0,
    0x1B,
    '1',
    '2',
    '3',
    '4',
    '5',
    '6',
    '7',
    '8',
    '9',
    '0',
    '-',
    '=',
    '\b',
    '\t',
    'q',
    'w',
    'e',
    'r',
    't',
    'y',
    'u',
    'i',
    'o',
    'p',
    '[',
    ']',
    '\n',
    0,
    'a',
    's',
    'd',
    'f',
    'g',
    'h',
    'j',
    'k',
    'l',
    ';',
    '\'',
    '`',
    0,
    '\\',
    'z',
    'x',
    'c',
    'v',
    'b',
    'n',
    'm',
    ',',
    '.',
    '/',
    0,
    '*',
    0,
    ' ',
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    '-',
    0,
    0,
    0,
    '+',
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,

    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
};

static struct KeyboardDriverState keyboard_state;

/* -- Driver Interfaces -- */

// Activate keyboard ISR / start listen keyboard & save to buffer
void keyboard_state_activate(void)
{
    keyboard_state.keyboard_input_on = 1;
}

// Deactivate keyboard ISR / stop listening keyboard interrupt
void keyboard_state_deactivate(void)
{
    keyboard_state.keyboard_input_on = 0;
}

// Get keyboard buffer values - @param buf Pointer to char buffer, recommended size at least KEYBOARD_BUFFER_SIZE
void get_keyboard_buffer(char *buf)
{
    memcpy(buf, keyboard_state.keyboard_buffer, KEYBOARD_BUFFER_SIZE);
}

// Check whether keyboard ISR is active or not - @return Equal with keyboard_input_on value
bool is_keyboard_blocking(void)
{
    return keyboard_state.keyboard_input_on;
}

/* -- Keyboard Interrupt Service Routine -- */

/**
 * Handling keyboard interrupt & process scancodes into ASCII character.
 * Will start listen and process keyboard scancode if keyboard_input_on.
 *
 * Will only print printable character into framebuffer.
 * Stop processing when enter key (line feed) is pressed.
 *
 * Note that, with keyboard interrupt & ISR, keyboard reading is non-blocking.
 * This can be made into blocking input with `while (is_keyboard_blocking());`
 * after calling `keyboard_state_activate();`
 */

int row = 0;
int col = 0;

void clear_screen()
{
    framebuffer_clear();
    row = 0;
    col = 0;
}
void keyboard_isr(void)
{
    if (!keyboard_state.keyboard_input_on)
        keyboard_state.buffer_index = 0;
    else
    {
        uint8_t scancode = in(KEYBOARD_DATA_PORT);
        char mapped_char = keyboard_scancode_1_to_ascii_map[scancode];
        // TODO : Implement scancode processing
        if (mapped_char != '\0') // check if the character is valid
        {
            if (mapped_char == '\b')
            {
                if (keyboard_state.buffer_index > 0)
                {
                    keyboard_state.buffer_index--;
                    keyboard_state.keyboard_buffer[keyboard_state.buffer_index] = 0;
                    col--;
                    if (col < 0)
                    {
                        row--;
                        col = COLUMN;
                    }
                    if (row < 0)
                    {
                        row = 0;
                        col = 0;
                    }

                    framebuffer_write(row, col, ' ', 0xFF, 0);
                }
            }
            else if (mapped_char == '\n')
            {
                memset(keyboard_state.keyboard_buffer, '\0', sizeof(keyboard_state.keyboard_buffer));
                keyboard_state.buffer_index = 0;
                keyboard_state.keyboard_input_on = 0;
                row++;
                col = 0;
            }
            else
            {
                keyboard_state.keyboard_buffer[keyboard_state.buffer_index] = mapped_char;
                keyboard_state.buffer_index++;

                // write the last character to the screen
                if (col >= COLUMN)
                {
                    row++;
                    col = 0;
                }
                framebuffer_write(row, col, mapped_char, 0xFF, 0);
                col++;
            }
            framebuffer_set_cursor(row, col);
        }
        // write the entire keyboard buffer to the screen

        // framebuffer_write(row, col, keyboard_state.keyboard_buffer[keyboard_state.buffer_index], 0xFF, 0x00);

        // memset(keyboard_state.keyboard_buffer, '\0', sizeof(keyboard_state.keyboard_buffer));
        // keyboard_state.buffer_index = 0;
    }
    pic_ack(IRQ_KEYBOARD);
}
