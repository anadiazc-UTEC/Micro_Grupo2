#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <avr/eeprom.h> // Librería para la memoria eeprom

// DEFINICIÓN DE PINES Y LÍMITES DE SEGURIDAD
#define HEATER_PIN PB0   // Pin del Calefactor (PB0 - Pin 8 Arduino)
#define FAN_PIN    PD6   // Pin PWM del Ventilador (PD6 - Pin 6 Arduino)
#define DHT_PIN    PD2   // Pin de datos del DHT11 (PD2 - Pin 2 Arduino)

#define MAX_TEMP_LIMITE 45
#define MIN_TEMP_LIMITE 5

// Dirección I2C del PCF8574 (0x27 << 1 = 0x4E)
#define LCD_ADDR   0x4E


// CONTROL I2C
void I2C_Init(void) {
	TWBR = 72; // Velocidad SCL a 100kHz para F_CPU = 16MHz
	TWSR = 0;
}

void I2C_Start(void) {
	TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
	while (!(TWCR & (1 << TWINT)));
}

void I2C_Stop(void) {
	TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);
}

void I2C_Write(uint8_t data) {
	TWDR = data;
	TWCR = (1 << TWINT) | (1 << TWEN);
	while (!(TWCR & (1 << TWINT)));
}


// CONTROL PANTALLA LCD I2C
void LCD_Write_I2C(uint8_t data) {
	I2C_Start();
	I2C_Write(LCD_ADDR);
	I2C_Write(data);
	I2C_Stop();
}

void LCD_Pulse_Enable(uint8_t data) {
	LCD_Write_I2C(data | 0x04);  // Enable HIGH
	_delay_us(1);
	LCD_Write_I2C(data & ~0x04); // Enable LOW
	_delay_us(50);
}

void LCD_Send(uint8_t value, uint8_t mode) {
	uint8_t highnib = (value & 0xF0) | mode | 0x08; // 0x08: Luz de fondo ON
	uint8_t lownib  = ((value << 4) & 0xF0) | mode | 0x08;
	
	LCD_Write_I2C(highnib);
	LCD_Pulse_Enable(highnib);
	LCD_Write_I2C(lownib);
	LCD_Pulse_Enable(lownib);
}

void LCD_Cmd(uint8_t cmd) {
	LCD_Send(cmd, 0x00);
}

void LCD_Char(uint8_t data) {
	LCD_Send(data, 0x01);
}

void LCD_Init(void) {
	I2C_Init();
	_delay_ms(50);
	
	// Forzar reseteo del controlador (asi evitamos bugs si Arduino reinicia pero LCD no)
	LCD_Write_I2C(0x38); LCD_Pulse_Enable(0x38); _delay_ms(5);
	LCD_Write_I2C(0x38); LCD_Pulse_Enable(0x38); _delay_ms(1);
	LCD_Write_I2C(0x38); LCD_Pulse_Enable(0x38); _delay_ms(1);
	
	LCD_Write_I2C(0x28); LCD_Pulse_Enable(0x28); _delay_ms(1);

	LCD_Cmd(0x28); // 2 líneas, formato 5x7
	LCD_Cmd(0x0C); // Display ON, Cursor OFF
	LCD_Cmd(0x06); // Incremento de cursor
	LCD_Cmd(0x01); // Limpiar pantalla
	_delay_ms(2);
}

void LCD_SetCursor(uint8_t row, uint8_t col) {
	uint8_t pos[] = {0x00, 0x40};
	LCD_Cmd(0x80 | (pos[row] + col));
}

