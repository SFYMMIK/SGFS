# NEWS FOR SGFS
### Anyone who has seen this project a little before knows that it is not compatible to open with normal explorers so,
### I am making it a package and i will be trying to make it avalible on AUR to download and compile, this newer version is also inspired by ReiserFS a little bit, SGFS will have its own utils as a package for the newer version which might be the first official release to the public to make sure to have it tested and to make it usable outside of a specialized file explorer, little bit of SGFS-Utils(this version of the readme is avalible here for only a bit, SGFS-Utils will have its own repo soon and right now the utils will be in the folder SGFS_UTILS:

# SGFS Utils

`sgfs-utils` is a package that provides utilities for managing the SGFS filesystem and SGPT partition table. Inspired by the efficiency of ReiserFS and designed with a unique SGPT partition table, SGFS offers fast file operations and robust filesystem utilities for Linux systems.

### Features:

1. **SGFS Filesystem**:
   - SGFS is a custom filesystem inspired by the efficiency of ReiserFS.
   - Optimized for both small and large file operations with a focus on performance.
   - Supports journaling for data integrity.

2. **SGPT Partition Table**:
   - SGPT (Szymon Grajner Partition Table) is a unique partition table format specifically designed to work efficiently with SGFS.
   - Unlike DOS or GPT, SGPT is structured to avoid misidentification by tools like `mkfs`.

3. **FUSE Integration**:
   - The package includes a robust FUSE implementation, allowing SGFS to be mounted and used with standard file explorers like Dolphin, Nautilus, and others.
   - Supports file operations such as create, read, write, copy, cut, delete, and rename within file explorers.

4. **`mkfs.sgfs` Utility**:
   - Easily format any disk to SGFS using the `mkfs.sgfs` command.
   - Automatically sets up the SGPT partition table and initializes the SGFS filesystem.

5. **`sgfs-convert` Utility**:
   - (This utility is used if mkfs.sgfs does not work i guess)
   - Convert existing filesystems (e.g., ext4, FAT32) to SGFS.
   - Convert partition tables (e.g., DOS, GPT) to SGPT without losing data.
   - Ensures a smooth migration to SGFS for existing disks.

6. **Compatibility with System Utilities**:
   - Once installed, the package allows tools like `mkfs`, `mount`, and `fsck` to recognize SGFS, providing native-like integration into Linux systems.
   - The utilities integrate seamlessly into the system, making SGFS available just like other filesystems (ext4, XFS, etc.).

7. **Easy Installation via AUR**:
   - Available for Arch Linux users through the AUR as `sgfs-utils`.
   - Simply install with `makepkg -si` or `makepkg -i` and get started with SGFS on your system.
## Now back to the original readme:


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
