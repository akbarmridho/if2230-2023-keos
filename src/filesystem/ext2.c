#include "ext2.h"
#include "../lib-header/stdmem.h"
#include "../lib-header/math.h"
#include "../lib-header/string.h"
#include "ext2-api.h"

#ifdef external
#include <time.h>

uint32_t get_timestamp()
{
  return (uint32_t)time(NULL);
}
#else
#include "../lib-header/cmos.h"
#endif

const uint8_t fs_signature[BLOCK_SIZE] = {
    'C',
    'o',
    'u',
    'r',
    's',
    'e',
    ' ',
    ' ',
    ' ',
    ' ',
    ' ',
    ' ',
    ' ',
    ' ',
    ' ',
    ' ',
    'D',
    'e',
    's',
    'i',
    'g',
    'n',
    'e',
    'd',
    ' ',
    'b',
    'y',
    ' ',
    ' ',
    ' ',
    ' ',
    ' ',
    'L',
    'a',
    'b',
    ' ',
    'S',
    'i',
    's',
    't',
    'e',
    'r',
    ' ',
    'I',
    'T',
    'B',
    ' ',
    ' ',
    'M',
    'a',
    'd',
    'e',
    ' ',
    'w',
    'i',
    't',
    'h',
    ' ',
    '<',
    '3',
    ' ',
    ' ',
    ' ',
    ' ',
    '-',
    '-',
    '-',
    '-',
    '-',
    '-',
    '-',
    '-',
    '-',
    '-',
    '-',
    '2',
    '0',
    '2',
    '3',
    '\n',
    [BLOCK_SIZE - 2] = 'O',
    [BLOCK_SIZE - 1] = 'k',
};

struct EXT2Superblock sblock = {};

struct BlockBuffer block_buffer = {};
struct EXT2BGDTable bgd_table = {};
struct EXT2INodeTable inode_table_buf = {};

uint32_t inode_to_bgd(uint32_t inode)
{
  return (inode - 1u) / INODES_PER_GROUP;
}

uint32_t inode_to_local(uint32_t inode)
{
  return (inode - 1) % INODES_PER_GROUP;
}

void init_directory_table(struct EXT2INode *node, uint32_t inode, uint32_t parent_inode)
{
  struct BlockBuffer block;

  struct EXT2DirectoryEntry *table = get_directory_entry(&block, 0);
  table->inode = inode;
  table->file_type = EXT2_FT_DIR;
  table->name_len = 1;
  memcpy(get_entry_name(table), ".", 2);
  table->rec_len = get_directory_record_length(table->name_len);

  struct EXT2DirectoryEntry *parent_table = get_next_directory_entry(table);
  parent_table->inode = parent_inode;
  parent_table->file_type = EXT2_FT_DIR;
  parent_table->name_len = 2;
  memcpy(get_entry_name(parent_table), "..", 3);
  parent_table->rec_len = get_directory_record_length(parent_table->name_len);

  struct EXT2DirectoryEntry *first_file = get_next_directory_entry(parent_table);
  first_file->inode = 0;

  node->mode = EXT2_S_IFDIR;
  node->size_low = 0;
  node->size_high = 0;

  // CMOS
  node->mtime = node->ctime = node->atime = get_timestamp();
  node->dtime = 0;

  node->blocks = 1;
  allocate_node_blocks(&block, node, inode_to_bgd(inode));

  sync_node(node, inode);
}

bool is_empty_storage(void)
{
  read_blocks(&block_buffer, BOOT_SECTOR, 1);
  return memcmp(&block_buffer, fs_signature, BLOCK_SIZE);
}

