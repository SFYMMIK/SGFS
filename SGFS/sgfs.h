#ifndef SGFS_H
#define SGFS_H

#include <stdint.h>

// Magic number to identify the SGFS filesystem
#define SGFS_MAGIC 0x53474653  // "SGFS" in ASCII

// Version number of SGFS
#define SGFS_VERSION 1

// Structure of the SGFS superblock
struct sgfs_superblock {
    uint32_t magic;            // Magic number identifying the filesystem
    uint32_t version;          // Version of SGFS
    uint32_t block_size;       // Size of each block
    uint32_t inode_size;       // Size of each inode
    uint32_t total_blocks;     // Total number of blocks in the filesystem
    uint32_t total_inodes;     // Total number of inodes
    uint32_t free_blocks;      // Number of free blocks
    uint32_t free_inodes;      // Number of free inodes
    uint32_t journal_start;    // Start of the journal
    uint32_t block_bitmap_start; // Start of the block bitmap
    uint32_t inode_bitmap_start; // Start of the inode bitmap
    uint32_t inode_table_start;  // Start of the inode table
    uint32_t data_block_start;   // Start of the data blocks
    uint32_t journal_size;      // Size of the journaling area
};

// Structure of an inode in SGFS
struct sgfs_inode {
    uint32_t inode_number;    // Inode number
    uint32_t file_size;       // Size of the file in bytes
    uint16_t file_type;       // 1 = regular file, 2 = directory
    uint16_t permissions;     // Standard permissions (e.g., 0777)
    uint32_t direct_block[12]; // Direct block pointers for smaller files
    uint32_t indirect_block;  // Single-indirect block pointer
    uint32_t double_indirect_block; // Double-indirect block pointer
    uint32_t creation_time;   // File creation time
    uint32_t modification_time; // Last modification time
    uint32_t access_time;     // Last access time
};

#endif // SGFS_H
