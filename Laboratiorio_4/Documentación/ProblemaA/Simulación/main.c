#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <avr/eeprom.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define LED_GREEN_PIN PD2
#define LED_RED_PIN   PD3
#define BUZZER_PIN    PD4

#define llaveEeprom 0x55

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

void uartInit(unsigned int baud) {
    unsigned int ubrr = F_CPU / 16 / baud - 1;
    UBRR0H = (unsigned char)(ubrr >> 8);
    UBRR0L = (unsigned char)ubrr;
    UCSR0B = (1 << RXEN0) | (1 << TXEN0);
    UCSR0C = (1 << USBS0) | (3 << UCSZ00);
}

void uartFlush(void) {
    while (UCSR0A & (1 << RXC0)) {
        (void)UDR0;
    }
}

uint8_t uartReadUid(uint8_t *uid_buffer) {
    if (!(UCSR0A & (1 << RXC0))) {
        return 0;
    }

    for (uint8_t i = 0; i < 4; i++) {
        uint16_t timeout = 5000;
        while (!(UCSR0A & (1 << RXC0))) {
            _delay_us(10);
            timeout--;
            if (timeout == 0) {
                uartFlush();
                return 0;
            }
        }
        uid_buffer[i] = UDR0;
    }
    return 1;
}

#define PCF8574_ADDR 0x4E

void i2cInit(void) {
    TWSR = 0x00;
    TWBR = 72;
}

void i2cStart(void) {
    TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN);
    while (!(TWCR & (1<<TWINT)));
}

void i2cStop(void) {
    TWCR = (1<<TWINT) | (1<<TWSTO) | (1<<TWEN);
}

void i2cWrite(uint8_t data) {
    TWDR = data;
    TWCR = (1<<TWINT) | (1<<TWEN);
    while (!(TWCR & (1<<TWINT)));
}

void lcdSendNibble(uint8_t nibble, uint8_t rs) {
    uint8_t data = (nibble & 0xF0) | (rs ? 0x01 : 0x00) | 0x08;
    i2cStart();
    i2cWrite(PCF8574_ADDR);
    i2cWrite(data | 0x04);
    _delay_us(1);
    i2cWrite(data & ~0x04);
    i2cStop();
    _delay_us(50);
}

void lcdSendByte(uint8_t byte, uint8_t rs) {
    lcdSendNibble(byte & 0xF0, rs);
    lcdSendNibble((byte << 4) & 0xF0, rs);
}

void lcdCommand(uint8_t cmd) { lcdSendByte(cmd, 0); }
void lcdChar(char data) { lcdSendByte(data, 1); }

void lcdInit(void) {
    i2cInit();
    _delay_ms(50);
    lcdSendNibble(0x30, 0); _delay_ms(5);
    lcdSendNibble(0x30, 0); _delay_us(150);
    lcdSendNibble(0x30, 0);
    lcdSendNibble(0x20, 0);

    lcdCommand(0x28);
    lcdCommand(0x0C);
    lcdCommand(0x01);
    _delay_ms(2);
}

void lcdClear(void) {
    lcdCommand(0x01);
    _delay_ms(2);
}

void lcdGotoxy(uint8_t x, uint8_t y) {
    uint8_t addr = (y == 0) ? (0x80 + x) : (0xC0 + x);
    lcdCommand(addr);
}

void lcdPrint(const char* str) {
    while (*str) lcdChar(*str++);
}

const char keypad_map[4][4] = {
    {'7', '8', '9', '/'},
    {'4', '5', '6', '*'},
    {'1', '2', '3', '-'},
    {'C', '0', '=', '+'}
};

void keypadInit(void) {
    DDRD |= (1<<PD5) | (1<<PD6) | (1<<PD7);
    DDRB |= (1<<PB0);
    PORTD |= (1<<PD5) | (1<<PD6) | (1<<PD7);
    PORTB |= (1<<PB0);

    DDRC &= ~((1<<PC0) | (1<<PC1) | (1<<PC2) | (1<<PC3));
    PORTC |= (1<<PC0) | (1<<PC1) | (1<<PC2) | (1<<PC3);
}

char keypadGetKey(void) {
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
                while (!(PINC & (1<<c)));
                _delay_ms(20);
                return keypad_map[f][c];
            }
        }
    }
    return '\0';
}

