; Context switch: save current regs, load next PCB, restore, iret or ret
; void switch_to(pcb_t *old, pcb_t *next);
; pcb_t: kernel_rsp=0, cr3=8, ..., is_user at offset 32

global switch_to
extern current_pcb
extern next_is_user

switch_to:
    ; rdi = old, rsi = next
    ; Save current state to old->kernel_rsp
    test rdi, rdi
    jz .load_next
    ; Save callee-saved registers before switching
    push r15
    push r14
    push r13
    push r12
    push rbp
    push rbx
    mov [rdi], rsp    ; save current stack pointer
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
    ; Check if user mode
    mov al, [rel next_is_user]
    test al, al
    jnz .do_iret
    ; Kernel task: restore callee-saved registers and return
    pop rbx
    pop rbp
    pop r12
    pop r13
    pop r14
    pop r15
    ret
.do_iret:
    ; User task: restore all registers then iret
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
    iretq