void create_ext2(void)
{
  write_blocks(fs_signature, BOOT_SECTOR, 1);
  sblock.inodes_count = GROUPS_COUNT * INODES_PER_GROUP;
  sblock.blocks_count = GROUPS_COUNT * BLOCKS_PER_GROUP;
  // sblock.free_blocks_count = sblock.blocks_count -
  sblock.free_inodes_count = sblock.inodes_count;
  sblock.first_data_block = 1;
  sblock.blocks_per_group = BLOCKS_PER_GROUP;
  sblock.frags_per_group = 1; // not implemented for now
  sblock.inodes_per_group = INODES_PER_GROUP;
  sblock.magic = EXT2_SUPER_MAGIC;
  sblock.first_ino = 1;

  write_blocks(&sblock, 1, 1);

  struct BlockBuffer bitmap_block;
  struct BlockBuffer bitmap_node;

  memset(&bitmap_block, 0, BLOCK_SIZE);
  memset(&bitmap_node, 0, BLOCK_SIZE);

  // initialize bgd table
  for (uint32_t i = 0; i < GROUPS_COUNT; i++)
  {
    uint32_t reserved_blocks = 2u + INODES_TABLE_BLOCK_COUNT; // block_bitmap, inode_bitmap, inodes_table
    if (i == 0)
    {
      // 3 first block is for boot sector, superblock, and bgd table
      reserved_blocks += 3;
      bgd_table.table[i].block_bitmap = 3;

      // set block_bitmap for first block
      uint32_t j = 0;
      uint32_t k = 0;
      while (j * 8 < reserved_blocks)
      {
        k = reserved_blocks - (j * 8);
        if (k > 8)
          k = 8;

        // set first k bit of a byte to 1 and rest to 0
        bitmap_block.buf[j] = 0xFFu - ((1u << (8u - k)) - 1u);
        j++;
      }

      // make j and k actually represent last byte and bit
      j--;
      k--;
      write_blocks(&bitmap_block, bgd_table.table[i].block_bitmap, 1);

      // set back the last 3 bitmap_block bit to 0
      for (uint8_t l = 0; l < 3; l++)
      {
        uint8_t offset = 7u - k;
        bitmap_block.buf[j] &= 0xFFu - (1u << offset);
        if (k == 0)
        {
          j--;
          k = 7;
        }
        else
          k--;
      }
    }
    else
    {
      bgd_table.table[i].block_bitmap = i * sblock.blocks_per_group + 0;
      write_blocks(&bitmap_block, bgd_table.table[i].block_bitmap, 1);
    }
    bgd_table.table[i].free_blocks_count = sblock.blocks_per_group - reserved_blocks;
    bgd_table.table[i].inode_bitmap = bgd_table.table[i].block_bitmap + 1;

    // write inode bitmap
    write_blocks(&bitmap_node, bgd_table.table[i].inode_bitmap, 1);
    bgd_table.table[i].inode_table = bgd_table.table[i].block_bitmap + 2;

    bgd_table.table[i].free_inodes_count = sblock.inodes_per_group;

    bgd_table.table[i].used_dirs_count = 0;
  }
  write_blocks(&bgd_table, 2, 1);
  struct EXT2INode root_dir_node;
  uint32_t root_dir_inode = allocate_node();

  init_directory_table(&root_dir_node, root_dir_inode, root_dir_inode);
}

void initialize_filesystem_ext2(void)
{
  if (is_empty_storage())
  {
    create_ext2();
  }
  else
  {
    read_blocks(&sblock, 1, 1);
    read_blocks(&bgd_table, 2, 1);
  }
}

bool is_directory_empty(uint32_t inode)
{
  uint32_t bgd = inode_to_bgd(inode);
  uint32_t local_idx = inode_to_local(inode);

  // get node corresponding to inode
  read_blocks(&inode_table_buf, bgd_table.table[bgd].inode_table, INODES_TABLE_BLOCK_COUNT);

  struct EXT2INode *node = &inode_table_buf.table[local_idx];

  // read the directory entry
  struct BlockBuffer block;
  read_blocks(&block, node->block[0], 1);

  uint32_t offset = get_directory_first_child_offset(&block);

  struct EXT2DirectoryEntry *first_child = get_directory_entry(&block, offset);

  // true if the first child is not valid entry
  return first_child->inode == 0;
}

