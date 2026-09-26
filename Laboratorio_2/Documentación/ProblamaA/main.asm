;TERCER COMMIT MATRIZ DE LED, IMPLEMENTACION UART CON MENU 

.include "m328Pdef.inc" 

.def temp      = r16 
.def col_idx   = r17 
.def offset    = r18 
.def max_off   = r19 
.def modo      = r20 
.def velocidad = r25 
.def min_off   = r15              ; Registro libre para inicio de scroll 

.cseg 
; Vector de Interrupciones 
.org 0x0000 
    rjmp RESET 
.org 0x0020              
    rjmp TIMER0_OVF_ISR    
.org 0x0024 
    rjmp USART_RX_ISR      


; RUTINA: guardar_codigos (Copia de Flash a SRAM) 

guardar_codigos: 
    ldi r30, low(TablaFlash * 2) 
    ldi r31, high(TablaFlash * 2) 
    ldi r28, 0x00                 ; YL 
    ldi r29, 0x01                 ; YH (SRAM inicio 0x0100) 
     
    ldi r23, 220                  ; Copiamos 220 bytes 
loop_copia: 
    lpm r24, Z+ 
    st Y+, r24 
    dec r23 
    brne loop_copia 
    ret 


;INICIALIZACIÓN 

RESET: 
    ; Configurar Stack Pointer 
    ldi temp, high(RAMEND) 
    out SPH, temp 
    ldi temp, low(RAMEND) 
    out SPL, temp 

    clr r1                        ; Registro r1 siempre en 0 
    rcall guardar_codigos         ; Copiar patrones a SRAM (0x0100) 

    ; Configuración de Direcciones de Puertos (DDR) 
    ldi temp, 0x3F                ; PB0...PB5 salidas (Columnas 1 a 6) 
    out DDRB, temp                 
    ldi temp, 0xFC                ; PD2..PD7 salidas (Filas 3 a 8), PD0/PD1 UART 
    out DDRD, temp                 
    ldi temp, 0x0F                ; PC0..PC1 salidas (filas 1-2), PC2..PC3 salidas (cols 7-8) 
    out DDRC, temp 

    ; Apagar la matriz al inicio (LÓGICA INVERSA) 
    ldi temp, 0x00                 
    out PORTB, temp               ; Columnas PB0..PB5 apagadas (LOW) 
    ldi temp, 0xFC                 
    out PORTD, temp               ; Filas PD2..PD7 apagadas (HIGH) 
    ldi temp, 0x03                 
    out PORTC, temp               ; Filas PC0..PC1 apagadas (HIGH), Cols PC2..PC3 apagadas (LOW) 

    ; Variables Iniciales 
    clr col_idx           
    clr offset           
    clr min_off                   ; Comienza a leer desde 0 por defecto 
    ldi max_off, 172              ; Límite exclusivo para el Mensaje 1 
    ldi velocidad, 15             ; Velocidad inicial por defecto 
     
    ldi temp, 0xFF 
    mov modo, temp                ; Estado de espera inicial 

    rcall Init_UART 
    rcall Mostrar_Menu 

    ; Configurar Timer0 Prescaler = 64 para ~122Hz de refresco total 
    ldi temp, (1<<CS01)|(1<<CS00)  
    out TCCR0B, temp 
    ldi temp, (1<<TOIE0)           
    sts TIMSK0, temp 
     
    sei                           ; Habilitar Interrupciones Globales 


MAIN_LOOP: 
    ; Rutina de retardo para el desplazamiento del texto 
    mov r24, velocidad   
d1: ldi r21, 150 
d2: ldi r22, 200 
d3: dec r22 
    brne d3 
    dec r21 
    brne d2 
    dec r24 
    brne d1 

    ; Si está en modo estático (1, 2) o en espera (0xFF), no desplazar 
    cpi modo, 1 
    breq CONTINUE_SCROLL 
    cpi modo, 2 
    breq CONTINUE_SCROLL 
    cpi modo, 0xFF 
    breq CONTINUE_SCROLL 

    ; Solo se llega acá si es Modo 0 (Mensaje 1) o Modo 3 (Mensaje XD) 
    inc offset 
    cp offset, max_off 
    brne CONTINUE_SCROLL 
    mov offset, min_off           ; Reiniciar desplazamiento a su origen respectivo 

CONTINUE_SCROLL: 
    rjmp MAIN_LOOP 


; INTERRUPCIÓN TIMER0: MULTIPLEXADO Y CONTROL DE PANTALLA 

