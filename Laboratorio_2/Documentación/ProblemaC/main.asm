
.include "m328pdef.inc"

.org 0x0000
rjmp start

start:
	ldi r16,	LOW(RAMEND)
	out SPL,	r16
	ldi r16,	HIGH(RAMEND)
	out SPH,	r16

loop_menu:

    rcall chequear_UART
    
    cpi r16, '1'
    breq figura_triangulo

    cpi r16, '2'
    breq figura_circulo

    cpi r16, '3'
    breq figura_pentagrama

    cpi r16, '4'
    breq figura_libre

    cpi r16, 'P'
    breq figura_pokemon

    cpi r16, 'T'
    breq ejecutar_todo

    rjmp loop_menu


chequear_UART:
	;r16 = uart
	ret