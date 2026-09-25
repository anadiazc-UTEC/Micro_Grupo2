.include "m328pdef.inc"

.def temp	= r16
.def estado	= r17

;CERRADA	= 0x00
;ABRIENDO	= 0x01
;ABIERTA	= 0x02
;CERRANDO	= 0x03
;BLOQUEADA	= 0x04

.org 0x0000
	rjmp RESET
.org 0x000A		;PCINT2 (PORTD)
	rjmp ISR_OBSTACULO
RESET:
	ldi temp,	high(RAMEND)
	out SPH,	temp
	ldi temp,	low(RAMEND)
	out SPL,	temp

	ldi temp,	(1<<PINB0) | (1<<PINB1) | (1<<PINB2)
	out DDRB,	temp
	ldi temp,	0x00
	out PORTB,	temp

	ldi temp,	0x00
	out DDRD,	temp
	ldi temp,	0x00
	out PORTD,	temp

	ldi temp,	LOW(103)
	sts UBRR0L,	temp
	ldi temp,	HIGH(103)
	sts UBRR0H,	temp

	ldi temp,	(1<<TXEN0)
	sts UCSR0B,	temp

	ldi temp,	(1<<UCSZ01) | (1<<UCSZ00)
	sts UCSR0C,	temp

	ldi temp,	(1<<PCIE2)
	sts PCICR,	temp
	ldi temp,	(1<<PCINT22)
	sts PCMSK2,	temp

	ldi estado, 0x00

main_loop:
	
	cpi estado, 0x00
	breq ejecutar_cerrada
	
	cpi estado, 0x01
	breq ejecutar_abriendo
	
	cpi estado, 0x02
	breq ejecutar_abierta
	
	cpi estado, 0x03
	breq ejecutar_cerrando
	
	cpi estado, 0x04
	breq ejecutar_bloqueada
	
	rjmp main_loop
	
ejecutar_cerrada:

	; apagar todo
	ldi temp, 0x00	
	out PORTB, temp
	
	; boton abrir
	sbic PIND, PIND2
	rjmp boton_abrir
	
	rjmp main_loop

ejecutar_abriendo:

	;abrir puerta y sonar alarma
	ldi temp, (1<<PINB0) | (1<<PINB2) 
	out PORTB, temp
	
	;sensor abierto
	sbic PIND, PIND4
	rjmp sensor_abierto
	
	rjmp main_loop

ejecutar_abierta:

	; apagar todo
	ldi temp, 0x00
	out PORTB, temp
	
	;boton cerrar
	sbic PIND, PIND3
	rjmp boton_cerrar
	
	rjmp main_loop

ejecutar_cerrando:

	;cerrar puerta y sonar alarma
	ldi temp, (1<<PINB1) | (1<<PINB2)
	out PORTB, temp
	
	;sensor cerrado
	sbic PIND, PIND5
	rjmp sensor_cerrado

	rjmp main_loop

ejecutar_bloqueada:
	; apagar todo
	ldi temp, 0x00
	out PORTB, temp
	
	; boton abrir
	sbic PIND, PIND2
	rjmp boton_abrir
	
	; boton cerrar
	sbic PIND, PIND3
	rjmp boton_cerrar
	
	rjmp main_loop

boton_abrir:
	ldi estado, 0x01
	rcall delay_12ms
	rjmp main_loop

boton_cerrar:
	ldi estado, 0x03
	rcall delay_12ms
	rjmp main_loop

sensor_abierto:
	ldi estado, 0x02
	rcall delay_12ms
	rjmp main_loop

sensor_abierto:
	ldi estado, 0x00
	rcall delay_12ms
	rjmp main_loop

delay_12ms:
	ldi r28, 255
l2:
	ldi r29, 250
l1:
	
	dec r29
	brne l1

	dec r28
	brne l2	
	
	ret