TIMER0_OVF_ISR: 
    push temp             
    in temp, SREG 
    push temp             
    push r30             
    push r31             
    push r23 
    push r24 

    ; 1.Apagar al incio 
    ldi temp, 0x00 
    out PORTB, temp 
     
    in r24, PORTC 
    andi r24, 0xF3                 ; Apagar columnas PC2 y PC3 (LOW) 
    ori r24, 0x03                  ; Apagar filas PC0 y PC1 (HIGH) 
    out PORTC, r24 
     
    in r24, PORTD 
    ori r24, 0xFC                  ; Apagar filas PD2..PD7 (HIGH) 
    out PORTD, r24 

    cpi modo, 0xFF 
    breq END_ISR 

    ; 2. OBTENER PATRÓN DIRECTO DE SRAM 
    ldi r30, low(0x0100) 
    ldi r31, high(0x0100) 
     
    ; Z = 0x0100 + offset + col_idx
    add r30, offset 
    adc r31, r1 
    add r30, col_idx 
    adc r31, r1 

    ld temp, Z                     ; Carga directa del byte de la columna

    ; 3. ACTIVAR FILAS (CÁTODOS: 0 = ENCENDIDO)
    com temp                       ; Invertir patrón: 1 pasa a 0 (Fila ON)

    in r24, PORTD 
    andi r24, 0x03 
    mov r23, temp 
    andi r23, 0xFC                 
    or r24, r23 
    out PORTD, r24 

    in r24, PORTC 
    andi r24, 0xFC 
    mov r23, temp 
    andi r23, 0x03                 
    or r24, r23 
    out PORTC, r24 

    ; 4. ACTIVAR LA COLUMNA CORRESPONDIENTE (ÁNODO: 1 = ENCENDIDO) 
    cpi col_idx, 6 
    brsh COL_ON_PORTC 

COL_ON_PORTB: 
    ldi temp, 1 
    mov r23, col_idx 
SHIFT_COL_PB: 
    cpi r23, 0 
    breq APPLY_PB 
    lsl temp 
    dec r23 
    rjmp SHIFT_COL_PB 
APPLY_PB: 
    out PORTB, temp 
    rjmp FIN_ISR 

COL_ON_PORTC: 
    cpi col_idx, 6 
    breq HIGH_PC2 
HIGH_PC3: 
    in r24, PORTC 
    ori r24, 0x08                  ; Activar columna PC3 (HIGH)
    out PORTC, r24 
    rjmp FIN_ISR 
HIGH_PC2: 
    in r24, PORTC 
    ori r24, 0x04                  ; Activar columna PC2 (HIGH)
    out PORTC, r24 

FIN_ISR: 
    inc col_idx 
    cpi col_idx, 8 
    brne END_ISR 
    clr col_idx 

END_ISR: 
    pop r24 
    pop r23 
    pop r31 
    pop r30 
    pop temp 
    out SREG, temp 
    pop temp 
    reti 


; INTERRUPCIÓN UART RX (MODO Y VELOCIDAD) 

USART_RX_ISR: 
    push temp 
    in temp, SREG 
    push temp 

    lds temp, UDR0                 

    cpi temp, '1' 
    breq modo0 
    cpi temp, '2' 
    breq modo1 
    cpi temp, '3' 
    breq modo2 
    cpi temp, '4' 
    breq modo3 
    cpi temp, '+' 
    breq subirvelocidad 
    cpi temp, '-' 
    breq bajarvelocidad 
    rjmp End_RXC_ISR               

modo0: 
    clr modo                      ; Modo 0: Texto principal 
    clr min_off 
    clr offset                     
    ldi max_off, 172 
    clr col_idx 
    rjmp End_RXC_ISR 

modo1: 
    ldi temp, 1 
    mov modo, temp                ; Modo 1: Corazón estático 
    ldi offset, 180                
    clr col_idx 
    rjmp End_RXC_ISR 

modo2: 
    ldi temp, 2 
    mov modo, temp                ; Modo 2: Carita feliz (:D) 
    ldi offset, 188                
    clr col_idx 
    rjmp End_RXC_ISR 

modo3: 
    ldi temp, 3 
    mov modo, temp                ; Modo 3: Mensaje "XD" Desplazable 
    ldi temp, 196                 ; El XD comienza en el byte 196 
    mov min_off, temp 
    mov offset, temp 
    ldi temp, 208                 ; Límite de scroll para el XD 
    mov max_off, temp 
    clr col_idx 
    rjmp End_RXC_ISR 

subirvelocidad: 
    cpi velocidad, 2              ; Límite máximo de velocidad 
    brlo End_RXC_ISR 
    dec velocidad 
    rjmp End_RXC_ISR 

bajarvelocidad: 
    cpi velocidad, 250            ; Límite mínimo de velocidad 
    brsh End_RXC_ISR 
    inc velocidad 
    rjmp End_RXC_ISR 

End_RXC_ISR: 
    pop temp 
    out SREG, temp 
    pop temp 
    reti 


; RUTINAS UART 
Init_UART: 
    ldi temp, 0 
    sts UBRR0H, temp 
    ldi temp, 103                 ; 9600 baudios @ 16MHz 
    sts UBRR0L, temp 
    ldi temp, (1<<RXEN0)|(1<<TXEN0)|(1<<RXCIE0) 
    sts UCSR0B, temp 
    ldi temp, (1<<UCSZ01)|(1<<UCSZ00) 
    sts UCSR0C, temp 
    ret 

