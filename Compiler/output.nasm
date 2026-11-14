bits 64
default rel
extern printf

section .data
    fmt db "%f", 10, 0

section .text
global main

main:
    push rbp
    mov rbp, rsp
    sub rsp, 8
    
    ; i's expression
    mov eax, 0
    
    ; i saved in stack
    mov dword [rbp - 36], eax
    
    ; (0) while expression
    mov eax, [rbp - 36]
    push rax
    mov eax, 10
    mov rcx, rax
    pop rax
    cmp eax, ecx
    setl al
    ; (0) while comparison and jumping
    cmp al, 1
    je while_body_0
    jmp while_end_0
    
while_body_0:
    ; (0) while body
    ; i reassignment expression
    mov eax, [rbp - 36]
    push rax
    mov eax, 1
    mov rcx, rax
    pop rax
    add eax, ecx
    ; i reassign to stack
    mov dword [rbp - 36], eax
    ; (0) while loopback check expression
    mov eax, [rbp - 36]
    push rax
    mov eax, 10
    mov rcx, rax
    pop rax
    cmp eax, ecx
    setl al
    ; (0) while loopback check action
    cmp al, 1
    je while_body_0
    jmp while_end_0
    
while_end_0:
    ; (0) while end
    ; output's expression
    mov eax, [rbp - 36]
    
    ; output saved in stack
    mov dword [rbp - 40], eax
    
    
    mov  rsp, rbp
    pop rbp
    ret
