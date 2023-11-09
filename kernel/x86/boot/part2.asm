bits 16
org 0x2000

%define ELF_SHF_ALLOC 0x02
%define ELF_SECT_NOBITS 0x08

main:
	; set up segment registers and stack
	xor ax, ax
	mov ds, ax
	mov es, ax
	mov ss, ax
	mov sp, 0x1ff0
	mov bp, sp
	cld
	; init FPU
	mov eax, cr0
	and eax, ~((1 << 2) | (1 << 3))
	or eax, 1 << 5
	mov cr0, eax
	fninit
	; get E820 memory map
	xor di, di
	mov bx, 0x110
	mov es, bx
	xor bx, bx
.next_run:
	mov eax, 0xE820
	mov cx, 20
	; set SMAP signature (it's reversed)
	mov edx, 'PAMS'
	int 15h
	jnc .no_error
	mov si, e820_error
	jmp error_message
.no_error:
	add di, 20
	; is this the last run?
	test bx, bx
	jnz .next_run
	mov es, bx
	xor bx, bx
	; set VESA video mode
	%define WIDTH 640
	%define HEIGHT 480
	%define BIT_DEPTH 32
	; get list of all supported VESA modes to 0x500
    mov ax, 0x4f00
	mov di, 0x50
	mov es, di
	xor di, di
	; set VBE2 signature (it's reversed)
	mov dword [es:di], '2EBV'
	int 10h
	; es = 0x100 (output segment of VESA mode information)
	; ds = word [es:16] (segment of video mode table)
	; si = word [es:14] (offset of video mode table)
	push word 0x100
	push word [es:16]
	mov si, word [es:14]
	pop ds
	pop es
	xor di, di
	; iterate over all supported video modes
.vloop:
	mov cx, [ds:si]
	cmp cx, 0xffff
	je .fail
	; get video mode info
	mov ax, 0x4f01
	int 10h
	; check for pixel depth, width and height
	cmp byte [es:25], BIT_DEPTH
	jne .next
	cmp word [es:18], WIDTH
	jne .next
	cmp word [es:20], HEIGHT
	jne .next
	; stop the loop if everything is correct
	; this video mode's data is now located
	; on physical address 0x00001000
	; clean ds
	push word 0
	pop ds
	jmp .end
.next:
	add si, 2
	jmp .vloop
.fail:
	mov si, vesa_error_1
	jmp error_message
.end:
	; set VESA video mode to the one specified in dx
	mov ax, 0x4f02
	mov bx, cx
	or bx, 0x4000 ; linear framebuffer
	int 10h
	jnc .a20
	mov si, vesa_error_2
	jmp error_message
.a20:
	cli
	; enable A20 line (really shitty quick way)
	in al, 0x92
	or al, 2
	out 0x92, al
	xor ax, ax
	mov ds, ax
	; load GDT
	lgdt [gdt_desc]
	; enter protected mode
	mov eax, cr0
	or al, 1
	mov cr0, eax
	; jump to PE loader code (32-bit)
	jmp 0x8:pmode_start

gdt_desc:
	dw gdt_end - gdt_entries
	dd gdt_entries

gdt_entries:
	dq 0x0000000000000000
	dq 0x00cf98000000ffff
	dq 0x00cf92000000ffff
	
gdt_end:

; 0:si - pointer to string
error_message:
	push word 0
	pop ds
.char:
	mov ah, 0xe
	lodsb
	test al, al
	jz .finish
	mov bx, 0xc
	int 10h
	dec dx
	jnz .char
.finish:
	cli
	hlt

bits 32

    %define DISK_BASE 0x10000
pmode_start:
	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax
    ; set up initial page table to bootstrap kernel
    ; page table physical base
    mov eax, 0x100000
    ; page directory physical base
    mov ebx, 0x500000
    ; stack end physical base
    mov edx, 0x501000
    mov dword [kphysical_start], edx
    mov ecx, 0x80000000 / 4096
    xor edi, edi
    ; identity map all entries from 0x00000000-0x80000000
.identity_map:
    mov dword [eax], edi
    or dword [eax], 3 ; read-write, present
    add eax, 4
    add edi, 4096
    dec ecx
    jnz .identity_map
    ; 4 MiB for stack and kernel
    mov ecx, 0x400000 / 4096
    ; map 0x80000000-0x80400000 to kernel space
.kernel_map:
    mov dword [eax], edx
    or dword [eax], 3 ; read-write, present
    add edx, 4096
    add eax, 4
    dec ecx
    jnz .kernel_map
    ; fill page directory
    mov ecx, 4096 / 4
    mov edx, 0x100000
.page_directory:
    mov dword [ebx], edx
    or dword [ebx], 3
    add ebx, 4
    add edx, 4096
    dec ecx
    jnz .page_directory
    ; set page directory and enable paging
    mov eax, 0x500000
    mov cr3, eax
    mov eax, cr0
    or eax, 0x80000001
    mov cr0, eax
pe_loader:
	mov esp, 0x80100000
	mov ebx, DISK_BASE ; kernel binary location
    cmp word [ebx], 'MZ' ; check for 'MZ' signature
    jne .error
    add ebx, [ebx+0x3c]
    cmp dword [ebx], 0x00004550 ; check for 'PE\0\0' signature
    jne .error
    add ebx, 0x4
    cmp word [ebx], 0x14c ; i386 machine ID
    jne .error
    ; NumberOfSections
    movzx ecx, word [ebx+2]
    ; SizeOfOptionalHeader
    movzx edx, word [ebx+16]
    add ebx, 20
    ; Copy PE headers for later self-reading in the kernel
    push ecx
    mov ecx, dword [ebx+60]
    mov esi, DISK_BASE
    mov edi, 0x80100000
    rep movsb
    pop ecx
    ; ImageBase
    mov eax, dword [ebx+0x1C]
    ; AddressOfEntryPoint
    mov ebp, dword [ebx+0x10]
    add ebp, eax
    add ebx, edx
.load_sec:
    mov esi, [ebx+20] ; PointerToRawData
    add esi, DISK_BASE
    mov edi, [ebx+12] ; VirtualAddress
    add edi, eax ; add ImageBase
    push ecx
    ; if absolute virtual address is 0, skip section
    jz .next
    ; SizeOfRawData
    mov ecx, [ebx+16]
    ; is SizeOfRawData > VirtualSize?
    cmp ecx, [ebx+8]
    jg .virtual_smaller
    rep movsb
    ; subtract SizeOfRawData from VirtualSize
    mov ecx, [ebx+8]
    sub ecx, [ebx+16]
    ; pad the remainder of VirtualSize with zeroes
    push eax
    xor eax, eax
    rep stosb
    pop eax
    jmp .next
.virtual_smaller:
    mov ecx, [ebx+8]
    rep movsb
.next:
    pop ecx
    ; add size of section header
    add ebx, 40
    dec ecx
    jnz .load_sec
    sub edi, 0x80000000
    add edi, dword [kphysical_start]
    mov dword [kphysical_end], edi
    mov ecx, edi
    sub ecx, dword [kphysical_start]
    ; all sections loaded, call PzKernelInit
    mov eax, ebp
    mov ebp, esp
    sub esp, 32
    ; push KernelBootInfo {
    ;     KernelVirtualBase = 0x80100000,
    ;     KernelSize = ecx,
    ;     KernelPhysicalStart = kphysical_start,
    ;     KernelPhysicalEnd = kphysical_end,
    ;     MemMapPointer = 0x1100,
    ;     BootstrapDriverCount = 2,
    ;     BootstrapDriverBases = {0x30000, 0x40000}
    ; }
    push dword 0x40000
    push dword 0x30000
    push dword 2
    push dword 0x1100
    push dword [kphysical_end]
    push dword [kphysical_start]
    push ecx
    push dword 0x80100000
    push esp
    call eax
    jmp $
.error:
    cli
    hlt

vesa_error_1: db 'VESA video mode not found', 0
vesa_error_2: db 'Failed to switch to VESA video mode', 0
e820_error: db 'Failed to get E820 memory map', 0
kphysical_start: dd 0
kphysical_end: dd 0