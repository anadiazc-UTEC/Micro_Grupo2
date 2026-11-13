/*
 * Lab5ParteC.c
 *
 * Created: 11/11/2026 10:45:59
 * Author : anidi
 *
 * Adquisicion de consumo usando el ADC interno con referencia de 1.1 V
 * y un shunt de 1 Ohm del lado de GND de la placa principal.
 *
 * Envia por la USART (38400 8N1), cada 10 ms, una linea CSV con enteros:
 *     tiempo_ms,adc_crudo\r\n
 * Los calculos (tension y corriente) se hacen en la PC (graficas.py), porque
 * snprintf de avr-libc no imprime float por defecto (imprime '?').
 */

#define F_CPU 16000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include <stdio.h>
#include <stdint.h>

// 38400 baudios: UBRR = 16e6 / (16 * 38400) - 1 = 25 (error 0,2 %)
// (115200 daba UBRR = 7 -> 125000 baudios reales, 8,5 % de error)
#define BAUD 38400UL
#define MYUBRR ((F_CPU / (16UL * BAUD)) - 1)

#define PERIODO_MUESTREO_MS 10   // 100 Hz

volatile uint32_t ms_ticks = 0;

// Timer0 en modo CTC: interrupcion cada 1 ms (16 MHz / 64 / 250)
static void timer0_init(void) {
    TCCR0A = (1 << WGM01);              // Modo CTC
    OCR0A = 249;                        // Interrupcion cada 1 ms
    TIMSK0 = (1 << OCIE0A);
    TCCR0B = (1 << CS01) | (1 << CS00); // Prescaler 64
}

ISR(TIMER0_COMPA_vect) {
    ms_ticks++;
}

// Lectura atomica: ms_ticks es de 32 bits y la ISR lo puede cambiar a mitad de la lectura
static uint32_t leer_ms(void) {
    uint32_t valor;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        valor = ms_ticks;
    }
    return valor;
}

static void usart_init(uint16_t ubrr) {
    UBRR0H = (uint8_t)(ubrr >> 8);
    UBRR0L = (uint8_t)ubrr;
    UCSR0A = 0;                             // Sin doble velocidad (U2X0 = 0)
    UCSR0B = (1 << TXEN0);                  // Solo transmision (TX)
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00); // 8 bits, sin paridad, 1 bit de parada
}

static void usart_transmit(char data) {
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = data;
}

static void usart_send_string(const char *str) {
    while (*str) {
        usart_transmit(*str++);
    }
}

// ADC con referencia interna de 1.1 V (REFS1 = 1, REFS0 = 1), prescaler 128 (125 kHz)
static void adc_init_1v1(void) {
    ADMUX = (1 << REFS1) | (1 << REFS0);
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

static uint16_t adc_read(uint8_t canal) {
    ADMUX = (ADMUX & 0xF0) | (canal & 0x0F);
    ADCSRA |= (1 << ADSC);
    while (ADCSRA & (1 << ADSC));
    return ADC;
}

int main(void) {
    usart_init(MYUBRR);
    adc_init_1v1();
    timer0_init();

    // Primera conversion descartada: la referencia de 1.1 V necesita estabilizarse
    (void)adc_read(0);

    sei();

    char buffer[24];
    uint32_t ultimo_envio = 0;

    while (1) {
        uint32_t ahora = leer_ms();

        // Muestreo cada 10 ms (100 Hz)
        if ((ahora - ultimo_envio) >= PERIODO_MUESTREO_MS) {
            ultimo_envio = ahora;

            uint16_t adc_raw = adc_read(0); // Tension en el shunt, entrada ADC0 (A0)

            // Formato CSV con enteros: tiempo_ms,adc_crudo
            snprintf(buffer, sizeof(buffer), "%lu,%u\r\n",
                     (unsigned long)ahora, adc_raw);
            usart_send_string(buffer);
        }
    }
    return 0;
}
