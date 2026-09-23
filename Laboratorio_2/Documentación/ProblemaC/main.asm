
.include "m328pdef.inc"
; (16MHz / 16 / 9600 baudios) - 1 = 103
.equ BPS = 103

; Asignación de bits en PORTD
.equ BIT_BAJAR		= PORTD2
.equ BIT_SUBIR		= PORTD3
.equ BIT_ABAJO		= PORTD4
.equ BIT_ARRIBA		= PORTD5
.equ BIT_IZQUIERDA	= PORTD6
.equ BIT_DERECHA	= PORTD7
.org 0x0000
rjmp start

start:

	; Inicializar sp
	ldi r16,	LOW(RAMEND)
	out SPL,	r16
	ldi r16,	HIGH(RAMEND)
	out SPH,	r16
	
	; Configurar D2-D7 como salidas 
	ldi r16,	0b11111100
	out DDRD,	r16
	clr r16
	out PORTD,	r16

	
	; Configurar UART
	ldi r16,	LOW(BPS)
	sts UBRR0L,	r16
	ldi r16,	HIGH(BPS)
	sts UBRR0H,	r16

	ldi r16,	(1<<RXEN0) | (1<<TXEN0)
	sts UCSR0B,	r16

	ldi r16,	(1<<UCSZ01) | (1<<UCSZ00)
	sts UCSR0C,	r16
	
	ldi r18, 0; lugar en la hoja
	ldi r19, 30 ;espacio para cada dibujo

	rcall subir_solenoide
	rcall mover_al_principio

loop_menu:
    rcall chequear_UART
    
	;RCALL delay_100ms
    cpi r16, '1'
    breq figura_triangulo

    cpi r16, '2'
    breq figura_circulo

    cpi r16, '3'
    breq figura_pentagrama

    cpi r16, '4'
    breq figura_libre

    cpi r16, 'P'
    breq figura_pokemon

    cpi r16, 'T'
    breq ejecutar_todo

    rjmp loop_menu


chequear_UART:
	lds r17, UCSR0A
	sbrs r17, RXC0
	rjmp chequear_UART

	lds r16, UDR0

	ret

figura_triangulo:
	
	rcall dibujar_triangulo
	rjmp loop_menu

figura_circulo:
	
	rcall dibujar_circulo
	rjmp loop_menu

figura_pentagrama:

	rcall dibujar_pentagrama
	rjmp loop_menu

figura_libre:
	
	rcall dibujar_libre
	rjmp loop_menu

figura_pokemon:
	
	rcall dibujar_pokemon
	rjmp loop_menu

ejecutar_todo:
	ldi r16, 0b10000000
	out PORTD, r16

	rjmp loop_menu

mover_al_principio:
	;aprox cada dibujo mide 20x20 (redondeando)
	;le agregamos 10 para espaciar
	;son 5 dibujos, por ende 30 x 5 = 150
	
	rcall pixel_izquierda
	inc r18
	
	cpi r18, 150
	brne mover_al_principio

	ldi r18, 150

	ret
	
subir_solenoide:

	sbi PORTD, BIT_SUBIR
	cbi PORTD, BIT_BAJAR
	rcall delay_100ms
	ret

bajar_solenoide:

	sbi PORTD, BIT_BAJAR
	cbi PORTD, BIT_SUBIR
	rcall delay_100ms
	ret

detener_movimiento:

	cbi PORTD, BIT_ARRIBA
	cbi PORTD, BIT_ABAJO
	cbi PORTD, BIT_IZQUIERDA
	cbi PORTD, BIT_DERECHA
	ret

pixel_abajo:
	;un pixel para abajo
	sbi PORTD, BIT_ABAJO
	rcall delay_pixel
	cbi PORTD, BIT_ABAJO
	ret

pixel_arriba:
	;un pixel para arriba
	sbi PORTD, BIT_ARRIBA
	rcall delay_pixel
	cbi PORTD, BIT_ARRIBA
	ret

