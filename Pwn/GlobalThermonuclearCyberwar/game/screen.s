%ifndef SCREEN_S
%define SCREEN_S

%include "globals.s"
%include "countryselect.s"
%include "gameplay.s"

; function pointer table of screens.
; dispatch by the index of the SCREEN_* macros above
screen_dispatch_table:
dw screen_countryselect
dw screen_gameplay

proc do_current_screen
%stacksize small
%assign %$localsize 0
    sub sp, %$localsize

    mov si, word [current_screen]
    shl si, 1
    call word [screen_dispatch_table + si]

    add sp, %$localsize
endproc

%endif