uint32_t map_node_blocks(void *ptr, uint32_t blocks, uint32_t *locations, uint32_t *mapped_count, uint8_t depth)
{
  if (blocks == 0)
    return 0;
  if (depth == 0)
  {
    write_blocks(ptr + (*mapped_count * BLOCK_SIZE), locations[0], 1);
    *mapped_count += 1;
    return locations[0];
  }
  uint32_t location;
  uint32_t found_count = 0;
  uint32_t buffer[BLOCK_SIZE / 4u];
  search_blocks(0, &location, 1, &found_count);
  uint32_t capacity = 1;
  for (uint8_t i = 0; i < depth - 1; i++)
  {
    capacity *= BLOCK_SIZE / 4u;
  }

  for (uint32_t i = 0; i < BLOCK_SIZE / 4u; i++)
  {
    buffer[i] = map_node_blocks(ptr, blocks, locations, mapped_count, depth - 1);
    locations += capacity;
    if (blocks <= capacity)
      break;
    blocks -= capacity;
  }
  write_blocks(&buffer, location, 1);
  return location;
}

void allocate_node_blocks(void *ptr, struct EXT2INode *node, uint32_t preferred_bgd)
{
  uint32_t locations[node->blocks];
  uint32_t found_count = 0;
  uint32_t mapped_count = 0;

  search_blocks(preferred_bgd, locations, node->blocks, &found_count);

  for (uint32_t i = 0; i < 15 && mapped_count < node->blocks; i++)
  {
    if (i < 12)
    {
      // direct blocks
      write_blocks(ptr + mapped_count * BLOCK_SIZE, locations[i], 1);
      mapped_count++;
      node->block[i] = locations[i];
    }
    else
    {
      // indirect blocks
      node->block[i] = map_node_blocks(ptr, node->blocks - mapped_count, locations + mapped_count, &mapped_count, i - 11);
    }
  }
}

void sync_node(struct EXT2INode *node, uint32_t inode)
{
  uint32_t bgd = inode_to_bgd(inode);
  uint32_t local_idx = inode_to_local(inode);

  read_blocks(&inode_table_buf, bgd_table.table[bgd].inode_table, INODES_TABLE_BLOCK_COUNT);
  inode_table_buf.table[local_idx] = *node;

  write_blocks(&inode_table_buf, bgd_table.table[bgd].inode_table, INODES_TABLE_BLOCK_COUNT);
}

// assume node always available
uint32_t allocate_node(void)
{
  uint32_t bgd = -1;
  for (uint32_t i = 0; i < GROUPS_COUNT; i++)
  {
    if (bgd_table.table[i].free_inodes_count > 0)
    {
      bgd = i;
      break;
    }
  }

  // search free node
  read_blocks(&block_buffer, bgd_table.table[bgd].inode_bitmap, 1);
  uint32_t inode = bgd * INODES_PER_GROUP + 1;
  uint32_t location = 0;
  for (uint32_t i = 0; i < INODES_PER_GROUP; i++)
  {
    uint8_t byte = block_buffer.buf[i / 8];
    uint8_t offset = 7 - i % 8;
    if (!((byte >> offset) & 1u))
    {
      location = i;
      // set flag of the inode
      block_buffer.buf[i / 8] |= 1u << offset;
      break;
    }
  }
  // update inode_bitmap
  write_blocks(&block_buffer, bgd_table.table[bgd].inode_bitmap, 1);

  inode += location;

  bgd_table.table[bgd].free_inodes_count--;

  write_blocks(&bgd_table, 2, 1);

  return inode;
}

void deallocate_node(uint32_t inode)
{
  uint32_t bgd = inode_to_bgd(inode);
  uint32_t local_idx = inode_to_local(inode);

  // get node corresponding to inode
  read_blocks(&inode_table_buf, bgd_table.table[bgd].inode_table, INODES_TABLE_BLOCK_COUNT);

  struct EXT2INode *node = &inode_table_buf.table[local_idx];

  deallocate_blocks(node->block, node->blocks);

  // search free node
  read_blocks(&block_buffer, bgd_table.table[bgd].inode_bitmap, 1);

  uint8_t offset = 7 - local_idx % 8;

  // set flag of the inode
  block_buffer.buf[local_idx / 8] &= 0xFFu - (1u << offset);

  // update inode_bitmap
  write_blocks(&block_buffer, bgd_table.table[bgd].inode_bitmap, 1);

  bgd_table.table[bgd].free_inodes_count++;
  if (node->mode == EXT2_S_IFDIR)
  {
    bgd_table.table[bgd].used_dirs_count--;
  }
  // sync bgd table
  write_blocks(&bgd_table, 2, 1);

  // CMOS
  node->dtime = get_timestamp();
  sync_node(node, inode);
}

