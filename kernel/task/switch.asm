; Context switch: save current regs, load next PCB, restore, iret or ret
; void switch_to(pcb_t *old, pcb_t *next);
; pcb_t: kernel_rsp=0, cr3=8, ..., is_user at offset 32

global switch_to
extern current_pcb
extern next_is_user

switch_to:
    ; rdi = old, rsi = next
    ; Save current rsp to old->kernel_rsp
    test rdi, rdi
    jz .load_next
    mov [rdi], rsp
.load_next:
    ; Set current_pcb = next
    mov [rel current_pcb], rsi
    ; Load is_user flag
    mov al, [rsi + 32]
    mov [rel next_is_user], al
    ; Load next->kernel_rsp
    mov rsp, [rsi]
    ; Load next->cr3
    mov rax, [rsi + 8]
    mov cr3, rax
    ; Restore registers
    pop rax
    pop rbx
    pop rcx
    pop rdx
    pop rsi
    pop rdi
    pop rbp
    pop r8
    pop r9
    pop r10
    pop r11
    pop r12
    pop r13
    pop r14
    pop r15
    ; Check if user mode
    mov al, [rel next_is_user]
    test al, al
    jnz .do_iret
    ret
.do_iret:
    iretq
