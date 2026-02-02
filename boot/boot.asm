; Multiboot2 header + bootstrap: 32-bit protected (from GRUB) -> long mode -> kernel_main
; GRUB loads kernel at 1MB and jumps to _start in 32-bit protected mode.

MB2_MAGIC     equ 0xE85250D6
MB2_ARCH      equ 0
MB2_TAG_END   equ 0
MB2_TAG_INFO  equ 1

section .multiboot
align 8
multiboot_header_start:
    dd MB2_MAGIC
    dd MB2_ARCH
    dd multiboot_header_end - multiboot_header_start
    dd -(MB2_MAGIC + MB2_ARCH + (multiboot_header_end - multiboot_header_start))
    ; optional tags here
    dw MB2_TAG_END
    dw 0
    dd 8
multiboot_header_end:

section .text
bits 32
global _start
extern kernel_main
extern __stack_top
extern __bss_start
extern __bss_end

; Page table constants (identity map first 16MB using 2MB huge pages)
PML4_TABLE    equ 0x1000
PDP_TABLE     equ 0x2000
PD_TABLE      equ 0x3000

_start:
    mov esp, __stack_top
    ; Clear BSS
    mov edi, __bss_start
.clear_bss:
    cmp edi, __bss_end
    jge .bss_done
    mov byte [edi], 0
    inc edi
    jmp .clear_bss
.bss_done:
    ; Save multiboot magic and info (GRUB passes EAX=magic, EBX=info)
    push ebx
    push eax

    ; Build identity-mapped page tables using 2MB huge pages
    ; Clear 3 pages for PML4, PDP, PD
    mov edi, PML4_TABLE
    xor eax, eax
    mov ecx, 3072       ; 3 pages * 4096 / 4 = 3072 dwords
    rep stosd

    ; Fill PD with 2MB huge pages (PS bit = 0x83: Present + Writable + Huge)
    ; Map first 16MB (8 * 2MB pages)
    mov edi, PD_TABLE
    mov eax, 0x083      ; Present + Writable + PS (huge page)
    mov ecx, 8          ; 8 * 2MB = 16MB
.fill_pd:
    mov dword [edi], eax
    mov dword [edi + 4], 0
    add edi, 8
    add eax, 0x200000   ; next 2MB
    loop .fill_pd

    ; PML4[0] -> PDP_TABLE
    mov dword [PML4_TABLE], PDP_TABLE | 0x003
    mov dword [PML4_TABLE + 4], 0
    ; PDP[0] -> PD_TABLE
    mov dword [PDP_TABLE], PD_TABLE | 0x003
    mov dword [PDP_TABLE + 4], 0

    ; CR3 = PML4
    mov eax, PML4_TABLE
    mov cr3, eax
    ; Enable PAE
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax
    ; Enable long mode (EFER MSR 0xC0000080)
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr
    ; Enable paging
    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax

    ; Load 64-bit GDT
    lgdt [gdt64_ptr]
    ; Long jump to 64-bit code
    jmp gdt64_code:long_mode_start

section .rodata
align 8
gdt64:
    dq 0                                                  ; 0x00: null
gdt64_code: equ $ - gdt64                                 ; 0x08
    dq (1<<43) | (1<<44) | (1<<47) | (1<<53)              ; kernel code: 64-bit, present, DPL=0
gdt64_data: equ $ - gdt64                                 ; 0x10
    dq (1<<41) | (1<<44) | (1<<47)                        ; kernel data: writable, present, DPL=0
gdt64_udata: equ $ - gdt64                                ; 0x18 (user data BEFORE user code for SYSRET)
    dq (1<<41) | (1<<44) | (1<<47) | (3<<45)              ; user data: writable, present, DPL=3
gdt64_ucode: equ $ - gdt64                                ; 0x20
    dq (1<<43) | (1<<44) | (1<<47) | (1<<53) | (3<<45)    ; user code: 64-bit, present, DPL=3
gdt64_len equ $ - gdt64

gdt64_ptr:
    dw gdt64_len - 1
    dq gdt64

section .text
bits 64
long_mode_start:
    ; Load data segment selector (must match GDT: null=0, code=8, data=16)
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov fs, ax
    mov gs, ax
    mov rsp, __stack_top

    ; Multiboot magic in EDI, info in ESI (ABI: 32-bit args in lower half)
    pop rdi
    pop rsi
    ; Call kernel_main(magic, info)
    call kernel_main
.hang:
    hlt
    jmp .hang