void deallocate_blocks(void *_locations, uint32_t blocks)
{
  if (blocks == 0)
    return;

  uint32_t *locations = (uint32_t *)_locations;
  uint32_t last_bgd = 0;
  struct BlockBuffer block;

  for (uint32_t i = 0; i < 15 && blocks > 0; i++)
  {
    uint32_t deallocated;
    if (i < 12)
    {
      deallocated = deallocate_block(locations + i, blocks, &block, 0, &last_bgd, i != 0);
    }
    else
    {
      deallocated = deallocate_block(locations + i, blocks, &block, i - 11, &last_bgd, TRUE);
    }
    blocks -= deallocated;
    uint32_t bgd = locations[i] / BLOCKS_PER_GROUP;
  }
  // update last bgd bitmap
  write_blocks(&block, bgd_table.table[last_bgd].block_bitmap, 1);
  // sync bgd
  write_blocks(&bgd_table, 2, 1);
}

uint32_t deallocate_block(uint32_t *locations, uint32_t blocks, struct BlockBuffer *bitmap, uint32_t depth, uint32_t *last_bgd, bool bgd_loaded)
{
  if (blocks == 0)
    return 0;
  uint32_t bgd = *locations / BLOCKS_PER_GROUP;
  if (!bgd_loaded || bgd != *last_bgd)
  {
    if (bgd_loaded)
    {
      // update previous bgd_bitmap
      write_blocks(bitmap, bgd_table.table[*last_bgd].block_bitmap, 1);
    }
    // load a new one
    read_blocks(bitmap, bgd_table.table[bgd].block_bitmap, 1);
    *last_bgd = bgd;
  }
  uint32_t local_idx = *locations % BLOCKS_PER_GROUP;
  uint8_t offset = 7 - local_idx % 8;
  // set flag of the block bitmap
  bitmap->buf[local_idx / 8] &= 0xFFu ^ (1u << offset);
  bgd_table.table[*last_bgd].free_blocks_count++;
  if (depth == 0)
  {
    return 1;
  }
  struct BlockBuffer child_buf;

  // load the direct block
  read_blocks(child_buf.buf, *locations, 1);
  uint32_t *child_loc = (uint32_t *)child_buf.buf;
  uint32_t deallocated = 0;
  for (uint32_t i = 0; i < BLOCK_SIZE / 4u; i++)
  {
    uint32_t new_deallocated = deallocate_block(child_loc, blocks, bitmap, depth - 1, last_bgd, TRUE);
    deallocated += new_deallocated;
    blocks -= new_deallocated;
    if (blocks == 0)
    {
      return deallocated;
    }
  }
  return deallocated;
}

// assume blocks always available
void search_blocks(uint32_t preferred_bgd, uint32_t *locations, uint32_t blocks, uint32_t *found_count)
{

  // search in preferred bgd first
  if (bgd_table.table[preferred_bgd].free_blocks_count > 0)
  {
    search_blocks_in_bgd(preferred_bgd, locations, blocks, found_count);
  }

  // search in other than preferred bgd
  for (uint32_t i = 0; i < GROUPS_COUNT && *found_count < blocks; i++)
  {
    if (i == preferred_bgd)
      continue;
    search_blocks_in_bgd(i, locations, blocks, found_count);
  }
}

