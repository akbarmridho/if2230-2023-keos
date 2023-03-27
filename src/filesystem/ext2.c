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
  return (struct EXT2DirectoryEntry *)(ptr + offset);
}

uint16_t get_directory_record_length(uint8_t name_len)
{
  // directory entry record length is based by name length
  uint16_t len = 4 + 2 + 1 + 1 + name_len;
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