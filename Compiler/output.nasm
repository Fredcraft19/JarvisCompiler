bits 64
default rel
extern printf

section .data
    fmt db "%f", 10, 0
    float_const_0 dd 10.500000

    float_const_1 dd 5.000000


section .text
global main

main:
    push rbp
    mov rbp, rsp
    sub rsp, 4
    
    ; a's expression
    movss xmm0, [rel float_const_0]
    sub rsp, 8
    movss [rsp], xmm0
    movss xmm0, [rel float_const_1]
    movss xmm1, [rsp]
    add rsp, 8
    addss xmm0, xmm1
    
    ; a saved in stack
    movss dword [rbp - 36], xmm0
    

    movss    xmm0, dword [rbp - 4]
    cvtss2sd xmm1, xmm0            ; printf takes doubles, so float32 must be widened
    movq     rdx, xmm1             ; variadic: the double goes in BOTH xmm1 and rdx
    lea      rcx, [rel fmt]        ; arg 1 = format string
    call     printf    
    mov  rsp, rbp
    pop rbp
    ret
