# SGFS
SGFS is a experimental FS made by me, its for my private use but why not share it, here is the source for the formatter to SGFS and a simple file explorer which can mount SGFS disks and manage them, the explorer is also usable as a normal one.

## 1. SGFS CLI Tool (`sgfs_cli/`)

### Commands:
- **`m <device>`**: Mount the specified SGFS disk.
- **`im`**: Show the currently mounted SGFS disk.
- **`mdd <device>`**: Unmount and mount a different disk.
- **`f <device> <block_size> <total_blocks>`**: Format the disk to SGFS.

### Building the SGFS CLI Tool:
```bash
cd sgfs_cli
make