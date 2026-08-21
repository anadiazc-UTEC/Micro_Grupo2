
;Estados: 
;0x00:	Espera
;0x01:	Lavado
;0x02:	Centrifugado
;0x03:	Secado
;0x04:	Finalizado

;Cargas:
;0x00:	Ligera
;0x01:	Media
;0x02:	Pesada

;Pines
;inputs
;Pin A0: PINC0: pulsador inicio
;Pin A1: PINC1: pulsador seleccionar carga
;Pin A2: PINC2: sensor puerta cerrada
;Pin A3: PINC3: sensor de agua

;outputs
;Pin 2:		PIND2: Motor izquierda (simulado led)
;Pin 3:		PIND3: No se usa (se usaria para controlar la velocidad del motor en caso real para controlar com pwd)
;Pin 4:		PIND4: Motor derecha (simulado led)
;Pin 5:		PIND5: Led Espera
;Pin 6:		PIND6: Led Lavado
;Pin 7:		PIND7: Led Centrifugado
;Pin 8:		PINB0: Led Secado
;Pin 9:		PINB1: Led Finalizado
;Pin 10:	PINB2: Led carga ligera
;Pin 11:	PINB3: Led carga media
;Pin 12:	PINB4: Led carga pesada

.include "m328pdef.inc"
.org 0x00

;inicializar puertos C como entrada pull down
ldi r31, 0x00
out DDRC, r31
out PORTC, r31

;inicializar puertos D del 2 al 7 como salidas (menos el 3, ese no lo vamos a configurar todavia)
ldi r31, 0b11110100
out DDRD, r31
;inicializo los leds apagados salvo el led de espera (default)
ldi r31, (1<<PIND5)
out PORTD, r31

;inicializar puertos B del 0 al 4 como salidas
ldi r31, 0b00011111
out DDRB, r31
;inicializo los leds apagados salvo el led de carga ligera (default)
ldi r31, (1<<PINB2)
out PORTB, r31

;declaro las variables para usar nombres mas amigables
.def estado	= r16
.def carga	= r17

;inicializo el estado en "Espera"
ldi estado, 0x00

;inicializo la carga en "Ligera"
ldi carga, 0x00

start:

    cpi estado, 0x00
	breq espera

	cpi estado, 0x01
	breq lavado

    cpi estado, 0x02
	breq centrifugado

	cpi estado, 0x03
	breq secado

	cpi estado, 0x04
	breq finalizado

	ldi estado, 0x00
	rjmp start

espera:
	
	rcall validar_carga

	;Valido si el pin A0 (inicio) recibe entrada. si la recibe, saltea el bucle
	sbis PINC, PINC0
	rjmp espera

	ldi estado, 0x01
	rjmp fin_if

lavado:
	
	;encendemos el led de lavado, apagando el de espera
	in r31, PORTD
	ori r31, (1 << PIND6)
	cbr r31, (1 << PIND5)
	out PORTD, r31
	rcall validar_agua

	;Desde ahora, debo validar que la puerta este cerrada para mover el tambor
	ldi estado, 0x02
	rjmp fin_if

centrifugado:
	ldi estado, 0x03
	rjmp fin_if

secado:
	ldi estado, 0x04
	rjmp fin_if

finalizado:
	ldi estado, 0x00
	rjmp fin_if

fin_if:
	rjmp start


validar_carga:
	;Valido si el pin A1 (seleccionar carga) recibe entrada. si la recibe, cambiar la carga
	sbis PINC, PINC1
	rjmp retornar

	inc carga

	cpi carga, 0x03
	brlo actualizar_carga
	;en caso de que la carga sea mayor a 0x02 (pesada) resetea el valor
	ldi carga, 0x00

	rjmp actualizar_carga

retornar: ret

actualizar_carga:
	;Leemos como estan los pines B, donde se encuentran los pines de carga
	in r31, PORTB
	;seteamos los pines 10, 11 y 12 (B2, B3, B4) en 0
	andi r31, 0b11100011 ;al usar and, los bits en 1 (1*1) se quedan en 1 y los bits en 0 (0*1) se quedan en 0.

	;Evaluamos que carga esta seleccionada para prender su led
	cpi carga, 0x00
	breq encender_ligera

	cpi carga, 0x01
	breq encender_media

	cpi carga, 0x02
	breq encender_pesada

	rjmp finalizar_acutalizar_carga

espera_carga:
	;Me quedo esperando hasta que el usuario suelte el boton
	sbic PINC, PINC1
	rjmp espera_carga

	rjmp retornar

encender_ligera:
	ori r31, (1<<PINB2)
	rjmp finalizar_acutalizar_carga

encender_media:
	ori r31, (1<<PINB3)
	rjmp finalizar_acutalizar_carga

encender_pesada:
	ori r31, (1<<PINB4)
	rjmp finalizar_acutalizar_carga

finalizar_acutalizar_carga:
	out PORTB, r31

	cpi estado, 0x00	;espera
	rjmp espera_carga

	cpi estado, 0x01	;lavado
	rjmp retornar

validar_agua:
	;Antes de mover el motor, el tambor tiene que estar lleno de agua (sensor de agua = 1)
	;Una vez se llena, ya no lo valido, ya que el agua puede variar por el movimiento
	sbis PINC, PINC3
	rjmp agua_no_llena

	ret


agua_no_llena:
	;Si el sensor no se activa, hacemos parpadear la luz de carga
	in r31, PORTB
	andi r31, 0b11100011
	out PORTB, r31
	rcall delay_500ms

	rcall actualizar_carga
	rcall delay_500ms

	rjmp validar_agua

delay_500ms:
	;Para 500ms necesitamos 16 MHz * 500 ms = 8.000.000 de ciclos
	;Ciclos1 = 3*N1 - 1 (max 764 ciclos)
	;Ciclos2 = N2*(3*N1 + 3) - 1 (max 195.839 ciclos)
	;Ciclos3 = N3*(N2*(3*N1 + 3) + 3) - 1 (max 49.939.964 ciclos)

	;factorizando, (Ciclos3 + 1)/3 = N3*(N2*(N1 + 1) + 1)

	;K = (8.000.000 + 1) / 3 = 2.666.667
	;M = N2 * (N1 + 1) + 1
	;K = N3 * M
	;M = K / N3 (tal que M y N3 sean redondos y maximizar N3 hasta 255)
	;N3 = 201 -> M = 2.666.667 / 201 = 13.267
	;M = N2 * (N1 + 1) + 1 -> (M - 1) = N2 * (N1 + 1)
	;13.266 / N2 = N1 + 1 (tal que N2 y N1 sean redondos y maximizar N2)
	;N2 = 201 -> 13.266 / 201 = 66 
	;N1 + 1 = 66 -> N1 = 66 - 1 = 65

	ldi r20, 201
l3:
	ldi r21, 201
l2:
	ldi r22, 65
l1:
	
	dec r22
	brne l1

	dec r21
	brne l2	

	dec r20
	brne l3
	
	ret