%ifndef INPUT_S
%define INPUT_S

%include "globals.s"

; Masks on keys_set for keys we care about
%define KEYMASK_UP    1 << 0
%define KEYMASK_DOWN  1 << 1
%define KEYMASK_LEFT  1 << 2
%define KEYMASK_RIGHT 1 << 3
%define KEYMASK_ENTER 1 << 4
%define KEYMASK_Q     1 << 5
%define KEYMASK_A     1 << 6

; Scancodes we care about
%define SCANCODE_UP    0x48
%define SCANCODE_DOWN  0x50
%define SCANCODE_LEFT  0x4B
%define SCANCODE_RIGHT 0x4D
%define SCANCODE_ENTER 0x1C
%define SCANCODE_Q     0x10 
%define SCANCODE_A     0x1E

; Read until keyboard buffer is empty, setting bits in keys_set appropriately
; Clears the keys that were set the previous time this function was called!
proc get_keys
%stacksize small
%assign %$localsize 0
%local saved_ax:word
	sub sp, %$localsize
    mov [saved_ax], ax

    ; clear keys set
    mov word [keys_set], 0

    .read_loop:
        ; check if a key is ready
        mov ah, 0x1
        int 0x16
        jz .no_more_input ; no key available, ret

        ; pop key off the input buffer
        mov ah, 0x0
        int 0x16
        ; fallthru to .got_key, ah = scan code
    .got_key:
        cmp ah, SCANCODE_UP
        je .set_up
        cmp ah, SCANCODE_DOWN
        je .set_down
        cmp ah, SCANCODE_LEFT
        je .set_left
        cmp ah, SCANCODE_RIGHT
        je .set_right
        cmp ah, SCANCODE_ENTER
        je .set_enter
        cmp ah, SCANCODE_Q
        je .set_q
        cmp ah, SCANCODE_A
        je .set_a
        jmp .read_loop ; didn't match any, just get next key

        ; set bits
        .set_up:
            or word [keys_set], KEYMASK_UP
            jmp .read_loop
        .set_down:
            or word [keys_set], KEYMASK_DOWN
            jmp .read_loop
        .set_left:
            or word [keys_set], KEYMASK_LEFT
            jmp .read_loop
        .set_right:
            or word [keys_set], KEYMASK_RIGHT
            jmp .read_loop
        .set_enter:
            or word [keys_set], KEYMASK_ENTER
            jmp .read_loop
        .set_q:
            or word [keys_set], KEYMASK_Q
            jmp .read_loop
        .set_a:
            or word [keys_set], KEYMASK_A
            jmp .read_loop

    .no_more_input: ; return when input exhausted

    mov ax, [saved_ax]
    add sp, %$localsize
endproc

%endif
