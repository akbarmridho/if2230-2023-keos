#include "lib-header/paging.h"

__attribute__((aligned(0x1000))) struct PageDirectory _paging_kernel_page_directory = {
    .table = {
        [0] = {
            // user virtual memory from 0x0 to 0x300 - 1
            .flag.present_bit = 1,
            .flag.read_write = 1,
            .lower_address = 0,
            .flag.page_size = 1,
        },
        [0x300] = {
            // kernel virtual memory 0x300 to 0x400
            .flag.present_bit = 1,
            .flag.read_write = 1,
            .lower_address = 0,
            .flag.page_size = 1,
        },
    }};

struct PhysicalPageDirectory _paging_physical_page_directory = {
    .table = {
        -1}};

static struct PageDriverState page_driver_state = {
    .last_available_physical_addr = (uint8_t *)0 + PAGE_FRAME_SIZE,
};

void update_page_directory_entry(void *physical_addr, void *virtual_addr, struct PageDirectoryEntryFlag flag)
{
  uint32_t page_index = ((uint32_t)virtual_addr >> 22) & 0x3FF;

  _paging_kernel_page_directory.table[page_index].flag = flag;
  _paging_kernel_page_directory.table[page_index].lower_address = ((uint32_t)physical_addr >> 22) & 0x3FF;
  flush_single_tlb(virtual_addr);
}

uint32_t get_physical_address_page_at(uint32_t idx)
{
  // Using default QEMU config (128 MiB max memory)
  uint32_t last_physical_addr = (uint32_t)page_driver_state.last_available_physical_addr;

  return last_physical_addr + (idx + 1) * PAGE_FRAME_SIZE;
}

int8_t allocate_single_user_page_frame(void *virtual_addr)
{
  uint32_t page_index = ((uint32_t)virtual_addr >> 22) & 0x3FF;
  struct PageDirectoryEntry entry = _paging_kernel_page_directory.table[page_index];

  if (entry.flag.present_bit)
  { // used entry
    return -1;
  }

  // Algorithm: first unallocated page frame match
  bool found = FALSE;
  uint32_t i = 0;

  while (!found && i < PAGE_FRAME_SIZE)
  {
    int16_t virtual_page_index = _paging_physical_page_directory.table[i];

    if (virtual_page_index != -1)
    {
      i++;
    }
    else
    {
      found = TRUE;
    }
  }

  if (!found) // full memory
  {
    return -1;
  }

  uint32_t allocated_physical_address = get_physical_address_page_at(i);

  struct PageDirectoryEntryFlag flag = {
      .present_bit = 1,
      .read_write = 1,
      .user_supervisor = 1,
      .page_size = 1};

  update_page_directory_entry((void *)allocated_physical_address, virtual_addr, flag);

  return 0;
}

void flush_single_tlb(void *virtual_addr)
{
  asm volatile("invlpg (%0)"
               : /* <Empty> */
               : "b"(virtual_addr)
               : "memory");
}