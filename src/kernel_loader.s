global loader                        ; the entry symbol for ELF
global enter_protected_mode          ; go to protected mode
extern kernel_setup                  ; kernel



KERNEL_STACK_SIZE equ 4096           ; size of stack in bytes
MAGIC_NUMBER      equ 0x1BADB002     ; define the magic number constant
FLAGS             equ 0x0            ; multiboot flags
CHECKSUM          equ -MAGIC_NUMBER  ; calculate the checksum
                                     ; (magic number + checksum + flags should equal 0)

section .bss
align 4                              ; align at 4 bytes
kernel_stack:                        ; label points to beginning of memory
    resb KERNEL_STACK_SIZE           ; reserve stack for the kernel

section .text                        ; start of the text (code) section
align 4                              ; the code must be 4 byte aligned
    dd MAGIC_NUMBER                  ; write the magic number to the machine code,
    dd FLAGS                         ; the flags,
    dd CHECKSUM                      ; and the checksum



loader:                                        ; the loader label (defined as entry point in linker script)
    mov  esp, kernel_stack + KERNEL_STACK_SIZE ; setup stack register to proper location
    call kernel_setup
.loop:
    jmp .loop                                  ; loop forever


; More details: https://en.wikibooks.org/wiki/X86_Assembly/Protected_Mode
enter_protected_mode:
    cli
    mov  eax, [esp+4]
    ; TODO: Load GDT from GDTDescriptor
    ;       eax at this line will carry GDTR location, dont forget to use square bracket [eax]

    mov  eax, cr0
    ; TODO: Set bit-0 (Protection Enable bit-flag) in Control Register 0 (CR0)
    ;       Set eax with above condition, eax will be copied to CR0 with next instruction
    mov  cr0, eax

    ; Far jump to update cs register
    ; Warning: Invalid GDT will raise exception in any instruction below
    jmp 0x8:flush_cs
flush_cs:
    mov ax, 10h
    ; TODO: Set all data segment register with 0x10
    ;       Segments register need to set with 0x10: ss, ds, es
    mov ss, ax

    ret
