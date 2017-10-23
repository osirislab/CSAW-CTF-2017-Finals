%ifndef GAMEPLAY_S
%define GAMEPLAY_S

%include "worldmap.s"
%include "launchsites.s"

%define PHASE_SELECTLAUNCHSITE 0
%define PHASE_SELECTTARGET     1
%define PHASE_ENEMYMOVE        2
%define PHASE_DEMO             3

proc screen_gameplay
%stacksize small
%assign %$localsize 0
%local \
	saved_ax:word, \
    saved_bx:word, \
    saved_cx:word

    sub sp, %$localsize
    mov [saved_ax], ax
    mov [saved_bx], bx
    mov [saved_cx], cx

    call draw_worldmap
    call draw_launchsites
    call draw_selected_launchsite

    cmp word [game_phase], PHASE_DEMO
    je .demo_phase

    .not_demo_phase:
        cmp word [game_phase], PHASE_SELECTTARGET
        jne .no_move_target
        .move_target:
            call move_target
            call draw_target
        .no_move_target:
        
        cmp word [game_phase], PHASE_SELECTLAUNCHSITE
        jne .no_select_launchsite
        .select_launchsite:
            call select_launchsite
        .no_select_launchsite:

        cmp word [game_phase], PHASE_ENEMYMOVE
        jne .no_enemy_move
        .enemy_move:
            xor ax, ax
            mov al, byte [selected_country]
            xor al, 1
            push_args ax
            call do_ai_move
            add sp, 2*1
            ; until ^, just go back to selecting launch site
            mov word [game_phase], PHASE_SELECTLAUNCHSITE
        .no_enemy_move:
        jmp .render_everything

    .demo_phase:
        dec word [demo_timer]
        cmp word [demo_timer], 0
        jne .render_everything
        .add_demo_missile:
            push_args word [selected_country]
            call do_ai_move
            add sp, 2*1
            xor word [selected_country], 1
            mov word [demo_timer], 10

    .render_everything:
    mov si, missile_slots
    .missile_loop:
        cmp byte [si + 12], 0 ; skip if not in_use
        je .skip_missile

        ; render missile
        push_args word [si + 0], word [si + 2], \
                  word [si + 4], word [si + 6], \
                  word [si + 14], word [si + 8]
        call draw_trajectory
        add sp, 2*6

        ; advance ticks_until_move
        dec word [si + 10]
        cmp word [si + 10], 0
        jne .no_reset_ticks
        .reset_ticks:
            mov word [si + 10], TICKS_BETWEEN_MOVES
            ; increment end_sweep, unless we're at max
            cmp word [si + 8], 0xf
            jg .no_inc_sweep
            .inc_sweep:
                inc word [si + 8]
                jmp .after_sweep
            .no_inc_sweep:
                push_args word [si + 4], word [si + 6]
                call create_explosion
                add sp, 2*2

                mov byte [si + 12], 0 ; missile.in_use = false
            .after_sweep:
        .no_reset_ticks:
        
        .skip_missile:
            add si, 16
            cmp si, end_missile_slots
            jl .missile_loop
    .after_missile_loop:

    mov si, explosion_slots
    .explosion_loop:
        cmp byte [si + 6], 0 ; skip if not in_use
        je .skip_explosion

        ; render explosion
        mov ax, [si + 4]

        ; ax = EXPLOSION_TICKS - ax
        ; -> (ax = - (ax - EXPLOSION_TICKS))
        sub ax, EXPLOSION_TICKS
        neg ax

        shr ax, 4 ; divide by a bunch to keep sizes under control

        push_args word [si + 0], word [si + 2], ax, 0xf
        call draw_filled_circle
        add sp, 2*4

        ; advance ticks_until_death
        dec word [si + 4]
        cmp word [si + 4], 0
        jne .no_remove_explosion
        .remove_explosion:
            mov byte [si + 6], 0 ; explosion.in_use = false
        .no_remove_explosion:

        .skip_explosion:
            add si, 8
            cmp si, end_explosion_slots
            jl .explosion_loop
        
        
    .end:

    call blit_screen

    mov cx, [saved_cx]
    mov bx, [saved_bx]
    mov ax, [saved_ax]
    add sp, %$localsize
