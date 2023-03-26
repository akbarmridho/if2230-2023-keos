#ifndef _EXT2_H
#define _EXT2_H

#include "disk.h"
#include "../lib-header/stdtype.h"

// Define the block size of the file system
// #define BLOCK_SIZE 512

/* -- IF2230 File System constants -- */
#define BOOT_SECTOR 0
#define CLUSTER_BLOCK_COUNT 4
#define CLUSTER_SIZE (BLOCK_SIZE * CLUSTER_BLOCK_COUNT)
#define CLUSTER_MAP_SIZE 512

// Define the superblock structure
typedef struct
{
    uint32_t block_size;   // Block size of the file system
    uint32_t blocks_count; // Total number of blocks in the file system
    // Add more fields as needed
} superblock_t;

// Define the inode structure
typedef struct
{
    uint16_t mode;         // File mode (permissions and type)
    uint16_t uid;          // Owner user ID
    uint32_t size;         // File size in bytes
    uint32_t blocks_count; // Total number of blocks used by the file
    // Add more fields as needed
} inode_t;

// Define the directory entry structure
typedef struct
{
    uint32_t inode;    // Inode number of the file or directory
    uint8_t name[255]; // Name of the file or directory
    // Add more fields as needed
} dir_entry_t;

// Define the block structure
typedef struct
{
    uint8_t data[BLOCK_SIZE]; // Data stored in the block
} block_t;

// Define the file system structure
typedef struct
{
    superblock_t sb;       // Superblock of the file system
    inode_t *inodes;       // Array of inodes
    block_t *blocks;       // Array of data blocks
    dir_entry_t *root_dir; // Root directory of the file system
    // Add more fields as needed
} file_system_t;

// int main(int argc, char **argv)
// {
//     // Initialize the file system
//     file_system_t fs;
//     memset(&fs, 0, sizeof(file_system_t));

//     // Load the superblock
//     fread(&fs.sb, sizeof(superblock_t), 1, stdin);

//     // Allocate memory for inodes and blocks
//     fs.inodes = malloc(fs.sb.blocks_count / 2 * sizeof(inode_t));
//     fs.blocks = malloc(fs.sb.blocks_count * sizeof(block_t));

//     // Load the inodes and blocks
//     fread(fs.inodes, sizeof(inode_t), fs.sb.blocks_count / 2, stdin);
//     fread(fs.blocks, sizeof(block_t), fs.sb.blocks_count, stdin);

//     // Load the root directory
//     fs.root_dir = malloc(sizeof(dir_entry_t) * fs.inodes[0].blocks_count);
//     fread(fs.root_dir, sizeof(dir_entry_t), fs.inodes[0].blocks_count, stdin);

//     // Use the file system
//     // ...

//     // Cleanup
//     free(fs.inodes);
//     free(fs.blocks);
//     free(fs.root_dir);

//     return 0;
// }

#endif