.include "m328pdef.inc"

.def A = r17
.def B = r18
.def Seleccion = r19

.cseg
.org 0x00


ldi r16, 0b00000000    
out DDRD, r16 ; Pines 0 a 7 ENTRADAS A y B

ldi r16, 0b00111000    
out DDRC, r16 ; Pines A0 A1 y A2 ENTRADAS SELECTOR, A3 A4 y A5 SALIDAS BANDERAS 

ldi r16, 0b00001111    
out DDRB, r16 ; Pines del 8 al 11 SALIDAS RESULTADO

ldi r16, 0b00000000
out PORTD, r16    ; Entradas sin pull-up
out PORTB, r16    ; Salidas en cero y entradas sin pull-up
out PORTC, r16    ; Salida en cero


start:
	in r22, PIND ; Lee el valor de PIND (Entradas) y lo guarda en r22

	mov A, r22 
	andi A, 0b00001111

	mov B, r22
	swap B
	andi B, 0b00001111

	in r21, PINC ; Lee el valor de PINC (Donde se conecta el dip) y lo guarda en r21
	andi r21, 0b00000111 ; Multiplica el valor por 0, exceptuando los valores que nos interesan (Los 3 puertos de los botones), para poder asi quedarnos solamente los valor que nos interesan. Se guarda en r21

	cpi r21, 0b00000000
	breq clear

	cpi r21, 0b00000001
	breq AmenosB

	cpi r21, 0b00000010
	breq AmasB

	cpi r21, 0b00000011
	breq AxorB

	cpi r21, 0b00000100
	breq AandB

	cpi r21, 0b00000101
	breq AorB

	cpi r21, 0b00000110
	breq SHLAmenorque1

	cpi r21, 0b00000111
	breq INCAmas1

	rjmp start


clear:
	
	
AmenosB:
	sub A,B
	out PORTB, A
	rjmp start

AmasB:
	add A,B
	rjmp start

AxorB:
	
AandB:
	and A,B
	rjmp start

AorB:
	or A,B
	rjmp start

SHLAmenorque1:
	lsl A
	rjmp start

INCAmas1:
	andi A,1
	rjmp start
