bits 64
default rel

section .data
    ifmt db "int: %d", 10, 0
    ffmt db "float: %f", 10, 0

section .text
    extern printf
    extern malloc
    extern free

    global main

_print:
    push rbp
    mov rbp, rsp
    sub rsp, 32
    lea rcx, [rel ifmt]
    mov edx, eax
    call printf
    
    
    mov rsp, rbp
    pop rbp
    ret
_print_int:
    push rbp
    mov rbp, rsp
    sub rsp, 48
    mov rax, qword[rbp + 16]
    mov qword [rbp - 40], rax
    ; ref reassignment expression
    mov rax, [rbp - 40]
    mov eax, [rax]
    ; ref reassign to stack
    mov rdx, [rbp - 40]
    mov [rdx], eax
    call _print
    
    
    mov rsp, rbp
    pop rbp
    ret
main:
    push rbp
    mov rbp, rsp
    sub rsp, 48
    ; (0) while expression
    mov eax, 1
    ; (0) while comparison and jumping
    cmp al, 1
    je while_body_0
    jmp while_end_0
    
while_body_0:
    ; (0) while body
    ; _'s expression
    ; new heap pointer
    mov ecx, 1
    call malloc
    push rax
    mov al, 0
    pop rcx
    mov byte [rcx], al
    mov rax, rcx
    mov qword [rbp - 40], rax
    mov rax, [rbp - 40]
    mov rcx, rax
    call free
    ; (0) while loopback check expression
    mov eax, 1
    ; (0) while loopback check action
    cmp al, 1
    je while_body_0
    jmp while_end_0
    
while_end_0:
    ; (0) while end
    mov eax, 0
    mov rsp, rbp
    pop rbp
    ret
    

