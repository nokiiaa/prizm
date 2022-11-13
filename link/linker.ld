ENTRY(_PzKernelInit)

SECTIONS
{
    . = 0x80101000;

	.text BLOCK(1) : ALIGN(1)
	{
		*(.text)
	}

	.rodata BLOCK(1) : ALIGN(1)
	{
		*(.rodata)
	}

	.data BLOCK(1) : ALIGN(1)
	{
		*(.data)
	}

	.bss BLOCK(1) : ALIGN(1)
	{
		*(COMMON)
		*(.bss)
	}
}