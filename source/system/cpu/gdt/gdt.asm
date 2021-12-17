; Copyright (C) 2021  ilobilo

LoadGDT:
    lgdt [rdi]
    mov ax, 0x30
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    pop rdi
    mov rax, 0x28
    push rax
    push rdi
    retfq
GLOBAL LoadGDT

LoadTSS:
    mov ax, (9 * 8) | 0
	ltr ax
	retq
GLOBAL LoadTSS