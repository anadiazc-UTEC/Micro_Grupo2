
;Estados: 
;0x00: Espera
;0x01: Lavado
;0x02: Centrifugado
;0x03: Secado
;0x04: Finalizar

.include "m328pdef.inc"
.org 0x00

;Inicializo el pin 8 como entrada
cbi DDRB, PINB0
cbi PORTB, PINB0

;inicializo el estado en "Espera"
ldi r16, 0x00

start:
	
    cpi r16, 0x00
	breq espera

	cpi r16, 0x01
	breq lavado

    cpi r16, 0x02
	breq centrifugado

	cpi r16, 0x03
	breq secado

	cpi r16, 0x04
	breq finalizar

	ldi r16, 0x00
	rjmp start

espera:
	;Valido si el pin 8 recibe entrada. si la recibe, saltea el bucle
	sbis PINB, PINB0
	rjmp espera

	ldi r16, 0x01
	rjmp fin_if

lavado:
	ldi r16, 0x02
	rjmp fin_if

centrifugado:
	ldi r16, 0x03
	rjmp fin_if

secado:
	ldi r16, 0x04
	rjmp fin_if

finalizar:
	ldi r16, 0x00
	rjmp fin_if

fin_if:
	rjmp start