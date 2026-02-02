; Syscall entry: user syscall -> kernel stack, call C handler, sysret
; RAX=syscall#, RDI,RSI,RDX=args. RCX=user RIP, R11=user RFLAGS.
global syscall_entry
extern current_pcb
extern syscall_handler

syscall_entry:
    ; Save user stack and flags
    mov r10, rsp            ; r10 = user RSP
    mov r9, r11             ; r9 = user RFLAGS
    ; Load kernel stack from current_pcb->kernel_rsp
    mov r11, [rel current_pcb]
    test r11, r11
    jz syscall_no_pcb       ; safety check
    mov rsp, [r11]          ; RSP = kernel_rsp
    ; Build frame for syscall_handler
    push r10                ; user RSP
    push r9                 ; user RFLAGS
    push rcx                ; user RIP
    push rax                ; syscall number
    push rdi                ; arg1
    push rsi                ; arg2
    push rdx                ; arg3
    mov rdi, rsp
    call syscall_handler
    ; Restore and return to user
    pop rdx
    pop rsi
    pop rdi
    pop rax
    pop rcx
    pop r11                 ; user RFLAGS
    pop r10                 ; user RSP
    mov rsp, r10
    sysretq
syscall_no_pcb:
    ; Fallback: halt if no PCB
    hlt
    jmp syscall_no_pcb