void search_blocks_in_bgd(uint32_t bgd, uint32_t *locations, uint32_t blocks, uint32_t *found_count)
{
  if (bgd_table.table[bgd].free_blocks_count == 0)
    return;

  // search free blocks
  read_blocks(&block_buffer, bgd_table.table[bgd].block_bitmap, 1);
  uint32_t bgd_offset = bgd * BLOCKS_PER_GROUP;
  uint32_t allocated = 0;

  // loop until found all the blocks needed or there is no block left
  for (uint32_t i = 0; i < BLOCKS_PER_GROUP &&
                       *found_count < blocks &&
                       allocated < bgd_table.table[bgd].free_blocks_count;
       i++)
  {
    uint8_t byte = block_buffer.buf[i / 8];
    uint8_t offset = 7 - i % 8;
    if (!((byte >> offset) & 1u))
    {
      // set flag of the block bitmap
      block_buffer.buf[i / 8] |= 1u << offset;
      locations[*found_count] = bgd_offset + i;
      *found_count += 1;
      allocated++;
    }
  }

  if (allocated > 0)
  {
    // update bgd and the block bitmap
    bgd_table.table[bgd].free_blocks_count -= allocated;

    write_blocks(&block_buffer, bgd_table.table[bgd].block_bitmap, 1);

    write_blocks(&bgd_table, 2, 1);
  }
}

void load_inode_blocks(void *ptr, void *_block, uint32_t size)
{
  uint32_t *block = (uint32_t *)_block;
  // full description on EXT2INode struct header

  uint32_t block_size = divceil(size, BLOCK_SIZE);
  uint32_t allocated = 0;

  // load all types of blocks, direct, indirect first, ...
  for (uint8_t i = 0; i < 15 && block_size > 0; i++)
  {
    if (i < 12)
    {
      // direct
      allocated = load_blocks_rec(ptr, block[i], block_size, size, 0);
    }
    else
    {
      // indirect
      allocated = load_blocks_rec(ptr, block[i], block_size, size, i - 11);
    }
    ptr += BLOCK_SIZE * allocated;
    block_size -= allocated;

    // wont error even overflow
    size -= allocated * BLOCK_SIZE;
  }
}

uint32_t load_blocks_rec(void *ptr, uint32_t block, uint32_t block_size, uint32_t size, uint8_t depth)
{
  if (depth == 0)
  {
    // base recursion

    // hati2 buffer overflow deck
    if (size >= BLOCK_SIZE)
    {
      read_blocks(ptr, block, 1);
    }
    else
    {
      struct BlockBuffer buffer;
      read_blocks(&buffer, block, 1);
      memcpy(ptr, &buffer, size);
    }
    return 1u;
  }

  struct BlockBuffer blocks;
  read_blocks(&blocks, block, 1);
  uint32_t allocated = 0;

  for (uint32_t i = 0; i < BLOCK_SIZE && block_size > 0; i++)
  {
    uint32_t new_allocated = load_blocks_rec(ptr, blocks.buf[i], block_size, size, depth - 1);
    ptr += new_allocated * BLOCK_SIZE;
    block_size -= new_allocated;
    allocated += new_allocated;

    // dont worry, if size < 0 (overflow), this loop will be ended tho
    size -= new_allocated * BLOCK_SIZE;
  }

  return allocated;
}

bool is_directory_entry_same(struct EXT2DirectoryEntry *entry, struct EXT2DriverRequest request, bool is_file)
{
  if (is_file && entry->file_type != EXT2_FT_REG_FILE)
    return FALSE;
  if (!is_file && entry->file_type != EXT2_FT_DIR)
    return FALSE;
  // name length must same
  if (request.name_len != entry->name_len)
    return FALSE;

  // entry name must same
  if (memcmp(request.name, get_entry_name(entry), request.name_len))
    return FALSE;

  // if folder, no need to check ext
  if (!is_file)
    return TRUE;

  // check extension
  return !strcmp(request.ext, entry->ext, 4);
}

