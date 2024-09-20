#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "sgfs.h"

// Mount and format operations for SGFS

void format_disk(const char* disk, uint32_t block_size, uint32_t total_blocks) {
    int fd = open(disk, O_RDWR);
    if (fd < 0) {
        perror("Failed to open disk");
        exit(1);
    }

    // Initialize the superblock
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

    // Write superblock
    if (write(fd, &sb, sizeof(sb)) != sizeof(sb)) {
        perror("Failed to write superblock");
        close(fd);
        exit(1);
    }

    // Initialize block and inode bitmaps (set everything to free)
    uint8_t bitmap[block_size];
    memset(bitmap, 0, block_size);  // Clear the bitmap (all blocks are free)

    // Write block bitmap
    for (uint32_t i = sb.block_bitmap_start; i < sb.inode_bitmap_start; i++) {
        if (write(fd, bitmap, block_size) != block_size) {
            perror("Failed to write block bitmap");
            close(fd);
            exit(1);
        }
    }

    // Write inode bitmap
    for (uint32_t i = sb.inode_bitmap_start; i < sb.inode_table_start; i++) {
        if (write(fd, bitmap, block_size) != block_size) {
            perror("Failed to write inode bitmap");
            close(fd);
            exit(1);
        }
    }

    // Zero out inode table (clear all inodes)
    struct sgfs_inode empty_inode = {0};
    for (uint32_t i = sb.inode_table_start; i < sb.data_block_start; i++) {
        if (write(fd, &empty_inode, sizeof(empty_inode)) != sizeof(empty_inode)) {
            perror("Failed to write inode table");
            close(fd);
            exit(1);
        }
    }

    printf("Disk %s formatted with SGFS (block size: %u, total blocks: %u).\n", disk, block_size, total_blocks);
    close(fd);
}

void mount_disk(const char* disk) {
    // Logic to mount the disk (for simplicity, just track it)
    FILE* f = fopen("/tmp/sgfs_mount_status", "w");
    if (f == NULL) {
        perror("Failed to open mount status file");
        return;
    }
    fprintf(f, "%s\n", disk);
    fclose(f);
    printf("Mounted disk: %s\n", disk);
}

void unmount_disk() {
    // Logic to unmount the disk (clear the status file)
    FILE* f = fopen("/tmp/sgfs_mount_status", "w");
    if (f == NULL) {
        perror("Failed to open mount status file");
        return;
    }
    fprintf(f, "none\n");
    fclose(f);
    printf("Disk unmounted.\n");
}

char* get_mounted_disk() {
    static char disk[64];
    FILE* f = fopen("/tmp/sgfs_mount_status", "r");
    if (f == NULL) {
        return NULL;
    }
    fgets(disk, sizeof(disk), f);
    fclose(f);
    return disk;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: sgfs_cli <command> <device> [block_size] [total_blocks]\n");
        return 1;
    }

    // Command handling logic
    if (strcmp(argv[1], "m") == 0 && argc == 3) {
        mount_disk(argv[2]);
    } else if (strcmp(argv[1], "im") == 0) {
        char* disk = get_mounted_disk();
        if (disk) {
            printf("Mounted disk: %s\n", disk);
        } else {
            printf("No disk is currently mounted.\n");
        }
    } else if (strcmp(argv[1], "mdd") == 0 && argc == 3) {
        unmount_disk();
        mount_disk(argv[2]);
    } else if (strcmp(argv[1], "f") == 0 && argc == 5) {
        const char* disk = argv[2];
        uint32_t block_size = atoi(argv[3]);
        uint32_t total_blocks = atoi(argv[4]);
        format_disk(disk, block_size, total_blocks);
    } else {
        printf("Unknown command or incorrect arguments.\n");
    }

    return 0;
}
