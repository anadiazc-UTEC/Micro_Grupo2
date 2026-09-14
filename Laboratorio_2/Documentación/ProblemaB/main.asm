
	
; Segmento de codigo
.def contador = r18
.def constante = r19

.org 0x0000
rjmp start

ldi r18, 0x00
ldi r19, 0x02

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
	ldi r16, 0x01
	rjmp start

guardar_codigos:
	ldi r28, 0x00 ;LOW(0x0100)
	ldi r29, 0x01 ;HIGH(0x0100)
	rjmp bucle_guardar

bucle_guardar:
	ST Y+, r18
	cpi r18, 0xfe
	breq fin_guardar

	add r18, r19
	rjmp bucle_guardar
	

fin_guardar:
	ret

	