%ifndef COUNTRYSELECT_S
%define COUNTRYSELECT_S

%include "globals.s"
%include "countries.s"
%include "input.s"
%include "screen.s"

proc screen_countryselect
%stacksize small
%assign %$localsize 0
%local \
	saved_ax:word, \
    saved_bx:word, \
    saved_cx:word, \
    saved_dx:word

    sub sp, %$localsize
    mov [saved_ax], ax
    mov [saved_bx], bx
    mov [saved_cx], cx
    mov [saved_dx], dx

    ; if enter is pressed, move to game screen and return now
    test word [keys_set], KEYMASK_ENTER
    jz .no_enter_pressed
    .enter_pressed:
        mov word [current_screen], SCREEN_GAMEPLAY
        jmp .end
    .no_enter_pressed:

    ; move selection cursor up/down
    test word [keys_set], KEYMASK_UP
    jz .no_up_pressed
    .up_pressed:
        dec byte [selected_country]
    .no_up_pressed:

    test word [keys_set], KEYMASK_DOWN
    jz .no_down_pressed
    .down_pressed:
        inc byte [selected_country]
    .no_down_pressed:

    and byte [selected_country], 0x1 ; wrap to 2 options

    ; "SELECT YOUR COUNTRY"
    push bp ; why int 0x10 uses bp i'll never know
    mov ax, 0x1300 ; draw string
    mov bx, 0x000f
    mov cx, end_select_country - select_country
    mov dx, 0x050a
    mov bp, select_country
    int 0x10
    pop bp

    ; "USA"
    push bp ; why int 0x10 uses bp i'll never know
    mov ax, 0x1300 ; draw string
    mov bx, 0x000f
    mov cx, end_select_usa - select_usa
    mov dx, 0x0712
    mov bp, select_usa
    int 0x10
    pop bp

    ; "USSR"
    push bp ; why int 0x10 uses bp i'll never know
    mov ax, 0x1300 ; draw string
    mov bx, 0x000f
    mov cx, end_select_ussr - select_ussr
    mov dx, 0x0912
    mov bp, select_ussr
    int 0x10
    pop bp
    
    ; " " over both options to clear
    push bp ; why int 0x10 uses bp i'll never know
    mov ax, 0x1300 ; draw string
    mov bx, 0x000f
    mov cx, end_country_clear - country_clear
    mov dx, 0x070f
    mov bp, country_clear
    int 0x10
    add dh, 2 ; clear second
    int 0x10
    pop bp


    ; ">" on selected option
    push bp ; why int 0x10 uses bp i'll never know
    mov ax, 0x1300 ; draw string
    mov bx, 0x000f
    mov cx, end_country_cursor - country_cursor
    mov dx, 0x070f
    ; 2 rows, so add twice
    add dh, byte [selected_country]
    add dh, byte [selected_country]
    mov bp, country_cursor
    int 0x10
    pop bp

    .end:
    mov [saved_dx], dx
    mov [saved_cx], cx
    mov [saved_bx], bx
    mov [saved_ax], ax
    add sp, %$localsize
endproc


%endif
