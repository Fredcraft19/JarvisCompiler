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
_pow:
    push rbp
    mov rbp, rsp
    sub rsp, 64
    ; (is parameter)
    mov eax, dword[rbp + 16]
    ; value saved in stack
    mov dword [rbp - 36], eax
    
    ; (is parameter)
    mov eax, dword[rbp + 24]
    ; exponent saved in stack
    mov dword [rbp - 40], eax
    
    ; out's expression
    mov eax, 1
    ; cast value from 32-bit to 64-bit
    movsxd rax, eax
    ; out saved in stack
    mov qword [rbp - 48], rax
    
    ; (0) 'for' decl
    ; i's expression
    mov eax, 0
    
    ; i saved in stack
    mov dword [rbp - 52], eax
    
    ; (0) 'for' condition
    mov eax, [rbp - 52]
    push rax
    mov eax, [rbp - 40]
    mov rcx, rax
    pop rax
    cmp eax, ecx
    setl al
    ; (0) 'for' comparison and jumping
    cmp al, 1
    je for_body_0
    jmp for_end_0
    
for_body_0:
    ; (0) 'for' body
    ; out reassignment expression
    mov rax, [rbp - 48]
    push rax
    mov eax, [rbp - 36]
    mov rcx, rax
    pop rax
    imul rax, rcx
    ; out reassign to stack
    mov qword [rbp - 48], rax
    ; (0) 'for' step
    ; i reassignment expression
    mov eax, [rbp - 52]
    push rax
    mov eax, 1
    mov rcx, rax
    pop rax
    add eax, ecx
    ; i reassign to stack
    mov dword [rbp - 52], eax
    ; (0) 'for' condition
    mov eax, [rbp - 52]
    push rax
    mov eax, [rbp - 40]
    mov rcx, rax
    pop rax
    cmp eax, ecx
    setl al
    cmp al, 1
    je for_body_0
    jmp for_end_0
    
for_end_0:
    ; (0) 'for' end
    mov rax, [rbp - 48]
    mov rsp, rbp
    pop rbp
    ret
    
_sqr:
    push rbp
    mov rbp, rsp
    sub rsp, 48
    mov rax, qword[rbp + 16]
    mov qword [rbp - 40], rax
    ; ref reassignment expression
    mov rax, [rbp - 40]
    mov eax, [rax]
    push rax
    mov rax, [rbp - 40]
    mov eax, [rax]
    mov rcx, rax
    pop rax
    imul eax, ecx
    ; ref reassign to stack
    mov rdx, [rbp - 40]
    mov [rdx], eax
    
    
    mov rsp, rbp
    pop rbp
    ret
main:
    push rbp
    mov rbp, rsp
    sub rsp, 48
    ; a's expression
    mov eax, 10
    
    ; a saved in stack
    mov dword [rbp - 36], eax
    
    call _print
    ; (1) while expression
    mov eax, 1
    ; (1) while comparison and jumping
    cmp al, 1
    je while_body_1
    jmp while_end_1
    
while_body_1:
    ; (1) while body
    ; ram's expression
    ; new heap array pointer
    mov eax, 99
    mov ecx, eax
    imul ecx, 8
    call malloc
    mov qword [rbp - 44], rax
    ; (1) while loopback check expression
    mov eax, 1
    ; (1) while loopback check action
    cmp al, 1
    je while_body_1
    jmp while_end_1
    
while_end_1:
    ; (1) while end
    mov eax, 0
    mov rsp, rbp
    pop rbp
    ret
    