endproc

; draw_trajectory(start_x, start_y, end_x, end_y, color, end_sweep)
proc draw_trajectory
%stacksize small
%assign %$localsize 0
%$start_x arg
%$start_y arg
%$end_x arg
%$end_y arg
%$color arg
%$end_sweep arg
%local \
    saved_ax:word, \
    mid_x:word, \
    mid_y:word

    sub sp, %$localsize
    mov [saved_ax], ax

    ; mid_x = interpolate(start_x, end_x, 0xb)
    push_args word [bp + %$start_x], word [bp + %$end_x], 0xb
    call interpolate
    add sp, 2*3
    mov [mid_x], ax

    ; mid_y = min(start_y, end_y) - 100
    mov ax, [bp + %$start_y]
    cmp ax, [bp + %$end_y]
    jl .no_min
    .min:
        mov ax, [bp + %$end_y]
    .no_min:

    sub ax, 100
    mov [mid_y], ax

    push_args word [bp + %$start_x], word [bp + %$start_y], \
              word [mid_x], word [mid_y], \
              word [bp + %$end_x], word [bp + %$end_y], \
              word [bp + %$color], \
              0, word [bp + %$end_sweep]
    call draw_bezier
    add sp, 2*9

    mov ax, [saved_ax]
    add sp, %$localsize
endproc

; move the target around based on arrow keys
; also chane intensity of shot (q/a)
proc move_target
%stacksize small
%assign %$localsize 0
%local \
    saved_ax:word, \
    saved_si:word, \
    saved_di:word

    sub sp, %$localsize
    mov [saved_ax], ax
    mov [saved_si], si

    test word [keys_set], KEYMASK_UP
    jz .no_up
    .up:
        cmp word [target_y], 0
        jle .no_up ; ignore if we're at 0
        dec word [target_y]
    .no_up:

    test word [keys_set], KEYMASK_DOWN
    jz .no_down
    .down:
        cmp word [target_y], SCREEN_HEIGHT
        jge .no_down ; ignore if we're at the top
        inc word [target_y]
    .no_down:

    test word [keys_set], KEYMASK_LEFT
    jz .no_left
    .left:
        cmp word [target_x], 0
        jle .no_left ; ignore if we're at the left
        dec word [target_x]
    .no_left:

    test word [keys_set], KEYMASK_RIGHT
    jz .no_right
    .right:
        cmp word [target_x], SCREEN_WIDTH
        jge .no_right ; ignore if we're at the right
        inc word [target_x]
    .no_right:

    test word [keys_set], KEYMASK_Q
    jz .no_q
    .q:
        inc word [target_strength]
        and word [target_strength], 0xff
    .no_q:

    test word [keys_set], KEYMASK_A
    jz .no_a
    .a:
        dec word [target_strength]
        and word [target_strength], 0xff
    .no_a:

    test word [keys_set], KEYMASK_ENTER
    jz .no_enter
    .enter:
        ; when enter is pressed assign a missile slot to this missle,
        ; and let the AI move
        call get_available_missile_slot
        ; just bail out (as if nothing was pressed) if no slot is
        cmp ax, -1
        je .no_missile_slot

        mov si, ax
        shl si, 4
        add si, missile_slots

        mov di, [selected_launch_site]
        shl di, 3
        add di, launchsites
        .fill_missile_slot:
            mov ax, [di + 4]
            mov word [si + 0], ax                   ; launch_x
            mov ax, [di + 6]
            mov word [si + 2], ax                   ; launch_y
            mov ax, [target_x]
            mov word [si + 4], ax                   ; target_x
            mov ax, [target_y]
            mov word [si + 6], ax                   ; target_y
            mov word [si + 8], 0x0                  ; end_sweep
            mov word [si + 10], TICKS_BETWEEN_MOVES ; ticks_until_move
            mov byte [si + 12], 1                   ; in_use
            mov al, [selected_country]
            mov byte [si + 13], al                  ; country
            mov ax, [target_strength]
            mov byte [si + 14], al                  ; yield
        .no_missile_slot:
            mov word [game_phase], PHASE_ENEMYMOVE
    .no_enter:

    mov di, [saved_di]
    mov si, [saved_si]
    mov ax, [saved_ax]
    add sp, %$localsize
