# SGFS
SGFS is a experimental FS made by me, its for my private use but why not share it, here is the source for the formatter to SGFS and a simple file explorer which can mount SGFS disks and manage them, the explorer is also usable as a normal one.

# Warning!, the folder named 'SGFS_ID' is a folder for the latest but not yet finished updates of SGFS in which i will try to introduce: FUSE, FUSE Operations, Newer Commands, Revert changes to disk (backup files to restore the disk to its original state).

## SGFS CLI Tool (`sgfs_cli/`)

The SGFS CLI tool provides the ability to format, mount, unmount, and manage disks with SGFS, and includes its own logic to create a GPT partition table.

### Features:
- **Create SGPT Partition Table**: The SGFS CLI uses its own logic to create a GPT partition table on the disk before formatting it with SGFS.
- **Format Disk with SGFS**: Formats a disk partition with the SGFS filesystem, including writing the superblock, inode table, and block bitmaps.
- **Mount/Unmount Disk**: Mount or unmount SGFS disks.
- **Root Privileges**: The program checks for root privileges and forces the user to run it with `sudo`.

### Commands:
- **`m <device>`**: Mount the specified SGFS disk.
- **`mdd <device>`**: Unmount the current disk and mount another one.
- **`f <device> <block_size>`**: Format the disk with SGFS and a SGPT partition table.

### Example Usage:

#### Creating The mount Point
To create the mount point just type this command:
```bash
sudo mkdir -p /mnt/sgfs
```

#### Mounting a Disk
To mount an SGFS-formatted disk:
```bash
sudo ./sgfs_cli m /dev/sdX
```

## To display currently mounted Disk:
sudo ./sgfs_cli im

## Changing Mounted Disk
To unmount the current SGFS disk and mount a different one:
```bash
sudo ./sgfs_cli mdd /dev/sdY
```
## Formatting a Disk
To format a disk with SGFS and create a GPT partition table:
```bash
sudo ./sgfs_cli f /dev/sdX 4096 1024
```

## In this example:

- /dev/sdX is the disk to be formatted.
- 4096 is the block size (in bytes).
- 1024 is the total number of blocks for SGFS.

### Key Updates:
1. **Added instructions** for the new GPT partitioning functionality.
2. **Clarified** the root privilege requirements for the SGFS CLI.
3. **Included commands and examples** for mounting, unmounting, and formatting a disk with the new GPT and SGFS logic.