pixel_izquierda:
	;un pixel para la izquierda
	sbi PORTD, BIT_IZQUIERDA
	rcall delay_pixel
	cbi PORTD, BIT_IZQUIERDA
	ret
	
pixel_derecha:
	;un pixel para la derecha
	sbi PORTD, BIT_DERECHA
	rcall delay_pixel
	cbi PORTD, BIT_DERECHA
	ret

pixel_izquierda_arriba:
	;un pixel para la izquierda arriba
	sbi PORTD, BIT_IZQUIERDA
	sbi PORTD, BIT_ARRIBA
	rcall delay_pixel
	cbi PORTD, BIT_IZQUIERDA
	cbi PORTD, BIT_ARRIBA
	ret

pixel_derecha_arriba:
	;un pixel para la derecha arriba
	sbi PORTD, BIT_DERECHA
	sbi PORTD, BIT_ARRIBA
	rcall delay_pixel
	cbi PORTD, BIT_DERECHA
	cbi PORTD, BIT_ARRIBA
	ret

pixel_izquierda_abajo:
	;un pixel para la izquierda abajo
	sbi PORTD, BIT_IZQUIERDA
	sbi PORTD, BIT_ABAJO
	rcall delay_pixel
	cbi PORTD, BIT_IZQUIERDA
	cbi PORTD, BIT_ABAJO
	ret

pixel_derecha_abajo:
	;un pixel para la derecha abajo
	sbi PORTD, BIT_DERECHA
	sbi PORTD, BIT_ABAJO
	rcall delay_pixel
	cbi PORTD, BIT_DERECHA
	cbi PORTD, BIT_ABAJO
	ret

dibujar_triangulo:
	;triangulo rectangulo 8x15x17

	rcall detener_movimiento
	rcall bajar_solenoide

	;primer cateto
	rcall pixel_abajo
	rcall pixel_abajo
	rcall pixel_abajo
	rcall pixel_abajo
	rcall pixel_abajo
	rcall pixel_abajo
	rcall pixel_abajo
	rcall pixel_abajo
	
	;segundo cateto
	rcall pixel_derecha
	rcall pixel_derecha
	rcall pixel_derecha
	rcall pixel_derecha
	rcall pixel_derecha
	rcall pixel_derecha
	rcall pixel_derecha
	rcall pixel_derecha
	rcall pixel_derecha
	rcall pixel_derecha
	rcall pixel_derecha
	rcall pixel_derecha
	rcall pixel_derecha

	;hipotenusa
	rcall pixel_izquierda_arriba
	rcall pixel_izquierda_arriba
	rcall pixel_izquierda_arriba
	rcall pixel_izquierda_arriba
	rcall pixel_izquierda_arriba
	rcall pixel_izquierda_arriba
	rcall pixel_izquierda_arriba
	rcall pixel_izquierda_arriba
	rcall pixel_izquierda_arriba
	rcall pixel_izquierda_arriba
	rcall pixel_izquierda_arriba
	rcall pixel_izquierda_arriba
	rcall pixel_izquierda_arriba
	rcall pixel_izquierda_arriba
	rcall pixel_izquierda_arriba
	rcall pixel_izquierda_arriba
	rcall pixel_izquierda_arriba

	rcall detener_movimiento
	rcall subir_solenoide

	ret

