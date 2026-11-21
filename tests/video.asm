; SPDX-License-Identifier: GPL-2.0-or-later
; Boot-only VGA test harness. Calls the exact compiled pc_plane_row bytes.
bits 16
org 0x7c00
jmp 0:start
start:
 cli
 xor ax,ax
 mov ds,ax
 mov es,ax
 mov ss,ax
 mov sp,0x7000
 sti
 mov bx,0x7e00
 mov ax,0x0204
 mov cx,2
 xor dh,dh
 int 0x13
 jc fail
 jmp stage
fail:
 mov al,'F'
 out 0xe9,al
 mov al,0x11
 out 0xf4,al
 hlt
 jmp $
times 510-($-$$) db 0
dw 0xaa55
stage:
 mov word [video_mode],0x12
next_mode:
 mov ax,[video_mode]
 int 0x10
 xor ax,ax
 mov ds,ax
 ; BIOS mode defaults: linear 80-byte rows, write mode 0, all bits writable.
 mov dx,0x3c4
 mov ax,0x0402 ; write red plane only
 out dx,ax
 mov ax,0xa000
 mov es,ax
 xor di,di
 mov cx,80
 xor al,al
fill:
 stosb
 inc al
 loop fill
 mov byte [ega],0
next_backend:
 mov dx,0x3ce
 mov ax,0x0304 ; read plane 3 before capture
 out dx,ax
 mov ax,0x0805 ; read mode 1 before capture
 out dx,ax
 mov al,7
 out dx,al
 xor ax,ax
 mov es,ax
 mov ax,state
 push ax
 xor ax,ax
 mov al,[ega]
 push ax
 mov ax,80
 push ax
 mov ax,0x9000
 push ax
 xor ax,ax
 push ax
 sti
 call pc_plane_row
 add sp,10
 test ax,ax
 jnz fail
 pushf
 pop ax
 test ax,512
 jz fail
 xor ax,ax
 mov ds,ax
 mov si,0x9000
 mov cx,80
 mov bl,0xff
check:
 lodsb
 cmp al,bl
 jne fail
 dec bl
 loop check
 mov dx,0x3ce
 in al,dx
 cmp al,7
 jne fail
 mov al,4
 out dx,al
 inc dx
 in al,dx
 cmp al,3
 jne fail
 dec dx
 mov al,5
 out dx,al
 inc dx
 in al,dx
 cmp al,8
 jne fail
 cmp byte [ega],0
 jne mode_done
 inc byte [ega]
 jmp next_backend
mode_done:
 cmp word [video_mode],0x10
 je passed
 mov word [video_mode],0x10
 jmp next_mode
passed:
 mov si,message
print:
 lodsb
 test al,al
 jz finish
 out 0xe9,al
 jmp print
finish:
 mov al,0x10
 out 0xf4,al
 hlt
 jmp $
video_mode dw 0
state db 7,3,8
ega db 0
message db 'PASS native VGA plane pixels and register restoration, modes 10h/12h, VGA and EGA access paths',10,0
pc_plane_row:
 incbin 'pcfn.bin'
times 2560-($-$$) db 0
