#include "interrupt.h"
#include "../lib-header/portio.h"
#include "../lib-header/keyboard.h"
#include "../lib-header/framebuffer.h"
#include "../filesystem/ext2.h"
#include "../lib-header/stdmem.h"
#include "../lib-header/memory.h"
#include "../lib-header/string.h"
#include "../lib-header/cmos.h"
#include "./idt.h"

struct TSSEntry _interrupt_tss_entry = {
    .ss0 = GDT_KERNEL_DATA_SEGMENT_SELECTOR};

void io_wait(void)
{
  out(0x80, 0);
}

void pic_ack(uint8_t irq)
{
  if (irq >= 8)
  {
    out(PIC2_COMMAND, PIC_ACK);
  }
  out(PIC1_COMMAND, PIC_ACK);
}

void pic_remap(void)
{
  uint8_t a1, a2;

  // Save masks
  a1 = in(PIC1_DATA);
  a2 = in(PIC2_DATA);

  // Starts the initialization sequence in cascade mode
  out(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
  io_wait();
  out(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
  io_wait();
  out(PIC1_DATA, PIC1_OFFSET); // ICW2: Master PIC vector offset
  io_wait();
  out(PIC2_DATA, PIC2_OFFSET); // ICW2: Slave PIC vector offset
  io_wait();
  out(PIC1_DATA, 0b0100); // ICW3: tell Master PIC that there is a slave PIC at IRQ2 (0000 0100)
  io_wait();
  out(PIC2_DATA, 0b0010); // ICW3: tell Slave PIC its cascade identity (0000 0010)
  io_wait();

  out(PIC1_DATA, ICW4_8086);
  io_wait();
  out(PIC2_DATA, ICW4_8086);
  io_wait();

  // Restore masks
  out(PIC1_DATA, a1);
  out(PIC2_DATA, a2);
}

void syscall(struct CPURegister cpu, __attribute__((unused)) struct InterruptStack info)
{
  char *chars;
  uint8_t *fgs;
  uint8_t *bgs;

  switch (cpu.eax)
  {
  case 0:
    *((int8_t *)cpu.ecx) = read(*(struct EXT2DriverRequest *)cpu.ebx);
    break;
  case 1:
    *((int8_t *)cpu.ecx) = read_directory((struct EXT2DriverRequest *)cpu.ebx);
    break;
  case 2:
    *((int8_t *)cpu.ecx) = read_next_directory_table(*(struct EXT2DriverRequest *)cpu.ebx);
    break;
  case 3:
    *((int8_t *)cpu.ecx) = write((struct EXT2DriverRequest *)cpu.ebx);
    break;
  case 4:
    *((int8_t *)cpu.ecx) = delete (*(struct EXT2DriverRequest *)cpu.ebx);
    break;
  case 5:
    keyboard_state_activate();
    __asm__("sti"); // Due IRQ is disabled when main_interrupt_handler() called
    while (is_keyboard_blocking())
      ;
    char buf[KEYBOARD_BUFFER_SIZE];
    get_keyboard_buffer(buf);
    memcpy((char *)cpu.ebx, buf, cpu.ecx);
    break;
  case 6:
    puts((char *)cpu.ebx, cpu.ecx, cpu.edx); // Modified puts() on kernel side
    break;
  case 7:
    *((uint32_t *)cpu.ebx) = malloc(cpu.ecx);
    break;
  case 8:
    *((bool *)cpu.ebx) = free((void *)cpu.ecx);
    break;
  case 9:
    // get text
    __asm__("sti"); // Due IRQ is disabled when main_interrupt_handler() called
    uint32_t allocated = 0;
    bool finished = FALSE;
    do
    {
      keyboard_state_activate();
      keyboard_text_mode();
      while (is_keyboard_blocking())
        ;
      // available buffer size left
      uint32_t available_space = cpu.ecx - allocated;
      if (available_space >= KEYBOARD_BUFFER_SIZE)
      {
        get_keyboard_buffer((char *)cpu.ebx + allocated);
        allocated += strlen((char *)cpu.ebx + allocated);
      }
      else
      {
        get_keyboard_buffer(buf);
        allocated += strlen(buf);
        memcpy((char *)cpu.ebx + allocated, buf, available_space);
      }
      // abort is keyboard aborted or space not available
      finished = is_keyboard_aborted() || available_space <= KEYBOARD_BUFFER_SIZE;
    } while (!finished);
    *((uint32_t *)cpu.edx) = allocated + 1;
    break;
  case 10:
    clear_screen();
    break;
  case 11:
    get_keyboard_events((struct KeyboardEvents *)cpu.ebx);
    break;
  case 12:
    reset_events();
    break;
  case 13:
    *((uint32_t *)cpu.ebx) = realloc((void *)cpu.ecx, cpu.edx);
    break;
  case 14:
    *((bool *)cpu.ebx) = get_timestamp();
    break;
  case 15:
    disable_cursor();
    chars = (char *)cpu.ebx;
    fgs = (uint8_t *)cpu.ecx;
    bgs = (uint8_t *)cpu.edx;
    for (uint32_t i = 0; i < ROW; i++)
    {
      for (uint32_t j = 0; j < COLUMN; j++)
      {
        uint32_t pos = i * COLUMN + j;
        framebuffer_write(i, j, chars[pos], fgs[pos], bgs[pos]);
      }
    }
    break;
  case 16:
    uint16_t **values = (uint16_t **)cpu.ebx;
    read_rtc(values[0], values[1], values[2], values[3], values[4], values[5]);
    break;
  case 17:
    *((int8_t *)cpu.ecx) = move_dir(*(struct EXT2DriverRequest *)cpu.ebx, *(struct EXT2DriverRequest *)cpu.edx);
    break;
  default:
    break;
  }
}

void main_interrupt_handler(
    __attribute__((unused)) struct CPURegister cpu,
    uint32_t int_number,
    __attribute__((unused)) struct InterruptStack info)
{
  switch (int_number)
  {
  case 0x21:
    keyboard_isr();
    break;
  case 0x30:
    syscall(cpu, info);
    break;
  }
}

void activate_keyboard_interrupt(void)
{
  out(PIC1_DATA, PIC_DISABLE_ALL_MASK ^ (1 << IRQ_KEYBOARD));
  out(PIC2_DATA, PIC_DISABLE_ALL_MASK);
}

void set_tss_kernel_current_stack(void)
{
  uint32_t stack_ptr;
  // Reading base stack frame instead esp
  __asm__ volatile("mov %%ebp, %0"
                   : "=r"(stack_ptr)
                   : /* <Empty> */);
  // Add 8 because 4 for ret address and other 4 is for stack_ptr variable
  _interrupt_tss_entry.esp0 = stack_ptr + 8;
}
