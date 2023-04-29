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

    .sys ALIGN(4):
    {
        sys.o(.text)
        sys.o(.data)
        sys.o(.rodata*)
    }

    .string ALIGN(4):
    {
        string.o(.text)
        string.o(.data)
        string.o(.rodata*)
    }

    .math ALIGN(4):
    {
        math.o(.text)
        math.o(.data)
        math.o(.rodata*)
    }

    .command ALIGN(4):
    {
        command.o(.text)
        command.o(.data)
        command.o(.rodata*)
    }

    .ext2-api ALIGN(4):
    {
        ext2-api.o(.text)
        ext2-api.o(.data)
        ext2-api.o(.rodata*)
    }

    .utils ALIGN(4):
    {
        utils.o(.text)
        utils.o(.data)
        utils.o(.rodata*)
    }

    .snake ALIGN(4):
    {
        snake.o(.text)
        snake.o(.data)
        snake.o(.rodata*)
    }

    _linker_user_program_end = .;
    ASSERT ((_linker_user_program_end <= 1 * 1024 * 1024), "Error: User program linking result is more than 1 MiB")
}