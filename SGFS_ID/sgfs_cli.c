#define FUSE_USE_VERSION 30

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <errno.h>
#include <fuse.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/fs.h>
#include <dirent.h>  // For directory operations
#include <time.h>    // For generating timestamp for the backup filename

#include "sgfs.h"

// SGFS Superblock structure
struct sgfs_superblock {
    uint32_t magic;
    uint32_t version;
    uint32_t block_size;
    uint32_t inode_size;
    uint32_t total_blocks;
    uint32_t total_inodes;
    uint32_t free_blocks;
    uint32_t free_inodes;
    uint32_t journal_size;
    uint32_t journal_start;
    uint32_t block_bitmap_start;
    uint32_t inode_bitmap_start;
    uint32_t inode_table_start;
    uint32_t data_block_start;
};

// Inode structure
struct sgfs_inode {
    uint32_t inode_number;
    uint32_t file_size;
    uint16_t file_type;       // 1 = regular file, 2 = directory
    uint16_t permissions;
    uint32_t direct_block[12];
    uint32_t indirect_block;
    uint32_t double_indirect_block;
    uint32_t creation_time;
    uint32_t modification_time;
    uint32_t access_time;
};

// SGPT Header structure
struct sgpt_header {
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

// SGPT Partition Entry structure
struct sgpt_partition_entry {
    uint8_t partition_type_guid[16];
    uint8_t unique_partition_guid[16];
    uint64_t first_lba;
    uint64_t last_lba;
    uint64_t attributes;
    uint8_t partition_name[72];  // UTF-16 partition name (36 characters)
};

// Mount point path for SGFS
const char* SGFS_MOUNT_POINT = "/mnt/sgfs";

// Function to ensure mount point exists
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

// Function to create SGFS superblock
void write_sgfs_superblock(int fd, uint32_t block_size, uint32_t total_blocks) {
    struct sgfs_superblock sb;
    sb.magic = 0x53474653;  // 'SGFS'
    sb.version = 1;
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

// Function to create an SGPT partition table and SGFS partition
void create_sgpt_partition_table(int fd, uint64_t disk_size) {
    struct sgpt_header sgpt;
    struct sgpt_partition_entry partition;

    // Initialize the SGPT header
    memset(&sgpt, 0, sizeof(sgpt));
    sgpt.signature = 0x5350475452415020ULL;  // "SGPT PART"
    sgpt.revision = 0x00010000;
    sgpt.header_size = sizeof(sgpt);
    sgpt.current_lba = 1;
    sgpt.backup_lba = disk_size - 1;
    sgpt.first_usable_lba = 34;
    sgpt.last_usable_lba = disk_size - 34;
    sgpt.partition_entry_lba = 2;  // Partition entry array starts at LBA 2
    sgpt.num_partition_entries = 128;
    sgpt.partition_entry_size = 128;

    // Write the SGPT header to the disk at LBA 1
    lseek(fd, 512, SEEK_SET);  // LBA 1 starts at byte offset 512 (512 bytes per sector)
    if (write(fd, &sgpt, sizeof(sgpt)) != sizeof(sgpt)) {
        perror("Failed to write SGPT header");
        exit(1);
    }

    // Initialize the first partition (SGFS partition)
    memset(&partition, 0, sizeof(partition));
    partition.first_lba = 34;  // First usable LBA after SGPT
    partition.last_lba = sgpt.last_usable_lba;

    // Write the partition entry array to the disk at LBA 2
    lseek(fd, 512 * 2, SEEK_SET);  // LBA 2 starts at byte offset 1024
    if (write(fd, &partition, sizeof(partition)) != sizeof(partition)) {
        perror("Failed to write partition entry array");
        exit(1);
    }

    // Write backup SGPT header (optional)
    lseek(fd, (disk_size - 1) * 512, SEEK_SET);  // Backup SGPT at last LBA
    if (write(fd, &sgpt, sizeof(sgpt)) != sizeof(sgpt)) {
        perror("Failed to write backup SGPT header");
        exit(1);
    }
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

// Function to initialize the disk and format it to SGFS
void init_sgfs(const char* device) {
    printf("Initializing disk %s with SGFS...\n", device);

    // Open the disk
    int fd = open(device, O_RDWR);
    if (fd < 0) {
        perror("Failed to open disk");
        exit(1);
    }

    // Get disk size
    uint64_t disk_size;
    if (ioctl(fd, BLKGETSIZE64, &disk_size) == -1) {
        perror("Failed to get device size");
        close(fd);
        exit(1);
    }

    // Create SGPT partition table and SGFS partition
    create_sgpt_partition_table(fd, disk_size);

    // Allocate blocks on the disk
    uint32_t block_size = 4096;  // Default block size of 4096 bytes
    allocate_blocks(fd, disk_size, block_size);

    // Write the SGFS superblock
    write_sgfs_superblock(fd, block_size, disk_size / block_size);

    printf("Disk %s formatted to SGFS successfully.\n", device);
    close(fd);
}

// Function to mount the SGFS filesystem using FUSE
void mount_disk(const char* device) {
    ensure_mount_point_exists();
    printf("Mounting SGFS at %s using FUSE...\n", SGFS_MOUNT_POINT);
    char* fuse_argv[] = {
        "sgfs_cli", (char*) SGFS_MOUNT_POINT
    };
    fuse_main(2, fuse_argv, NULL, NULL);
}

// Function to unmount the disk
void unmount_disk(const char* device) {
    printf("Unmounting SGFS from %s...\n", SGFS_MOUNT_POINT);
    if (umount(SGFS_MOUNT_POINT) == -1) {
        perror("Failed to unmount SGFS");
        exit(1);
    }
    printf("SGFS unmounted successfully from %s.\n", SGFS_MOUNT_POINT);
}

// Main function to handle commands
int main(int argc, char* argv[]) {
    if (argc < 3) {
        printf("Usage:\n");
        printf("  sudo ./sgfs_cli init /dev/sdX\n");
        printf("  sudo ./sgfs_cli mount /dev/sdX\n");
        printf("  sudo ./sgfs_cli umount /dev/sdX\n");
        return 1;
    }

    const char* device = argv[2];

    // Command handling logic
    if (strcmp(argv[1], "init") == 0) {
        init_sgfs(device);
    } else if (strcmp(argv[1], "mount") == 0) {
        mount_disk(device);
    } else if (strcmp(argv[1], "umount") == 0) {
        unmount_disk(device);
    } else {
        printf("Unknown command or incorrect arguments.\n");
    }

    return 0;
}
