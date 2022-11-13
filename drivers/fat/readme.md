# FAT32 driver
Provides access to a FAT32 filesystem.
If Device\Vol0 is registered as a FAT32 volume, then a file on it may be opened using PzCreateFile at the path "Device\Vol0\file.txt".