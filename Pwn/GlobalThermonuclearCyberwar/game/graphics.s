; Graphics routines
%ifndef GRAPHICS_S
%define GRAPHICS_S

%include "globals.s"

; void draw_pixel(uint16_t x, uint16_t y, uint16_t vga_color)
proc draw_pixel
%$x arg
%$y arg
%$color arg
    ; save regs
    push ecx
    push esi
    push edx

    ; dl = vga_color
    mov dx, [bp + %$color]
    ; cx = y
    mov cx, [bp + %$y]
    ; si = x
    mov si, [bp + %$x]

    movsx ecx, cx
    imul ecx, SCREEN_WIDTH
    mov byte [FRAMEBUFFER_BASE + ecx + esi], dl

    ; restore regs
    pop edx
    pop esi
    pop ecx
endproc

proc clear_framebuffer
%stacksize small
%assign %$localsize 0
%local saved_ecx:dword
	sub sp, %$localsize

    mov [saved_ecx], ecx

    mov ecx, FRAMEBUFFER_BASE

    .loop:
        mov dword [ecx], 0
        add ecx, 4
        cmp ecx, FRAMEBUFFER_BASE + (SCREEN_WIDTH * SCREEN_HEIGHT)
        jle .loop

    mov ecx, [saved_ecx]
    add sp, %$localsize
endproc

proc blit_screen
%stacksize small
%assign %$localsize 0
%local \
    saved_ebx:dword, \
    saved_ecx:dword

	sub sp, %$localsize

    mov [saved_ecx], ecx
    mov [saved_ebx], ebx

    mov ecx, 0

    .loop:
        mov ebx,  dword [FRAMEBUFFER_BASE - (DEBUG_FRAMEBUFFER_OFFSET * SCREEN_WIDTH) + ecx]
        mov dword [VMEM_BASE + ecx], ebx
        add ecx, 4
        cmp ecx, SCREEN_WIDTH * SCREEN_HEIGHT
        jle .loop

    mov ebx, [saved_ebx]
    mov ecx, [saved_ecx]
    add sp, %$localsize
endproc

; draw_line(x0, y0, x1, y1, color)
;   dx = x1 - x0
;   dy = y1 - y0
;   D = 2*dy - dx
;   y = y0
; 
;   for x from x0 to x1
;     draw_pixel(x, y, RED)
;     if D > 0
;        y = y + 1
;        D = D - 2*dx
;     end if
;     D = D + 2*dy
proc draw_line
%stacksize small
%assign %$localsize 0
%$x0 arg
%$y0 arg
%$x1 arg
%$y1 arg
%$color arg
%local \
    saved_ax:word, \
    saved_bx:word, \
    octant:word, \
    delta_x:word, \
    delta_y:word, \
    D:word, \
    y:word, \
    tmp:word


	sub sp, %$localsize
    ; save regs
    mov [saved_ax], ax
    mov [saved_bx], bx

    ; dx = x1 - x0
    mov ax, [bp + %$x1]
    sub ax, [bp + %$x0]
    mov [delta_x], ax

    ; dy = y1 - y0
    mov ax, [bp + %$y1]
    sub ax, [bp + %$y0]
    mov [delta_y], ax

    ; get octant
	push word [delta_y]
	push word [delta_x]
	call get_octant
	mov [octant], ax
    add sp, 2 * 2

    ; normalize first coord pair
    push word [bp + %$y0]
    push word [bp + %$x0]
    push word [octant]
    call normalize_to_octant
    add sp, 2 * 3
    mov [bp + %$x0], ax
    mov [bp + %$y0], bx

    ; normalize second coord pair
    push word [bp + %$y1]
    push word [bp + %$x1]
    push word [octant]
    call normalize_to_octant
    add sp, 2 * 3
    mov [bp + %$x1], ax
    mov [bp + %$y1], bx

    ; recompute deltas for normalized values

    ; dx = x1 - x0
    mov ax, [bp + %$x1]
    sub ax, [bp + %$x0]
    mov [delta_x], ax

    ; dy = y1 - y0
    mov ax, [bp + %$y1]
    sub ax, [bp + %$y0]
    mov [delta_y], ax

    ; D = 2*dy - dx
    mov ax, [delta_y]
    shl ax, 1 ; dy * 2
    sub ax, [delta_x] ; - dx
    mov [D], ax

    ; y = y0
    mov ax, [bp + %$y0]
    mov [y], ax


