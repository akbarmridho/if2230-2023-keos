#ifndef _EXT2_H
#define _EXT2_H

#include "disk.h"
#include "../lib-header/stdtype.h"

#define BOOT_SECTOR 0
#define DISK_SPACE 4194304      // 4MB
#define EXT2_SUPER_MAGIC 0xEF53 // value for superblock magic
#define INODE_SIZE sizeof(struct EXT2INode)
#define INODES_PER_GROUP BLOCK_SIZE / INODE_SIZE
#define GROUPS_COUNT BLOCK_SIZE / sizeof(struct EXT2BGD)
#define BLOCKS_PER_GROUP DISK_SPACE / GROUPS_COUNT / BLOCK_SIZE

// inode constants
// modes
#define EXT2_S_IFREG 0x8000 // regular file
#define EXT2_S_IFDIR 0x4000 // directory

// file type constant
#define EXT2_FT_REG_FILE 1
#define EXT2_FT_DIR 2
#define EXT2_FT_NEXT 3

struct EXT2DriverRequest
{
    void *buf;
    char *name;
    uint8_t name_len;
    char ext[3];
    uint32_t inode;
    uint32_t buffer_size;
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
    uint32_t atime;       // access time, not used
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
    char ext[3];
    char *name;
} __attribute__((packed));

uint32_t inode_to_bgd(uint32_t inode);

uint32_t inode_to_local(uint32_t inode);

void init_directory_table(struct EXT2INode *node, uint32_t inode, uint32_t parent_inode);

bool is_empty_storage(void);

void create_ext2(void);

void initialize_filesystem_ext2(void);

struct EXT2DirectoryEntry *get_directory_entry(void *ptr, uint32_t offset);

uint16_t get_directory_record_length(uint8_t name_len);

struct EXT2DirectoryEntry *get_next_directory_entry(struct EXT2DirectoryEntry *entry);

void allocate_node_blocks(struct EXT2INode *node, uint32_t preferred_bgd, uint32_t blocks);

void sync_node(struct EXT2INode *node, uint32_t inode);

uint32_t allocate_node(void);

void search_blocks_in_bgd(uint32_t bgd, uint32_t *locations, uint32_t blocks, uint32_t *found);

uint32_t get_directory_first_child_offset(void *ptr);

void load_inode_blocks(void *ptr, uint32_t block[15], uint32_t size);

uint32_t load_blocks_rec(void *ptr, uint32_t block, uint32_t block_size, uint8_t depth);

int8_t read_directory(struct EXT2DriverRequest request);

int8_t read(struct EXT2DriverRequest request);

int8_t write(struct EXT2DriverRequest request);

int8_t delete(struct EXT2DriverRequest request);

#endif