endproc

; switch selected launchsite with left/right
; select launchsite with return
proc select_launchsite
%stacksize small
    test word [keys_set], KEYMASK_LEFT
    jz .no_left
    .left:
        dec word [selected_launch_site]
    .no_left:

    test word [keys_set], KEYMASK_RIGHT
    jz .no_right
    .right:
        inc word [selected_launch_site]
    .no_right:

    ; keep bounds in range to selected_country
    and word [selected_launch_site], 0b11

    cmp byte [selected_country], COUNTRY_AMERICA
    je .country_america
    .country_ussr:
        add word [selected_launch_site], 4
    .country_america:

    test word [keys_set], KEYMASK_ENTER
    jz .no_enter
    .enter:
        mov word [game_phase], PHASE_SELECTTARGET
    .no_enter:
endproc

; create an explosion in a free explosion slot
; does nothing if none available
; create_explosion(uint16_t x, uint16_t y)
proc create_explosion
%stacksize small
%assign %$localsize 0
%$x arg
%$y arg
%local \
    saved_ax:word, \
    saved_si:word
    
    sub sp, %$localsize
    mov [saved_ax], ax
    mov [saved_si], si

    mov si, explosion_slots
    .loop:
        cmp byte [si + 6], 0 ; skip if in_use
        jne .next

        ; we found one. set it up, then we're done
        mov ax, [bp + %$x]
        mov word [si + 0], ax ; x
        mov ax, [bp + %$y]
        mov word [si + 2], ax ; y
        mov word [si + 4], EXPLOSION_TICKS ; ticks
        mov byte [si + 6], 1 ; in_use
        jmp .end

        .next:
            add si, 8
            cmp si, end_explosion_slots
            jle .loop


    .end:
    mov si, [saved_si]
    mov ax, [saved_ax]
    add sp, %$localsize
endproc

; find a missile slot that is available, or return -1 if none
proc get_available_missile_slot
%stacksize small
%assign %$localsize 0
%local \
    saved_si:word

    sub sp, %$localsize
    mov [saved_si], si

    mov ax, 0
    .loop:
        ; if in_use == 0, we found an unused slot, end
        mov si, ax
        shl si, 4
        cmp byte [missile_slots + si + 12], 0
        je .end

        inc ax
        cmp ax, MAX_MISSLES
        jl .loop
        ; fallthru to none_found

    .none_found: ; if we don't find any, return -1
        mov ax, -1

    .end:
    mov si, [saved_si]
    add sp, %$localsize
endproc

proc draw_target
%stacksize small
%assign %$localsize 0
%local \
    saved_ax:word, \
    saved_bx:word, \
    x:word, \
    y:word

    sub sp, %$localsize
    mov [saved_ax], ax
    mov [saved_bx], bx

    ; Target looks like this:
    ;  +
    ; + +
    ;  +

    mov ax, [target_x]
    mov bx, [target_y]
    ;     
    ; +    
    ;     
    dec ax
    push_args ax, bx, word [target_strength]
    call draw_pixel
    add sp, 2*3

    ;     
    ; +
    ;  +   
    inc ax
    inc bx
    push_args ax, bx, word [target_strength]
    call draw_pixel
    add sp, 2*3

    ;     
    ; + +
    ;  +   
    inc ax
    dec bx
    push_args ax, bx, word [target_strength]
    call draw_pixel
    add sp, 2*3

    ;  +
    ; + +
    ;  +   
    dec ax
    dec bx
    push_args ax, bx, word [target_strength]
    call draw_pixel
    add sp, 2*3

    mov bx, [saved_bx]
    mov ax, [saved_ax]
    add sp, %$localsize