; for x from x0 to x1
    mov ax, [bp + %$x0] ; x = ax
    .loop:
        mov [tmp], ax

        ; denormalize values for drawing
        push word [y]      ; y
        push ax            ; x
        push word [octant] ; octant
        call denormalize_from_octant
        add sp, 2 * 3

        ; draw_pixel(denorm(x), denorm(y), RED)
        push word [bp + %$color] ; color
        push bx       ; y 
        push ax       ; x
        call draw_pixel
        add sp, 2 * 3

        mov ax, [tmp]

        ; if D > 0
        mov bx, [D]
        cmp bx, 0
        jle .end_if
           ; y = y + 1
           inc word [y]
           ; D = D - 2*dx
           sub bx, [delta_x]
           sub bx, [delta_x]
        .end_if:
        ; D = D + 2*dy
        add bx, [delta_y]
        add bx, [delta_y]
        mov [D], bx

        ; check at end of loop
        inc ax
        cmp ax, [bp + %$x1]
        jle .loop

    mov ax, [saved_ax]
    mov bx, [saved_bx]
    add sp, %$localsize
endproc


;short get_octant(short x, short y) {
;    short lut[8] = {0, 1, 3, 2, 7, 6, 4, 5};
;    short flags = 0;
;    short a, b, c;
;    a = y < 0;
;    b = x < 0;
;    c = abs(x) < abs(y);
;
;    if (a) flags |= (1 << 2);
;    if (b) flags |= (1 << 1);
;    if (c) flags |= (1 << 0);
;
;    return lut[flags];
;}
; Find what octant (x, y) lies in, as a number
; Returns into ax (and clobbers the old thing)
proc get_octant
%stacksize small
%assign %$localsize 0
%$x arg
%$y arg
%local \
	flags:word

    sub sp, %$localsize

;    short flags = 0;
    mov word [flags], 0

;    a = y < 0;
;    if (a) flags |= (1 << 2);
    cmp word [bp + %$y], 0
    jge .unset_a
    .set_a:
        or word [flags], 1 << 2
    .unset_a:

;    b = x < 0;
;    if (b) flags |= (1 << 1);
    cmp word [bp + %$x], 0
    jge .unset_b
    .set_b:
        or word [flags], 1 << 1
    .unset_b:

; TODO: We can compute abs here *much* more efficiently!!!
;    x = abs(x)
    cmp word [bp + %$x], 0
    jge .noneg_x
    .neg_x:
        neg word [bp + %$x]
    .noneg_x:
;    y = abs(y)
    cmp word [bp + %$y], 0
    jge .noneg_y
    .neg_y:
        neg word [bp + %$y]
    .noneg_y:
;    c = x < y;
;    if (c) flags |= (1 << 0);
    mov ax, [bp + %$x]
    cmp ax, [bp + %$y]
    jge .unset_c
    .set_c:
        or word [flags], 1
    .unset_c:

;    return lut[flags];
    mov bx, word [flags]
    shl bx, 1
    mov ax, word [octant_lut + bx]
    add sp, %$localsize
endproc

octant_lut:
    dw 0x0000 
    dw 0x0001
    dw 0x0003
    dw 0x0002
    dw 0x0007 
    dw 0x0006
    dw 0x0004
    dw 0x0005

