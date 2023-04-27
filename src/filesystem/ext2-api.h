#ifndef EXT2_API_H
#define EXT2_API_H

#include "disk.h"
#include "../lib-header/stdtype.h"

#define BOOT_SECTOR 0u
#define DISK_SPACE 4194304u     // 4MB
#define EXT2_SUPER_MAGIC 0xEF53 // value for superblock magic
#define INODE_SIZE sizeof(struct EXT2INode)
#define INODES_PER_TABLE (BLOCK_SIZE / INODE_SIZE)
#define GROUPS_COUNT (BLOCK_SIZE / sizeof(struct EXT2BGD) / 2u) // 2u can be tweaked
#define BLOCKS_PER_GROUP (DISK_SPACE / BLOCK_SIZE / GROUPS_COUNT)
#define INODES_TABLE_BLOCK_COUNT 16u // can be tweaked
#define INODES_PER_GROUP (INODES_PER_TABLE * INODES_TABLE_BLOCK_COUNT)

// max files/folder number that can be container is INODES_PER_GROUP * GROUP_COUNT

// inode constants
// modes
#define EXT2_S_IFREG 0x8000u // regular file
#define EXT2_S_IFDIR 0x4000u // directory

// file type constant
#define EXT2_FT_REG_FILE 1u
#define EXT2_FT_DIR 2u
#define EXT2_FT_NEXT 3u

struct EXT2DriverRequest
{
  void *buf;
  char *name;
  uint8_t name_len;
  char ext[4];
  uint32_t inode;
  uint32_t buffer_size;

  // for delete
  bool is_dir;

  // inode_only means just get the directory inode without loading the buffers
  bool inode_only;
} __attribute__((packed));

struct EXT2Superblock
{
  uint32_t inodes_count; // the total number of inodes, both used and free
  uint32_t blocks_count; // total number of blocks in the system including all used, free and reserved

  uint32_t free_blocks_count;  // the total number of free blocks, including the number of reserved blocks
  uint32_t free_inodes_count;  // the total number of free inodes
  uint32_t first_data_block;   // always 1 for block size 1KB
  uint32_t blocks_per_group;   // total number of blocks per group
  uint32_t frags_per_group;    // total number of fragments per group
  uint32_t inodes_per_group;   // total number of inodes per group
  uint16_t magic;              // will be equal EXT2_SUPER_MAGIC
  uint32_t first_ino;          // index to first inode useable to standard files
  uint8_t prealloc_blocks;     // the number of blocks the implementation should attempt to pre-allocate when creating a new regular file
  uint8_t prealloc_dir_blocks; // the number of blocks the implementation should attempt to pre-allocate when creating a new directory
} __attribute__((packed));

struct EXT2BGD // block group descriptor
{
  uint32_t block_bitmap;      // id of the first block of the “block bitmap” for the group represented.
  uint32_t inode_bitmap;      // block id of the first block of the “inode bitmap” for the group represented.
  uint32_t inode_table;       // block id of the first block of the “inode table” for the group represented.
  uint16_t free_blocks_count; // the total number of free blocks for the represented group.
  uint16_t free_inodes_count; // total number of free inodes for the represented group.
  uint16_t used_dirs_count;   // the number of inodes allocated to directories for the represented group.
  uint16_t pad;               // padding for 32bit boundary
  uint32_t reserved[3];       // 12 bytes of reserved space for future revisions. for now used to make struct size 32 bytes
} __attribute__((packed));

struct EXT2BGDTable
{
  struct EXT2BGD table[GROUPS_COUNT];
} __attribute__((packed));