endproc

proc draw_launchsites
%stacksize small
%assign %$localsize 0
%local \
    saved_ax:word, \
    saved_bx:word, \
    saved_cx:word, \
    saved_si:word

    sub sp, %$localsize
    mov [saved_ax], ax
    mov [saved_bx], bx
    mov [saved_cx], cx
    mov [saved_si], si

    mov si, launchsites
    .loop:
        mov ax, [si + 4]
        mov bx, [si + 6]
        mov cx, 3
        add cx, [si + 2]

        ; +
        ;  
        ;
        dec ax
        dec bx
        push_args ax, bx, cx
        call draw_pixel
        add sp, 2*3

        ; +
        ;  
        ;  +
        inc ax
        inc bx
        inc bx
        push_args ax, bx, cx
        call draw_pixel
        add sp, 2*3

        ; + +
        ;    
        ;  +
        inc ax
        dec bx
        dec bx
        push_args ax, bx, cx
        call draw_pixel
        add sp, 2*3

        add si, 8
        cmp si, end_launchsites
        jl .loop

    mov si, [saved_si]
    mov cx, [saved_cx]
    mov bx, [saved_bx]
    mov ax, [saved_ax]
    add sp, %$localsize
endproc

proc draw_selected_launchsite
%stacksize small
%assign %$localsize 0
%local \
    saved_ax:word, \
    saved_bx:word, \
    saved_si:word
    sub sp, %$localsize
    mov [saved_ax], ax
    mov [saved_bx], bx
    mov [saved_si], si

    mov si, [selected_launch_site]
    shl si, 3
    add si, launchsites

    mov ax, [si + 4]
    mov bx, [si + 6]

    ;   
    ;
    ; +
    dec ax
    push_args ax, bx, 0x0c
    call draw_pixel
    add sp, 2*3

    ;   
    ;
    ; + +
    inc ax
    inc ax
    push_args ax, bx, 0x0c
    call draw_pixel
    add sp, 2*3

    ;  +
    ;
    ; + +
    dec ax
    dec bx
    dec bx
    push_args ax, bx, 0x0c
    call draw_pixel
    add sp, 2*3


    mov si, [saved_si]
    mov bx, [saved_bx]
    mov ax, [saved_ax]
    add sp, %$localsize
endproc

proc draw_worldmap
%stacksize small
%assign %$localsize 0
%local \
    saved_ax:word, \
    saved_bx:word, \
    saved_cx:word, \
    saved_si:word

    sub sp, %$localsize
    mov [saved_ax], ax
    mov [saved_bx], bx
    mov [saved_cx], cx
    mov [saved_si], si

    xor si, si
    xor bx, bx
    .loop:
        mov ax, [worldmap + si + 0 * 2]
        mov bl, [worldmap + si + 1 * 2]

        push_args ax, bx, 0x1f
        call draw_pixel
        add sp, 6

        ; label for self-modifying code so we can nop this out
        .map_render_pixel_sleep_amount_begin:
        ; sleep a bit
        push ax
        mov cx, 0x0000
        mov dx, 0x04ff
        mov ah, 0x86
        int 0x15
        call blit_screen
        pop ax
        ; end label for self-modifying code
        .map_render_pixel_sleep_amount_end:

        add si, 2+1
        cmp si, (n_worldmap_pixels-1) * (2+1)
        jle .loop

    ; nop out the between-pixel sleep
    .nop_out_area_begin:
    mov si, .map_render_pixel_sleep_amount_begin
    .nop_out_pixel_sleep:
        mov byte [si], 0x90
        inc si
        cmp si, .map_render_pixel_sleep_amount_end
        jne .nop_out_pixel_sleep
    ; nop out the nop-outer, including this nop-outer nop-outer :)
    ; I don't know why i made this but I think it's beautiful
    mov cx, .nop_out_area_end - .nop_out_area_begin
    mov al, 0x90
    mov di, .nop_out_area_begin
    rep stosb
    .nop_out_area_end:

    mov ax, [saved_ax]
    mov bx, [saved_bx]
    mov cx, [saved_cx]
    mov si, [saved_si]
    add sp, %$localsize
