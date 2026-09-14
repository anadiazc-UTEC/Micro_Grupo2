
.def contador = r18
.def constante = r19
.def cantidadValores = r16

.org 0x0000
rjmp start

configurar:
	ldi r20, 255
	out DDRD, r20
	ldi r20, 0xff
	out DDRB, r20
	clr r20
	out PORTC, r20
	call guardar_codigos
	ret

esperar_inicio:
	nop
	ret

start:
    ldi r16, HIGH(RAMEND)
	out SPH, r16
	ldi r16, LOW(RAMEND)
	out SPL, r16
	call configurar
	call esperar_inicio

bucle_infinito:
	call leer_derecho    
	call leer_reves      
	rjmp bucle_infinito

guardar_codigos:
	ldi r28, 0x00 ;LOW(0x0100)
	ldi r29, 0x01 ;HIGH(0x0100)
	ldi contador, 0x00
	ldi constante, 0x02
	rjmp bucle_guardar

bucle_guardar:
	ST Y+, contador
	cpi contador, 0xfe
	breq fin_guardar

	add contador, constante
	rjmp bucle_guardar

fin_guardar:
	ret

leer_derecho:
	ldi r28, 0x00
	ldi r29, 0x01
	ldi cantidadValores, 128

bucle_derecho:
	ld r20, Y+
	out PORTD, r20
	dec cantidadValores
	brne bucle_derecho
	ret

leer_reves:
	ldi cantidadValores, 128

bucle_reves:
	ld r20, -Y
    out PORTD, r20      
    dec cantidadValores
    brne bucle_reves
	ret
	