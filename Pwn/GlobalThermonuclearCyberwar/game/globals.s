; Global variables, etc
%ifndef GLOBALS_S
%define GLOBALS_S

%include "c16.mac"

; Dispatch macro to find sizes of registers (in bytes)... very hacky but w/e
%define sizeof(reg) __sizeof__ %+ reg
; 2-byte registers
%assign __sizeof__ax 2
%assign __sizeof__bx 2
%assign __sizeof__cx 2
%assign __sizeof__dx 2
%assign __sizeof__si 2
%assign __sizeof__di 2
%assign __sizeof__bp 2
%assign __sizeof__sp 2

%macro push_args 1-* 
  %rep  %0 
  %rotate -1 
        push %1 
  %endrep 
%endmacro

selected_country: db 0

; mask to get what keys are pressed. See input.s for the mask constants
keys_set: dw 0x0

; screen state. see screens.s for possible values
current_screen: dw 0x0 ; 0 = SCREEN_MENU

; game phase. check against the enum PHASE_* values in game/gameplay.s
game_phase: dw 0x0 ; 0 = PHASE_SELECTLAUNCHSITE

%define FRAMEBUFFER_BASE 0x10000
%define VMEM_BASE 0xA0000
%define DEBUG_FRAMEBUFFER_OFFSET 0 ; set this to 100 to see the stack :)
%define SCREEN_WIDTH 320
%define SCREEN_HEIGHT 200

; screen selector constants
%define SCREEN_MENU 0
%define SCREEN_GAMEPLAY 1

; targetting
target_x: dw 160
target_y: dw 100
target_strength: dw 0xc

; RNG state
rng_state: dw 0x4c

; demo interval timer
demo_timer: dw 100

; selected launch site index
selected_launch_site: dw 0x1

%define TICKS_BETWEEN_MOVES 30


%define MAX_MISSLES 0x40
; missiles in flight
; we _could_ use nasm's struct abstraction here, but i'm lazy
missile_slots:
%assign i 0
%rep MAX_MISSLES

missile_slot_ %+ i:
; these initial values are basically all junk. just want to make the hexdump
; slightly less boring...
.launch_x: dw 0x6546         ; +0
.launch_y: dw 0x3124         ; +2
.target_x: dw 0xcfff         ; +4
.target_y: dw 0xf3ac         ; +6
.end_sweep: dw 0x234c        ; +8
.ticks_until_move: dw 0x41ac ; +10
.in_use: db 0x0              ; +12
.country: db 0x12            ; +13
.color: dw 0x4334            ; +14

%assign i i+1
%endrep
end_missile_slots:


%define EXPLOSION_TICKS 100

%define MAX_EXPLOSIONS MAX_MISSLES
; Explosions currently goin'
explosion_slots:
%assign i 0
%rep MAX_EXPLOSIONS
explosion_slot_ %+ i:
; these initial values are basically all junk. just want to make the hexdump
; slightly less boring...
.explosion_x: dw 0xc23        ; +0
.explosion_y: dw 0x3943       ; +2
.ticks_until_death: dw 0x3431 ; +4
.in_use: db 0x0               ; +6
.pad: db 0xcc                 ; +7

%assign i i+1
%endrep
end_explosion_slots:


; ==== STRINGS ====

select_country: db "SELECT YOUR COUNTRY:"
end_select_country:

select_usa: db "USA"
end_select_usa:

select_ussr: db "USSR"
end_select_ussr:

country_cursor: db ">"
end_country_cursor:

country_clear: db " "
end_country_clear:

; basically just a checkpoint at finding the password
ez_flag: db "flag{ok_you_decrypted_it_now_plz_pwn_it!}"
end_ez_flag:

; the binary they get will have this scrubbed
pwn_flag: db "flag{c4n_4ny0n3_really_w1n_1n_cyb3rw4r?}"
end_pwn_flag:

%endif
