#include "fat32.h"
#include "disk.h"
#include "../lib-header/stdtype.h"
#include "../lib-header/stdmem.h"
#include "../lib-header/string.h"
#include "../lib-header/math.h"

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

struct FAT32DriverState driver_state =
    {};

/**
 * Convert cluster number to logical block address
 *
 * @param cluster Cluster number to convert
 * @return uint32_t Logical Block Address
 */
uint32_t cluster_to_lba(uint32_t cluster)
{
    return cluster * CLUSTER_SIZE;
}

/**
 * Initialize DirectoryTable value with parent DirectoryEntry and directory name
 *
 * @param dir_table          Pointer to directory table
 * @param name               8-byte char for directory name
 * @param parent_dir_cluster Parent directory cluster number
 */
void init_directory_table(struct FAT32DirectoryTable *dir_table, char *name, uint32_t parent_dir_cluster)
{
    struct FAT32DirectoryEntry *table = dir_table->table;

    memcpy(table[0].name, name, 8);
    table[0].user_attribute = UATTR_NOT_EMPTY;
    table[0].undelete = FALSE;
    table[0].cluster_low = parent_dir_cluster & 0xffff;
    table[0].cluster_high = (parent_dir_cluster >> 16) & 0xffff;

    for (int i = 1; i < 64; i++)
    {
        table[i].user_attribute = !UATTR_NOT_EMPTY;
    }

    driver_state.fat_table.cluster_map[parent_dir_cluster] = FAT32_FAT_END_OF_FILE;
    write_clusters(&driver_state.fat_table, 1, 1);
    write_clusters(dir_table->table, parent_dir_cluster, 1);
}

/**
 * Checking whether filesystem signature is missing or not in boot sector
 *
 * @return True if memcmp(boot_sector, fs_signature) returning inequality
 */
bool is_empty_storage(void)
{
    struct BlockBuffer block;
    read_blocks(block.buf, BOOT_SECTOR, 1);
    return memcmp(block.buf, fs_signature, BLOCK_SIZE) != 0;
}

/**
 * Create new FAT32 file system. Will write fs_signature into boot sector and
 * proper FileAllocationTable (contain CLUSTER_0_VALUE, CLUSTER_1_VALUE,
 * and initialized root directory) into cluster number 1
 */
void create_fat32(void)
{
    write_blocks(fs_signature, BOOT_SECTOR, 1);

    driver_state.fat_table.cluster_map[0] = CLUSTER_0_VALUE;
    driver_state.fat_table.cluster_map[1] = CLUSTER_1_VALUE;
    driver_state.fat_table.cluster_map[2] = FAT32_FAT_END_OF_FILE;

    // emptying FAT
    for (uint16_t i = 3; i < 512; i++)
    {
        driver_state.fat_table.cluster_map[i] = 0;
    }

    // write fat table
    write_clusters(&driver_state.fat_table, 1, 1);

    // initialize root directory
    init_directory_table(&driver_state.dir_table_buf, "root", 2);
}

/**
 * Initialize file system driver state, if is_empty_storage() then create_fat32()
 * Else, read and cache entire FileAllocationTable (located at cluster number 1) into driver state
 */
void initialize_filesystem_fat32(void)
{
    if (is_empty_storage())
    {
        create_fat32();
    }
    else
    {
        read_clusters(&driver_state.fat_table, 1, 1);
    }
}

/**
 * Write cluster operation, wrapper for write_blocks().
 * Recommended to use struct ClusterBuffer
 *
 * @param ptr            Pointer to source data
 * @param cluster_number Cluster number to write
 * @param cluster_count  Cluster count to write, due limitation of write_blocks block_count 255 => max cluster_count = 63
 */
void write_clusters(const void *ptr, uint32_t cluster_number, uint8_t cluster_count)
{
    write_blocks(ptr, cluster_to_lba(cluster_number), CLUSTER_BLOCK_COUNT * cluster_count);
}

/**
 * Read cluster operation, wrapper for read_blocks().
 * Recommended to use struct ClusterBuffer
 *
 * @param ptr            Pointer to buffer for reading
 * @param cluster_number Cluster number to read
 * @param cluster_count  Cluster count to read, due limitation of read_blocks block_count 255 => max cluster_count = 63
 */
