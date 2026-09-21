.include "m328Pdef.inc"


.def temp      = r16    ; Registro temporal de uso general
.def col_idx   = r17    
.def offset    = r18    ; Desplazamiento actual del mensaje
.def max_off   = r19    ; Longitud total del mensaje
.def delay_reg1= r20    ; Registros para retardo del scroll
.def delay_reg2= r21
.def delay_reg3= r22


.cseg
.org 0x0000
    rjmp RESET          

.org 0x0020           
    rjmp TIMER0_OVF_ISR


RESET:
  
    ldi temp, high(RAMEND)
    out SPH, temp
    ldi temp, low(RAMEND)
    out SPL, temp


    ldi temp, 0xFF
    out DDRB, temp      ; PORTB como salida Filas 
    out DDRD, temp      ; PORTD como salida Columnas 
    
    ; Apagar todo inicialmente
    ldi temp, 0x00
    out PORTB, temp     ; Filas a 0 
    ldi temp, 0xFF
    out PORTD, temp     ; Columnas a 1 

    ; 3. Inicializar variables
    clr col_idx         ; col_idx = 0
    clr offset          ; offset = 0
    ldi max_off, 90     ; Longitud aprox en columnas del mensaje en memoria

    ; Frecuencia de refresco
    ; Modo Normal, Prescaler = 64
    ldi temp, (1<<CS01) | (1<<CS00)
    out TCCR0B, temp
    
    ; Habilitar interrupción por desbordamiento (OVF) del Timer0
    ldi temp, (1<<TOIE0)
    sts TIMSK0, temp

    ; Habilitar interrupciones globales
    sei


MAIN_LOOP:
    ; Llamar a la subrutina de retardo (velocidad del scroll)
    rcall DELAY_SCROLL

    ; Incrementar el desplazamiento
    inc offset
    
    ; Comprobar si llegamos al final del mensaje
    cp offset, max_off
    brne CONTINUE_SCROLL
    clr offset          ; Reiniciar el mensaje si llegó al final

CONTINUE_SCROLL:
    rjmp MAIN_LOOP


TIMER0_OVF_ISR:
    push temp           ; Guardar estado del registro temp
    in temp, SREG
    push temp           ; Guardar registro de estado (SREG)
    push r30            ; Guardar Z Low
    push r31            ; Guardar Z High

    ; 1. Apagamos todas las columnas para evitar el efecto ghosting
    ldi temp, 0xFF
    out PORTD, temp
    
    ; Calculamos la dirección en memoria del dato a mostrar
    ; Direccion = Mensaje + offset + col_idx
    ldi r30, low(Mensaje * 2)   ; Cargar dirección base de la tabla (palabra a byte)
    ldi r31, high(Mensaje * 2)
    
    add r30, offset     ; Sumar el offset del scroll
    adc r31, r1         ; r1 debe estar en 0 
    
    add r30, col_idx    ; Sumar el índice de la columna actual (0 a 7)
    adc r31, r1
    
    ;  Leer el dato de la memoria de programa (Flash)
    lpm temp, Z
    
    ; Enviar el dato a las filas
    out PORTB, temp
    
    ; Activar la columna correspondiente
    
    ldi temp, 0x01      ; Cargar el valor inicial
    mov r23, col_idx
SHIFT_COL:
    cpi r23, 0
    breq OUT_COL
    lsl temp
    dec r23
    rjmp SHIFT_COL
OUT_COL:
    com temp            ; Invertir (asumimos cátodo común activo en bajo)
    out PORTD, temp     ; Encender la columna actual
    
    ; 6. Incrementar índice de columna para la próxima interrupción
    inc col_idx
    cpi col_idx, 8
    brne END_ISR
    clr col_idx         ; Volver a 0 si llega a 8

END_ISR:
    pop r31
    pop r30
    pop temp
    out SREG, temp
    pop temp
    reti


; Subrutina de Retardo (para controlar la velocidad del texto)

DELAY_SCROLL:
    ; Acá podemos ajustar los valores para cambiar su velocidad
    ldi delay_reg1, 30
d1: ldi delay_reg2, 255
d2: ldi delay_reg3, 255
d3: dec delay_reg3
    brne d3
    dec delay_reg2
    brne d2
    dec delay_reg1
    brne d1
    ret


Mensaje:
    ; M
    .db 0x7F, 0x02, 0x04, 0x02, 0x7F, 0x00
    ; I
    .db 0x00, 0x41, 0x7F, 0x41, 0x00, 0x00
    ; C
    .db 0x3E, 0x41, 0x41, 0x41, 0x22, 0x00
    ; R
    .db 0x7F, 0x09, 0x19, 0x29, 0x46, 0x00
    ; O
    .db 0x3E, 0x41, 0x41, 0x41, 0x3E, 0x00
    ; S
    .db 0x26, 0x49, 0x49, 0x49, 0x32, 0x00
    ; Espacio
    .db 0x00, 0x00, 0x00, 0x00
    ; L
    .db 0x7F, 0x40, 0x40, 0x40, 0x40, 0x00
    ; O
    .db 0x3E, 0x41, 0x41, 0x41, 0x3E, 0x00
    ; Espacio
    .db 0x00, 0x00, 0x00, 0x00
    ; M
    .db 0x7F, 0x02, 0x04, 0x02, 0x7F, 0x00
    ; E
    .db 0x7F, 0x49, 0x49, 0x49, 0x41, 0x00
    ; J
    .db 0x20, 0x40, 0x41, 0x3F, 0x01, 0x00
    ; O
    .db 0x3E, 0x41, 0x41, 0x41, 0x3E, 0x00
    ; R
    .db 0x7F, 0x09, 0x19, 0x29, 0x46, 0x00
    ; Espacios finales 
    .db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00