#include "lib-header/portio.h"
#include "lib-header/stdtype.h"
#include "lib-header/stdmem.h"
#include "lib-header/gdt.h"
#include "lib-header/framebuffer.h"
#include "lib-header/kernel_loader.h"
#include "interrupt/interrupt.h"
#include "lib-header/keyboard.h"
#include "interrupt/idt.h"
#include "filesystem/fat32.h"
#include "lib-header/cmos.h"

void kernel_setup(void)
{
    enter_protected_mode(&_gdt_gdtr);
    pic_remap();
    initialize_idt();
    activate_keyboard_interrupt();
    framebuffer_clear();
    framebuffer_set_cursor(0, 0);

    initialize_filesystem_fat32();

    struct ClusterBuffer cbuf[5];
    for (uint32_t i = 0; i < 5; i++)
    {
        for (uint32_t j = 0; j < CLUSTER_SIZE; j++)
        {
            cbuf[i].buf[j] = i + 'a';
        }
    }

    struct FAT32DriverRequest request = {
        .buf = cbuf,
        .name = "ikanaide",
        .ext = "uwu",
        .parent_cluster_number = ROOT_CLUSTER_NUMBER,
        .buffer_size = 0,
    };

    write(request); // Create folder "ikanaide"
    memcpy(request.name, "kano1\0\0\0", 8);
    write(request); // Create folder "kano1"
    memcpy(request.name, "ikanaide", 8);
    delete (request); // Delete first folder, thus creating hole in FS

    memcpy(request.name, "daijoubu", 8);
    request.buffer_size = 5 * CLUSTER_SIZE;
    write(request); // Create fragmented file "daijoubu"

    struct ClusterBuffer readcbuf;
    read_clusters(&readcbuf, ROOT_CLUSTER_NUMBER + 1, 1);
    // If read properly, readcbuf should filled with 'a'

    request.buffer_size = CLUSTER_SIZE;
    read(request); // Failed read due not enough buffer size
    request.buffer_size = 5 * CLUSTER_SIZE;
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