dibujar_circulo:
	;circulo 18px diametro
	rcall detener_movimiento
	rcall bajar_solenoide

	;primer cuarto
	rcall pixel_derecha
	rcall pixel_derecha

	rcall pixel_abajo
	
	rcall pixel_derecha
	rcall pixel_derecha

	rcall pixel_abajo

	rcall pixel_derecha
	rcall pixel_derecha
	
	rcall pixel_abajo
	
	rcall pixel_derecha
	
	rcall pixel_abajo
	rcall pixel_abajo
	
	rcall pixel_derecha

	rcall pixel_abajo
	rcall pixel_abajo
	
	rcall pixel_derecha

	rcall pixel_abajo
	rcall pixel_abajo

	;segundo cuarto
	rcall pixel_abajo
	rcall pixel_abajo
	
	rcall pixel_izquierda
	
	rcall pixel_abajo
	rcall pixel_abajo

	rcall pixel_izquierda

	rcall pixel_abajo
	rcall pixel_abajo
	
	rcall pixel_izquierda
	
	rcall pixel_abajo
	
	rcall pixel_izquierda
	rcall pixel_izquierda

	rcall pixel_abajo
	
	rcall pixel_izquierda
	rcall pixel_izquierda

	rcall pixel_abajo
	
	rcall pixel_izquierda
	rcall pixel_izquierda

	;tercer cuarto
	rcall pixel_izquierda
	rcall pixel_izquierda

	rcall pixel_arriba
	
	rcall pixel_izquierda
	rcall pixel_izquierda

	rcall pixel_arriba
	
	rcall pixel_izquierda
	rcall pixel_izquierda
	
	rcall pixel_arriba
	
	rcall pixel_izquierda
	
	rcall pixel_arriba
	rcall pixel_arriba
	
	rcall pixel_izquierda
	
	rcall pixel_arriba
	rcall pixel_arriba
	
	rcall pixel_izquierda
	
	rcall pixel_arriba
	rcall pixel_arriba
	
	;cuarto cuarto
	rcall pixel_arriba
	rcall pixel_arriba

	rcall pixel_derecha
	
	rcall pixel_arriba
	rcall pixel_arriba

	rcall pixel_derecha
	
	rcall pixel_arriba
	rcall pixel_arriba

	rcall pixel_derecha
	
	rcall pixel_arriba

	rcall pixel_derecha
	rcall pixel_derecha

	rcall pixel_arriba

	rcall pixel_derecha
	rcall pixel_derecha

	rcall pixel_arriba

	rcall pixel_derecha
	rcall pixel_derecha
	
	rcall detener_movimiento
	rcall subir_solenoide

	ret

dibujar_pentagrama:
	
	rcall detener_movimiento
	rcall bajar_solenoide
	;linea 1
	rcall pixel_derecha_abajo
	
	rcall pixel_abajo
	rcall pixel_abajo

	rcall pixel_derecha_abajo
	
	rcall pixel_abajo
	rcall pixel_abajo
	
	rcall pixel_derecha_abajo

	rcall pixel_abajo
	rcall pixel_abajo
	rcall pixel_abajo
	
	rcall pixel_derecha_abajo

	rcall pixel_abajo

	rcall pixel_derecha_abajo

	rcall pixel_abajo
	rcall pixel_abajo
	
	rcall pixel_derecha_abajo

	rcall pixel_abajo

	;linea 2

	rcall pixel_izquierda_arriba
	rcall pixel_izquierda_arriba
	
	rcall pixel_izquierda
	
	rcall pixel_izquierda_arriba
	rcall pixel_izquierda_arriba
	
	rcall pixel_izquierda
	
	rcall pixel_izquierda_arriba
	rcall pixel_izquierda_arriba
	rcall pixel_izquierda_arriba
	
	rcall pixel_izquierda
	
	rcall pixel_izquierda_arriba
	rcall pixel_izquierda_arriba
	
	rcall pixel_izquierda
	
	rcall pixel_izquierda_arriba
	
	;linea 3
	rcall pixel_derecha
	rcall pixel_derecha
	rcall pixel_derecha
	rcall pixel_derecha
	rcall pixel_derecha
	rcall pixel_derecha
	rcall pixel_derecha
	rcall pixel_derecha
	rcall pixel_derecha
	rcall pixel_derecha
	rcall pixel_derecha
	rcall pixel_derecha
	rcall pixel_derecha
	rcall pixel_derecha
	rcall pixel_derecha
	rcall pixel_derecha
	rcall pixel_derecha
	rcall pixel_derecha
	
	;linea 4
	rcall pixel_izquierda_abajo
	rcall pixel_izquierda_abajo
	
	rcall pixel_izquierda
	
	rcall pixel_izquierda_abajo
	rcall pixel_izquierda_abajo
	
	rcall pixel_izquierda
	
	rcall pixel_izquierda_abajo
	rcall pixel_izquierda_abajo
	rcall pixel_izquierda_abajo
	
	rcall pixel_izquierda
	
	rcall pixel_izquierda_abajo
	rcall pixel_izquierda_abajo
	
	rcall pixel_izquierda
	
	rcall pixel_izquierda_abajo
	rcall pixel_izquierda_abajo

	;linea 5
	
	rcall pixel_arriba
	
	rcall pixel_derecha_arriba
	
	rcall pixel_arriba
	rcall pixel_arriba
	
	rcall pixel_derecha_arriba
	
	rcall pixel_arriba
	
	rcall pixel_derecha_arriba
	
	rcall pixel_arriba
	rcall pixel_arriba
	rcall pixel_arriba

	rcall pixel_derecha_arriba
	
	rcall pixel_arriba
	rcall pixel_arriba
	
	rcall pixel_derecha_arriba
	
	rcall pixel_arriba
	rcall pixel_arriba
	
	rcall pixel_derecha_arriba

	rcall detener_movimiento
	rcall subir_solenoide
	ret

