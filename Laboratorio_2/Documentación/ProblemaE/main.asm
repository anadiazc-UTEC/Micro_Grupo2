.include "m328pdef.inc"

.def temp		= r16
.def estado		= r17
.def msg_blk	= r19
.def flag_obs	= r20
;CERRADA	= 0x00
;ABRIENDO	= 0x01
;ABIERTA	= 0x02
;CERRANDO	= 0x03
;BLOQUEADA	= 0x04

;PIND2		= ABRIR
;PIND3		= CERRAR
;PIND4		= SENSOR ABIERTO
;PIND5		= SENSOR CERRADO
;PIND6		= SENSOR OBSTACULO

;PINB0		= MOTOR ABRIR
;PINB1		= MOTOR CERRAR
;PINB2		= ALARMA
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

	sei

	ldi estado,		0x00
	
	ldi msg_blk,	0x00
	ldi flag_obs,	0x00

	sbis PIND,		PIND6
	rjmp main_loop
	
	ldi msg_blk,	0x01
	ldi flag_obs,	0x01
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
	
	cpi msg_blk, 0x01
	brne verificar_botones_bloqueo

	ldi r31, HIGH(msg_obstaculo << 1)
	ldi r30, LOW(msg_obstaculo << 1)
	rcall imprimir_cadena
	ldi msg_blk, 0x00

	rjmp verificar_botones_bloqueo

verificar_botones_bloqueo:
	; boton abrir
	sbic PIND, PIND2
	rjmp boton_abrir
	
	; boton cerrar
	sbic PIND, PIND3
	rjmp boton_cerrar
	
	rjmp main_loop

boton_abrir:

	rcall delay_12ms
	sbic PIND, PIND2
	rjmp boton_abrir

	ldi r31, HIGH(msg_abriendo << 1)
	ldi r30, LOW(msg_abriendo << 1)
	rcall imprimir_cadena
	
	cpi flag_obs, 0x01
	breq re_obstaculo

	ldi estado, 0x01
	rjmp main_loop

boton_cerrar:

	rcall delay_12ms
	sbic PIND, PIND3
	rjmp boton_cerrar

	ldi r31, HIGH(msg_cerrando << 1)
	ldi r30, LOW(msg_cerrando << 1)
	rcall imprimir_cadena

	cpi flag_obs, 0x01
	breq re_obstaculo
	
	ldi estado, 0x03
	rjmp main_loop

sensor_abierto:

	ldi r31, HIGH(msg_abierta << 1)
	ldi r30, LOW(msg_abierta << 1)
	rcall imprimir_cadena

	ldi estado, 0x02
	rcall delay_12ms
	rjmp main_loop

sensor_cerrado:
	ldi r31, HIGH(msg_cerrada << 1)
	ldi r30, LOW(msg_cerrada << 1)
	rcall imprimir_cadena

	ldi estado, 0x00
	rcall delay_12ms
	rjmp main_loop

re_obstaculo:
	ldi estado, 0x04
	ldi msg_blk, 0x01
	rjmp main_loop

imprimir_cadena:
	lpm temp, Z+
	tst temp
	breq fin_impresion
	rcall transmitir_UART
	rjmp imprimir_cadena

fin_impresion:
	ret

transmitir_UART:
	lds r18, UCSR0A
	sbrs r18, UDRE0
	rjmp transmitir_UART
	sts UDR0, temp
	ret

msg_abriendo:
	.db "Abriendo puerta... ", 0x0D, 0x0A, 0
msg_cerrando:
	.db "Cerrando puerta... ", 0x0D, 0x0A, 0
msg_abierta:
	.db "Puerta abierta.", 0x0D, 0x0A, 0
msg_cerrada:
	.db "Puerta cerrada.", 0x0D, 0x0A, 0
msg_obstaculo:
	.db "Obstaculo detectado. Movimiento detenido.", 0x0D, 0x0A, 0

ISR_OBSTACULO:
	push temp
	in temp, SREG
	push temp
	
	sbis PIND, PIND6
	rjmp obstaculo_retirado

obstaculo_detectado:
	ldi flag_obs, 0x01

	cpi estado, 0x01
	breq bloquear_puerta
	
	cpi estado, 0x03
	breq bloquear_puerta
	
	rjmp FIN_ISR

obstaculo_retirado:
	ldi flag_obs, 0x00
	rjmp FIN_ISR

bloquear_puerta:
	ldi estado, 0x04
	ldi msg_blk, 0x01
	rjmp FIN_ISR

FIN_ISR:
	pop temp
	out SREG, temp
	pop temp
	
	reti

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

