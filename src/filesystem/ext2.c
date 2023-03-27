#include "ext2.h"
#include "../lib-header/stdmem.h"
#include "../lib-header/math.h"

const char fs_signature[BLOCK_SIZE] = "Haduh Capek bikin EXT2 tapi yaudahlah biar bagus";

struct EXT2Superblock sblock = {};

struct BlockBuffer block_buffer = {};
struct EXT2BGDTable bgd_table = {};
struct EXT2INodeTable inode_table_buf = {};

uint32_t inode_to_bgd(uint32_t inode)
{
  return (inode - 1) / INODES_PER_GROUP;
}

uint32_t inode_to_local(uint32_t inode)
{
  return (inode - 1) % INODES_PER_GROUP;
}

void init_directory_table(struct EXT2INode *node, uint32_t inode, uint32_t parent_inode)
{
  struct BlockBuffer block;

  struct EXT2DirectoryEntry *table = get_directory_entry(block.buf, 0);
  table->inode = inode;
  table->file_type = EXT2_S_IFDIR;
  table->name_len = 1;
  memcpy(table->name, ".", 1);
  table->rec_len = get_directory_record_length(table->name_len);

  struct EXT2DirectoryEntry *parent_table = get_next_directory_entry(table);
  parent_table->inode = parent_inode;
  parent_table->file_type = EXT2_S_IFDIR;
  parent_table->name_len = 2;
  memcpy(parent_table->name, "..", 2);
  parent_table->rec_len = get_directory_record_length(table->name_len);

  struct EXT2DirectoryEntry *first_file = get_next_directory_entry(parent_table);
  first_file->inode = 0;

  node->mode = EXT2_FT_DIR;
  node->size_low = 0;
  node->size_high = 0;

  // TODO: time

  node->blocks = 1;
  allocate_node_blocks(node, inode_to_bgd(inode), 1);

  write_blocks(&block, node->block[0], 1);
  sync_node(node, inode);
}