dibujar_libre:

	;dibujar un corazon
	rcall detener_movimiento
	;iniciaremos desde mas abajo
	rcall subir_solenoide
	
	rcall pixel_abajo
	rcall pixel_abajo
	rcall pixel_abajo
	rcall pixel_abajo

	rcall bajar_solenoide
	
	rcall pixel_derecha_arriba
	rcall pixel_derecha_arriba
	rcall pixel_derecha_arriba
	rcall pixel_derecha_arriba
	
	rcall pixel_derecha
	rcall pixel_derecha
	rcall pixel_derecha
	
	rcall pixel_derecha_abajo
	rcall pixel_derecha_abajo
	
	rcall pixel_abajo
	rcall pixel_abajo
	rcall pixel_abajo
	rcall pixel_abajo
	
	rcall pixel_izquierda_abajo
	rcall pixel_izquierda_abajo
	rcall pixel_izquierda_abajo
	rcall pixel_izquierda_abajo
	rcall pixel_izquierda_abajo
	rcall pixel_izquierda_abajo
	rcall pixel_izquierda_abajo
	rcall pixel_izquierda_abajo
	rcall pixel_izquierda_abajo
	
	rcall pixel_izquierda_arriba
	rcall pixel_izquierda_arriba
	rcall pixel_izquierda_arriba
	rcall pixel_izquierda_arriba
	rcall pixel_izquierda_arriba
	rcall pixel_izquierda_arriba
	rcall pixel_izquierda_arriba
	rcall pixel_izquierda_arriba
	rcall pixel_izquierda_arriba
	
	rcall pixel_arriba
	rcall pixel_arriba
	rcall pixel_arriba
	rcall pixel_arriba
	
	rcall pixel_derecha_arriba
	rcall pixel_derecha_arriba
	
	rcall pixel_derecha
	rcall pixel_derecha
	rcall pixel_derecha
	
	rcall pixel_derecha_abajo
	rcall pixel_derecha_abajo
	rcall pixel_derecha_abajo
	rcall pixel_derecha_abajo
	
	rcall subir_solenoide
	
	rcall pixel_arriba
	rcall pixel_arriba
	rcall pixel_arriba
	rcall pixel_arriba

	rcall detener_movimiento
	ret

