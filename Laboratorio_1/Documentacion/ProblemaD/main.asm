.include "m328pdef.inc"

def A = r18
def B = r19
def Seleccion = r20

.cseg
.org 0x00

ldi r16, 0b00000000 
ldi r17, 0b11111111

out DDRD,r16 ; Pines 0 al 7 ENTRADA
out DDRB,r17 ; Pines 8 a 13 SALIDA

out PORTD,r16 ; Usé r16 para no activar el pull-up interno ya que le voy a conectar un botón con pull-down :)
out PORTB, r16 ; Entradas en 0

start:

	in r21, PIND ; Lee el valor de PIND (Donde se conectan los botones) y lo guarda en el registro 21
	andi r21, 0b00000111 ; Multiplica el valor por 0, exceptuando los valores que nos interesan (Los 3 puertos de los botones), para poder asi quedarnos solamente los valor que nos interesan. Se guarda en r21

	cpi r21, 0b00000000 ; Compara el valor de PIND con el valor esperado.
	breq clear; Si los valores anteriores son iguales salta a clear

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

AmenosB:
	sub A,B
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