void keypadReadString(char* buffer, uint8_t max_len, uint8_t es_pin) {
    uint8_t idx = 0;
    buffer[0] = '\0';

    while (1) {
        char key = keypadGetKey();
        if (key != '\0') {
            if (key == '=') {
                buffer[idx] = '\0';
                break;
            } else if (key == 'C' && idx > 0) {
                idx--;
                buffer[idx] = '\0';
                lcdGotoxy(idx, 1);
                lcdChar(' ');
                lcdGotoxy(idx, 1);
            } else if (key >= '0' && key <= '9' && idx < max_len) {
                buffer[idx] = key;
                lcdGotoxy(idx, 1);
                lcdChar(es_pin ? '*' : key);
                idx++;
            }
        }
    }
}

void activarExito(void) {
    PORTD |= (1<<LED_GREEN_PIN);
    _delay_ms(1000);
    PORTD &= ~(1<<LED_GREEN_PIN);
}

void activarError(void) {
    PORTD |= (1<<LED_RED_PIN);
    _delay_ms(1500);
    PORTD &= ~(1<<LED_RED_PIN);
}

void activarBloqueoBuzzer(void) {
    PORTD |= (1<<LED_RED_PIN) | (1<<BUZZER_PIN);
    _delay_ms(3000);
    PORTD &= ~((1<<LED_RED_PIN) | (1<<BUZZER_PIN));
}