int8_t read_directory(struct EXT2DriverRequest *prequest)
{
  struct EXT2DriverRequest request = *prequest;
  char dot[2] = ".";
  if (!strcmp(request.name, "/", request.name_len))
  {
    request.inode = sblock.first_ino;
    request.name = dot;
    request.name_len = 1;
  }
  int8_t retval = resolve_path(&request);
  if (retval != 0)
    return 3;
  uint32_t bgd = inode_to_bgd(request.inode);
  uint32_t local_idx = inode_to_local(request.inode);

  // get node corresponding to request.inode
  read_blocks(&inode_table_buf, bgd_table.table[bgd].inode_table, INODES_TABLE_BLOCK_COUNT);

  struct EXT2INode *node = &inode_table_buf.table[local_idx];

  if (node->mode != EXT2_S_IFDIR)
  {
    // parent folder invalid: code 3
    return 3;
  }

  // read the directory entry
  struct BlockBuffer block;
  read_blocks(&block, node->block[0], 1);

  // read from . to .. to child entries
  uint32_t offset = 0;
  struct EXT2DirectoryEntry *entry = get_directory_entry(&block, offset);

  while (offset < BLOCK_SIZE && entry->inode != 0)
  {
    if (entry->file_type == EXT2_FT_NEXT)
    {
      // continue to next directory table list
      read_blocks(&block, entry->inode, 1);
      offset = 0;
      entry = get_directory_entry(&block, offset);
      continue;
    }

    if (is_directory_entry_same(entry, request, FALSE))
    {
      // found
      if (offset != 0)
      {
        // if offset = 0, current block will be used
        bgd = inode_to_bgd(entry->inode);
        local_idx = inode_to_local(entry->inode);
        read_blocks(&inode_table_buf, bgd_table.table[bgd].inode_table, INODES_TABLE_BLOCK_COUNT);
        node = &inode_table_buf.table[local_idx];
        if (!request.inode_only)
          read_blocks(request.buf, node->block[0], 1);
      }
      else if (!request.inode_only)
      {
        memcpy(request.buf, block.buf, BLOCK_SIZE);
      }
      prequest->inode = entry->inode;

      // CMOS
      node->atime = get_timestamp();
      sync_node(node, entry->inode);
      return 0;
    }

    // get next linked list item
    offset += entry->rec_len;
    entry = get_directory_entry(&block, offset);
  }

  // not found
  return 2;
}

int8_t read(struct EXT2DriverRequest request)
{
  int8_t retval = resolve_path(&request);
  if (retval != 0)
    return 4;
  uint32_t bgd = inode_to_bgd(request.inode);
  uint32_t local_idx = inode_to_local(request.inode);

  // get node corresponding to request.inode
  read_blocks(&inode_table_buf, bgd_table.table[bgd].inode_table, INODES_TABLE_BLOCK_COUNT);

  struct EXT2INode *node = &inode_table_buf.table[local_idx];

  if (node->mode != EXT2_S_IFDIR)
  {
    // parent folder invalid: code 4
    return 4;
  }

  // read the directory entry
  struct BlockBuffer block;
  read_blocks(&block, node->block[0], 1);

  // get the first children entry
  uint32_t offset = get_directory_first_child_offset(&block);
  struct EXT2DirectoryEntry *entry = get_directory_entry(&block, offset);

  while (offset < BLOCK_SIZE && entry->inode != 0)
  {
    if (entry->file_type == EXT2_FT_NEXT)
    {
      // continue to next directory table list
      read_blocks(&block, entry->inode, 1);
      offset = 0;
      entry = get_directory_entry(&block, offset);
      continue;
    }

    if (is_directory_entry_same(entry, request, TRUE))
    {
      // found
      bgd = inode_to_bgd(entry->inode);
      local_idx = inode_to_local(entry->inode);
      read_blocks(&inode_table_buf, bgd_table.table[bgd].inode_table, INODES_TABLE_BLOCK_COUNT);
      node = &inode_table_buf.table[local_idx];
      if (request.buffer_size < node->size_low)
      {
        // not enough buffer
        return 2;
      }

      load_inode_blocks(request.buf, node->block, node->size_low);

      // CMOS
      node->atime = get_timestamp();
      sync_node(node, entry->inode);
      return 0;
    }

    // get next linked list item
    offset += entry->rec_len;
    entry = get_directory_entry(&block, offset);
  }

  // not found
  return 3;
}