dibujar_pokemon:

	rcall detener_movimiento
	rcall bajar_solenoide
	;estrella
	rcall pixel_derecha_abajo
	rcall pixel_derecha_abajo
	
	rcall pixel_abajo
	
	rcall pixel_derecha_abajo
	
	rcall pixel_abajo
	
	rcall pixel_derecha
	rcall pixel_derecha
	rcall pixel_derecha
	rcall pixel_derecha
	
	rcall pixel_derecha_abajo
	
	rcall pixel_abajo
	
	rcall pixel_izquierda_abajo
	rcall pixel_izquierda_abajo
	rcall pixel_izquierda_abajo
	
	rcall pixel_abajo
	rcall pixel_abajo
	rcall pixel_abajo
	
	rcall pixel_derecha_abajo
	rcall pixel_abajo

	rcall pixel_izquierda_abajo
	
	rcall pixel_izquierda
	
	rcall pixel_izquierda_arriba
	
	rcall pixel_izquierda

	rcall pixel_izquierda_arriba
	
	rcall pixel_izquierda_abajo
	
	rcall pixel_izquierda
	
	rcall pixel_izquierda_arriba

	rcall pixel_izquierda
	
	rcall pixel_izquierda_abajo
	rcall pixel_izquierda_abajo
	
	rcall pixel_izquierda_arriba
	rcall pixel_izquierda_arriba
	
	rcall pixel_arriba
	rcall pixel_arriba
	
	rcall pixel_derecha_arriba
	
	rcall pixel_arriba
	rcall pixel_arriba

	rcall pixel_izquierda_arriba
	
	rcall pixel_arriba

	rcall pixel_izquierda_arriba
	
	rcall pixel_arriba
	
	rcall pixel_derecha_arriba
	
	rcall pixel_derecha
	rcall pixel_derecha
	
	rcall pixel_derecha_abajo
	
	rcall pixel_derecha_arriba
	
	rcall pixel_arriba

	rcall pixel_derecha_arriba

	rcall pixel_arriba
	
	rcall pixel_derecha_arriba

	rcall pixel_derecha

	rcall subir_solenoide

	rcall pixel_abajo
	rcall pixel_abajo
	rcall pixel_abajo
	rcall pixel_abajo
	rcall pixel_abajo
	
	rcall bajar_solenoide
	;circulo exterior
	
	rcall pixel_derecha_abajo
	
	rcall pixel_derecha
	
	rcall pixel_derecha_abajo
	
	rcall pixel_abajo
	
	rcall pixel_derecha_abajo
	
	rcall pixel_abajo
	
	rcall pixel_izquierda_abajo
	
	rcall pixel_abajo

	rcall pixel_izquierda_abajo
	
	rcall pixel_izquierda
	
	rcall pixel_izquierda_abajo
	
	rcall pixel_izquierda
	
	rcall pixel_izquierda_arriba
	
	rcall pixel_izquierda
	
	rcall pixel_izquierda_arriba
	
	rcall pixel_arriba
	
	rcall pixel_izquierda_arriba
	
	rcall pixel_arriba
	rcall pixel_arriba
	
	rcall pixel_derecha_arriba
	rcall pixel_derecha_arriba
	
	rcall pixel_derecha
	
	rcall pixel_derecha_arriba
	
	rcall pixel_derecha

	rcall subir_solenoide

	rcall pixel_abajo
	rcall pixel_abajo
	rcall pixel_abajo
	
	rcall bajar_solenoide
	;circulo interior
	
	rcall pixel_derecha_abajo
	
	rcall pixel_abajo
	rcall pixel_abajo
	
	rcall pixel_izquierda_abajo
	
	rcall pixel_izquierda
	rcall pixel_izquierda
	
	rcall pixel_izquierda_arriba
	
	rcall pixel_arriba
	rcall pixel_arriba
	
	rcall pixel_derecha_arriba
	
	rcall pixel_derecha
	rcall pixel_derecha


	rcall detener_movimiento
	rcall subir_solenoide
	ret

delay_100ms:
	ldi r28, 157
l6:
	ldi r29, 14
l5:
	ldi r30, 241
l4:
	dec r30
	brne l4

	dec r29
	brne l5	

	dec r28
	brne l6
	
	ret


delay_2500ms:
	ldi r28, 251
l3:
	ldi r29, 234
l2:
	ldi r30, 226
l1:
	dec r30
	brne l1

	dec r29
	brne l2	

	dec r28
	brne l3
	
	ret

delay_pixel:
	;1px  = 1s
	ldi r28, 201
l9:
	ldi r29, 169
l8:
	ldi r30, 156
l7:
	dec r30
	brne l7

	dec r29
	brne l8

	dec r28
	brne l9
	
	ret