endproc

; do_ai_move(uint16_t country)
; make a move as the ai as `country`
proc do_ai_move
%stacksize small
%assign %$localsize 0
%$country arg
%local \
    saved_ax:word, \
    saved_bx:word, \
    saved_cx:word, \
    saved_si:word, \
    target_x:word, \
    target_y:word

    sub sp, %$localsize
    mov [saved_ax], ax
    mov [saved_bx], bx
    mov [saved_cx], bx
    mov [saved_si], si

    ; bx = lower bound for target x
    ; cx = upper bound for target x
    mov bx, SCREEN_WIDTH / 2
    mov cx, SCREEN_WIDTH - 30

    ; if we're looking at america, target the right half of the globe
    cmp word [bp + %$country], COUNTRY_USSR
    jne .is_ussr
    .is_america:
        mov bx, 30
        mov cx, 90
    .is_ussr:

    .get_x_rng:
        call rng_next
        ; if we're out of range, just regen
        ; this is kinda lame but i'm kinda too lazy to do modulo
        cmp bx, ax
        jg .get_x_rng
        cmp ax, cx
        jg .get_x_rng
    mov [target_x], ax

    .get_y_rng:
        call rng_next
        ; if we're out of range, just regen
        ; this is kinda lame but i'm kinda too lazy to do modulo
        cmp ax, 60 ; dont' want ai accidentally triggering the bugs ;)
        jl .get_y_rng
        cmp ax, 100
        jg .get_y_rng
    mov [target_y], ax

    ; when enter is pressed assign a missile slot to this missle,
    ; and let the AI move
    call get_available_missile_slot
    ; just bail out (as if nothing was pressed) if no slot is
    cmp ax, -1
    je .no_missile_slot

    mov si, ax
    shl si, 4
    add si, missile_slots

    ; pick a random launch site for the right team
    call rng_next
    and ax, 0b11
    ; check what we're playing as, jmp to the other
    cmp word [bp + %$country], COUNTRY_AMERICA
    je .site_america
    .sites_ussr:
        add ax, 4
    .site_america:

    mov di, ax
    shl di, 3
    add di, launchsites
    .fill_missile_slot:
        mov ax, [di + 4]
        mov word [si + 0], ax                   ; launch_x
        mov ax, [di + 6]
        mov word [si + 2], ax                   ; launch_y
        mov ax, [target_x]
        mov word [si + 4], ax                   ; target_x
        mov ax, [target_y]
        mov word [si + 6], ax                   ; target_y
        mov word [si + 8], 0x0                  ; end_sweep
        mov word [si + 10], TICKS_BETWEEN_MOVES ; ticks_until_move
        mov byte [si + 12], 1                   ; in_use
        mov al, [bp + %$country]
        mov byte [si + 13], al                  ; country
        mov ax, [target_x]
        add ax, [target_y]
        mov byte [si + 14], al                  ; yield
    .no_missile_slot:

    mov [saved_si], si
    mov [saved_cx], cx
    mov [saved_bx], bx
    mov [saved_ax], ax
    add sp, %$localsize
endproc

; really crappy LCG-based rng
proc rng_next
%stacksize small
    mov ax, [rng_state]
    imul ax, 0x2331
    add ax, 12345
    mov [rng_state], ax
endproc


%endif
