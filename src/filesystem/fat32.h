#ifndef _FAT32_H
#define _FAT32_H

#include "disk.h"
#include "stdtype.h"

/**
 * FAT32 - IF2230 edition - 2023
 * Check "IF2230 - Guidebook - Milestone 2" for more details
 * https://docs.google.com/document/d/1IFyxHSYYpKgecHcS0T64oDc4bVElaq8tBcm1_mjjGGM/edit#
 */

/* -- IF2230 File System constants -- */
#define BOOT_SECTOR 0
#define CLUSTER_BLOCK_COUNT 4
#define CLUSTER_SIZE (BLOCK_SIZE * CLUSTER_BLOCK_COUNT)
#define CLUSTER_MAP_SIZE 512

/* -- FAT32 FileAllocationTable constants -- */
// FAT reserved value for cluster 0 and 1 in FileAllocationTable
#define CLUSTER_0_VALUE 0x0FFFFFF0
#define CLUSTER_1_VALUE 0x0FFFFFFF

// EOF also double as valid cluster / "this is last valid cluster in the chain"
#define FAT32_FAT_END_OF_FILE 0x0FFFFFFF
#define FAT32_FAT_EMPTY_ENTRY 0x00000000

#define FAT_CLUSTER_NUMBER 1
#define ROOT_CLUSTER_NUMBER 2

/* -- FAT32 DirectoryEntry constants -- */
#define ATTR_SUBDIRECTORY 0b00010000
#define UATTR_NOT_EMPTY 0b10101010

// Boot sector signature for this file system "FAT32 - IF2230 edition"
extern const uint8_t fs_signature[BLOCK_SIZE];

// Cluster buffer data type - @param buf Byte buffer with size of CLUSTER_SIZE
struct ClusterBuffer
{
    uint8_t buf[CLUSTER_SIZE];
} __attribute__((packed));

/* -- FAT32 Data Structures -- */

/**
 * FAT32 FileAllocationTable, for more information about this, check guidebook
 *
 * @param cluster_map Containing cluster map of FAT32
 */
struct FAT32FileAllocationTable
{
    uint32_t cluster_map[CLUSTER_MAP_SIZE];
} __attribute__((packed));

/**
 * FAT32 standard 8.3 format - 32 bytes DirectoryEntry, Some detail can be found at:
 * https://en.wikipedia.org/wiki/Design_of_the_FAT_file_system#Directory_entry, and click show table.
 *
 * @param name           Entry name
 * @param ext            File extension
 * @param attribute      Will be used exclusively for subdirectory flag / determining this entry is file or folder
 * @param user_attribute If this attribute equal with UATTR_NOT_EMPTY then entry is not empty
 *
 * @param undelete       Unused / optional
 * @param create_time    Unused / optional
 * @param create_date    Unused / optional
 * @param access_time    Unused / optional
 * @param cluster_high   Upper 16-bit of cluster number
 *
 * @param modified_time  Unused / optional
 * @param modified_date  Unused / optional
 * @param cluster_low    Lower 16-bit of cluster number
 * @param filesize       Filesize of this file, if this is directory / folder, filesize is 0
 */
struct FAT32DirectoryEntry
{
    char name[8];
    char ext[3];
    uint8_t attribute;
    uint8_t user_attribute;

    bool undelete;
    uint16_t create_time;
    uint16_t create_date;
    uint16_t access_date;
    uint16_t cluster_high;

    uint16_t modified_time;
    uint16_t modified_date;
    uint16_t cluster_low;
    uint32_t filesize;
} __attribute__((packed));

// FAT32 DirectoryTable, containing directory entry table - @param table Table of DirectoryEntry that span within 1 cluster
struct FAT32DirectoryTable
{
    struct FAT32DirectoryEntry table[CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry)];
} __attribute__((packed));

/* -- FAT32 Driver -- */

/**
 * FAT32DriverState - Contain all driver states
 *
 * @param fat_table     FAT of the system, will be loaded during initialize_filesystem_fat32()
 * @param dir_table_buf Buffer for directory table
 * @param cluster_buf   Buffer for cluster
 */
struct FAT32DriverState
{
    struct FAT32FileAllocationTable fat_table;
    struct FAT32DirectoryTable dir_table_buf;
    struct ClusterBuffer cluster_buf;
} __attribute__((packed));