bool is_empty_storage(void)
{
  read_blocks(block_buffer.buf, BOOT_SECTOR, 1);
  return memcmp(block_buffer.buf, fs_signature, BLOCK_SIZE);
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

  // initialize bgd table
  for (int i = 0; i < GROUPS_COUNT; i++)
  {
    if (i == 0)
    {
      // 3 first block is for boot sector, superblock, and bgd table
      bgd_table.table[i].block_bitmap = 3;
    }
    else
    {
      bgd_table.table[i].block_bitmap = i * sblock.blocks_per_group + 0;
    }

    bgd_table.table[i].inode_bitmap = bgd_table.table[i].block_bitmap + 1;
    bgd_table.table[i].inode_table = bgd_table.table[i].block_bitmap + 2;

    bgd_table.table[i].free_inodes_count = sblock.inodes_per_group;

    // free blocks = block per group - reserved blocks
    bgd_table.table[i].free_blocks_count = sblock.blocks_per_group - bgd_table.table[i].inode_table - 1;
    bgd_table.table[i].used_dirs_count = 0;
  }
  write_blocks(&bgd_table, 2, 1);
  struct EXT2INode root_dir_node;
  uint32_t root_dir_inode = allocate_node();

  init_directory_table(&root_dir_inode, root_dir_inode, root_dir_inode);
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

struct EXT2DirectoryEntry *get_directory_entry(void *ptr, uint32_t offset)
{
  struct EXT2DirectoryEntry *entry = (struct EXT2DirectoryEntry *)(ptr + offset);
  entry->name = (ptr + offset + 11);

  return entry;
}

uint16_t get_directory_record_length(uint8_t name_len)
{
  // directory entry record length is based by name length
  uint16_t len = 4 + 2 + 1 + 1 + 3 + name_len;
  return divceil(len, 4);
}

struct EXT2DirectoryEntry *get_next_directory_entry(struct EXT2DirectoryEntry *entry)
{
  return get_directory_entry(entry, entry->rec_len);
}

void allocate_node_blocks(struct EXT2INode *node, uint32_t preferred_bgd, uint32_t blocks)
{
  uint32_t locations[blocks];
  uint32_t found = 0;
  if (bgd_table.table[preferred_bgd].free_blocks_count > 0)
  {
    search_blocks_in_bgd(preferred_bgd, locations, blocks, &found);
  }
  for (int i = 0; i < GROUPS_COUNT && found < blocks; i++)
  {
    if (i == preferred_bgd)
      continue;
    search_blocks_in_bgd(locations, blocks, &found, i);
  }

  for (int i = 0; i < blocks; i++)
  {
    node->block[i] = locations[i];
  }
}

void sync_node(struct EXT2INode *node, uint32_t inode)
{
  uint32_t bgd = inode_to_bgd(inode);
  uint32_t local_idx = inode_to_local(inode);

  read_blocks(&inode_table_buf, bgd_table.table[bgd].inode_table, 1);
  inode_table_buf.table[local_idx] = *node;
  write_blocks(&inode_table_buf, bgd_table.table[bgd].inode_table, 1);
}

uint32_t allocate_node(void)
{
  uint32_t bgd = -1;
  for (int i = 0; i < GROUPS_COUNT; i++)
  {
    if (bgd_table.table[i].free_inodes_count > 0)
    {
      bgd = i;
      break;
    }
  }

  // TODO: handle if no node available

  // search free node
  read_blocks(&block_buffer, bgd_table.table[bgd].inode_bitmap, 1);
  uint32_t inode = bgd * INODES_PER_GROUP + 1;
  uint32_t location = 0;
  for (int i = 0; i < INODES_PER_GROUP; i++)
  {
    uint8_t byte = block_buffer.buf[i / 8];
    uint8_t offset = 7 - i % 8;
    if ((byte >> offset) & 1u)
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

void search_blocks_in_bgd(uint32_t bgd, uint32_t *locations, uint32_t blocks, uint32_t *found)
{
  if (bgd_table.table[bgd].free_blocks_count == 0)
    return;

  // search free blocks
  read_blocks(&block_buffer, bgd_table.table[bgd].block_bitmap, 1);
  uint32_t bgd_offset = bgd * BLOCKS_PER_GROUP;
  for (int i = 0; i < BLOCKS_PER_GROUP && *found < blocks; i++)
  {
    uint8_t byte = block_buffer.buf[i / 8];
    uint8_t offset = 7 - i % 8;
    if ((byte >> offset) & 1u)
    {
      // set flag of the block bitmap
      block_buffer.buf[i / 8] |= 1u << offset;
      locations[*found] = bgd_offset + i;
      *found++;
    }
  }
}

uint32_t get_directory_first_child_offset(void *ptr)
{
  uint32_t offset = 0;
  struct EXT2DirectoryEntry *entry = get_directory_entry(ptr, 0);
  offset += entry->rec_len;
  entry = get_next_directory_entry(entry);
  offset += entry->rec_len;
  return offset;
}

void load_inode_blocks(void *ptr, uint32_t block[15], uint32_t size)
{
  // full description on EXT2INode struct header

  uint32_t block_size = divceil(size, BLOCK_SIZE);
  uint32_t allocated = 0;

  for (int i = 0; i < block_size && i < 12; i++)
  {
    read_blocks(ptr, block[i], 1);
    ptr += BLOCK_SIZE;
    allocated++;
  }

  block_size -= allocated;

  if (block_size > 0)
  {
    // first indirect block
    allocated = load_blocks_rec(ptr, block[12], block_size, 1);
    ptr += BLOCK_SIZE * allocated;
    block_size -= allocated;
  }

  if (block_size > 0)
  {
    // doubly-indirect block
    allocated = load_blocks_rec(ptr, block[13], block_size, 2);
    ptr += BLOCK_SIZE * allocated;
    block_size -= allocated;
  }

  if (block_size > 0)
  {
    // triply-indirect block
    load_blocks_rec(ptr, block[14], block_size, 3);
  }
}

uint32_t load_blocks_rec(void *ptr, uint32_t block, uint32_t block_size, uint8_t depth)
{
  if (depth == 0)
  {
    // base recursion
    read_blocks(ptr, block, 1);
    return 1u;
  }

  struct BlockBuffer blocks;
  read_blocks(&blocks, block, 1);
  uint32_t allocated = 0;

  for (int i = 0; i < BLOCK_SIZE && block_size > 0; i++)
  {
    uint32_t new_allocated = load_blocks_rec(ptr, blocks.buf[i], block_size, depth - 1);
    ptr += new_allocated * BLOCK_SIZE;
    block_size -= new_allocated;
    allocated += new_allocated;
  }

  return allocated;
}

int8_t read_directory(struct EXT2DriverRequest request)
{
  uint32_t bgd = inode_to_bgd(request.inode);
  uint32_t local_idx = inode_to_local(request.inode);

  // get node corresponding to request.inode
  read_blocks(&inode_table_buf, bgd_table.table[bgd].inode_table, 1);

  struct EXT2INode *node = &inode_table_buf.table[local_idx];

  if (node->mode != EXT2_S_IFDIR)
  {
    // parent folder invalid
    // TODO: different error code
    return 2;
  }

  // read the directory entry
  struct BlockBuffer block;
  read_blocks(&block, node->block[0], 1);

  // get the first children entry
  uint32_t offset = get_directory_first_child_offset(block.buf);
  struct EXT2DirectoryEntry *entry = get_directory_entry(block.buf, offset);

  while (offset < BLOCK_SIZE && entry->inode != 0)
  {
    if (entry->file_type == EXT2_FT_NEXT)
    {
      // continue to next directory table list
      read_blocks(&block, entry->inode, 1);
      offset = 0;
      entry = get_directory_entry(block.buf, offset);
      continue;
    }
    // check if entry is a directory, same name length, and same name
    if (entry->file_type == EXT2_FT_DIR &&
        entry->name_len == request.name_len &&
        !memcmp(entry->name, request.name, request.name_len))
    {
      // found
      bgd = inode_to_bgd(entry->inode);
      local_idx = inode_to_local(entry->inode);
      read_blocks(&inode_table_buf, bgd_table.table[bgd].inode_table, 1);
      node = &inode_table_buf.table[local_idx];
      read_blocks(request.buf, node->block[0], 1);
      return 0;
    }

    // get next linked list item
    offset += entry->rec_len;
    entry = get_directory_entry(block.buf, offset);
  }

  // not found
  return 2;
}

int8_t read(struct EXT2DriverRequest request)
{
  uint32_t bgd = inode_to_bgd(request.inode);
  uint32_t local_idx = inode_to_local(request.inode);

  // get node corresponding to request.inode
  read_blocks(&inode_table_buf, bgd_table.table[bgd].inode_table, 1);

  struct EXT2INode *node = &inode_table_buf.table[local_idx];

  if (node->mode != EXT2_S_IFDIR)
  {
    // parent folder invalid
    // TODO: different error code
    return 3;
  }

  // read the directory entry
  struct BlockBuffer block;
  read_blocks(&block, node->block[0], 1);

  // get the first children entry
  uint32_t offset = get_directory_first_child_offset(block.buf);
  struct EXT2DirectoryEntry *entry = get_directory_entry(block.buf, offset);

  while (offset < BLOCK_SIZE && entry->inode != 0)
  {
    if (entry->file_type == EXT2_FT_NEXT)
    {
      // continue to next directory table list
      read_blocks(&block, entry->inode, 1);
      offset = 0;
      entry = get_directory_entry(block.buf, offset);
      continue;
    }
    // check if entry is a file, same name length, same name, same file extension
    if (entry->file_type == EXT2_FT_REG_FILE &&
        entry->name_len == request.name_len &&
        !memcmp(entry->name, request.name, request.name_len) &&
        !memcmp(entry->ext, request.ext, 3))
    {
      // found
      bgd = inode_to_bgd(entry->inode);
      local_idx = inode_to_local(entry->inode);
      read_blocks(&inode_table_buf, bgd_table.table[bgd].inode_table, 1);
      node = &inode_table_buf.table[local_idx];
      if (request.buffer_size < node->size_low)
      {
        // not enough buffer
        return 2;
      }

      load_inode_blocks(request.buf, node->block, node->size_low);
      return 0;
    }
  }

  return 2;
}