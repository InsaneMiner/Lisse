

bits 32
section .text
        ;multiboot spec
        align 4
        dd 0x1BADB002               ;magic
        dd 0x00000001
        dd 0x00                    ;flags
        dd - (0x1BADB002 + 0x00)   ;checksum. m+f+c should be zero

global start
global keyboard_handler
global read_port
global write_port
global load_idt

extern kmain 		;this is defined in the c file
extern keyboard_handler_main






  
  gdt:
 
  gdt_null:
  dq 0
 
  gdt_code:
  dw 0FFFFh
  dw 0
 
  db 0
  db 10011010b
  db 11001111b
  db 0
 
  gdt_data:
  dw 0FFFFh
  dw 0
 
  db 0
  db 10010010b
  db 11001111b
  db 0
 
  gdt_end:
 
  gdt_desc:
   dw gdt_end - gdt - 1
   dd gdt
 
  load_gdt:
    cli  ;disable interrupts
    lgdt [gdt_desc]  ;load GDT
    sti  ;enable interrupts





read_port:
	mov edx, [esp + 4]
			;al is the lower 8 bits of eax
	in al, dx	;dx is the lower 16 bits of edx
	ret

write_port:
	mov   edx, [esp + 4]    
	mov   al, [esp + 4 + 4]  
	out   dx, al  
	ret

load_idt:
	mov edx, [esp + 4]
	lidt [edx]
	sti 				;turn on interrupts
	ret

keyboard_handler:                 
	call    keyboard_handler_main
	iretd

start:
cli ;block interrupts
;we must load gdt
lgdt [gdt_desc]
jmp 0x0008:fix_cs
fix_cs:
mov ax, 0x0010
mov ds, ax
mov es, ax
mov fs, ax
mov gs, ax
mov ss, ax
mov esp, stack_space   ;set stack pointer
call kmain
jmp $   ;halt the CPU

section .bss
resb 8192; 8KB for stack
stack_space:
