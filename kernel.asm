org 0x7C00
bits 16
mov ah, 0x0e
mov al, 'A'
int 0x10
times 510-($-$$) db 0
dw 0AA55h
