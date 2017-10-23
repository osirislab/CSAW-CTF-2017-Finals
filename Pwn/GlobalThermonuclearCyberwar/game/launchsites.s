%ifndef LAUNCHSITES_S
%define LAUNCHSITES_S
%include "countries.s"
%assign n_launchsites 0
; launchsite country_code, x, y
%macro launchsite 3
    dw 0x0 ; padding, used to be a pointer to the name
    dw %1 ; +2 country_id
    dw %2 ; +4 x
    dw %3 ; +6 y
    %assign n_launchsites n_launchsites + 1
%endmacro


launchsites:
; us:
launchsite COUNTRY_AMERICA, 70, 77
launchsite COUNTRY_AMERICA, 73, 84
launchsite COUNTRY_AMERICA, 65, 75
launchsite COUNTRY_AMERICA, 60, 79
; ussr:
launchsite COUNTRY_USSR, 186, 74
launchsite COUNTRY_USSR, 202, 68
launchsite COUNTRY_USSR, 158, 76
launchsite COUNTRY_USSR, 215, 77
end_launchsites:



%endif