void read_clusters(void *ptr, uint32_t cluster_number, uint8_t cluster_count)
{
    read_blocks(ptr, cluster_to_lba(cluster_number), CLUSTER_BLOCK_COUNT * cluster_count);
}

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
int8_t read_directory(struct FAT32DriverRequest request)
{
    read_clusters(&driver_state.dir_table_buf, request.parent_cluster_number, 1);

    struct FAT32DirectoryEntry *table = driver_state.dir_table_buf.table;

    for (uint8_t i = 1; i < DIRECTORY_TABLE_SIZE; i++)
    {
        if (!strcmp(table[i].name, request.name, 8))
        {
            if (table[i].attribute == 1)
            {
                // found
                read_clusters(request.buf, table[i].cluster_low, 1);

                return 0;
            }
            // not a folder
            return 1;
        }
    }

    // not found
    return 2;
}

/**
 * FAT32 read, read a file from file system.
 *
 * @param request All attribute will be used for read, buffer_size will limit reading count
 * @return Error code: 0 success - 1 not a file - 2 not enough buffer - 3 not found - -1 unknown
 */
int8_t read(struct FAT32DriverRequest request)
{
    read_clusters(&driver_state.dir_table_buf, request.parent_cluster_number, 1);

    struct FAT32DirectoryEntry *table = driver_state.dir_table_buf.table;

    for (uint8_t i = 1; i < DIRECTORY_TABLE_SIZE; i++)
    {
        if (!strcmp(table[i].name, request.name, 8) && !strcmp(table[i].ext, request.ext, 3))
        {
            if (table[i].attribute == 1)
            {
                // not a file
                return 1;
            }

            if (request.buffer_size < table[i].filesize)
            {
                // not enough buffer
                return 2;
            }

            // load request buffer
            uint16_t cluster_count = divceil(table[i].filesize, CLUSTER_SIZE);
            uint16_t cluster = table[i].cluster_low;
            for (uint16_t j = 0; j < cluster_count; j++)
            {
                read_clusters(request.buf + j * CLUSTER_SIZE, cluster, 1);
                cluster = driver_state.fat_table.cluster_map[cluster];
            }

            return 0;
        }
    }

    // not found
    return 3;
}

/**
 * FAT32 write, write a file or folder to file system.
 *
 * @param request All attribute will be used for write, buffer_size == 0 then create a folder / directory
 * @return Error code: 0 success - 1 file/folder already exist - 2 invalid parent cluster - -1 unknown
 */
int8_t write(struct FAT32DriverRequest request)
{
    if (driver_state.fat_table.cluster_map[request.parent_cluster_number] != FAT32_FAT_END_OF_FILE)
    {
        return 2;
    }

    read_clusters(&driver_state.dir_table_buf, request.parent_cluster_number, 1);

    struct FAT32DirectoryEntry *table = driver_state.dir_table_buf.table;
    uint16_t cluster_count = divceil(request.buffer_size, CLUSTER_SIZE);

    if (cluster_count == 0)
        cluster_count = 1;

    uint16_t locations[cluster_count];
    uint16_t current_cluster = 3;
    uint16_t cluster_found = 0;
    uint16_t directory_location = -1;

    for (uint8_t i = 1; i < DIRECTORY_TABLE_SIZE; i++)
    {
        if (table[i].user_attribute != UATTR_NOT_EMPTY)
        {
            // search for empty fat
            while (cluster_found < cluster_count && current_cluster < 512)
            {
                if (driver_state.fat_table.cluster_map[current_cluster] == 0)
                {
                    locations[cluster_found] = current_cluster;
                    cluster_found++;
                }
                current_cluster++;
            }
            directory_location = i;
            break;
        }
        if (!strcmp(table[i].name, request.name, 8))
        {
            if (request.buffer_size == 0 && table[i].attribute == 1)
            {
                // folder already exists
                return 1;
            }
            else if (request.buffer_size != 0 && !strcmp(table[i].ext, request.ext, 3))
                // file already exists
                return 1;
        }
    }
    if (cluster_found < cluster_count)
        return -1;

    memcpy(table[directory_location].name, request.name, 8);
    table[directory_location].cluster_low = locations[0] & 0xffff;
    table[directory_location].cluster_high = (locations[0] >> 16) & 0xffff;
    table[directory_location].user_attribute = UATTR_NOT_EMPTY;

    if (request.buffer_size == 0)
    {
        // make folder
        table[directory_location].attribute = 1;
        init_directory_table(request.buf, request.name, locations[0]);
    }
    else
    {
        // make file
        table[directory_location].attribute = 0;
        memcpy(table[directory_location].ext, request.ext, 3);
        table[directory_location].filesize = request.buffer_size;

        // write file buffer
        for (uint16_t i = 0; i < cluster_count; i++)
        {
            write_clusters(request.buf + i * CLUSTER_SIZE, locations[i], 1);

            if (i == cluster_count - 1)
            {
                driver_state.fat_table.cluster_map[locations[i]] = FAT32_FAT_END_OF_FILE;
            }
            else
            {
                driver_state.fat_table.cluster_map[locations[i]] = locations[i + 1];
            }
        }

        // syncronize fat table
        write_clusters(&driver_state.fat_table, 1, 1);
    }

    // syncronize parent directory table
    write_clusters(table, request.parent_cluster_number, 1);

    return 0;
}