Mostrar_Menu: 
    ldi ZL, low(msg_welcome*2) 
    ldi ZH, high(msg_welcome*2) 
    rcall Print_String 
    ldi ZL, low(msg_op1*2) 
    ldi ZH, high(msg_op1*2) 
    rcall Print_String 
    ldi ZL, low(msg_op2*2) 
    ldi ZH, high(msg_op2*2) 
    rcall Print_String 
    ldi ZL, low(msg_op3*2) 
    ldi ZH, high(msg_op3*2) 
    rcall Print_String 
    ldi ZL, low(msg_op4*2) 
    ldi ZH, high(msg_op4*2) 
    rcall Print_String 
    ldi ZL, low(msg_spd*2) 
    ldi ZH, high(msg_spd*2) 
    rcall Print_String 
    ldi ZL, low(msg_prompt*2) 
    ldi ZH, high(msg_prompt*2) 
    rcall Print_String 
    ret 

Print_String: 
    lpm temp, Z+ 
    tst temp 
    breq Print_String_End 
    rcall UART_Tx 
    rjmp Print_String 
Print_String_End: 
    ret 

UART_Tx: 
    push r16 
UART_Tx_Wait: 
    lds r16, UCSR0A 
    sbrs r16, UDRE0                
    rjmp UART_Tx_Wait 
    pop r16 
    sts UDR0, r16                  
    ret 

;MENU MATRIZ 
msg_welcome: .db " MENU MATRIZ LED DOLANG ", 13, 10, 0, 0 
msg_op1:     .db "1)-->Mensaje principal", 13, 10, 0, 0 
msg_op2:     .db "2)-->Corazon", 13, 10, 0, 0 
msg_op3:     .db "3)-->:D", 13, 10, 0 
msg_op4:     .db "4)-->XD", 13, 10, 0 
msg_spd:     .db "5)-->Ajustar velocidad +/-", 13, 10, 0, 0 
msg_prompt:  .db "Elige opcion: ", 0, 0 

; DATOS DE MATRIZ EN MEMORIA FLASH 

TablaFlash: 
    ; --- NO --- 
    .db 0x7F, 0x04, 0x08, 0x10, 0x7F, 0x00  
    .db 0x3E, 0x41, 0x41, 0x41, 0x3E, 0x00  
    .db 0x00, 0x00, 0x00, 0x00              
     
    ; --- MAS --- 
    .db 0x7F, 0x02, 0x04, 0x02, 0x7F, 0x00  
    .db 0x7C, 0x12, 0x11, 0x12, 0x7C, 0x00  
    .db 0x26, 0x49, 0x49, 0x49, 0x32, 0x00  
    .db 0x00, 0x00, 0x00, 0x00              

    ; --- EVALUACION --- 
    .db 0x7F, 0x49, 0x49, 0x49, 0x41, 0x00  
    .db 0x1F, 0x20, 0x40, 0x20, 0x1F, 0x00  
    .db 0x7C, 0x12, 0x11, 0x12, 0x7C, 0x00  
    .db 0x7F, 0x40, 0x40, 0x40, 0x40, 0x00  
    .db 0x3F, 0x40, 0x40, 0x40, 0x3F, 0x00  
    .db 0x7C, 0x12, 0x11, 0x12, 0x7C, 0x00  
    .db 0x3E, 0x41, 0x41, 0x41, 0x22, 0x00  
    .db 0x00, 0x41, 0x7F, 0x41, 0x00, 0x00  
    .db 0x3E, 0x41, 0x41, 0x41, 0x3E, 0x00  
    .db 0x7F, 0x04, 0x08, 0x10, 0x7F, 0x00  
    .db 0x00, 0x00, 0x00, 0x00              

    ; --- CONTINUA --- 
    .db 0x3E, 0x41, 0x41, 0x41, 0x22, 0x00  
    .db 0x3E, 0x41, 0x41, 0x41, 0x3E, 0x00  
    .db 0x7F, 0x04, 0x08, 0x10, 0x7F, 0x00  
    .db 0x01, 0x01, 0x7F, 0x01, 0x01, 0x00  
    .db 0x00, 0x41, 0x7F, 0x41, 0x00, 0x00  
    .db 0x7F, 0x04, 0x08, 0x10, 0x7F, 0x00  
    .db 0x3F, 0x40, 0x40, 0x40, 0x3F, 0x00  
    .db 0x7C, 0x12, 0x11, 0x12, 0x7C, 0x00  
    .db 0x00, 0x00, 0x00, 0x00              

    ; --- :) --- 
    .db 0x00, 0x36, 0x36, 0x00, 0x00, 0x00  
    .db 0x00, 0x41, 0x22, 0x1C, 0x00, 0x00  
    .db 0x00, 0x00, 0x00, 0x00              

    ;  ESPACIO AL FINAL DEL MENSAJE 1  
    .db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 

    ; --- IMAGEN 1: CORAZON  
    .db 0x00, 0x1C, 0x3E, 0x78, 0x78, 0x3E, 0x1C, 0x00 
     
    ; --- IMAGEN 2: :D 
    .db 0x00, 0x66, 0x66, 0x00, 0x7E, 0x7E, 0x7E, 0x3C 

    ; ---  "XD"  
    .db 0x63, 0x14, 0x08, 0x14, 0x63, 0x00  
    .db 0x7F, 0x41, 0x41, 0x22, 0x1C, 0x00  
    .db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00