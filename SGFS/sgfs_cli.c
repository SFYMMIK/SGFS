#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <sys/mount.h>   // For mounting functions
#include <sys/stat.h>    // For filesystem creation
#include <errno.h>
#include <sys/types.h>

#include "sgfs.h"

// Forward declaration of format_disk function
void format_disk(const char* disk, uint32_t block_size);

// Mount point path for SGFS
const char* SGFS_MOUNT_POINT = "/mnt/sgfs";

// Function to get the size of the device using ioctl
uint64_t get_device_size(int fd) {
    uint64_t size;
    if (ioctl(fd, BLKGETSIZE64, &size) == -1) {
        perror("Failed to get device size");
        return 0;
    }
    return size;
}

// Function to allocate blocks by writing one block at a time
void allocate_blocks(int fd, uint64_t size, uint32_t block_size) {
    uint8_t* zero_block = (uint8_t*)calloc(1, block_size);
    if (!zero_block) {
        perror("Failed to allocate memory for zero block");
        exit(1);
    }

    uint64_t total_blocks = size / block_size;  // Calculate total number of blocks
    printf("Starting block allocation (%lu total blocks)...\n", total_blocks);

    for (uint64_t i = 0; i < total_blocks; i++) {
        if (write(fd, zero_block, block_size) != block_size) {
            perror("Failed to write block");
            free(zero_block);
            exit(1);
        }
        // Print progress every 1000 blocks for better tracking
        if (i % 1000 == 0) {
            printf("Allocated %lu/%lu blocks...\n", i, total_blocks);
        }
    }

    printf("Block allocation completed.\n");
    free(zero_block);
}

// Function to write SGFS superblock
void write_sgfs_superblock(int fd, uint32_t block_size, uint32_t total_blocks) {
    struct sgfs_superblock sb;
    sb.magic = SGFS_MAGIC;
    sb.version = SGFS_VERSION;
    sb.block_size = block_size;
    sb.inode_size = sizeof(struct sgfs_inode);
    sb.total_blocks = total_blocks;
    sb.total_inodes = total_blocks / 10;  // Inodes are 10% of total blocks
    sb.free_blocks = total_blocks - 1;    // Superblock takes 1 block
    sb.free_inodes = sb.total_inodes;
    sb.journal_size = 128;  // Example size for journal
    sb.journal_start = 1;   // Journal starts after superblock
    sb.block_bitmap_start = sb.journal_start + sb.journal_size;
    sb.inode_bitmap_start = sb.block_bitmap_start + (total_blocks / 8);
    sb.inode_table_start = sb.inode_bitmap_start + (total_blocks / 8);
    sb.data_block_start = sb.inode_table_start + sb.total_inodes;

    printf("Writing SGFS superblock...\n");
    lseek(fd, 34 * block_size, SEEK_SET);  // Write superblock at the start of the partition
    if (write(fd, &sb, sizeof(sb)) != sizeof(sb)) {
        perror("Failed to write SGFS superblock");
        exit(1);
    }

    printf("Superblock written successfully.\n");
}

// Function to simulate syncing data in chunks with progress
void sync_data_with_progress(int fd, uint64_t total_size, uint32_t block_size) {
    uint64_t synced_size = 0;
    uint64_t chunk_size = block_size * 1024;  // Sync in chunks of 1024 blocks at a time
    uint64_t total_chunks = total_size / chunk_size;

    printf("Syncing data to disk...\n");

    for (uint64_t i = 0; i < total_chunks; i++) {
        // Simulate syncing a chunk of data
        fsync(fd);  // Sync data after each chunk
        synced_size += chunk_size;
        printf("Synced %lu/%lu bytes...\n", synced_size, total_size);
    }

    // Handle any remaining data (if total_size is not a multiple of chunk_size)
    if (synced_size < total_size) {
        fsync(fd);  // Final sync for remaining data
        printf("Synced %lu/%lu bytes...\n", total_size, total_size);
    }

    printf("Data sync completed.\n");
}

// Function to check if mount point exists, if not, create it
void ensure_mount_point_exists() {
    struct stat st = {0};

    if (stat(SGFS_MOUNT_POINT, &st) == -1) {
        printf("Creating mount point at %s...\n", SGFS_MOUNT_POINT);
        if (mkdir(SGFS_MOUNT_POINT, 0755) != 0) {
            perror("Failed to create mount point");
            exit(1);
        }
    }
}

// Function to mount the SGFS disk
void mount_disk(const char* disk) {
    ensure_mount_point_exists();

    printf("Mounting %s to %s...\n", disk, SGFS_MOUNT_POINT);

    // Replace "sgfs" with the actual filesystem type you register
    if (mount(disk, SGFS_MOUNT_POINT, "sgfs", 0, NULL) == -1) {
        perror("Failed to mount SGFS disk");
        exit(1);
    }

    printf("Mounted %s to %s successfully.\n", disk, SGFS_MOUNT_POINT);
}

// Function to unmount the disk (clear the mount status)
void unmount_disk() {
    printf("Unmounting disk from %s...\n", SGFS_MOUNT_POINT);

    if (umount(SGFS_MOUNT_POINT) == -1) {
        perror("Failed to unmount SGFS disk");
        exit(1);
    }

    printf("Disk unmounted from %s successfully.\n", SGFS_MOUNT_POINT);
}

// Main function to handle commands
int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: sgfs_cli <command> <device>\n");
        return 1;
    }

    // Command handling logic
    if (strcmp(argv[1], "m") == 0 && argc == 3) {
        // Mount the disk
        mount_disk(argv[2]);
    } else if (strcmp(argv[1], "mdd") == 0) {
        // Unmount the disk
        unmount_disk();
    } else if (strcmp(argv[1], "f") == 0 && argc == 5) {
        // Format the disk
        const char* disk = argv[2];
        uint32_t block_size = atoi(argv[3]);
        format_disk(disk, block_size);
    } else {
        printf("Unknown command or incorrect arguments.\n");
    }

    return 0;
}

// Function to format the disk with SGFS
void format_disk(const char* disk, uint32_t block_size) {
    int fd = open(disk, O_RDWR);
    if (fd < 0) {
        perror("Failed to open disk");
        exit(1);
    }

    // Get the size of the device
    uint64_t device_size = get_device_size(fd);
    if (device_size == 0) {
        close(fd);
        exit(1);
    }
    uint32_t total_blocks = device_size / block_size;

    printf("Starting formatting of %s with SGFS...\n", disk);
    printf("Total disk size: %lu bytes (%u blocks, block size: %u bytes)\n", device_size, total_blocks, block_size);

    // Allocate blocks on the disk (one block at a time)
    allocate_blocks(fd, device_size, block_size);

    // Write the SGFS superblock
    write_sgfs_superblock(fd, block_size, total_blocks);

    // Simulate syncing data to disk with progress
    sync_data_with_progress(fd, device_size, block_size);

    // Close the file descriptor
    close(fd);

    printf("Disk %s formatted with SGFS successfully.\n", disk);
}
