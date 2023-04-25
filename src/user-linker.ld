ENTRY(_start)
OUTPUT_FORMAT("binary")

SECTIONS {
    . = 0x00000000;    /* Assuming OS will load this program at virtual address 0x00000000 */
    
    .text ALIGN(4):
    {
        user-entry.o(.text)  /* Put user-entry at front of executable */
        *(.text)
    }

    .data ALIGN(4):
    {
        *(.data)
    }

    .rodata ALIGN(4):
    {
        *(.rodata*)
    }
    _linker_user_program_end = .;
    ASSERT ((_linker_user_program_end <= 1 * 1024 * 1024), "Error: User program linking result is more than 1 MiB")
}