/**
 * FAT32DriverRequest - Request for Driver CRUD operation
 *
 * @param buf                   Pointer pointing to buffer
 * @param name                  Name for directory entry
 * @param ext                   Extension for file
 * @param parent_cluster_number Parent directory cluster number, for updating metadata
 * @param buffer_size           Buffer size, CRUD operation will have different behaviour with this attribute
 */
struct FAT32DriverRequest
{
    void *buf;
    char name[8];
    char ext[3];
    uint32_t parent_cluster_number;
    uint32_t buffer_size;
} __attribute__((packed));

/* -- Driver Interfaces -- */

/**
 * Convert cluster number to logical block address
 *
 * @param cluster Cluster number to convert
 * @return uint32_t Logical Block Address
 */
uint32_t cluster_to_lba(uint32_t cluster);

/**
 * Initialize DirectoryTable value with parent DirectoryEntry and directory name
 *
 * @param dir_table          Pointer to directory table
 * @param name               8-byte char for directory name
 * @param parent_dir_cluster Parent directory cluster number
 */
void init_directory_table(struct FAT32DirectoryTable *dir_table, char *name, uint32_t parent_dir_cluster);

/**
 * Checking whether filesystem signature is missing or not in boot sector
 *
 * @return True if memcmp(boot_sector, fs_signature) returning inequality
 */
bool is_empty_storage(void);

/**
 * Create new FAT32 file system. Will write fs_signature into boot sector and
 * proper FileAllocationTable (contain CLUSTER_0_VALUE, CLUSTER_1_VALUE,
 * and initialized root directory) into cluster number 1
 */
void create_fat32(void);

/**
 * Initialize file system driver state, if is_empty_storage() then create_fat32()
 * Else, read and cache entire FileAllocationTable (located at cluster number 1) into driver state
 */
void initialize_filesystem_fat32(void);

/**
 * Write cluster operation, wrapper for write_blocks().
 * Recommended to use struct ClusterBuffer
 *
 * @param ptr            Pointer to source data
 * @param cluster_number Cluster number to write
 * @param cluster_count  Cluster count to write, due limitation of write_blocks block_count 255 => max cluster_count = 63
 */
void write_clusters(const void *ptr, uint32_t cluster_number, uint8_t cluster_count);

/**
 * Read cluster operation, wrapper for read_blocks().
 * Recommended to use struct ClusterBuffer
 *
 * @param ptr            Pointer to buffer for reading
 * @param cluster_number Cluster number to read
 * @param cluster_count  Cluster count to read, due limitation of read_blocks block_count 255 => max cluster_count = 63
 */
void read_clusters(void *ptr, uint32_t cluster_number, uint8_t cluster_count);

/* -- CRUD Operation -- */

/**
 *  FAT32 Folder / Directory read
 *
 * @param request buf point to struct FAT32DirectoryTable,
 *                name is directory name,
 *                ext is unused,
 *                parent_cluster_number is target directory table to read,
 *                buffer_size must be exactly sizeof(struct FAT32DirectoryTable)
 * @return Error code: 0 success - 1 not a folder - 2 not found - -1 unknown
 */
int8_t read_directory(struct FAT32DriverRequest request);

/**
 * FAT32 read, read a file from file system.
 *
 * @param request All attribute will be used for read, buffer_size will limit reading count
 * @return Error code: 0 success - 1 not a file - 2 not enough buffer - 3 not found - -1 unknown
 */
int8_t read(struct FAT32DriverRequest request);

/**
 * FAT32 write, write a file or folder to file system.
 *
 * @param request All attribute will be used for write, buffer_size == 0 then create a folder / directory
 * @return Error code: 0 success - 1 file/folder already exist - 2 invalid parent cluster - -1 unknown
 */
int8_t write(struct FAT32DriverRequest request);

/**
 * FAT32 delete, delete a file or empty directory (only 1 DirectoryEntry) in file system.
 *
 * @param request buf and buffer_size is unused
 * @return Error code: 0 success - 1 not found - 2 folder is not empty - -1 unknown
 */
int8_t delete(struct FAT32DriverRequest request);

#endif