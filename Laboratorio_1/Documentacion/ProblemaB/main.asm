;
; Laboratorio1Micro.asm
;
; PIN 0 a
; PIN 1 b
; PIN 2 c
; PIN 3 d
; PIN 4 e
; PIN 5 f
; PIN 6 g 
; PIN 8 botón de decrementar
; PIN 9 botón de reiniciar contador
; PIN 10 botón de incrementar contador

.include "m328pdef.inc"
.def num0 = r18
.def num1 = r19
.def num2 = r20
.def num3 = r21
.def num4 = r22
.def num5 = r23
.def num6 = r24
.def num7 = r25
.def num8 = r26
.def num9 = r27
.cseg
.org 0x00

ldi r16,0b00000000
ldi r17,0b11111111

ldi num0, 0b00111111
ldi num1, 0b00000110
ldi num2, 0b01011011
ldi num3, 0b01001111
ldi num4, 0b01100110
ldi num5, 0b01101101
ldi num6, 0b01111101
ldi num7, 0b00000111
ldi num8, 0b01111111
ldi num9, 0b01101111


out DDRD,r17 ; Pines 0 al 7 SALIDA
out DDRB,r16 ; Pines 8 a 13 ENTRADA

out PORTD,num0 ; Usé num0 en vez de r17 para que el display muestre 0 siempre al inicio
out PORTB,r16 ; Usé r16 para no activar el pull-up interno ya que le voy a conectar un botón con pull-down :)

start:
	in r28, PINB ; Lee el valor de PINB (Donde se conectan los botones) y lo guarda en el registro 28
	andi r28, 0b00000111 ; Multiplica el valor por 0, exceptuando los valores que nos interesan (Los 3 puertos de los botones), para poder asi quedarnos solamente los valor que nos interesan. Se guarda en r28

	cpi r28, 0b00000001 ; Compara el valor de PINB con el valor esperado cuando el primer botón está pulsado.
	breq decrementar ; Si los valores anteriores son iguales salta a decrementar

	cpi r28, 0b00000010 ; Compara el valor de PINB con el valor esperado cuando el segundo botón está pulsado.
	breq reiniciar ; Si los valores anteriores son iguales salta a reiniciar

	cpi r28, 0b00000100 ; Compara el valor de PINB con el valor esperado cuando el tercer botón está pulsado.
	breq incrementar ; Si los valores anteriores son iguales salta a incrementar
    
	rjmp start

decrementar:
	in r29, PIND ; Lee el valor de PIND (Pines de salida que prenden el display) y lo guarda en r29

	rjmp start

reiniciar:
	out PORTD, num0 ; Carga la secuencia que prende el 0 en PORTD
    rjmp start

incrementar:
	in r29, PIND ; Lee el valor de PIND (Pines de salida que prenden el display) y lo guarda en r29
	rjmp start