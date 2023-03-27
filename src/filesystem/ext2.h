#ifndef _EXT2_H
#define _EXT2_H

#include "disk.h"
#include "../lib-header/stdtype.h"

#define DISK_SPACE 4194304      // 4MB
#define BLOCK_SIZE 1024         // 1KB
#define EXT2_SUPER_MAGIC 0xEF53 // value for superblock magic
#define BLOCKS_PER_GROUP 128
#define GROUPS_COUNT DISK_SPACE / BLOCKS_PER_GROUP / BLOCK_SIZE
// GROUPS_COUNT * 32 must be maximum BLOCK_SIZE

// inode constants
// modes
#define EXT2_S_IFREG 0x8000 // regular file
#define EXT2_S_IFDIR 0x4000 // directory

// file type constant
#define EXT2_FT_REG_FILE 1
#define EXT2_FT_DIR 2

struct EXT2BlockBuffer
{
    uint8_t buf[BLOCK_SIZE];
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
    uint16_t mode;       // for now just files and directories
    uint32_t lower_size; // lower 32 bit for filesize
    uint32_t ctime;      // created time
    uint32_t mtime;      // modified time
    uint32_t dtime;      // deleted time
    uint32_t blocks;     // total number of 512-bytes (not same as BLOCK_SIZE) block reserved to node
    uint32_t flags;      // not used for now
    /* 15 x 32bit block numbers pointing to the blocks containing the data for this inode. The first 12 blocks are direct blocks. The 13th entry in this array is the block number of the first indirect block; which is a block containing an array of block ID containing the data. Therefore, the 13th block of the file will be the first block ID contained in the indirect block. With a 1KiB block size, blocks 13 to 268 of the file data are contained in this indirect block.

    The 14th entry in this array is the block number of the first doubly-indirect block; which is a block containing an array of indirect block IDs, with each of those indirect blocks containing an array of blocks containing the data. In a 1KiB block size, there would be 256 indirect blocks per doubly-indirect block, with 256 direct blocks per indirect block for a total of 65536 blocks per doubly-indirect block.

    The 15th entry in this array is the block number of the triply-indirect block; which is a block containing an array of doubly-indrect block IDs, with each of those doubly-indrect block containing an array of indrect block, and each of those indirect block containing an array of direct block. In a 1KiB file system, this would be a total of 16777216 blocks per triply-indirect block. */
    uint32_t block[15];

    uint32_t generation; // file version, not used for now
    uint32_t file_acl;   // not used for now
    uint32_t dir_acl;    // not used for now
    uint32_t faddr;      // location of the file fragment
} __attribute__((packed));

// directory
struct EXT2DirectoryEntry
{
    uint32_t inode;   // value 0 indicate entry not used
    uint16_t rec_len; // displacement to the next directory entry from the start of the current directory entry.
    uint8_t name_len;
    uint8_t file_type;
    char *name;
} __attribute__((packed));

#endif