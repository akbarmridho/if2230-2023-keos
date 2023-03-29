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

char shift_map[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // a
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // b
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // c
    0, 0, 0, 0, 0, 0, 0, 0, 0, '"',
    0, 0, 0, 0, '<', '_', '>', '?', ')', '!',
    '@', '#', '$', '%', '^', '&', '*', '(', 0, ':',
    0, '+', 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, '{', '|', '}', 0, 0, '~'

};

static struct KeyboardDriverState keyboard_state;

/* -- Driver Interfaces -- */

// Activate keyboard ISR / start listen keyboard & save to buffer
void keyboard_state_activate(void)
{
    keyboard_state.keyboard_input_on = TRUE;
}

// Deactivate keyboard ISR / stop listening keyboard interrupt
void keyboard_state_deactivate(void)
{
    keyboard_state.keyboard_input_on = FALSE;
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

bool is_shift()
{
    return keyboard_state.shift_left || keyboard_state.shift_right;
}

void clear_screen()
{
    framebuffer_clear();
    keyboard_state.row = 0;
    keyboard_state.col = 0;
}

void keyboard_isr(void)
{
    if (!keyboard_state.keyboard_input_on)
        keyboard_state.buffer_index = 0;
    else
    {
        uint8_t scancode = in(KEYBOARD_DATA_PORT);

        // handle capslock and shift
        switch (scancode)
        {
        case 0x3a:
            keyboard_state.capslock ^= TRUE;
            break;
        case 0x2a:
            keyboard_state.shift_left = TRUE;
            break;
        case 0xaa:
            keyboard_state.shift_left = FALSE;
            break;
        case 0x36:
            keyboard_state.shift_right = TRUE;
            break;
        case 0xb6:
            keyboard_state.shift_right = FALSE;
            break;
        default:
            break;
        }
        char mapped_char = keyboard_scancode_1_to_ascii_map[scancode];

        // check char valid
        if (mapped_char == 0)
        {
            pic_ack(IRQ_KEYBOARD);
            return;
        }
        if (mapped_char == '\b')
        {
            // backspace
            if (keyboard_state.buffer_index > 0)
            {
                keyboard_state.buffer_index--;
                keyboard_state.keyboard_buffer[keyboard_state.buffer_index] = 0;
                if (keyboard_state.col == 0)
                {
                    keyboard_state.row--;
                    keyboard_state.col = COLUMN - 1;
                    if (keyboard_state.row < 0)
                    {
                        keyboard_state.row = 0;
                        keyboard_state.col = 0;
                    }
                }
                else
                {
                    keyboard_state.col--;
                }

                framebuffer_write(keyboard_state.row, keyboard_state.col, ' ', 0xFF, 0);
            }
        }
        else if (mapped_char == '\n')
        {
            memset(keyboard_state.keyboard_buffer, '\0', sizeof(keyboard_state.keyboard_buffer));
            keyboard_state.buffer_index = 0;
            keyboard_state.keyboard_input_on = 0;
            keyboard_state.row++;
            keyboard_state.col = 0;
        }
        else
        {
            // check if letter needs to be uppercased
            if (mapped_char >= 'a' && mapped_char <= 'z')
            {
                if (keyboard_state.capslock ^ is_shift())
                    mapped_char = 'A' + mapped_char - 'a';
            }
            // map character to its shift representation if there any
            else if (is_shift() && mapped_char < 97 && shift_map[(uint8_t)mapped_char] != 0)
            {
                mapped_char = shift_map[(uint8_t)mapped_char];
            }
            keyboard_state.keyboard_buffer[keyboard_state.buffer_index] = mapped_char;
            keyboard_state.buffer_index++;

            // write the last character to the screen
            if (keyboard_state.col >= COLUMN)
            {
                keyboard_state.row++;
                keyboard_state.col = 0;
            }
            framebuffer_write(keyboard_state.row, keyboard_state.col, mapped_char, 0xFF, 0);
            keyboard_state.col++;
        }
        framebuffer_set_cursor(keyboard_state.row, keyboard_state.col);
    }
    pic_ack(IRQ_KEYBOARD);
}