int8_t read_next_directory_table(struct EXT2DriverRequest request)
{
  // check if blocks is legitly allocated
  uint32_t bgd = request.inode / BLOCKS_PER_GROUP;
  uint32_t local_idx = request.inode % BLOCKS_PER_GROUP;
  struct BlockBuffer block;
  read_blocks(&block, bgd_table.table[bgd].block_bitmap, 1);
  uint8_t byte = block.buf[local_idx / 8];
  uint8_t offset = 7 - local_idx % 8;
  if (!((byte >> offset) & 1u))
  {
    // not allocated error
    return 1;
  }
  read_blocks(request.buf, request.inode, 1);
  return 0;
}

int8_t write(struct EXT2DriverRequest request)
{
  int8_t retval = resolve_path(&request);
  if (retval != 0)
    // invalid parent folder
    return 2;
  uint32_t bgd = inode_to_bgd(request.inode);
  uint32_t local_idx = inode_to_local(request.inode);

  // get node corresponding to request.inode
  read_blocks(&inode_table_buf, bgd_table.table[bgd].inode_table, INODES_TABLE_BLOCK_COUNT);

  struct EXT2INode *node = &inode_table_buf.table[local_idx];

  if (node->mode != EXT2_S_IFDIR)
  {
    // parent folder invalid
    return 2;
  }

  uint32_t block_num = node->block[0];

  // read the directory entry
  struct BlockBuffer block;
  read_blocks(&block, block_num, 1);

  // get the first children entry
  uint32_t offset = get_directory_first_child_offset(&block);
  struct EXT2DirectoryEntry *entry = get_directory_entry(&block, offset);

  uint16_t space_needed = get_directory_record_length(request.name_len);

  // need space for last item to be pointer to next directory table, so maximum space in block will be this
  uint16_t space_total = BLOCK_SIZE - get_directory_record_length(0);

  bool found = FALSE;

  // search for space in directory
  while (!found)
  {
    if (entry->file_type == EXT2_FT_NEXT)
    {
      // continue to next directory table list
      block_num = entry->inode;
      read_blocks(&block, block_num, 1);
      offset = 0;
      entry = get_directory_entry(&block, offset);
      continue;
    }
    if (entry->inode != 0)
    {
      if (is_directory_entry_same(entry, request, request.buffer_size != 0))
      {
        // folder / file already exist
        return 1;
      }
      // get next linked list item
      offset += entry->rec_len;
      entry = get_directory_entry(&block, offset);
      continue;
    }

    // directory entry is available
    if (offset + space_needed <= space_total)
    {
      // can be used
      found = TRUE;
    }
    else
    {
      // allocate new directory table
      uint32_t next_block;
      uint32_t found_count = 0;
      search_blocks(bgd, &next_block, 1, &found_count);

      if (found_count == 0)
      {
        // not found any block available
        return -1;
      }

      entry->inode = next_block;
      entry->name_len = 0;
      entry->rec_len = get_directory_record_length(0);
      entry->file_type = EXT2_FT_NEXT;

      // update prev linked list directory
      write_blocks(&block, block_num, 1);

      // load new directory table
      read_blocks(&block, next_block, 1);
      found = TRUE;
      block_num = next_block;
      offset = 0;
      entry = get_directory_entry(block.buf, 0);
    }
  }

  // yey got em
  struct EXT2INode new_node;
  uint32_t new_inode = allocate_node();
  entry->inode = new_inode;
  // + 1 to also store null terminator if exist
  memcpy(get_entry_name(entry), request.name, request.name_len + 1);
  entry->name_len = request.name_len;
  entry->rec_len = get_directory_record_length(entry->name_len);

  // shift linked list terminator
  get_next_directory_entry(entry)->inode = 0;

  if (request.buffer_size == 0)
  {
    // create folder
    entry->file_type = EXT2_FT_DIR;
    init_directory_table(&new_node, new_inode, request.inode);
  }
  else
  {
    entry->file_type = EXT2_FT_REG_FILE;
    memcpy(entry->ext, request.ext, 3);
    new_node.mode = EXT2_S_IFREG;
    new_node.size_low = request.buffer_size;
    new_node.size_high = 0;

    // CMOS
    new_node.mtime = new_node.ctime = new_node.atime = get_timestamp();
    new_node.dtime = 0;

    new_node.blocks = divceil(request.buffer_size, BLOCK_SIZE);

    allocate_node_blocks(request.buf, &new_node, inode_to_bgd(new_inode));
    sync_node(&new_node, new_inode);
  }

  // update directory entry
  write_blocks(&block, block_num, 1);

  return 0;
}