; (short, short) normalize_to_octant(octant, x, y) 
;   switch(octant)  
;     case 0: return (x, y)
;     case 1: return (y, x)
;     case 2: return (y, -x)
;     case 3: return (-x, y)
;     case 4: return (-x, -y)
;     case 5: return (-y, -x)
;     case 6: return (-y, x)
;     case 7: return (x, -y)
; returns norm(x) in ax, norm(y) in bx
proc normalize_to_octant
%stacksize small
%assign %$localsize 0
%$octant arg
%$x arg
%$y arg
%local \
    saved_si:word

	sub sp, %$localsize
    mov [saved_si], si

    ; load up ax and bx
    mov ax, [bp + %$x]
    mov bx, [bp + %$y]

    ; jump table because fuck 'em
	mov si, word [bp + %$octant]
    shl si, 1
	jmp [si + .jump_table]

	.jump_table:
		dw .octant_0
		dw .octant_1
		dw .octant_2
		dw .octant_3
		dw .octant_4
		dw .octant_5
		dw .octant_6
		dw .octant_7

  	; case 0: return (x, y)
	.octant_0:
		jmp .ret
    ; case 1: return (y, x)
	.octant_1:
		xchg ax, bx
		jmp .ret
    ; case 2: return (y, -x)
	.octant_2:
        neg ax
		xchg ax, bx
		jmp .ret
    ; case 3: return (-x, y)
	.octant_3:
        neg ax
		jmp .ret
    ; case 4: return (-x, -y)
	.octant_4:
        neg ax
        neg bx
		jmp .ret
    ; case 5: return (-y, -x)
	.octant_5:
        neg ax
        neg bx
        xchg ax, bx
		jmp .ret
    ; case 6: return (-y, x)
	.octant_6:
        neg bx
        xchg ax, bx
		jmp .ret
    ; case 7: return (x, -y)
	.octant_7:
        neg bx
		jmp .ret

	.ret:
    mov si, [saved_si]
	add sp, %$localsize
endproc

; (short, short) denormalize_from_octant(octant, x, y) 
;   switch(octant)  
;     case 0: return (x, y)
;     case 1: return (y, x)
;     case 2: return (-y, x)
;     case 3: return (-x, y)
;     case 4: return (-x, -y)
;     case 5: return (-y, -x)
;     case 6: return (y, -x)
;     case 7: return (x, -y)
; returns denorm(x) in ax, denorm(y) in bx
proc denormalize_from_octant
%stacksize small
%assign %$localsize 0
%$octant arg
%$x arg
%$y arg
%local \
    saved_si:word

	sub sp, %$localsize
    mov [saved_si], si

    ; load up ax and bx
    mov ax, [bp + %$x]
    mov bx, [bp + %$y]

    ; jump table because fuck 'em
	mov si, word [bp + %$octant]
    shl si, 1
	jmp [si + .jump_table]

	.jump_table:
		dw .octant_0
		dw .octant_1
		dw .octant_2
		dw .octant_3
		dw .octant_4
		dw .octant_5
		dw .octant_6
		dw .octant_7

  	; case 0: return (x, y)
	.octant_0:
		jmp .ret
    ; case 1: return (y, x)
	.octant_1:
		xchg ax, bx
		jmp .ret
    ; case 2: return (-y, x)
	.octant_2:
        neg bx
        xchg ax, bx
		jmp .ret
    ; case 3: return (-x, y)
	.octant_3:
        neg ax
		jmp .ret
    ; case 4: return (-x, -y)
	.octant_4:
        neg ax
        neg bx
		jmp .ret
    ; case 5: return (-y, -x)
	.octant_5:
        neg ax
        neg bx
        xchg ax, bx
		jmp .ret
    ; case 6: return (y, -x)
	.octant_6:
        neg ax
		xchg ax, bx
		jmp .ret
    ; case 7: return (x, -y)
	.octant_7:
        neg bx
		jmp .ret

	.ret:
    mov si, [saved_si]
	add sp, %$localsize
