
; Segundo arduino: RECEPTOR

.include "m328pdef.inc"

.equ F_CPU = 16000000 ; Frecuencia del reloj del microcontrolador (16 MHz)
.equ baud = 9600 ; Velocidad de comunicación
.equ bps = (F_CPU/16/baud) - 1 ; Valor del prescaler del baud rate (103 para 9600 baudios)

.org 0x0000
    rjmp inicio  

inicio:
	; Configuración del stack pointer
	ldi r16, HIGH(RAMEND) ; 0x08
	out SPH, r16
	ldi r16, LOW(RAMEND) ; 0xFF
	out SPL, r16

    ldi r16, 0b11111100
    out DDRD, r16 ; Configuro PORTD como salida, menos D0 y D1 (Tx y Rx)

    ldi r16, 0b00000011  
    out DDRC, r16 ; Configuro dos bits de PORTC como saldia, para reemplazar D0 y D1

	; Carga el valor del baud rate en registros
    LDI R16, LOW(bps)
    LDI R17, HIGH(bps)

    ; Llama a la subrutina de inicialización UART
    RCALL initUART

loop:
	
	rcall getc ; Espera hasta recibir un dato por UART
	andi r16, 0b00000111 ; Conserva solamente los 3 bits que pueden ser enviados (binario del 0 al 7)
	
	; Compara r16 (Donde se guardaron los valores a mostrar) con cada numero y si es igual salta a la rutina que le corresponde
    cpi r16, 0 
    breq mostrar_0

    cpi r16, 1
    breq mostrar_1

    cpi r16, 2
    breq mostrar_2

    cpi r16, 3
    breq mostrar_3

    cpi r16, 4
    breq mostrar_4

    cpi r16, 5
    breq mostrar_5

    cpi r16, 6
    breq mostrar_6

    cpi r16, 7
    breq mostrar_7

; Para cada numero guarda su valor en r17 (Para los numeros del 0 al 5) o en r19 (Para los numeros 6 y 7) y los muestra en su rutina correspondiente.
mostrar_0:
    ldi r17, 0b00000100
    ldi r19, 0
    out PORTC, r19
    out PORTD, r17
    rjmp loop

mostrar_1:
    ldi r17, 0b00001000
    ldi r19, 0
    out PORTC, r19
    out PORTD, r17
    rjmp loop

mostrar_2:
    ldi r17, 0b00010000
    ldi r19, 0
    out PORTC, r19
    out PORTD, r17
    rjmp loop

mostrar_3:
    ldi r17, 0b00100000
    ldi r19, 0
    out PORTC, r19
    out PORTD, r17
    rjmp loop

mostrar_4:
    ldi r17, 0b01000000
    ldi r19, 0
    out PORTC, r19
    out PORTD, r17
    rjmp loop

mostrar_5:
    ldi r17, 0b10000000
    ldi r19, 0
    out PORTC, r19
    out PORTD, r17
    rjmp loop

mostrar_6:
    ldi r17, 0b00000001
    ldi r19, 0
    out PORTC, r17
    out PORTD, r19
    rjmp loop

mostrar_7:
    ldi r17, 0b00000010
    ldi r19, 0
    out PORTC, r17
    out PORTD, r19
    rjmp loop


initUART:
    STS UBRR0L, R16 ; Carga byte bajo del divisor del baud rate
    STS UBRR0H, R17 ; Carga byte alto
    LDI R16, (1 << RXEN0)  ; Solo habilitar recepcion	
    STS UCSR0B, R16            
    RET

getc:
    LDS R17, UCSR0A ; Lee el registro de estado A
    SBRS R17, RXC0 ; ¿Se recibió un dato (bit RXC0 = 1)?
    RJMP getc ; Si no, sigue esperando
    LDS R16, UDR0 ; Lee el carácter recibido
    RET ; Retorna con el dato en R16


