; Pin 0: LIBRE
; Pin 1: LIBRE
; Pin 2: A0
; Pin 3: A1
; Pin 4: A2
; Pin 5: A3
; Pin 6: B2
; Pin 7: B3
; Pin 8: Salida0
; Pin 9: Salida1
; Pin 10: Salida2
; Pin 11: Salida3
; Pin 12: B0
; Pin 13: B1
; Pin A0: Selector0
; Pin A1: Selector1
; Pin A2: Selector2
; Pin A3: Flag Negativo
; Pin A4: Flag Zero
; Pin A5: Flag Carry

; B tuvo que quedar separado porque usar los pines 0 y 1 me estaba dando problemas, ya que son los pines de comunicación del arduino y esto proboca una interferencia cuando se usan como entradas.

.include "m328pdef.inc"

.def A = r17
.def B = r18
.def Seleccion = r19
.def status = r20

.cseg
.org 0x00


ldi r16, 0b00000011    
out DDRD, r16 ; Pines 2 a 7 ENTRADAS A y B (los dos pines faltantes para poder ingresar A y B estan en el grupo DDRB pines 12 y 13)
ldi r16, 0b00111000    
out DDRC, r16 ; Pines A0 A1 y A2 ENTRADAS SELECTOR, A3 A4 y A5 SALIDAS BANDERAS 

ldi r16, 0b00001111    
out DDRB, r16 ; Pines del 8 al 11 SALIDAS RESULTADO (pines 12 y 13 ENTRADAS)

ldi r16, 0b00000000
out PORTD, r16    ; Entradas sin pull-up
out PORTB, r16    ; Salidas en cero y entradas sin pull-up
out PORTC, r16    ; Salida en cero


start:
	in r22, PIND ; Lee el valor de PIND (Entradas) y lo guarda en r22
	in r23, PINB ; Lee el valor de PINB (2 entradas faltantes) y lo guarda en r23

	mov A, r22 ; Mueve los datos registrados en r22 a A
	andi A, 0b00111100 ; Conservo unicamente los datos que interesan, multiplicando el resto por 0
	lsr A ; Mueve A hacia la derecha para alinearlo a bit 0
	lsr A ; Mueve A hacia la derecha para alinearlo a bit 0 formato final de A: 0000 5 4 3 2

	andi r22, 0b11000000 ; Obtiene los bits de las entradas 6 y 7 multiplicando el resto por 0
	andi r23, 0b00110000 ; Obtiene los bits de las entradas 12 y 13 multiplicando el resto por 0
	or r23, r22 ; une los dos registros
	mov B, r23 ; mueve la union de los dos registros a B
	swap B ; formato final de B: 0000 7 6 13 12

	in r21, PINC ; Lee el valor de PINC (Donde se conecta el dip) y lo guarda en r21
	andi r21, 0b00000111 ; Multiplica el valor por 0, exceptuando los valores que nos interesan (Los 3 puertos de los botones), para poder asi quedarnos solamente los valor que nos interesan. Se guarda en r21

	; Compara el valor de PINC con el valor esperado al pulsar cada botón, y salta a la función correspondiente
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
	breq SHLDesplazarA

	cpi r21, 0b00000111
	breq INCAmas1

	rjmp start

clear:
    clr A ; Setea A en 0
    rjmp enviarSalidas ; Salta a enviarSalidas
	
AmenosB:
	sub A,B ; Resta A - B
	brmi negativo ; Si el valor anterior es negativo (prende la flag de negativo) salta a negativo
	rjmp enviarSalidas ; Si no es negativo envia las salidas

AmasB:
	add A,B ; Suma A + B
	rjmp enviarSalidas ; Salta a enviarSalidas

AxorB:
	eor A,B ; Exor entre A y B
	rjmp enviarSalidas ; Salta a enviarSalidas
AandB:
	and A,B ; And entre A y B
	rjmp enviarSalidas ; Salta a enviarSalidas

AorB:
	or A,B ; or entre A y B
	rjmp enviarSalidas ; Salta a enviarSalidas

SHLDesplazarA:
	lsl A ; Desplaza A a la izquierda
	rjmp enviarSalidas ; Salta a enviarSalidas

INCAmas1:
	inc A ; Incrementa A en 1
	rjmp enviarSalidas ; Salta a enviarSalidas

enviarSalidas:
    mov r16,A ; Mueve A al registro 16
    andi r16,0b00001111 ; Conservo unicamente los datos que interesan (las 4 leds), multiplicando el resto por 0
    out PORTB,r16 ; Saca r16 por el PORTB prendiendo las leds que correspondan
    rjmp calcularFlags ; Salta a calcular flags

calcularFlags:
	ldi r16,0b00000000 ; Limpia el registro r16
    out PORTC,r16 ; Muestra el registro vacio en las banderas (Apaga las banderas)

    cpi A, 0x10 ; Compara A con 0x10 (16 en decimal)
    brsh carry ; Si A es mayor o igual a 16 prende la bandera carry

    tst A ; Hace un and de A consigo mismo, si este es 0 prenderá la flag de 0
    breq zero ; Como breq lee la flag de 0, ira a zero si A es 0
    rjmp start

carry:
	ldi r16, 0b00100000	; Prende la salida A5
	out PORTC,r16 
	rjmp start

zero:
	ldi r16, 0b00010000	; Prende la salida A4
	out PORTC,r16 
	rjmp start

negativo:
	; Aca me aseguro de prender las leds de la resta, ya que cuando es negativo no prendian bien.
    mov r16,A ; Mueve A al registro 16
    andi r16,0b00001111 ; Conservo unicamente los datos que interesan (las 4 leds), multiplicando el resto por 0
    out PORTB,r16 ; Saca r16

	ldi r16, 0b00001000	; Prende la salida A3
	out PORTC,r16 
	rjmp start