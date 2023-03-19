extern main_interrupt_handler
global isr_stub_table

; Generic handler section for interrupt
call_generic_handler:
    ; Before interrupt_handler_n is called (caller of this generic handler section), 
    ; stack will have these value that pushed automatically by CPU
    ; [esp + 12] eflags
    ; [esp + 8 ] cs
    ; [esp + 4 ] eip
    ; [esp + 0 ] error code

    ; CPURegister
    push    esp
    push    ebp
    push    edx
    push    ecx
    push    ebx
    push    eax


    ; call the C function
    call    main_interrupt_handler

    ; restore the registers
    pop     eax
    pop     ebx
    pop     ecx
    pop     edx
    pop     ebp
    pop     esp

    ; restore the esp (interrupt number & error code)
    add     esp, 8

    ; return to the code that got interrupted
    ; at this point, stack should be structured like this
    ; [esp], [esp+4], [esp+8]
    ;   eip,   cs,    eflags
    ; improper value will cause invalid return address & register
    sti
    iret



; Macro for creating interrupt handler that only push interrupt number
%macro no_error_code_interrupt_handler 1
interrupt_handler_%1:
    push    dword 0                 ; push 0 as error code
    push    dword %1                ; push the interrupt number
    jmp     call_generic_handler    ; jump to the common handler
%endmacro

%macro error_code_interrupt_handler 1
interrupt_handler_%1:
    push    dword %1
    jmp     call_generic_handler
%endmacro

; CPU exception handlers
no_error_code_interrupt_handler 0  ; 0x0  - Division by zero
no_error_code_interrupt_handler 1  ; 0x1  - Debug Exception
no_error_code_interrupt_handler 2  ; 0x2  - NMI, Non-Maskable Interrupt
no_error_code_interrupt_handler 3  ; 0x3  - Breakpoint Exception
no_error_code_interrupt_handler 4  ; 0x4  - INTO Overflow
no_error_code_interrupt_handler 5  ; 0x5  - Out of Bounds
no_error_code_interrupt_handler 6  ; 0x6  - Invalid Opcode
no_error_code_interrupt_handler 7  ; 0x7  - Device Not Available
error_code_interrupt_handler    8  ; 0x8  - Double Fault
no_error_code_interrupt_handler 9  ; 0x9  - Deprecated
error_code_interrupt_handler    10 ; 0xA  - Invalid TSS
error_code_interrupt_handler    11 ; 0xB  - Segment Not Present
error_code_interrupt_handler    12 ; 0xC  - Stack-Segment Fault
error_code_interrupt_handler    13 ; 0xD  - General Protection Fault
error_code_interrupt_handler    14 ; 0xE  - Page Fault
no_error_code_interrupt_handler 15 ; 0xF  - Reserved
no_error_code_interrupt_handler 16 ; 0x10 - x87 Floating-Point Exception
error_code_interrupt_handler    17 ; 0x11 - Alignment Check Exception
no_error_code_interrupt_handler 18 ; 0x12 - Machine Check Exception
no_error_code_interrupt_handler 19 ; 0x13 - SIMD Floating-Point Exception
no_error_code_interrupt_handler 20 ; 0x14 - Virtualization Exception
no_error_code_interrupt_handler 21 ; 0x15 - Control Protection Exception
no_error_code_interrupt_handler 22 ; 0x16 - Reserved
no_error_code_interrupt_handler 23 ; 0x17 - Reserved
no_error_code_interrupt_handler 24 ; 0x18 - Reserved
no_error_code_interrupt_handler 25 ; 0x19 - Reserved
no_error_code_interrupt_handler 26 ; 0x1A - Reserved
no_error_code_interrupt_handler 27 ; 0x1B - Reserved
no_error_code_interrupt_handler 28 ; 0x1C - Hypervisor Injection Exception
no_error_code_interrupt_handler 29 ; 0x1D - VMM Communication Exception
error_code_interrupt_handler    30 ; 0x1E - Security Exception
no_error_code_interrupt_handler 31 ; 0x1F - Reserved

; User defined interrupt handler
; Assuming PIC1 & PIC2 offset is 0x20 and 0x28
; 32 - 0x20 - IRQ0:  Programmable Interval Timer
; 33 - 0x21 - IRQ1:  Keyboard
; 34 - 0x22 - IRQ2:  PIC Cascade, used internally
; 35 - 0x23 - IRQ3:  COM2, if enabled
; 36 - 0x24 - IRQ4:  COM1, if enabled
; 37 - 0x25 - IRQ5:  LPT2, if enabled
; 38 - 0x26 - IRQ6:  Floppy Disk
; 39 - 0x27 - IRQ7:  LPT1

; 40 - 0x28 - IRQ8:  CMOS real-time clock
; 41 - 0x29 - IRQ9:  Free
; 42 - 0x2A - IRQ10: Free
; 43 - 0x2B - IRQ11: Free
; 44 - 0x2C - IRQ12: PS2 Mouse
; 45 - 0x2D - IRQ13: Coprocessor
; 46 - 0x2E - IRQ14: Primary ATA Hard Disk
; 47 - 0x2F - IRQ15: Secondary ATA Hard Disk
%assign i 32 
%rep    32
no_error_code_interrupt_handler i
%assign i i+1 
%endrep



; ISR stub table, useful for reducing code repetition
isr_stub_table:
    %assign i 0 
    %rep    64 
    dd interrupt_handler_%+i
    %assign i i+1 
    %endrep