int main(void) {
    uartInit(9600);
    lcdInit();
    keypadInit();

    DDRD |= (1<<LED_GREEN_PIN) | (1<<LED_RED_PIN) | (1<<BUZZER_PIN);

    uint8_t flag = eeprom_read_byte(&eeprom_flag_inicializada);
    if (flag != llaveEeprom) {
        Cuenta cuentas_iniciales[3] = {
            {{0xDE, 0xAD, 0xBE, 0xEF}, "1234", 5000},
            {{0x11, 0x22, 0x33, 0x44}, "5678", 1500},
            {{0xAA, 0xBB, 0xCC, 0xDD}, "0000", 300}
        };
        eeprom_update_block(&cuentas_iniciales, &cuentas_guardadas, sizeof(cuentas_iniciales));
        eeprom_write_byte(&eeprom_flag_inicializada, llaveEeprom);
    }

    uint8_t uid_leido[16];
    char entrada_texto[10];
    uint8_t intentos_pin = 0;
    Cuenta cuenta_actual;

    while (1) {
        if (estado_actual != estado_anterior) {
            lcdClear();
            if (estado_actual == ESTADO_ESPERANDO_TARJETA) {
                lcdGotoxy(0, 0);
                lcdPrint("   CAJERO ATM   ");
                lcdGotoxy(0, 1);
                lcdPrint("Acerque Tarjeta");
                uartFlush();
            } else if (estado_actual == ESTADO_INGRESO_PIN) {
                lcdGotoxy(0, 0);
                lcdPrint("Ingrese PIN (=):");
            } else if (estado_actual == ESTADO_MENU_PRINCIPAL) {
                lcdGotoxy(0, 0);
                lcdPrint("1:Saldo 2:Retiro");
                lcdGotoxy(0, 1);
                lcdPrint("3:Dep  4:Salir");
            }
            estado_anterior = estado_actual;
        }

        switch (estado_actual) {

            case ESTADO_ESPERANDO_TARJETA:
            memset(uid_leido, 0, sizeof(uid_leido));

            if (uartReadUid(uid_leido)) {
                uint8_t encontrada = 0;
                for (uint8_t i = 0; i < 3; i++) {
                    Cuenta temp;
                    eeprom_read_block(&temp, &cuentas_guardadas[i], sizeof(Cuenta));
                    if (memcmp(uid_leido, temp.uid, 4) == 0) {
                        cuenta_actual_index = i;
                        encontrada = 1;
                        break;
                    }
                }

                if (encontrada) {
                    activarExito();
                    intentos_pin = 0;
                    estado_actual = ESTADO_INGRESO_PIN;
                } else {
                    lcdClear();
                    lcdGotoxy(0, 0);
                    lcdPrint("  Tarjeta No");
                    lcdGotoxy(0, 1);
                    lcdPrint("  Registrada!");
                    activarError();
                    estado_anterior = (EstadoCajero)-1;
                }
            }
            break;

            case ESTADO_INGRESO_PIN:
            keypadReadString(entrada_texto, 4, 1);

            eeprom_read_block(&cuenta_actual, &cuentas_guardadas[cuenta_actual_index], sizeof(Cuenta));

            if (strcmp(entrada_texto, cuenta_actual.pin) == 0) {
                activarExito();
                estado_actual = ESTADO_MENU_PRINCIPAL;
            } else {
                intentos_pin++;

                if (intentos_pin >= 3) {
                    lcdClear();
                    lcdGotoxy(0, 0);
                    lcdPrint("Intentos Maximos");
                    lcdGotoxy(0, 1);
                    lcdPrint("   Bloqueado!");

                    activarBloqueoBuzzer();

                    estado_actual = ESTADO_ESPERANDO_TARJETA;
                } else {
                    lcdClear();
                    lcdGotoxy(0, 0);
                    lcdPrint("PIN Incorrecto!");
                    activarError();
                    estado_anterior = (EstadoCajero)-1;
                }
            }
            break;

            case ESTADO_MENU_PRINCIPAL:
            {
                char opcion = '\0';
                while (opcion < '1' || opcion > '4') {
                    opcion = keypadGetKey();
                }

                if (opcion == '1') estado_actual = ESTADO_SALDO;
                else if (opcion == '2') estado_actual = ESTADO_RETIRO;
                else if (opcion == '3') estado_actual = ESTADO_DEPOSITO;
                else if (opcion == '4') estado_actual = ESTADO_ESPERANDO_TARJETA;
            }
            break;

            case ESTADO_SALDO:
            eeprom_read_block(&cuenta_actual, &cuentas_guardadas[cuenta_actual_index], sizeof(Cuenta));
            lcdClear();
            lcdGotoxy(0, 0);
            lcdPrint("Saldo Actual:");
            lcdGotoxy(0, 1);

            char str_saldo[16];
            sprintf(str_saldo, "$%lu", cuenta_actual.saldo);
            lcdPrint(str_saldo);

            _delay_ms(3000);
            estado_actual = ESTADO_MENU_PRINCIPAL;
            break;

            case ESTADO_RETIRO:
            lcdClear();
            lcdGotoxy(0, 0);
            lcdPrint("Monto Retiro(=):");

            keypadReadString(entrada_texto, 6, 0);
            uint32_t monto_retiro = atol(entrada_texto);

            eeprom_read_block(&cuenta_actual, &cuentas_guardadas[cuenta_actual_index], sizeof(Cuenta));

            if (monto_retiro > 0 && monto_retiro <= cuenta_actual.saldo) {
                cuenta_actual.saldo -= monto_retiro;
                eeprom_update_block(&cuenta_actual, &cuentas_guardadas[cuenta_actual_index], sizeof(Cuenta));

                lcdClear();
                lcdGotoxy(0, 0);
                lcdPrint("Retiro Exitoso!");
                activarExito();
            } else {
                lcdClear();
                lcdGotoxy(0, 0);
                lcdPrint("     Saldo");
                lcdGotoxy(0, 1);
                lcdPrint(" Insuficiente!");
                activarError();
            }
            _delay_ms(2000);
            estado_actual = ESTADO_MENU_PRINCIPAL;
            break;

            case ESTADO_DEPOSITO:
            lcdClear();
            lcdGotoxy(0, 0);
            lcdPrint("Monto Dep. (=):");

            keypadReadString(entrada_texto, 6, 0);
            uint32_t monto_deposito = atol(entrada_texto);

            if (monto_deposito > 0) {
                eeprom_read_block(&cuenta_actual, &cuentas_guardadas[cuenta_actual_index], sizeof(Cuenta));
                cuenta_actual.saldo += monto_deposito;
                eeprom_update_block(&cuenta_actual, &cuentas_guardadas[cuenta_actual_index], sizeof(Cuenta));

                lcdClear();
                lcdGotoxy(0, 0);
                lcdPrint("Deposito Exitoso");
                activarExito();
            } else {
                lcdClear();
                lcdGotoxy(0, 0);
                lcdPrint("Monto Invalido!");
                activarError();
            }
            _delay_ms(2000);
            estado_actual = ESTADO_MENU_PRINCIPAL;
            break;
        }
    }
    return 0;
}