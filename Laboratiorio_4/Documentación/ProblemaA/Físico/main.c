#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <avr/eeprom.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "spi.h"
#include "rc522.h"

#define LED_GREEN_PIN PD2
#define LED_RED_PIN   PD3
#define BUZZER_PIN    PD4
#define EEPROM_LLAVE 0x58

typedef enum {
    ESTADO_ESPERANDO_TARJETA,
    ESTADO_INGRESO_PIN,
    ESTADO_MENU_PRINCIPAL,
    ESTADO_RETIRO,
    ESTADO_DEPOSITO,
    ESTADO_SALDO
} EstadoCajero;

EstadoCajero estado_actual = ESTADO_ESPERANDO_TARJETA;
EstadoCajero estado_anterior = (EstadoCajero)-1;

typedef struct {
    uint8_t uid[4];
    char pin[5];
    uint32_t saldo;
} Cuenta;

Cuenta EEMEM cuentas_guardadas[3];
uint8_t EEMEM eeprom_flag_inicializada;
uint8_t cuenta_actual_index = 0;

#define PCF8574_ADDR 0x4E

void i2c_init(void) {
    TWSR = 0x00;
    TWBR = 72;
}

uint8_t i2c_start(void) {
    TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN);
    uint16_t timer = 1000;
    while (!(TWCR & (1<<TWINT))) {
        if (--timer == 0) return 0;
    }
    return 1;
}

void i2c_stop(void) {
    TWCR = (1<<TWINT) | (1<<TWSTO) | (1<<TWEN);
}

uint8_t i2c_write(uint8_t data) {
    TWDR = data;
    TWCR = (1<<TWINT) | (1<<TWEN);
    uint16_t timer = 1000;
    while (!(TWCR & (1<<TWINT))) {
        if (--timer == 0) return 0;
    }
    return 1;
}

void lcd_send_nibble(uint8_t nibble, uint8_t rs) {
    uint8_t data = (nibble & 0xF0) | (rs ? 0x01 : 0x00) | 0x08;
    if (!i2c_start()) return;
    i2c_write(PCF8574_ADDR);
    i2c_write(data | 0x04);
    _delay_us(1);
    i2c_write(data & ~0x04);
    i2c_stop();
    _delay_us(50);
}

void lcd_send_byte(uint8_t byte, uint8_t rs) {
    lcd_send_nibble(byte & 0xF0, rs);
    lcd_send_nibble((byte << 4) & 0xF0, rs);
}

void lcd_command(uint8_t cmd) { lcd_send_byte(cmd, 0); }
void lcd_char(char data) { lcd_send_byte(data, 1); }

void lcd_init(void) {
    i2c_init();
    _delay_ms(50);
    lcd_send_nibble(0x30, 0); _delay_ms(5);
    lcd_send_nibble(0x30, 0); _delay_us(150);
    lcd_send_nibble(0x30, 0);
    lcd_send_nibble(0x20, 0);
    lcd_command(0x28);
    lcd_command(0x0C);
    lcd_command(0x01);
    _delay_ms(2);
}

void lcd_clear(void) {
    lcd_command(0x01);
    _delay_ms(2);
}

void lcd_gotoxy(uint8_t x, uint8_t y) {
    uint8_t addr = (y == 0) ? (0x80 + x) : (0xC0 + x);
    lcd_command(addr);
}

void lcd_print(const char* str) {
    while (*str) lcd_char(*str++);
}

const char keypad_map[4][4] = {
    {'/', '*', '-', '+'},
    {'3', '6', '9', '='},
    {'2', '5', '8', '0'},
    {'1', '4', '7', 'C'}
};

void keypad_init(void) {
    DDRD |= (1<<PD5) | (1<<PD6) | (1<<PD7);
    DDRB |= (1<<PB0);
    PORTD |= (1<<PD5) | (1<<PD6) | (1<<PD7);
    PORTB |= (1<<PB0);
    DDRC &= ~((1<<PC0) | (1<<PC1) | (1<<PC2) | (1<<PC3));
    PORTC |= (1<<PC0) | (1<<PC1) | (1<<PC2) | (1<<PC3);
}

char keypad_get_key(void) {
    for (uint8_t f = 0; f < 4; f++) {
        PORTD |= (1<<PD5) | (1<<PD6) | (1<<PD7);
        PORTB |= (1<<PB0);
        if (f == 0) PORTD &= ~(1<<PD5);
        else if (f == 1) PORTD &= ~(1<<PD6);
        else if (f == 2) PORTD &= ~(1<<PD7);
        else if (f == 3) PORTB &= ~(1<<PB0);
        _delay_us(10);
        for (uint8_t c = 0; c < 4; c++) {
            if (!(PINC & (1<<c))) {
                _delay_ms(20);
                if (!(PINC & (1<<c))) {
                    char key = keypad_map[f][c];
                    uint16_t release_timer = 0;
                    while (!(PINC & (1<<c)) && release_timer < 500) {
                        _delay_ms(5);
                        release_timer++;
                    }
                    _delay_ms(20);
                    return key;
                }
            }
        }
    }
    return '\0';
}

void keypad_read_string(char* buffer, uint8_t max_len, uint8_t es_pin) {
    uint8_t idx = 0;
    buffer[0] = '\0';
    while (1) {
        char key = keypad_get_key();
        if (key != '\0') {
            if (key == '=') {
                buffer[idx] = '\0';
                break;
            } else if (key == 'C') {
                if (idx > 0) {
                    idx--;
                    buffer[idx] = '\0';
                    lcd_gotoxy(idx, 1);
                    lcd_char(' ');
                    lcd_gotoxy(idx, 1);
                }
            } else if (key >= '0' && key <= '9') {
                if (idx < max_len) {
                    buffer[idx] = key;
                    lcd_gotoxy(idx, 1);
                    lcd_char(es_pin ? '*' : key);
                    idx++;
                }
            }
        }
    }
}

void activar_exito(void) {
    PORTD |= (1<<LED_GREEN_PIN);
    _delay_ms(1000);
    PORTD &= ~(1<<LED_GREEN_PIN);
}

void activar_error(void) {
    PORTD |= (1<<LED_RED_PIN);
    _delay_ms(1500);
    PORTD &= ~(1<<LED_RED_PIN);
}

void activar_bloqueo_buzzer(void) {
    PORTD |= (1<<LED_RED_PIN) | (1<<BUZZER_PIN);
    _delay_ms(3000);
    PORTD &= ~((1<<LED_RED_PIN) | (1<<BUZZER_PIN));
}