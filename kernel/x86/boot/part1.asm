	org 0x7c00
	bits 16
	jmp start
	nop
	times 0x57 db 0 ; BPB space
	base equ $$

start:
	jmp 0x0000:.fix ; fix CS
.fix:
	mov byte [drive], dl
	xor ax, ax
	mov ds, ax
	mov es, ax
	mov ss, ax
	mov sp, 0x7bf0
	mov bp, sp
	cld
	; number of hidden sectors
	mov eax, dword [base+0x1c]
	; number of reserved sectors
	movzx ecx, word [base+0x0e]
	add eax, ecx
	mov dword [fat_start], eax
	; number of FATs in partition
	movzx ecx, byte [base+0x10]
	; sectors per FAT
	mov edx, dword [base+0x24]
	imul ecx, edx
	add eax, ecx
	; eax = start of data 
	mov dword [data_start], eax
	; root directory cluster
	mov ecx, dword [base+0x2c]
.loop:
	mov di, 0xc00
	call follow_cluster_chain
	push ecx
.check_file:
	cmp byte [di], 0
	je .stop_check
	mov bx, files
.file_search_loop:
	push di
	mov cx, 11
	mov si, bx
	repz cmpsb
	pop di
	jnz .next_file
	push es
	push di
	mov si, di
	mov di, word [bx+13]
	mov es, di
	mov di, word [bx+11]
	call read_file
	pop di
	pop es
	; if all files found, halt the search
	inc byte [files_found]
	cmp byte [files_found], file_count
	je .end
.next_file:
	add bx, 15
	cmp bx, files + file_count * 15
	jl .file_search_loop
.advance:
	add di, 32
	jmp .check_file
.stop_check:
	pop ecx
	test ecx, ecx
	jnz .loop
.end:
	; jump to part 2 bootloader
	jmp 0:0x2000

	; ecx = sector number
	; es:di = destination
	; ax = count
read_sectors:
	; set number of sectors to read
	mov word [dap + 2], ax
	; set destination (es:di)
	mov word [dap + 4], di
	mov ax, es
	mov word [dap + 6], ax
	; set LBA of starting sector (ecx)
	mov dword [dap + 8], ecx
	push si
	mov si, dap
	mov ah, 42h
	mov dl, byte [drive]
	int 13h
	pop si
	ret

	; ecx = cluster number
	; es:di = destination
	; output: ecx = next cluster
follow_cluster_chain:
	; read FAT sector
	push ecx
	push es
	push di
	shr ecx, 7
	add ecx, dword [fat_start]
	xor ax, ax
	mov es, ax
	mov di, 0x500
	mov ax, 1
	call read_sectors
	pop di
	pop es
	pop ecx
	
	; read the current cluster
	push ecx
	movzx esi, byte [base+0xd]
	sub ecx, 2 ; subtract 2 reserved clusters
	imul ecx, esi
	add ecx, dword [data_start]
	mov ax, si
	call read_sectors
	pop ecx
	
	shl ecx, 2
	and ecx, 511
	mov ecx, dword [ecx+0x500]
	and ecx, 0x0fffffff
	cmp ecx, 0x0ffffff8
	jl .return
	; this is the end of the chain
	xor ecx, ecx
.return:
	ret

	; ds:si = root directory entry
	; es:di = destination
read_file:
	; Cluster count
	xor bx, bx
	; bp = bytes per cluster
	movzx bp, byte [base+0xd] ; sectors per cluster
	shl bp, 9
	; ecx = cluster number
	movzx ecx, word [si+20] ; high cluster number
	shl ecx, 16
	mov cx, word [si+26] ; low cluster number
.loop:
	call follow_cluster_chain
	jz .end
	; advance destination
	add di, bp
	jnz .dont_advance_segment
	; add 0x10000 to the physical address
	; if we hit a 64K boundary (offset wrapped back to 0)
	mov ax, es
	add ax, 0x1000
	mov es, ax
.dont_advance_segment:
	jmp .loop
.end:
	ret

file_count equ 4
files:
	db "PART2   BIN"
	dw 0x2000, 0x0000 ; 0x0000:0x2000
	db "PZKERNELEXE"
	dw 0x0000, 0x1000 ; 0x1000:0x0000
    db "DISK    SYS"
    dw 0x0000, 0x3000 ; 0x3000:0x0000
    db "FAT     SYS"
    dw 0x0000, 0x4000 ; 0x4000:0x0000

dap:
	db 0x10
	db 0
	dw 0
	dw 0, 0
	dq 0

drive: db 0
fat_start: dd 0
data_start: dd 0
files_found: db 0

times 510-$+$$ db 0
dw 0xaa55