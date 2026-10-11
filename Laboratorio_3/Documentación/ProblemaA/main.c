#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>

#define HEATER_PIN PB0   
#define FAN_PIN    PD6   
#define DHT_PIN    PD2   

#define MAX_TEMP_LIMITE 45
#define MIN_TEMP_LIMITE 5
#define LCD_ADDR   0x4E

// [INSERTAR AQUI LAS FUNCIONES I2C, LCD Y UART DEL COMMIT 1]
// ... (I2C_Init hasta UART_SendString) ...

// LECTURA DHT11
uint8_t DHT11_Read(int8_t *temp, int8_t *hum) {
    uint8_t data[5] = {0};
    uint8_t i, j;
    uint16_t timeout;

    DDRD |= (1 << DHT_PIN);
    PORTD &= ~(1 << DHT_PIN);
    _delay_ms(18);
    PORTD |= (1 << DHT_PIN);
    _delay_us(30);
    
    DDRD &= ~(1 << DHT_PIN); 
    
    timeout = 10000; while (PIND & (1 << DHT_PIN)) if (--timeout == 0) return 1;
    timeout = 10000; while (!(PIND & (1 << DHT_PIN))) if (--timeout == 0) return 1;
    timeout = 10000; while (PIND & (1 << DHT_PIN)) if (--timeout == 0) return 1;

    for (j = 0; j < 5; j++) {
        for (i = 0; i < 8; i++) {
            timeout = 10000; while (!(PIND & (1 << DHT_PIN))) if (--timeout == 0) return 1;
            _delay_us(30);
            if (PIND & (1 << DHT_PIN)) data[j] |= (1 << (7 - i));
            timeout = 10000; while (PIND & (1 << DHT_PIN)) if (--timeout == 0) return 1;
        }
    }
    if ((data[0] + data[1] + data[2] + data[3]) == data[4]) {
        *hum = data[0]; *temp = data[2]; return 0;
    }
    return 1;
}

// PWM Y CONTROL
void PWM_Init(void) {
    DDRD |= (1 << FAN_PIN);
    TCCR0A |= (1 << COM0A1) | (1 << WGM01) | (1 << WGM00);
    TCCR0B |= (1 << CS01) | (1 << CS00);
}
void Set_Fan_Speed(uint8_t speed) {
    if (speed == 0) {
        TCCR0A &= ~(1 << COM0A1); PORTD &= ~(1 << FAN_PIN); OCR0A = 0;
    } else {
        TCCR0A |= (1 << COM0A1); OCR0A = speed;
    }
}

int temp_min_medio = 16;
int temp_max_medio = 25;

void Actualizar_Control(int8_t temp, const char** accion) {
    if (temp < temp_min_medio) {
        PORTB |= (1 << HEATER_PIN); Set_Fan_Speed(0); *accion = "Calefactor ON  ";
    } else if (temp >= temp_min_medio && temp <= temp_max_medio) {
        PORTB &= ~(1 << HEATER_PIN); Set_Fan_Speed(0); *accion = "Rango Ideal   ";
    } else if (temp > temp_max_medio && temp <= 35) {
        PORTB &= ~(1 << HEATER_PIN); Set_Fan_Speed(85); *accion = "Ventilador BAJO";
    } else if (temp >= 36 && temp <= 45) {
        PORTB &= ~(1 << HEATER_PIN); Set_Fan_Speed(170); *accion = "Ventilador MEDIO";
    } else if (temp >= 46) {
        PORTB &= ~(1 << HEATER_PIN); Set_Fan_Speed(255); *accion = "Ventilador ALTO";
    }
}

int main(void) {
    DDRB |= (1 << HEATER_PIN); PORTB &= ~(1 << HEATER_PIN);
    PWM_Init(); Set_Fan_Speed(0);
    UART_Init(9600); LCD_Init();

    int8_t temperatura = 0, humedad = 0;
    char buffer[64];
    const char* accion = "Iniciando...";
    uint16_t contador_muestreo = 100;

    LCD_SetCursor(0, 0); LCD_Print("Iniciando...    ");
    _delay_ms(1500);

    while (1) {
        if (contador_muestreo >= 100) {
            contador_muestreo = 0;
            if (DHT11_Read(&temperatura, &humedad) == 0) {
                Actualizar_Control(temperatura, &accion);
                
                snprintf(buffer, sizeof(buffer), "DATA:%d,%d,%d,%d,%d\r\n",
                         temperatura, temp_min_medio, temp_max_medio,
                         (PORTB & (1 << HEATER_PIN)) ? 1 : 0, OCR0A);
                UART_SendString(buffer);

                LCD_SetCursor(0, 0);
                snprintf(buffer, sizeof(buffer), "Temp: %2d C    ", temperatura);
                LCD_Print(buffer);
                LCD_SetCursor(1, 0);
                snprintf(buffer, sizeof(buffer), "Rango: %2d-%2d C  ", temp_min_medio, temp_max_medio);
                LCD_Print(buffer);
            }
        }
        _delay_ms(50);
        contador_muestreo++;
    }
    return 0;
}