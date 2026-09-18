.include "m328pdef.inc"

.equ F_CPU = 16000000          ; Frecuencia del reloj del microcontrolador (16 MHz)
.equ baud = 9600               ; Velocidad de comunicación
.equ bps = (F_CPU/16/baud) - 1 ; Valor del prescaler del baud rate (103 para 9600 baudios)

.org 0x0000
    rjmp inicio  
    
inicio:

	ldi r16, HIGH(RAMEND) ; 0x08
	out SPH, r16
	ldi r16, LOW(RAMEND) ; 0xFF
	out SPL, r16

	; Carga el valor del baud rate en registros
    LDI R16, LOW(bps)
    LDI R17, HIGH(bps)

    ; Llama a la subrutina de inicialización UART
    RCALL initUART
	
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
