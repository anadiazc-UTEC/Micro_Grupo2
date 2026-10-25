#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>

void uart_init(unsigned int baud) {
    unsigned int ubrr = F_CPU / 16 / baud - 1;
    UBRR0H = (unsigned char)(ubrr >> 8);
    UBRR0L = (unsigned char)ubrr;
    UCSR0B = (1 << TXEN0); // Habilitar Transmisión (TX)
    UCSR0C = (1 << USBS0) | (3 << UCSZ00); // 8 bits, 2 bits de parada
}

void uart_send_byte(uint8_t data) {
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = data;
}

void enviar_uid(uint8_t u0, uint8_t u1, uint8_t u2, uint8_t u3) {
    uart_send_byte(u0);
    uart_send_byte(u1);
    uart_send_byte(u2);
    uart_send_byte(u3);
}

int main(void) {
    uart_init(9600);

    // Configurar PD2, PD3 y PD4 como entradas con resistencia Pull-Up
    DDRD &= ~((1 << PD2) | (1 << PD3) | (1 << PD4));
    PORTD |= (1 << PD2) | (1 << PD3) | (1 << PD4);

    while (1) {
        // Botón 1 presionado (Tarjeta 1: DE AD BE EF)
        if (!(PIND & (1 << PD2))) {
            enviar_uid(0xDE, 0xAD, 0xBE, 0xEF);
            _delay_ms(500); // Anti-rebote
        }
        // Botón 2 presionado (Tarjeta 2: 11 22 33 44)
        else if (!(PIND & (1 << PD3))) {
            enviar_uid(0x11, 0x22, 0x33, 0x44);
            _delay_ms(500);
        }
        // Botón 3 presionado (Tarjeta 3: AA BB CC DD)
        else if (!(PIND & (1 << PD4))) {
            enviar_uid(0xAA, 0xBB, 0xCC, 0xDD);
            _delay_ms(500);
        }
    }
    return 0;
}