struct EXT2INode
{
  uint16_t mode;        // for now just files and directories
  uint32_t size_low;    // lower 32 bit for filesize, for now only use lower bit (size maximum 2^32-1 aka 4GB), org ukuran disk-nya aja cuma 4MB aowkaowkawo
  uint32_t atime;       // access time
  uint32_t ctime;       // created time
  uint32_t mtime;       // modified time
  uint32_t dtime;       // deleted time
  uint32_t gid;         // not used
  uint16_t links_count; // not used
  uint32_t blocks;      // total number of 512-bytes (not same as BLOCK_SIZE) block reserved to node
  uint32_t osd1;        // not used
  uint32_t flags;       // not used for now
  /* 15 x 32bit block numbers pointing to the blocks containing the data for this inode. The first 12 blocks are direct blocks. The 13th entry in this array is the block number of the first indirect block; which is a block containing an array of block ID containing the data. Therefore, the 13th block of the file will be the first block ID contained in the indirect block. With a 1KiB block size, blocks 13 to 268 of the file data are contained in this indirect block.

  The 14th entry in this array is the block number of the first doubly-indirect block; which is a block containing an array of indirect block IDs, with each of those indirect blocks containing an array of blocks containing the data. In a 1KiB block size, there would be 256 indirect blocks per doubly-indirect block, with 256 direct blocks per indirect block for a total of 65536 blocks per doubly-indirect block.

  The 15th entry in this array is the block number of the triply-indirect block; which is a block containing an array of doubly-indrect block IDs, with each of those doubly-indrect block containing an array of indrect block, and each of those indirect block containing an array of direct block. In a 1KiB file system, this would be a total of 16777216 blocks per triply-indirect block. */
  uint32_t block[15];

  uint32_t generation; // file version, not used for now
  uint32_t file_acl;   // not used for now
  uint32_t size_high;  // not used for now
  uint32_t faddr;      // location of the file fragment
  uint32_t osd2[3];
} __attribute__((packed));

struct EXT2INodeTable
{
  struct EXT2INode table[INODES_PER_GROUP];
};

// directory

// directory entry is a linked list that has dynamic size of each item (depends on name_len), so 1 block of directory table can consist of various entries length
// if one block of directory table already full, the last element will have file_type of next to the next directory table (linkedlist of linkedlist) so subdirectory count can be infinity (kind of)
// this is customly designed by Keos Team :)
struct EXT2DirectoryEntry
{
  uint32_t inode;   // value 0 indicate entry not used, if file_type is next, this is not inode but block index
  uint16_t rec_len; // displacement to the next directory entry from the start of the current directory entry.
  uint8_t name_len;
  uint8_t file_type;
  char ext[4];

  // name array separated because it is dynamic
} __attribute__((packed));

/**
 * @brief get name of a directory entry, because name is dynamic, its available after the struct
 * @param entry pointer of the directory entry
 * @return entry name with length of entry->name_len
 */
char *get_entry_name(void *entry);

/**
 * @brief get directory entry from directory table, because
 * its list with dynamic size of each item, offset
 * (byte location) is used instead of index
 * @param ptr pointer of directory table
 * @param offset entry offset from ptr
 * @return pointer of directory entry with such offset
 */
struct EXT2DirectoryEntry *get_directory_entry(void *ptr, uint32_t offset);

/**
 * @brief get next entry of a directory entry, it is located
 * after the entry rec len
 * @param entry pointer of entry
 * @returns next entry
 */
struct EXT2DirectoryEntry *get_next_directory_entry(struct EXT2DirectoryEntry *entry);

/**
 * @brief get record length of a directory entry
 * that has dynamic size based on its name length, struct size is 12
 * and after that the buffer will contain its name char * that needs to aligned at 4 bytes boundaries
 * @param name_len entry name length (includes null terminator)
 * @returns sizeof(EXT2DirectoryEntry) + name_len aligned 4 bytes
 */
uint16_t get_directory_record_length(uint8_t name_len);

/**
 * @brief get first directory entry child offset
 * @return offset from the directory table
 */
uint32_t get_directory_first_child_offset(void *ptr);

/**
 * @brief separate filename with extension
 * @return filename, len_name, ext ( 0 success, 1 fail )
 */
int8_t separate_filename_extension(char **filename, uint8_t *len_name, char **ext);

#endif
