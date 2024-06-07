bits 32
section .text

extern _CpuExceptionHandler, _IrqHandler, _SyscallHandler
global _HalDisableInterrupts, _HalEnableInterrupts, _HalLoadGdt, _HalHaltCpu
global _HalLoadIdt, _HalSwitchContext, _HalReadIfFlag, _HalWriteIfFlag
global _HalSwitchPageTable, _HalIdtHandlerArray, _HalFlushCacheForPage
global _HalReadCr0, _HalReadCr1, _HalReadCr2, _HalReadCr3
global _HalWriteCr0, _HalWriteCr1, _HalWriteCr2, _HalWriteCr3
global _HalAcquireSpinlock, _HalReleaseSpinlock
global _HalSwitchContextKernel, _HalSwitchContextUser
global _HalLoadFs, _HalLoadGs, _HalSyscallEntry, _HalRepMemcpy, _HalEnableSSE, _HalFloatingPointSave

irq_handler:
    pusha
    push ds
    push es
    push fs
    push gs
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov gs, ax
    mov ax, 0x18
    mov fs, ax
    push esp
    call _IrqHandler
    add esp, 4
    pop gs
    pop fs
    pop es
    pop ds
    popa
    add esp, 8
    iret

exception_handler:
    pusha
    push ds
    push es
    push fs
    push gs
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov gs, ax
    mov ax, 0x18
    mov fs, ax
    push esp
    call _CpuExceptionHandler
    add esp, 4
    pop gs
    pop fs
    pop es
    pop ds
    popa
    add esp, 8
    iret

_HalSyscallEntry:
    push 0x80
    push 0
    pusha
    push ds
    push es
    push fs
    push gs
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov gs, ax
    mov ax, 0x18
    mov fs, ax
    push esp
    call _SyscallHandler
    add esp, 4
    pop gs
    pop fs
    pop es
    pop ds
    popa
    add esp, 8
    iret

_HalEnableSSE:
    mov eax, cr0
    and ax, 0xFFFB
    or ax, 0x2
    mov cr0, eax
    mov eax, cr4
    or ax, 3 << 9
    mov cr4, eax
    ret
    
_HalRepMemcpy:
    push esi
    push edi
    mov esi, [esp+16]
    mov edi, [esp+12]
    mov ecx, [esp+20]
    mov edx, ecx
    and edx, 3
    shr ecx, 2
    rep movsd
    mov ecx, edx
    rep movsb
    pop edi
    pop esi
    ret

%assign i 0
%rep 32
stub%+i:
    %if i != 8 && i != 10 && i != 11 && i != 11 && i != 12 && i != 13 && i != 14 && i != 17 && i != 30
        push dword 0
    %endif
    push dword i
    jmp exception_handler
    %assign i i+1
%endrep

%rep 17
stub%+i:
    push dword 0
    push dword i - 32
    jmp irq_handler
    %assign i i+1
%endrep

%assign i 0
_HalIdtHandlerArray:
%rep 48+1
    dd stub%+i
    %assign i i+1
%endrep

_HalDisableInterrupts:
    cli
    ret
    
_HalEnableInterrupts:
    sti
    ret
    
_HalHaltCpu:
    hlt
    ret
    
    ; 
_HalLoadIdt:
    push ebp
    mov ebp, esp
    mov eax, [ebp+8]
    lidt [eax]
    pop ebp
    ret
    
    ; HalLoadGdt(void *desc, int tss_descriptor);
_HalLoadGdt:
    mov eax, dword [esp+4]
    mov cx, word [esp+8]
    lgdt [eax]
    push dword 0x10 ; ds
    push dword 0x10 ; es
    push dword 0x18 ; fs
    push dword 0x10 ; gs
    push dword 0x10 ; ss
    ltr cx ; load task register
    pop ss
    pop gs
    pop fs
    pop es
    pop ds
    ret

_HalFloatingPointSave:
    mov eax, [esp+4]
    fxsave [eax]
    ret

    ; Context switch 1 (only restoring e registers is required)   
_HalSwitchContextKernel:
    add esp, 4
    mov eax, [fs:68]
    fxrstor [eax]
    mov eax, dword [fs:4]
    mov ecx, dword [fs:8]
    mov edx, dword [fs:12]
    mov ebx, dword [fs:16]
    mov ebp, dword [fs:24]
    mov esi, dword [fs:28]
    mov edi, dword [fs:32]
    mov esp, dword [fs:20]
    push dword [fs:0 ] ; eflags
    push dword [fs:40] ; cs
    push dword [fs:36] ; eip
    iret

    ; Context switch 2 (restores entire state, including
    ; segment registers and user stack pointer through
    ; inter-privilege iret)
_HalSwitchContextUser:
    add esp, 4
    mov eax, [fs:68]
    fxrstor [eax]
    mov eax, dword [fs:4]
    mov ecx, dword [fs:8]
    mov edx, dword [fs:12]
    mov ebx, dword [fs:16]
    mov ebp, dword [fs:24]
    mov esi, dword [fs:28]
    mov edi, dword [fs:32]
    push dword [fs:60] ; ss
    push dword [fs:20] ; esp
    push dword [fs:0]  ; eflags
    push dword [fs:40] ; cs
    push dword [fs:36] ; eip
    push dword [fs:44] ; ds
    push dword [fs:48] ; es
    push dword [fs:52] ; fs
    push dword [fs:56] ; gs
    pop gs
    pop fs
    pop es
    pop ds
    iret
    
_HalReadIfFlag:
    pushf
    pop eax
    and eax, 1 << 9
    shr eax, 9
    ret

_HalWriteIfFlag:
    push ebp
    mov ebp, esp
    cmp dword [ebp+8], 0
    je .cli
    sti
    jmp .skip
.cli:
    cli
.skip:
    pop ebp
    ret

_HalSwitchPageTable:
    mov eax, [esp+4]
    mov cr3, eax
    ret

_HalFlushCacheForPage:
    mov eax, [esp+4]
    invlpg [eax]
    ret

_HalLoadFs:
    push dword [esp+4]
    pop fs
    ret

_HalLoadGs:
    push dword [esp+4]
    pop gs
    ret

%assign i 0
%rep 4
_HalReadCr%+i:
    mov eax, cr%+i
    ret

_HalWriteCr%+i:
    mov eax, [esp+4]
    mov cr%+i, eax
    ret
%assign i i+1
%endrep

_HalAcquireSpinlock:
    mov edx, [esp+4]
    mov ecx, 1
.retry:
    xor eax, eax
    xacquire lock cmpxchg [edx], ecx
    je .out
.pause:
    mov eax, [edx]
    test eax, eax
    jz .retry
    rep nop
    jmp .pause
.out:
    ret

_HalReleaseSpinlock:
    xor eax, eax
    mov ecx, dword [esp+4]
    xrelease lock xchg dword [ecx], eax
    ret