int8_t delete(struct EXT2DriverRequest request)
{
  int8_t retval = resolve_path(&request);
  if (retval != 0)
    return 1;
  uint32_t bgd = inode_to_bgd(request.inode);
  uint32_t local_idx = inode_to_local(request.inode);

  // get node corresponding to request.inode
  read_blocks(&inode_table_buf, bgd_table.table[bgd].inode_table, INODES_TABLE_BLOCK_COUNT);

  struct EXT2INode *node = &inode_table_buf.table[local_idx];

  if (node->mode != EXT2_S_IFDIR)
  {
    // parent folder invalid: code 3
    return 3;
  }

  uint32_t block_num = node->block[0];

  // read the directory entry
  struct BlockBuffer block;
  read_blocks(&block, block_num, 1);

  // get the first children entry
  uint32_t offset = get_directory_first_child_offset(&block);
  struct EXT2DirectoryEntry *entry = get_directory_entry(&block, offset);

  struct BlockBuffer prev_table_block;
  uint32_t prev_table_block_num = 0;
  struct EXT2DirectoryEntry *prev_table_pointer_entry;

  bool found = FALSE;

  // search the requested entry
  while (!found)
  {
    if (entry->inode == 0)
      return 1;
    if (entry->file_type == EXT2_FT_NEXT)
    {
      // continue to next directory table list
      prev_table_block_num = block_num;
      prev_table_block = block;
      prev_table_pointer_entry = get_directory_entry(prev_table_block.buf, offset);

      block_num = entry->inode;
      read_blocks(&block, block_num, 1);
      offset = 0;
      entry = get_directory_entry(&block, offset);
      continue;
    }
    if (is_directory_entry_same(entry, request, !request.is_dir))
    {
      found = TRUE;
    }
    else
    {
      // get next linked list item
      offset += entry->rec_len;
      entry = get_directory_entry(&block, offset);
    }
  }

  struct EXT2DirectoryEntry *next_entry = get_next_directory_entry(entry);

  // check if request is folder and the folder requested to be deleted is not empty
  if (request.is_dir && !is_directory_empty(entry->inode))
    return 2;

  deallocate_node(entry->inode);

  if (offset == 0 && (next_entry->inode == 0 || next_entry->file_type == EXT2_FT_NEXT))
  {
    // deallocate directory table
    prev_table_pointer_entry->inode = next_entry->inode;
    prev_table_pointer_entry->file_type = next_entry->file_type;
    write_blocks(&prev_table_block, prev_table_block_num, 1);

    deallocate_blocks(&block_num, 1);
  }
  else
  {
    // shift
    memcpy(block.buf + offset, block.buf + offset + entry->rec_len, BLOCK_SIZE - offset - entry->rec_len);

    write_blocks(&block, block_num, 1);
  }

  return 0;
}

int8_t resolve_path(struct EXT2DriverRequest *request)
{
  if (request->name_len == 0)
    return 1;
  if (request->name[0] == '/')
  {
    // absolute path
    request->inode = sblock.first_ino;
    request->name++;
    request->name_len--;
    return resolve_path(request);
  }
  uint32_t len = 0;
  while (len < request->name_len && request->name[len] != '/')
  {
    len++;
  }
  if (len == request->name_len)
  {
    // got the basic path already
    return 0;
  }
  // read directory
  uint8_t prev_name_len = request->name_len;
  bool prev_inode_only = request->inode_only;
  request->name_len = len;
  request->inode_only = TRUE;
  int8_t retval = read_directory(request);
  if (retval != 0)
    return retval;
  // if abc/de, from name length 6 to 6 - 3 - 1 = 2 (de)
  request->name_len = prev_name_len - len - 1;
  request->name += len + 1;
  request->inode_only = prev_inode_only;
  return resolve_path(request);
}