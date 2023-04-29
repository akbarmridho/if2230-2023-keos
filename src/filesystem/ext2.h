#ifndef _EXT2_H
#define _EXT2_H
#include "ext2-api.h"

/**
 * @brief get bgd index from inode, inode will starts at index 1
 * @param inode 1 to INODES_PER_GROUP * GROUP_COUNT
 * @return bgd index (0 to GROUP_COUNT - 1)
 */
uint32_t inode_to_bgd(uint32_t inode);

/**
 * @brief get inode local index in the corrresponding bgd
 * @param inode 1 to INODES_PER_GROUP * GROUP_COUNT
 * @return local index
 */
uint32_t inode_to_local(uint32_t inode);

/**
 * @brief create a new directory using given node
 * first item of directory table is its node location (name will be .)
 * second item of directory is its parent location (name will be ..)
 * @param node pointer of inode
 * @param inode inode that already allocated
 * @param parent_inode inode of parent directory (if root directory, the parent is itself)
 */
void init_directory_table(struct EXT2INode *node, uint32_t inode, uint32_t parent_inode);

/**
 * @brief check whether filesystem signature is missing or not in boot sector
 *
 * @return true if memcmp(boot_sector, fs_signature) returning inequality
 */
bool is_empty_storage(void);

/**
 * @brief create a new EXT2 filesystem. Will write fs_signature into boot sector,
 * initialize super block, bgd table, block and inode bitmap, and create root directory
 */
void create_ext2(void);

/**
 * @brief Initialize file system driver state, if is_empty_storage() then create_ext2()
 * Else, read and cache super block (located at block 1) and bgd table (located at block 2) into state
 */
void initialize_filesystem_ext2(void);

/**
 * @brief check whether a directory table has children or not
 * @param inode of a directory table
 * @return true if first_child_entry->inode = 0
 */
bool is_directory_empty(uint32_t inode);

/**
 * @brief recursively map blocks of a node and write the buffer
 * requested, if depth = 0, write the buffer to the first locations,
 * if depth > 0 (indirect), alocate a block to contain the indirect blocks
 * and recursively allocate lower depth
 * @param ptr source buffer that needs to write to node blocks
 * @param blocks number of blocks that needed to be written to
 * @param locations the locations (block index array), its size = blocks
 * @param mapped_count reference to caller total mapped count to keep track of ptr offset
 * @param depth 0 if direct blocks, 1 if first indirect blocks, 2 if doubly indirect blocks, 3 if triple indirect blocks
 *
 * @return location of node blocks that written
 */
uint32_t map_node_blocks(void *ptr, uint32_t blocks, uint32_t *locations, uint32_t *mapped_count, uint8_t depth);

/**
 * @brief write node->block in the given node, will allocate
 * at least node->blocks number of blocks, if first 12 item of node-> block
 * is not enough, will use indirect blocks
 * @param ptr the buffer that needs to be written
 * @param node pointer of the node
 * @param preffered_bgd it is located at the node inode bgd
 */
void allocate_node_blocks(void *ptr, struct EXT2INode *node, uint32_t preferred_bgd);

/**
 * @brief update the node to the disk
 * @param node pointer of node
 * @param inode location of the node
 */
void sync_node(struct EXT2INode *node, uint32_t inode);

/**
 * @brief get a free inode from the disk, assuming it is always
 * available
 * @return new inode
 */
uint32_t allocate_node(void);

/**
 * @brief deallocate node from the disk, will also deallocate its used blocks
 * also all of the blocks of indirect blocks if necessary
 * @param inode that needs to be deallocated
 */
void deallocate_node(uint32_t inode);

/**
 * @brief deallocate node blocks
 * @param locations node->block
 * @param blocks number of blocks
 */
void deallocate_blocks(void *_locations, uint32_t blocks);

uint32_t deallocate_block(uint32_t *locations, uint32_t blocks, struct BlockBuffer *bitmap, uint32_t depth, uint32_t *last_bgd, bool bgd_loaded);

