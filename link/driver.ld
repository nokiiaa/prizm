ENTRY(_DriverEntry)

SECTIONS
{
    . = 0x80101000;

	.text BLOCK(4096) : ALIGN(4096)
	{
		*(.text)
	}

	.rodata BLOCK(4096) : ALIGN(4096)
	{
		*(.rodata)
	}

	.data BLOCK(4096) : ALIGN(4096)
	{
		*(.data)
	}

	.bss BLOCK(4096) : ALIGN(4096)
	{
		*(COMMON)
		*(.bss)
	}
}