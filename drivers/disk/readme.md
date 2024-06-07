# Disk driver
Provides raw access to a disk's contents.
May be accessed by opening a handle to "Device\Disk\X" or "Device\Disk\X\Y", where the former form is opening a handle to the entire drive number X, while the latter form opens a specific partition of it, numbered Y.