/**
 * @brief search for available blocks in the disk
 * @param preferred_bgd bgd location priority
 * @param locations array of block num that will be written
 * @param blocks num of blocks that needs to be searched
 * @param found_count num of blocks allocated
 */
void search_blocks(uint32_t preferred_bgd, uint32_t *locations, uint32_t blocks, uint32_t *found_count);

/**
 * @brief search for available blocks in specific bgd
 * @param locations array of block num that will be written
 * @param blocks num of blocks that needs to be searched
 * @param found_count num of blocks allocated
 */
void search_blocks_in_bgd(uint32_t bgd, uint32_t *locations, uint32_t blocks, uint32_t *found_count);

/**
 * @brief load inode blocks from disk recursively (indirect blocks too)
 * @param ptr buffer that will contain the blocks buffer
 * @param block node->block
 * @param size filesize
 */
void load_inode_blocks(void *ptr, void *_block, uint32_t size);

/**
 * @brief load blocks of direct or indirect blocks
 * @param ptr buffer that will contain the blocks buffer
 * @param block_size number of blocks that needs to be written
 * @param block node->block
 * @param size filesize
 * @param depth 0 if direct, 1 if first indirect, etc.
 */
uint32_t load_blocks_rec(void *ptr, uint32_t block, uint32_t block_size, uint32_t size, uint8_t depth);

/**
 * @brief check whether the directory entry is the requested one
 * @param entry
 * @param request
 * @param is_file true if the requested is a file
 *
 * @return true if the entry is the requested one
 */
bool is_directory_entry_same(struct EXT2DirectoryEntry *entry, struct EXT2DriverRequest request, bool is_file);

/* -- CRUS Operation -- */

/**
 * @brief EXT2 Folder / Directory read
 * @param request buf point to struct EXT2 Directory
 *          name is directory name
 *          name_len is directory name length (includes null terminator)
 *          ext is unused
 *          inode is parent directory inode to read
 *          buffer_size must be exactly BLOCK_SIZE
 * @return Error code: 0 success - 1 not a folder - 2 not found - 3 parent folder invalid - -1 unknown
 */
int8_t read_directory(struct EXT2DriverRequest *prequest);

/**
 * @brief EXT2 read, read a file from file system
 * @param request All attribute will be used except is_dir for read, buffer_size will limit reading count
 * @return Error code: 0 success - 1 not a file - 2 not enough buffer - 3 not found - 4 parent folder invalid - -1 unknown
 */
int8_t read(struct EXT2DriverRequest request);

int8_t read_next_directory_table(struct EXT2DriverRequest request);

/**
 * @brief EXT2 write, write a file or a folder to file system
 *
 * @param All attribute will be used for write except is_dir, buffer_size == 0 then create a folder / directory. It is possible that exist file with name same as a folder
 * @return Error code: 0 success - 1 file/folder already exist - 2 invalid parent folder - -1 unknown
 */
int8_t write(struct EXT2DriverRequest *request);

/**
 * @brief EXT2 move, move a folder to another folder
 *
 * @param request request is the folder that will be moved
 * @param dst_request dst_request is the destination folder
 *
 * @return Error code: 0 success - 1 source folder not found - 2 invalid parent folder - 3 invalid destination folder - 4 file / folder already exist - -1 not found any block
 */
int8_t move_dir(struct EXT2DriverRequest request_src, struct EXT2DriverRequest dst_request);

/**
 * @brief EXT2 delete, delete a file or empty directory in file system
 *  @param request buf and buffer_size is unused, is_dir == true means delete folder (possible file with name same as folder)
 * @return Error code: 0 success - 1 not found - 2 folder is not empty - 3 parent folder invalid -1 unknown
 */
int8_t delete(struct EXT2DriverRequest request);

int8_t resolve_path(struct EXT2DriverRequest *request);

#endif