endproc

; draw_filled_circle(uint16_t x, uint16_t y, uint16_t r, uint16_t color)
proc draw_filled_circle
%stacksize small
%assign %$localsize 0
%$x     arg
%$y     arg
%$r     arg
%$color arg
%local \
    saved_ax:word, \
    saved_bx:word, \
    x_i:word, \
    y_i:word, \
    delta_x:word, \
    delta_y:word, \
    err:word

    sub sp, %$localsize
    mov [saved_ax], ax
    mov [saved_bx], bx

    ; x_i = r - 1
    mov ax, [bp + %$r]
    dec ax
    mov [x_i], ax
    ; y_i = 0 
    mov word [y_i], 0
    ; delta_x = 1
    mov word [delta_x], 1
    ; delta_y = 1
    mov word [delta_y], 1
    ; err = dx - (radius << 1)
    mov ax, [delta_x]
    sub ax, [bp + %$r]
    sub ax, [bp + %$r]
    mov [err], ax

    ; while (x_i >= y_i)
    .loop:
        mov bx, [bp + %$y]
        add bx, [y_i]
        ; putpixel(x0 + x_i, y0 + y_i);
        ; putpixel(x0 - x_i, y0 + y_i);
            mov ax, [bp + %$x]
            add ax, [x_i]
            mov cx, [bp + %$x]
            sub cx, [x_i]
            push_args ax, bx, cx, bx, word [bp + %$color]
            call draw_line
            add sp, 2*5

        mov bx, [bp + %$y]
        add bx, [x_i]
        ; putpixel(x0 + y_i, y0 + x_i);
        ; putpixel(x0 - y_i, y0 + x_i);
            mov ax, [bp + %$x]
            add ax, [y_i]
            mov cx, [bp + %$x]
            sub cx, [y_i]
            push_args ax, bx, cx, bx, word [bp + %$color]
            call draw_line
            add sp, 2*5

        mov bx, [bp + %$y]
        sub bx, [y_i]
        ; putpixel(x0 - x_i, y0 - y_i);
        ; putpixel(x0 + x_i, y0 - y_i);
            mov ax, [bp + %$x]
            sub ax, [x_i]
            mov cx, [bp + %$x]
            add cx, [x_i]
            push_args ax, bx, cx, bx, word [bp + %$color]
            call draw_line
            add sp, 2*5

        mov bx, [bp + %$y]
        sub bx, [x_i]
        ; putpixel(x0 - y_i, y0 - x_i);
        ; putpixel(x0 + y_i, y0 - x_i);
            mov ax, [bp + %$x]
            sub ax, [y_i]
            mov cx, [bp + %$x]
            add cx, [y_i]
            push_args ax, bx, cx, bx, word [bp + %$color]
            call draw_line
            add sp, 2*5


        cmp word [err], 0
        jg .err_gt_zero_first
        .err_lte_zero_first:
            ; y_i++;
            inc word [y_i]
            ; err += delta_y;
            mov ax, [delta_y]
            add [err], ax
            ; delta_y += 2;
            add word [delta_y], 2
        .err_gt_zero_first:

        cmp word [err], 0
        jle .err_lte_zero_second
        .err_gt_zero_second:
            ; x_i--;
            dec word [x_i]
            ; delta_x += 2;
            add word [delta_x], 2
            ; err += (-radius << 1) + delta_x;
            xor ax, ax
            sub ax, [bp + %$r]
            sub ax, [bp + %$r]
            add ax, [delta_x]
            add [err], ax
        .err_lte_zero_second:

        mov ax, [x_i]
        cmp ax, word [y_i]
        jge .loop

    mov bx, [saved_bx]
    mov ax, [saved_ax]
    add sp, %$localsize
endproc