/**
 * FAT32 delete, delete a file or empty directory (only 1 DirectoryEntry) in file system.
 *
 * @param request buf and buffer_size is unused
 * @return Error code: 0 success - 1 not found - 2 folder is not empty - -1 unknown
 */
int8_t delete(struct FAT32DriverRequest request)
{
    // Only a file and empty directories can be deleted
    char *filename = request.name;

    read_clusters(&driver_state.dir_table_buf, request.parent_cluster_number, 1);
    struct FAT32DirectoryEntry *table = driver_state.dir_table_buf.table;

    for (uint8_t i = 1; i < DIRECTORY_TABLE_SIZE; i++)
    {
        if (table[i].user_attribute != UATTR_NOT_EMPTY)
            continue;
        if (strcmp(filename, table[i].name, 8) == 0)
        {
            // Name found, check if the entry is a file or a directory
            if (table[i].attribute)
            {
                // Entry is a directory
                // Check if the directory is empty or not
                struct FAT32DirectoryTable dt;
                read_clusters(&dt, table[i].cluster_low, 1);
                for (unsigned int i = 1; i < DIRECTORY_TABLE_SIZE; i++)
                {
                    if (dt.table[i].user_attribute == UATTR_NOT_EMPTY)
                    {
                        // not empty
                        return 2;
                    }
                }
                // empty
                driver_state.fat_table.cluster_map[table[i].cluster_low] = 0;
                table[i].user_attribute = !UATTR_NOT_EMPTY;
                table[i].undelete = TRUE;

                // sync parent directory table
                write_clusters(&driver_state.dir_table_buf, request.parent_cluster_number, 1);

                // sync fat
                write_clusters(&driver_state.fat_table, 1, 1);
                return 0;
            }
            else
            {
                // Entry is a file
                // Check if the ext is same or not
                if (strcmp(table[i].ext, request.ext, 3) == 0)
                {
                    // ext is same
                    table[i].user_attribute = !UATTR_NOT_EMPTY;
                    table[i].undelete = TRUE;
                    write_clusters(&driver_state.dir_table_buf, request.parent_cluster_number, 1);
                    uint16_t cluster_count = divceil(table[i].filesize, CLUSTER_SIZE);
                    uint16_t to_zeros[cluster_count];
                    uint16_t current_cluster = table[i].cluster_low;

                    for (uint16_t i = 0; i < cluster_count; i++)
                    {
                        to_zeros[i] = current_cluster;
                        current_cluster = driver_state.fat_table.cluster_map[current_cluster];
                    }

                    for (uint16_t i = 0; i < cluster_count; i++)
                    {
                        driver_state.fat_table.cluster_map[to_zeros[i]] = 0;
                    }

                    write_clusters(&driver_state.fat_table, 1, 1);
                    return 0;
                }
                else
                {
                    // ext is different
                    continue;
                }
            }
        }
    }

    // not found
    return 1;
}