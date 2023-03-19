#ifndef _DISK_H
#define _DISK_H

#include "../lib-header/stdtype.h"

/* -- ATA PIO status codes -- */
#define ATA_STATUS_BSY 0x80
#define ATA_STATUS_RDY 0x40
#define ATA_STATUS_DRQ 0x08
#define ATA_STATUS_DF 0x20
#define ATA_STATUS_ERR 0x01

#define BLOCK_SIZE 512
#define HALF_BLOCK_SIZE (BLOCK_SIZE / 2)

// Block buffer data type - @param buf Byte buffer with size of BLOCK_SIZE
struct BlockBuffer
{
    uint8_t buf[BLOCK_SIZE];
} __attribute__((packed));

/**
 * ATA PIO logical block address read blocks. Will blocking until read is completed.
 * Note: ATA PIO will use 2-bytes per read/write operation.
 * Recommended to use struct BlockBuffer
 *
 * @param ptr                   Pointer for storing reading data, this pointer should point to already allocated memory location.
 *                              With allocated size positive integer multiple of BLOCK_SIZE, ex: buf[1024]
 * @param logical_block_address Block address to read data from. Use LBA addressing
 * @param block_count           How many block to read, starting from block logical_block_address to lba-1
 */
void read_blocks(void *ptr, uint32_t logical_block_address, uint8_t block_count);

/**
 * ATA PIO logical block address write blocks. Will blocking until write is completed.
 * Note: ATA PIO will use 2-bytes per read/write operation.
 * Recommended to use struct BlockBuffer
 *
 * @param ptr                   Pointer to data that to be written into disk. Memory pointed should be positive integer multiple of BLOCK_SIZE
 * @param logical_block_address Block address to write data into. Use LBA addressing
 * @param block_count           How many block to write, starting from block logical_block_address to lba-1
 */
void write_blocks(const void *ptr, uint32_t logical_block_address, uint8_t block_count);

#endif