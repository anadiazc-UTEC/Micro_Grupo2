;
; AssemblerApplication1.asm
;
; Created: 21/08/2026 11:27:19
; Author : marti


.include "m328Pdef.inc"  

.def estado = r16        ; Registro para guardar la secuencia actual (1 a 8)
.def temp   = r17        ; Registro temporal
.def delay1 = r20        ; Contadores para el retardo
.def delay2 = r21
.def delay3 = r22


.org 0x0000
    rjmp INICIO          ; Vector de Reset, sirve para  saltar a la configuracion inicial


INICIO:
    ldi temp, high(RAMEND)
    out SPH, temp
    ldi temp, low(RAMEND)
    out SPL, temp

    ;Configurar PORTD como SALIDA (Para los 8 LEDs)
    ldi temp, 0xFF       ; 0xFF = 0b11111111 (Todos los pines como salida)
    out DDRD, temp
    ldi temp, 0x00       ; Iniciar con todos los LEDs apagados
    out PORTD, temp

    ;Configurar PORTB como ENTRADA 
										; PB0 = Boton 1 (Avanzar)
										; PB1 = Boton 2 (Retroceder)
										; PB2 = Boton 3 (Reset)
    ldi temp, 0x00       ; 0x00 = 0b00000000 (Todos los pines como entrada)
    out DDRB, temp
    ldi temp, 0x07       ; 0x07 = 0b00000111 (Activar resistencias Pull-up en PB0, PB1, PB2)
    out PORTB, temp

    ; 4. Inicializar Estado
    ldi estado, 1        ; Arrancamos en la Secuencia 1


DESPACHADOR:
    cpi estado, 1        ; pregunta si el estado actual es 1
    breq SEC_1           ; Si es igual salta a SEC_1
    
   

    rjmp DESPACHADOR     ; Falla de seguridad: si no coincide nada, vuelve a leer


; SECUENCIAS DE LEDs

SEC_1:
    ;(PARPADEO DE LEDS)
    ldi temp, 0xFF       ; Enciende todos los LEDs
    out PORTD, temp
    rcall RETARDO_BOTONES ; Llama al retardo 

    ldi temp, 0x00       ; Apaga todos los LEDs
    out PORTD, temp
    rcall RETARDO_BOTONES

    rjmp SEC_1           ; Repite el patron


; SUBRUTINA: RETARDO Y LECTURA DE BOTONES
RETARDO_BOTONES:
						
						
    ldi delay3, 15			 
Bucle_Externo:
    ldi delay2, 255
Bucle_Medio:
    ldi delay1, 255
Bucle_Interno:
    
    
    ;si el pin es 0 , el boton esta presionado.
    sbis PINB, 0         ; SBIS: Salta la siguiente instruccion si el bit 0 de PINB es 1
    rjmp BTN_AVANZAR

    sbis PINB, 1         ; Revisa Boton 2
    rjmp BTN_RETROCEDER

    sbis PINB, 2         ; Revisa Boton 3
    rjmp BTN_RESET

    ; --- CONTINUA EL RETARDO ---
    dec delay1
    brne Bucle_Interno
    dec delay2
    brne Bucle_Medio
    dec delay3
    brne Bucle_Externo

    ret                 


BTN_AVANZAR:
    ; Esperar a que el usuario suelte el boton 
    sbis PINB, 0
    rjmp BTN_AVANZAR
    
    inc estado           ; Suma 1 al estado
    cpi estado, 9        ; compara con 9 
    brne APLICAR_CAMBIO  ; Si no llego a 9, aplica el cambio
    ldi estado, 1        ; Si llego a 9, fuerza el retorno a 1 
    rjmp APLICAR_CAMBIO

BTN_RETROCEDER:
    sbis PINB, 1         ; Esperar a que suelte el boton
    rjmp BTN_RETROCEDER
    
    dec estado           ; Resta 1 al estado
    cpi estado, 0        ; Lleg a 0?
    brne APLICAR_CAMBIO
    ldi estado, 8        ; Si llego a 0, fuerza el salto a 8 (Wrap-around)
    rjmp APLICAR_CAMBIO

BTN_RESET:
    sbis PINB, 2         ; Esperar a que suelte el boton
    rjmp BTN_RESET
    ldi estado, 1        ; Fuerza el estado a 1
    rjmp APLICAR_CAMBIO

APLICAR_CAMBIO:
    ldi temp, high(RAMEND)
    out SPH, temp
    ldi temp, low(RAMEND)
    out SPL, temp

    rjmp DESPACHADOR     ; Vuelve al menu principal para ejecutar la nueva secuencia