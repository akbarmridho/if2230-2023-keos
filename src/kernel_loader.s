global loader                                 ; the entry symbol for ELF
global enter_protected_mode                   ; go to protected mode
global set_tss_register                       ; set tss register to GDT entry
extern kernel_setup                           ; kernel C entrypoint
extern _paging_kernel_page_directory          ; kernel page directory

KERNEL_VIRTUAL_BASE equ 0xC0000000            ; kernel virtual memory
KERNEL_STACK_SIZE   equ 2097152               ; size of stack in bytes
MAGIC_NUMBER        equ 0x1BADB002            ; define the magic number constant
FLAGS               equ 0x0                   ; multiboot flags
CHECKSUM            equ -MAGIC_NUMBER         ; calculate the checksum
                                              ; (magic number + checksum + flags should equal 0)


section .bss
align 4                                       ; align at 4 bytes
kernel_stack:                                 ; label points to beginning of memory
    resb KERNEL_STACK_SIZE                    ; reserve stack for the kernel


section .multiboot                            ; GRUB multiboot header
align 4                                       ; the code must be 4 byte aligned
    dd MAGIC_NUMBER                           ; write the magic number to the machine code,
    dd FLAGS                                  ; the flags,
    dd CHECKSUM                               ; and the checksum



section .setup.text                           ; start of the text (code) section
loader equ (loader_entrypoint - KERNEL_VIRTUAL_BASE)
loader_entrypoint:                            ; the loader label (defined as entry point in linker script)
    ; Set CR3 (CPU page register)
    mov eax, _paging_kernel_page_directory - KERNEL_VIRTUAL_BASE
    mov cr3, eax

    ; Use 4 MB paging
    mov eax, cr4
    or  eax, 0x00000010    ; PSE (4 MB paging)
    mov cr4, eax

    ; Enable paging
    mov eax, cr0
    or  eax, 0x80000000    ; PG flag
    mov cr0, eax

    ; Jump into higher half first, cannot use C because call stack is still not working
    lea eax, [loader_virtual]
    jmp eax

loader_virtual:
    mov dword [_paging_kernel_page_directory], 0
    invlpg [0] ; Delete identity mapping and invalidate TLB cache for first page
    mov esp, kernel_stack + KERNEL_STACK_SIZE ; Setup stack register to proper location
    call kernel_setup
.loop:
    jmp .loop                                 ; loop forever



section .text
; More details: https://en.wikibooks.org/wiki/X86_Assembly/Protected_Mode
enter_protected_mode:
    ; Load GDT from GDTDescriptor
    cli
    mov  eax, [esp+4]
    lgdt [eax]
    
    ; Set Protection Enable bit-flag in Control Register 0 (CR0)
    ; Or in other words: Switch to protected mode
    mov  eax, cr0
    or   eax, 1
    mov  cr0, eax

    ; Far jump to update cs register
    ; Warning: Invalid GDT will raise exception in any instruction below
    jmp 0x8:flush_cs
flush_cs:
    ; Update all segment register
    mov ax, 10h
    mov ss, ax
    mov ds, ax
    mov es, ax

    ret

set_tss_register:
    mov ax, 0x28 | 0   ; GDT TSS Selector, ring 0
    ltr ax
    ret
