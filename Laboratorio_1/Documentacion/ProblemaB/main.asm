;
; Laboratorio1Micro.asm
;
; PB0 Es el pin 8 del Arduino
; PB1 Es el pin 9 del Arduino
; PB2 Es el pin 10 del Arduino 

.include "m328pdef.inc"

.cseg
.org 0x00
ldi r16,0
ldi r17,1

out DDRD,r17 ; Pines 0 al 7 SALIDA
out DDRB,r16 ; Pines 8 a 13 ENTRADA

out PORTD,r16 ; Usé r16 en vez de r17 para que las salidas queden en 0 y no en 1
out PORTB,r16 ; Usé r16 para no activar el pull-up interno ya que le voy a conectar un botón con pull-down :)

start:
    rjmp start
