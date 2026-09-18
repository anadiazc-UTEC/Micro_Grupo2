
; Primer arduino: EMISOR

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

	LDI R16, 0b00000000 
    OUT DDRB, R16 ; Configurar PORTB como ENTRADA para los switches
    OUT PORTB, R16 ; Desactiva las Pull-ups internas ya que los switches tienen pull down

	; Carga el valor del baud rate en registros
    LDI R16, LOW(bps)
    LDI R17, HIGH(bps)

    ; Llama a la subrutina de inicialización UART
    RCALL initUART
	
	in R20, PINB ; Guarda el valor de los pulsadores en r20
	andi R20, 0b00000111 ; Mascara para quedarme solo con el valor de los switch
	
loop:
	in r16, PINB ; Lee el valor de PINB
	andi r16, 0b00000111 ; Me quedo solamente con los tres bits de los switch 

	CP R16, R20 ; Verifica si cambio el estado del switch
    BREQ loop ; Si no cambio repite

	MOV R20, R16 ; Guardar el estado actual
    RCALL putc ; Transmitir el valor binario (0 a 7)
    RJMP loop ; Repite el bucle

initUART:
    STS UBRR0L, R16 ; Carga byte bajo del divisor del baud rate
    STS UBRR0H, R17 ; Carga byte alto
    LDI R16, (1 << TXEN0) ; Solo habilitar transmisor
    STS UCSR0B, R16   
    RET

putc:
    LDS R17, UCSR0A ; Lee el registro de estado A
    SBRS R17, UDRE0 ; ¿Buffer de transmisión vacío?
    RJMP putc ; Si no, espera
    STS UDR0, R16 ; Envía el carácter a transmitir
    LDI R16, 0 ; Limpia R16 (opcional)
    RET ; Retorna



