org 7C00h


; wow, it's a bootloader


%define LOAD_ADDR 0x1000 
; boot signature
%define SIG_ADDR LOAD_ADDR + 3


_start:
	cli

; clear segment registers
    mov ax, 0        ; Reset segment registers to 0.
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; set text mode (80x25)
    mov ax, 0x0003
    int 0x10

	.after_init:
        mov si, login_prompt
        .login_prompt_loop:
            lodsb
            mov ah, 0x0e ; tty output
            int 0x10
            
            ; sleep between outputs to simulate baudrate :)
            xor cx, cx
            mov dx, 0x3fff
            mov ah, 0x86
            int 0x15

            test al, al
            jnz .login_prompt_loop


		mov si, daps ; disk packet address
		mov ah, 0x42 ; al unused
		mov dl, 0x80 ; what to copy
		int 0x13     ; do it

        ; read input
        xor si, si
        .login_read_loop:
            ; get keyboard key
            xor ah, ah
            int 0x16

            ; write it back out...
            mov ah, 0x0e ; tty output
            int 0x10

            mov [passwd_input + si], al
            inc si
            cmp si, 8
            jl .login_read_loop

        ; newline
        mov ah, 0x0e ; tty output
        mov al, 0xd ; \r
        int 0x10
        mov ah, 0x0e ; tty output
        mov al, 0xa ; \n
        int 0x10

		; decrypt blob
		xor di, di
		xor si, si

		.dec_loop:
			mov al, [passwd_input + si]
			xor al, 0x3c
			xor byte [LOAD_ADDR + di], al

			inc di
			inc si
			and si, 0b111
			
			cmp di, 512 * NUM_SECTORS
			jl .dec_loop
		
		; check boot sig
		cmp word [SIG_ADDR + 0], 'WA'
		jne .sig_fail
		cmp word [SIG_ADDR + 2], 'RG'
		jne .sig_fail
		cmp word [SIG_ADDR + 4], 'AM'
		jne .sig_fail
		cmp word [SIG_ADDR + 6], 'ES'
		jne .sig_fail

		.sig_win:
			; Set video mode
			; VGA (chunky mode) 320×200
			mov ax, 0x0013
			int 0x10
			
			; setup stack
			mov sp, 0xF000
			mov bp, sp

			jmp LOAD_ADDR ; start it up :D

		.sig_fail:
			; TODO: print error message
			jmp .after_init ; try again


align 16

daps: ; disk address packet structure
.size: db 0x10
db 0 ; always 0
.num_sectors: dw NUM_SECTORS ; this value come from the environment, see the makefile
.transfer_buffer: dd LOAD_ADDR
.lba_lower: dd 0x1
.lba_upper: dd 0x0


; correct passwd is "-JOSHUA-"
; Guess it's a bit of a credits thing?
passwd_input: db "        "

db 0x0

login_prompt: db "WOPR SUPERCOMPUTER", 0xd, 0xa, \
                 "THIS IS A CSAW OWNED COMPUTER SYSTEM - ALL ATTEMPTS AT ILLEGAL ACCESS WILL", 0xd, 0xa, \
                 "BE SEVERELY PUNISHED TO THE FULL EXTENT OF THE LAW.", 0xd, 0xa, \
                 "FOR AUTHORIZATION, PLEASE WRITE TO PROF. STEVEN FALKEN.", 0xd, 0xa, \
                 "LOGIN>", 0


; A little note in the spare bootloader space
%macro credits_line 1
align 16
db %1
%endmacro

credits_line "CSAW FINALS 2017"
credits_line "=== REGIONS: ==="
credits_line " EUROPE, INDIA, "
credits_line " ISRAEL, MENA,  "
credits_line " NORTH AMERICA  "
credits_line "I hope you enjoy"
credits_line "<3, Hyper"

times 0200h - 2 - ($ - $$)  db 0    ;zerofill up to 510 bytes
dw 0AA55h       ;Boot Sector signature
