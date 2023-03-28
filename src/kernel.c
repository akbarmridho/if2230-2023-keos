#include "lib-header/portio.h"
#include "lib-header/stdtype.h"
#include "lib-header/stdmem.h"
#include "lib-header/gdt.h"
#include "lib-header/framebuffer.h"
#include "lib-header/kernel_loader.h"
#include "interrupt/interrupt.h"
#include "lib-header/keyboard.h"
#include "interrupt/idt.h"
#include "filesystem/ext2.h"
#include "lib-header/cmos.h"

void kernel_setup(void)
{
    enter_protected_mode(&_gdt_gdtr);
    pic_remap();
    initialize_idt();
    activate_keyboard_interrupt();
    framebuffer_clear();
    framebuffer_set_cursor(0, 0);

    initialize_filesystem_ext2();

    struct BlockBuffer bbuf[10];
    for (uint32_t i = 0; i < 10; i++)
    {
        for (uint32_t j = 0; j < BLOCK_SIZE; j++)
        {
            bbuf[i].buf[j] = i + 'a';
        }
    }

    struct EXT2DriverRequest request = {
        .buf = bbuf,
        .name = "ikanaide",
        .ext = "uwu",
        .inode = 1,
        .buffer_size = 0,
        .name_len = 9,
    };

    write(request); // Create folder "ikanaide"
    memcpy(request.name, "kano1\0\0\0", 8);
    request.name_len = 6;
    write(request); // Create folder "kano1"
    memcpy(request.name, "ikanaide", 8);
    request.name_len = 9;
    request.is_dir = TRUE;
    delete (request); // Delete first folder, thus creating hole in FS

    memcpy(request.name, "daijoubu", 8);
    request.name_len = 9;
    request.buffer_size = 10 * BLOCK_SIZE;
    write(request); // Create fragmented file "daijoubu"

    struct BlockBuffer readbbuf[4];
    request.buf = readbbuf;
    read(request);
    read_blocks(&readbbuf, 2, 1);
    // If read properly, readbbuf should filled with 'a'

    request.buf = bbuf;
    request.buffer_size = BLOCK_SIZE;
    read(request); // Failed read due not enough buffer size
    request.buffer_size = 10 * BLOCK_SIZE;
    read(request); // Success read on file "daijoubu"

    uint16_t year;
    uint16_t month;
    uint16_t day;
    uint16_t hour;
    uint16_t minute;
    uint16_t second;

    read_rtc(&year, &month, &day, &hour, &minute, &second);

    while (TRUE)
        keyboard_state_activate();
}