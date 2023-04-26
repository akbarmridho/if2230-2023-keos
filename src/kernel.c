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
#include "lib-header/paging.h"

void kernel_setup(void)
{
    enter_protected_mode(&_gdt_gdtr);
    pic_remap();
    initialize_idt();
    activate_keyboard_interrupt();
    framebuffer_clear();
    framebuffer_set_cursor(0, 0);

    initialize_filesystem_ext2();

    gdt_install_tss();
    set_tss_register();

    // struct BlockBuffer bbuf[10];
    // for (uint32_t i = 0; i < 10; i++)
    // {
    //     for (uint32_t j = 0; j < BLOCK_SIZE; j++)
    //     {
    //         bbuf[i].buf[j] = 'a';
    //     }
    // }

    // struct EXT2DriverRequest request = {
    //     .buf = bbuf,
    //     .name = "ikanaide",
    //     .ext = "uwu",
    //     .inode = 1,
    //     .buffer_size = 0,
    //     .name_len = 9,
    // };

    // char ikanaide[9] = "ikanaide";

    // ignore error, just for debugging
    // #pragma GCC diagnostic push
    // #pragma GCC diagnostic ignored "-Wunused-but-set-variable"
    //     int8_t response;
    // #pragma GCC diagnostic pop

    // response = write(request); // Create folder "ikanaide"
    // memcpy(request.name, "kano1\0\0\0", 8);
    // request.name_len = 6;
    // response = write(request); // Create folder "kano1"
    // memcpy(request.name, ikanaide, 9);
    // request.name_len = 9;
    // request.is_dir = TRUE;
    // response = delete (request); // Delete first folder, thus creating hole in FS

    // memcpy(request.name, "daijoubu", 9);
    // request.name_len = 9;
    // char to_write[] = "wangy wangy oessssss";
    // request.buf = (void *)to_write;
    // request.buffer_size = 21;
    // response = write(request); // Create fragmented file "daijoubu"

    // struct BlockBuffer readbbuf[20];
    // request.buf = readbbuf;
    // response = read(request);
    // read_blocks(&readbbuf, 9, 1);
    // // If read properly, readbbuf should starts with 'wangy wangy oessssss'
    // for (int i = 0; i < 20; i++)
    // {
    //     memset(readbbuf[i].buf, 'z' - i, BLOCK_SIZE);
    // }
    // request.name = "testbigfile";
    // request.name_len = 13;
    // request.buffer_size = BLOCK_SIZE * 20;
    // response = write(request);

    // request.buf = bbuf;
    // request.buffer_size = 5;

    // read(request); // Failed read due not enough buffer size
    // request.buffer_size = BLOCK_SIZE * 20;
    // read(request); // Success read on file "daijoubu"

    // uint16_t year;
    // uint16_t month;
    // uint16_t day;
    // uint16_t hour;
    // uint16_t minute;
    // uint16_t second;

    // read_rtc(&year, &month, &day, &hour, &minute, &second);

    // allocate_single_user_page_frame((void *)0x500000);
    // *((uint8_t *)0x500000) = 1;

    allocate_single_user_page_frame((uint8_t *)0);

    struct EXT2DriverRequest req =
        {
            .buf = (uint8_t *)0,
            .name = "shell",
            .ext = "\0\0\0",
            .inode = 1,
            .buffer_size = 0x100000,
            .name_len = 5,
        };

    int retcode = read(req);
    (void)retcode;

    set_tss_kernel_current_stack();
    kernel_execute_user_program((uint8_t *)0);

    keyboard_state_activate();

    while (TRUE)
        ;
}