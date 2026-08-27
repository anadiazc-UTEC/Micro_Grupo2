



.include "m328Pdef.inc"

.def estado = r16      ; Secuencia actual (1-8)
.def temp   = r17      ; Auxiliar general (patrones de LEDs)
.def dly1   = r20      ; Contador retardo - bucle externo
.def dly2   = r21      ; Contador retardo - bucle medio
.def dly3   = r22      ; Contador retardo - bucle interno
                                                                ;dly = delay    
.org 0x0000
    rjmp INICIO
                                                                    ;usé 3 buclés anidados para así poder lograr un retardo de 500ms
                                                                    ;usé .def para asignar nombres a los registros

INICIO:

    ; PORTD como salida -> 8 LEDs
    ldi temp, 0xFF ;Pone todo el registro en "unos"
    out DDRD, temp
    ldi temp, 0x00 ;Pone el registro en "ceros" 
    out PORTD, temp

    ; PB0, PB1, PB2 como entradas con pull-down botones
    ldi temp, 0x00  ;
    out DDRB, temp
    out PORTB, temp

    ldi estado, 1       ; Secuencia inicial


DESPACHADOR:
    ;cpi = compare immediate , estamos comparando el numero guardado en el registro r16 con una constante
    ;breq = branch if equal,  si el valor actual del estado es el mismo, salta a la subrutina, sino pasa a la siguiente instruccion
    cpi estado, 1
    breq SEC_1
   
    cpi estado, 2
    breq SEC_2
   
    cpi estado, 3
    breq SEC_3
   
    cpi estado, 4
    breq SEC_4
   
    cpi estado, 5
    breq SEC_5
   
    cpi estado, 6
    breq SEC_6
   
    cpi estado, 7
    breq SEC_7
   
    cpi estado, 8
    breq SEC_8
  
    rjmp DESPACHADOR     ;Esto es para evitar que no tenga un estado mayor a 8


SEC_1:
    ldi temp, 0xFF
    out PORTD, temp ; envia el valor que se cargó en temp al puerto D
    rcall RETARDO
    ldi temp, 0x00
    out PORTD, temp
    rcall RETARDO
    rjmp SEC_1  ;salto al incio de la secuencia para que quede en bucle

SEC_2:
    ldi temp, 0xAA
    out PORTD, temp
    rcall RETARDO
    ldi temp, 0x55
    out PORTD, temp
    rcall RETARDO
    rjmp SEC_2

SEC_3:
    ldi temp, 0x33
    out PORTD, temp
    rcall RETARDO
    ldi temp, 0xCC
    out PORTD, temp
    rcall RETARDO
    rjmp SEC_3

SEC_4:
    ldi temp, 0x01
SEC_4_BUCLE:
    out PORTD, temp
    rcall RETARDO
    lsl temp
    cpi temp, 0x00
    breq SEC_4
    rjmp SEC_4_BUCLE

SEC_5:
    ldi temp, 0x80                                                      ;se ejecuta una sola vez por fuera del bucle
SEC_5_BUCLE:                        ;se agrega para que podamos ver el patron de desplazamiento
    out PORTD, temp
    rcall RETARDO
    lsr temp
    cpi temp, 0x00
    breq SEC_5
    rjmp SEC_5_BUCLE

SEC_6:
    ldi temp, 0x03
SEC_6_BUCLE:
    out PORTD, temp
    rcall RETARDO
    lsl temp
    cpi temp, 0x00
    breq SEC_6
    rjmp SEC_6_BUCLE

SEC_7:
    ldi temp, 0xC0
SEC_7_BUCLE:
    out PORTD, temp
    rcall RETARDO
    lsr temp
    cpi temp, 0x00
    breq SEC_7
    rjmp SEC_7_BUCLE

SEC_8:
    ldi temp, 0x0F
SEC_8_BUCLE:
    out PORTD, temp
    rcall RETARDO
    swap temp
    rjmp SEC_8_BUCLE


; RETARDO 500 ms con lectura de botones 

RETARDO:
    ldi dly1, 201
RET_L3:
    ldi dly2, 201
RET_L2:
    ldi dly3, 65
RET_L1:
	
	sbic PINB, 0
	rjmp BTN_AVANZAR

    sbic PINB, 1
    rjmp BTN_RETROCEDER

    sbic PINB, 2
    rjmp BTN_RESET

    dec dly3
    brne RET_L1

    dec dly2
    brne RET_L2

    dec dly1
    brne RET_L3
    ret


; ACCIONES DE BOTONES

BTN_AVANZAR:
    sbic PINB, 0            ; esperar a que se suelte el boton
    rjmp BTN_AVANZAR

    inc estado
    cpi estado, 9
    brne APLICAR
    ldi estado, 1
    rjmp APLICAR

BTN_RETROCEDER:
    sbic PINB, 1
    rjmp BTN_RETROCEDER

    dec estado
    cpi estado, 0
    brne APLICAR
    ldi estado, 8

    rjmp APLICAR

BTN_RESET:
    sbic PINB, 2
    rjmp BTN_RESET

    ldi estado, 1
    rjmp APLICAR

APLICAR:
	rcall delay_boton
    rjmp DESPACHADOR

delay_boton: ;12ms
	ldi dly1, 255
l5:
	ldi dly2, 250
l4:
	
	dec dly2
	brne l4

	dec dly1
	brne l5	
	
	ret