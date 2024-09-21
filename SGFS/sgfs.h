#ifndef SGFS_H
#define SGFS_H

#include <stdint.h>  // Include for uint32_t, uint16_t, etc.

// Define the SGFS magic number and version
#define SGFS_MAGIC 0x53474653  // "SGFS" in hexadecimal
#define SGFS_VERSION 1         // Version number of the filesystem

// Superblock definition
struct sgfs_superblock {
    uint32_t magic;
    uint32_t version;
    uint32_t block_size;       // Size of each block
    uint32_t inode_size;       // Size of each inode
    uint32_t total_blocks;     // Total number of blocks in the FS
    uint32_t total_inodes;     // Total number of inodes in the FS
    uint32_t free_blocks;      // Number of free blocks
    uint32_t free_inodes;      // Number of free inodes
    uint32_t journal_start;    // Start of journal
    uint32_t block_bitmap_start; // Start of block bitmap
    uint32_t inode_bitmap_start; // Start of inode bitmap
    uint32_t inode_table_start;  // Start of inode table
    uint32_t data_block_start;   // Start of data blocks
    uint32_t journal_size;      // Size of journaling area
};

// Inode structure for files and directories
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

#endif