void LCD_Print(const char* str) {
	while (*str) {
		LCD_Char(*str++);
	}
}


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
	
	DDRD &= ~(1 << DHT_PIN); // Entrada
	
	timeout = 10000;
	while (PIND & (1 << DHT_PIN)) if (--timeout == 0) return 1;
	timeout = 10000;
	while (!(PIND & (1 << DHT_PIN))) if (--timeout == 0) return 1;
	timeout = 10000;
	while (PIND & (1 << DHT_PIN)) if (--timeout == 0) return 1;

	for (j = 0; j < 5; j++) {
		for (i = 0; i < 8; i++) {
			timeout = 10000;
			while (!(PIND & (1 << DHT_PIN))) if (--timeout == 0) return 1;
			_delay_us(30);
			if (PIND & (1 << DHT_PIN)) {
				data[j] |= (1 << (7 - i));
			}
			timeout = 10000;
			while (PIND & (1 << DHT_PIN)) if (--timeout == 0) return 1;
		}
	}

	if ((data[0] + data[1] + data[2] + data[3]) == data[4]) {
		*hum = data[0];
		*temp = data[2];
		return 0;
	}
	return 1;
}


// PERIFÉRICOS PWM Y UART
void PWM_Init(void) {
	DDRD |= (1 << FAN_PIN);
	TCCR0A |= (1 << COM0A1) | (1 << WGM01) | (1 << WGM00); // Fast PWM
	TCCR0B |= (1 << CS01) | (1 << CS00);                   // Prescaler 64
}

void Set_Fan_Speed(uint8_t speed) {
	if (speed == 0) {
		TCCR0A &= ~(1 << COM0A1);
		PORTD &= ~(1 << FAN_PIN);
		OCR0A = 0;
		} else {
		TCCR0A |= (1 << COM0A1);
		OCR0A = speed;
	}
}

