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

// Mount point path for SGFS
const char* SGFS_MOUNT_POINT = "/mnt/sgfs";

// Backup file structure
struct sgfs_backup {
    char filesystem[16];
    char partition_table[16];
    uint64_t total_size;
    uint64_t used_size;
    uint32_t block_size;
};

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

// Function to mount the SGFS filesystem using FUSE
void mount_disk(const char* device) {
    ensure_mount_point_exists();
    printf("Mounting SGFS at %s using FUSE...\n", SGFS_MOUNT_POINT);
    char* fuse_argv[] = {
        "sgfs_cli", SGFS_MOUNT_POINT
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

// Function to get a list of files and directories on the disk
void get_file_list(const char* path, FILE* backup_file) {
    DIR* dir = opendir(path);
    if (!dir) {
        perror("Failed to open directory for backup");
        return;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            fprintf(backup_file, "File: %s\n", entry->d_name);
        }
    }

    closedir(dir);
}

// Function to create a unique backup filename with a timestamp
void generate_backup_filename(const char* backup_dir, char* backup_filename) {
    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    snprintf(backup_filename, 256, "%s/backup_%04d%02d%02d_%02d%02d%02d.sgfsbackup",
             backup_dir, t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
}

// Function to parse the `FILESYSTEM` and `PARTITION_TABLE` arguments
void parse_fs_and_pt(const char* fs_arg, const char* pt_arg, char* fs_type, char* partition_table) {
    if (sscanf(fs_arg, "FILESYSTEM='%15[^']'", fs_type) != 1) {
        fprintf(stderr, "Invalid FILESYSTEM format. Expected: FILESYSTEM='ext4'\n");
        exit(1);
    }
    if (sscanf(pt_arg, "PARTITION_TABLE='%15[^']'", partition_table) != 1) {
        fprintf(stderr, "Invalid PARTITION_TABLE format. Expected: PARTITION_TABLE='gpt'\n");
        exit(1);
    }
}

// Function to create a backup of the disk
void backup_disk(const char* device, const char* backup_dir, const char* fs_type, const char* partition_table) {
    char backup_filename[256];
    generate_backup_filename(backup_dir, backup_filename);

    printf("Backing up disk %s to %s...\n", device, backup_filename);

    FILE* backup_file = fopen(backup_filename, "w");
    if (!backup_file) {
        perror("Failed to create backup file");
        return;
    }

    // Backup disk information (filesystem type, partition table, etc.)
    struct sgfs_backup backup;
    strncpy(backup.filesystem, fs_type, sizeof(backup.filesystem) - 1);
    strncpy(backup.partition_table, partition_table, sizeof(backup.partition_table) - 1);
    fprintf(backup_file, "Filesystem: %s\n", backup.filesystem);
    fprintf(backup_file, "Partition Table: %s\n", backup.partition_table);

    // Get file list and store it
    get_file_list(SGFS_MOUNT_POINT, backup_file);

    fclose(backup_file);
    printf("Backup completed: %s\n", backup_filename);
}

// Function to restore the disk from a backup file
void revert_disk(const char* device, const char* backup_path) {
    printf("Reverting disk %s from backup %s...\n", device, backup_path);

    FILE* backup_file = fopen(backup_path, "r");
    if (!backup_file) {
        perror("Failed to open backup file");
        return;
    }

    // Read and parse the backup file
    char line[256];
    while (fgets(line, sizeof(line), backup_file)) {
        if (strncmp(line, "File:", 5) == 0) {
            // Here you would restore the file
            printf("Restoring %s", line + 6);
        }
    }

    fclose(backup_file);
    printf("Revert completed.\n");
}

// Main function to handle commands
int main(int argc, char* argv[]) {
    if (argc < 3) {
        printf("Usage:\n");
        printf("  sudo ./sgfs_cli mount /dev/sdX\n");
        printf("  sudo ./sgfs_cli umount /dev/sdX\n");
        printf("  sudo ./sgfs_cli backup /dev/sdX /backup/directory/ FILESYSTEM='ext4' PARTITION_TABLE='gpt'\n");
        printf("  sudo ./sgfs_cli revert /dev/sdX /path/to/backup.sgfsbackup\n");
        return 1;
    }

    const char* device = argv[2];

    // Command handling logic
    if (strcmp(argv[1], "mount") == 0) {
        mount_disk(device);
    } else if (strcmp(argv[1], "umount") == 0) {
        unmount_disk(device);
    } else if (strcmp(argv[1], "backup") == 0 && argc == 6) {
        const char* backup_dir = argv[3];
        char fs_type[16];
        char partition_table[16];
        parse_fs_and_pt(argv[4], argv[5], fs_type, partition_table);
        backup_disk(device, backup_dir, fs_type, partition_table);
    } else if (strcmp(argv[1], "revert") == 0 && argc == 4) {
        const char* backup_path = argv[3];
        revert_disk(device, backup_path);
    } else {
        printf("Unknown command or incorrect arguments.\n");
    }

    return 0;
}