; uint16_t interpolate(uint16_t lo, uint16_t hi, uint16_t t)
; interpolate between lo and hi by t / 256
proc interpolate
%stacksize small
%assign %$localsize 0
%$lo arg
%$hi arg
%$t arg
%local \
    saved_bx:word

    %assign LERP_POWER_OF_2 4

    sub sp, %$localsize
    mov [saved_bx], bx

    ; ax = (256 - t) * lo
    mov ax, 1 << LERP_POWER_OF_2
    sub ax, [bp + %$t]
    imul ax, [bp + %$lo]

    ; ax /= 256
    sar ax, LERP_POWER_OF_2

    ; bx = t * hi
    mov bx, [bp + %$t]
    imul bx, word [bp + %$hi]

    ; bx /= 256
    sar bx, LERP_POWER_OF_2

    ; ax += bx
    add ax, bx

    mov bx, [saved_bx]
    add sp, %$localsize
endproc


; void draw_bezier(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color, uint16_t start_sweep, uint16_t end_sweep)
; Draw a 2nd order bezier curve with control points (x0, y0), (x1, y1), (x2, y2)
;    for (t = start_sweep; t <= end_sweep; t++) {
;        ix0, iy0 = interp_pt(x0, y0, x1, y1, t)
;        ix1, iy1 = interp_pt(x1, y1, x2, y2, t)
;        ox0, ox1 = interp_pt(ix0, iy0, ix1, iy1, t)
;        plot(ox0, ox1)
;    }
proc draw_bezier
%stacksize small
%assign %$localsize 0
%$x0 arg
%$y0 arg
%$x1 arg
%$y1 arg
%$x2 arg
%$y2 arg
%$color arg
%$start_sweep arg
%$end_sweep arg
%local \
    saved_ax:word, \
    saved_bx:word, \
    saved_cx:word, \
    interp_count:word, \
    ix0:word, \
    iy0:word, \
    ix1:word, \
    iy1:word, \
    ox:word, \
    oy:word, \
    px:word, \
    py:word

    sub sp, %$localsize
    mov [saved_ax], ax
    mov [saved_bx], bx
    mov [saved_cx], cx

    mov ax, [bp + %$x0]
    mov [px], ax
    mov ax, [bp + %$y0]
    mov [py], ax

    ; XXX: We could unroll here and save cx!
    mov cx, [bp + %$start_sweep]
    .loop:
        ; ix0 = interpolate(x0, x1, t)
        push_args word [bp + %$x0], word [bp + %$x1], cx
        call interpolate
        add sp, 2*3
        mov [ix0], ax

        ; ix1 = interpolate(x1, x2, t)
        push_args word [bp + %$x1], word [bp + %$x2], cx
        call interpolate
        add sp, 2*3
        mov [ix1], ax

        ; iy0 = interpolate(y0, y1, t)
        push_args word [bp + %$y0], word [bp + %$y1], cx
        call interpolate
        add sp, 2*3
        mov [iy0], ax

        ; iy1 = interpolate(y1, y2, t)
        push_args word [bp + %$y1], word [bp + %$y2], cx
        call interpolate
        add sp, 2*3
        mov [iy1], ax

        ; ox = interpolate(ix0, ix1, t)
        push_args word [ix0], word [ix1], cx
        call interpolate
        add sp, 2*3
        mov [ox], ax

        ; oy = interpolate(iy0, iy1, t)
        push_args word [iy0], word [iy1], cx
        call interpolate
        add sp, 2*3
        mov [oy], ax

        ; draw line between current and previous points
        push_args word [px], word [py], word [ox], word [oy], word [bp + %$color]
        call draw_line
        add sp, 2*5
        
        ; update px, py
        mov ax, [ox]
        mov [px], ax
        mov ax, [oy]
        mov [py], ax

        ; t++
        inc cx
        cmp cx, [bp + %$end_sweep]
        jle .loop

    mov cx, [saved_cx]
    mov bx, [saved_bx]
    mov ax, [saved_ax]
    add sp, %$localsize
endproc

%endif
