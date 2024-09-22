#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>  // For geteuid()
#include <fcntl.h>
#include <stdint.h>  // For uint32_t, etc.
#include "sgfs.h"
#include <sys/wait.h>  // For handling system commands

#define GPT_HEADER_SIGNATURE 0x5452415020494645ULL  // "EFI PART"
#define GPT_REVISION 0x00010000
#define GPT_ENTRY_SIZE 128
#define GPT_ENTRIES 128

// Function to check if the program is running as root
void check_root() {
    if (geteuid() != 0) {
        fprintf(stderr, "This program must be run as root. Please use sudo.\n");
        exit(EXIT_FAILURE);
    }
}

// GPT Header structure
struct gpt_header {
    uint64_t signature;
    uint32_t revision;
    uint32_t header_size;
    uint32_t header_crc32;
    uint32_t reserved;
    uint64_t current_lba;
    uint64_t backup_lba;
    uint64_t first_usable_lba;
    uint64_t last_usable_lba;
    uint8_t disk_guid[16];
    uint64_t partition_entry_lba;
    uint32_t num_partition_entries;
    uint32_t partition_entry_size;
    uint32_t partition_entries_crc32;
};

// GPT Partition Entry structure
struct gpt_partition_entry {
    uint8_t partition_type_guid[16];
    uint8_t unique_partition_guid[16];
    uint64_t first_lba;
    uint64_t last_lba;
    uint64_t attributes;
    uint8_t partition_name[72];  // UTF-16 partition name (36 characters)
};

// Function to create a GPT partition table
void create_gpt_partition_table(int fd, uint64_t disk_size) {
    struct gpt_header gpt;
    struct gpt_partition_entry partition;

    // Initialize the GPT header
    memset(&gpt, 0, sizeof(gpt));
    gpt.signature = GPT_HEADER_SIGNATURE;
    gpt.revision = GPT_REVISION;
    gpt.header_size = sizeof(gpt);
    gpt.current_lba = 1;
    gpt.backup_lba = disk_size - 1;
    gpt.first_usable_lba = 34;
    gpt.last_usable_lba = disk_size - 34;
    gpt.partition_entry_lba = 2;  // Partition entry array starts at LBA 2
    gpt.num_partition_entries = GPT_ENTRIES;
    gpt.partition_entry_size = GPT_ENTRY_SIZE;

    // Write the GPT header to the disk at LBA 1
    lseek(fd, 512, SEEK_SET);  // LBA 1 starts at byte offset 512 (512 bytes per sector)
    if (write(fd, &gpt, sizeof(gpt)) != sizeof(gpt)) {
        perror("Failed to write GPT header");
        exit(1);
    }

    // Initialize the first partition (SGFS partition)
    memset(&partition, 0, sizeof(partition));
    partition.first_lba = 34;  // First usable LBA after GPT
    partition.last_lba = gpt.last_usable_lba;

    // Write the partition entry array to the disk at LBA 2
    lseek(fd, 512 * 2, SEEK_SET);  // LBA 2 starts at byte offset 1024
    if (write(fd, &partition, sizeof(partition)) != sizeof(partition)) {
        perror("Failed to write partition entry array");
        exit(1);
    }

    // Write backup GPT header (optional)
    lseek(fd, (disk_size - 1) * 512, SEEK_SET);  // Backup GPT at last LBA
    if (write(fd, &gpt, sizeof(gpt)) != sizeof(gpt)) {
        perror("Failed to write backup GPT header");
        exit(1);
    }
}

// Function to format the disk with SGFS
void format_disk(const char* disk, uint32_t block_size, uint32_t total_blocks) {
    int fd = open(disk, O_RDWR);
    if (fd < 0) {
        perror("Failed to open disk");
        exit(1);
    }

    // Determine disk size (in LBAs)
    uint64_t disk_size = total_blocks;

    // Create GPT partition table
    create_gpt_partition_table(fd, disk_size);

    // Now format the partition with SGFS
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

// Main function to handle commands
int main(int argc, char* argv[]) {
    // Ensure the program is run with sudo/root privileges
    check_root();

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