void UART_Init(unsigned int baud) {
	unsigned int ubrr = F_CPU / 16 / baud - 1;
	UBRR0H = (unsigned char)(ubrr >> 8);
	UBRR0L = (unsigned char)ubrr;
	UCSR0B = (1 << TXEN0) | (1 << RXEN0);
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void UART_SendString(const char* str) {
	while (*str) {
		while (!(UCSR0A & (1 << UDRE0)));
		UDR0 = *str++;
	}
}

uint8_t UART_ReceiveNonBlocking(char *c) {
	if (UCSR0A & (1 << RXC0)) {
		*c = UDR0;
		return 1;
	}
	return 0;
}

void ImprimirMenu(int min, int max) {
	char buf[80];
	UART_SendString("\r\n--- CONTROL DE TEMPERATURA ---\r\n");
	snprintf(buf, sizeof(buf), "Rango Ideal: %d C a %d C\r\n", min, max);
	UART_SendString(buf);
	UART_SendString("Comandos:\r\n");
	UART_SendString(" [+] Subir rango (1 C)\r\n");
	UART_SendString(" [-] Bajar rango (1 C)\r\n");
	UART_SendString(" [m] Ver menu\r\n");
	UART_SendString("------------------------------\r\n");
}


// LÓGICA DE CONTROL
int temp_min_medio = 16;
int temp_max_medio = 25;

void Actualizar_Control(int8_t temp, const char** accion) {
	if (temp < temp_min_medio) {
		PORTB |= (1 << HEATER_PIN);
		Set_Fan_Speed(0);
		*accion = "Calefactor ON  ";

		} else if (temp >= temp_min_medio && temp <= temp_max_medio) {
		PORTB &= ~(1 << HEATER_PIN);
		Set_Fan_Speed(0);
		*accion = "Rango Ideal   ";

		} else if (temp > temp_max_medio && temp <= 35) {
		PORTB &= ~(1 << HEATER_PIN);
		Set_Fan_Speed(85);
		*accion = "Ventilador BAJO";

		} else if (temp >= 36 && temp <= 45) {
		PORTB &= ~(1 << HEATER_PIN);
		Set_Fan_Speed(170);
		*accion = "Ventilador MEDIO";

		} else if (temp >= 46) {
		PORTB &= ~(1 << HEATER_PIN);
		Set_Fan_Speed(255);
		*accion = "Ventilador ALTO";
	}
}


// MAIN
int main(void) {
	DDRB |= (1 << HEATER_PIN);
	PORTB &= ~(1 << HEATER_PIN);

	PWM_Init();
	Set_Fan_Speed(0);
	
	UART_Init(9600);
	LCD_Init();

	
	// RECUPERAR RANGO DE LA MEMORIA EEPROM AL INICIAR

	uint8_t min_guardado = eeprom_read_byte((uint8_t*)0); // Dirección 0
	uint8_t max_guardado = eeprom_read_byte((uint8_t*)1); // Dirección 1


	if (min_guardado >= MIN_TEMP_LIMITE && max_guardado <= MAX_TEMP_LIMITE && min_guardado < max_guardado) {
		temp_min_medio = min_guardado;
		temp_max_medio = max_guardado;
	}
	// ----------------------------------------------------

	int8_t temperatura = 0;
	int8_t humedad = 0;
	char buffer[64];
	char rx_char;
	const char* accion = "Iniciando...";
	
	LCD_SetCursor(0, 0);
	LCD_Print("Iniciando...    ");

	ImprimirMenu(temp_min_medio, temp_max_medio);
	_delay_ms(1500);

	// Inicia en 100 para forzar lectura inmediata
	uint16_t contador_muestreo = 100;

	while (1) {
		// 1. Muestreo cada 5 segundos (100 * 50ms = 5000ms)
		if (contador_muestreo >= 100) {
			contador_muestreo = 0;

			if (DHT11_Read(&temperatura, &humedad) == 0) {
				Actualizar_Control(temperatura, &accion);

				// Trama para Python
				snprintf(buffer, sizeof(buffer), "DATA:%d,%d,%d,%d,%d\r\n",
				temperatura, temp_min_medio, temp_max_medio,
				(PORTB & (1 << HEATER_PIN)) ? 1 : 0, OCR0A);
				UART_SendString(buffer);

				// Actualizar LCD
				LCD_SetCursor(0, 0);
				snprintf(buffer, sizeof(buffer), "Temp: %2d C    ", temperatura);
				LCD_Print(buffer);

				LCD_SetCursor(1, 0);
				snprintf(buffer, sizeof(buffer), "Rango: %2d-%2d C  ", temp_min_medio, temp_max_medio);
				LCD_Print(buffer);
			}
		}

		// 2. Recepción de comandos (+, -, m)
		if (UART_ReceiveNonBlocking(&rx_char)) {
			uint8_t cambio = 0;

			if (rx_char == '+') {
				if (temp_max_medio < MAX_TEMP_LIMITE) {
					temp_min_medio++;
					temp_max_medio++;
					cambio = 1;
					} else {
					UART_SendString("\r\n[!] Limite maximo alcanzado.\r\n");
				}
			}
			else if (rx_char == '-') {
				if (temp_min_medio > MIN_TEMP_LIMITE) {
					temp_min_medio--;
					temp_max_medio--;
					cambio = 1;
					} else {
					UART_SendString("\r\n[!] Limite minimo alcanzado.\r\n");
				}
			}
			else if (rx_char == 'm' || rx_char == 'M') {
				ImprimirMenu(temp_min_medio, temp_max_medio);
			}

			// Si se modificó el rango, actualizar inmediatamente salidas, LCD y EEPROM
			if (cambio) {
				snprintf(buffer, sizeof(buffer), "\r\n>> Nuevo Rango: %d C - %d C\r\n", temp_min_medio, temp_max_medio);
				UART_SendString(buffer);

				Actualizar_Control(temperatura, &accion);

				LCD_SetCursor(1, 0);
				snprintf(buffer, sizeof(buffer), "Rango: %2d-%2d C  ", temp_min_medio, temp_max_medio);
				LCD_Print(buffer);

				// GUARDAR LOS NUEVOS LIMITES EN EEPROM
				eeprom_update_byte((uint8_t*)0, temp_min_medio); // Guardar en dirección 0
				eeprom_update_byte((uint8_t*)1, temp_max_medio); // Guardar en dirección 1
			}
		}

		_delay_ms(50);
		contador_muestreo++;
	}

	return 0;
}