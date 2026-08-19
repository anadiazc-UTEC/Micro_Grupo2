;
; Laboratorio1Micro.asm
;

.include "m328pdef.inc"

.cseg
.org 0x00
ldi r16,0b00000000
ldi r17,0b11111111

out DDRD,r17 ; Pines 0 al 7 SALIDA
out DDRB,r16 ; Pines 8 a 13 ENTRADA

out PORTD,r16 ; Usé r16 en vez de r17 para que las salidas queden en 0 y no se me prenda el display al inicio
out PORTB,r16 ; Usé r16 para no activar el pull-up interno ya que le voy a conectar un botón con pull-down :)

start:
    rjmp start
