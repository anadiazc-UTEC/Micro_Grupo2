/*
 * Lab4ParteD.c
 *
 * Created: 14/10/2026 16:50:06
 * Author : anidi
 * Arriba = 0 = Amarillo
 * Abajo = 1 = Azul
 * Izquierda = 2 = Verde
 * Derecha = 3 = Rojo
 */

#define F_CPU 16000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <util/delay.h>
#include <stdlib.h>
#include <stdint.h>

#define LED_PIN 6
#define NUM_LEDS 64
#define MAX_RONDAS 10
#define BAJO 350
#define ALTO 650
#define EEPROM_RECORD 0x00
#define EEPROM_NIVEL 0x01

uint8_t recordHistorico = 1;
uint8_t nivelGuardado = 1;
uint8_t leds[NUM_LEDS][3];

// Maquina de estados
typedef enum {
	EST_INICIALIZACION,
	EST_MODO_LIBRE,
	EST_GENERAR_SECUENCIA,
	EST_MOSTRAR_SECUENCIA,
	EST_ESPERAR_RESPUESTA,
	EST_ACIERTO,
	EST_DERROTA,
	EST_VICTORIA
} EstadoSistema;

void uart_init(unsigned int ubrr) {
	UBRR0H = (unsigned char)(ubrr >> 8);
	UBRR0L = (unsigned char)ubrr;
	UCSR0B = (1 << TXEN0) | (1 << RXEN0);
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void uart_send(char c) {
	while (!(UCSR0A & (1 << UDRE0)));
	UDR0 = c;
}

void uart_print(const char *s) {
	while (*s) uart_send(*s++);
}

void transmitirNumero(uint8_t numero) {
	char texto[4];
	itoa(numero, texto, 10);
	uart_print(texto);
}

void cargarEEPROM(void) {
	recordHistorico = eeprom_read_byte((uint8_t*)EEPROM_RECORD);
	nivelGuardado = eeprom_read_byte((uint8_t*)EEPROM_NIVEL);

	if (recordHistorico == 0xFF || recordHistorico > MAX_RONDAS) {
		recordHistorico = 1;
		eeprom_update_byte((uint8_t*)EEPROM_RECORD, 1);
	}

	if (nivelGuardado == 0xFF || nivelGuardado > MAX_RONDAS) {
		nivelGuardado = 1;
		eeprom_update_byte((uint8_t*)EEPROM_NIVEL, 1);
	}
}

void guardarProgresoEEPROM(uint8_t nivelActual) {
	nivelGuardado = nivelActual;
	eeprom_update_byte((uint8_t*)EEPROM_NIVEL, nivelActual);

	if (nivelActual > recordHistorico) {
		recordHistorico = nivelActual;
		eeprom_update_byte((uint8_t*)EEPROM_RECORD, recordHistorico);
	}
}

void setLedRGB(int ledIndex, uint8_t r, uint8_t g, uint8_t b) {
	if (ledIndex < 0 || ledIndex >= NUM_LEDS) {
		return;
	}

	leds[ledIndex][0] = r;
	leds[ledIndex][1] = g;
	leds[ledIndex][2] = b;
}

void sendBit(uint8_t bitVal) {
	if (bitVal) {
		PORTD |= (1 << LED_PIN);
		asm volatile (
			"nop\n\t""nop\n\t""nop\n\t""nop\n\t"
			"nop\n\t""nop\n\t""nop\n\t""nop\n\t"
		);
		PORTD &= ~(1 << LED_PIN);
		asm volatile (
			"nop\n\t""nop\n\t""nop\n\t""nop\n\t"
		);
	} else {
		PORTD |= (1 << LED_PIN);
		asm volatile (
			"nop\n\t""nop\n\t""nop\n\t"
		);
		PORTD &= ~(1 << LED_PIN);
		asm volatile (
			"nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t"
			"nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t"
		);
	}
}

void sendByte(uint8_t byte) {
	for (uint8_t i = 0; i < 8; i++) {
		sendBit(byte & (1 << (7 - i)));
	}
}

void inicializarMatriz(void) {
	DDRD |= (1 << LED_PIN);
}

void show(uint8_t colors[NUM_LEDS][3]) {
	uint8_t estadoAnterior = SREG;
	cli();

	for (int i = 0; i < NUM_LEDS; i++) {
		sendByte(colors[i][1]);
		sendByte(colors[i][0]);
		sendByte(colors[i][2]);
	}

	SREG = estadoAnterior;
	_delay_us(60);
}

void limpiarMatriz(void) {
	for (int i = 0; i < NUM_LEDS; i++) {
		setLedRGB(i, 0, 0, 0);
	}
	show(leds);
}

void mostrarCursor(uint8_t x, uint8_t y, uint8_t color) {
	limpiarMatriz();

	if (x > 7 || y > 7) {
		return;
	}

	uint8_t r = 0;
	uint8_t g = 0;
	uint8_t b = 0;

	switch (color) {
		case 0:
			r = 255;
			g = 255;
			break;
		case 1:
			b = 255;
			break;
		case 2:
			g = 255;
			break;
		case 3:
			r = 255;
			break;
	}

	int ledIndex = y * 8 + x;
	setLedRGB(ledIndex, r, g, b);
	show(leds);
}

void configurarPuertos(void) {
	DDRC &= ~((1 << PC0) | (1 << PC1));
	PORTC &= ~((1 << PC0) | (1 << PC1));
	DDRD &= ~(1 << PD2);
	PORTD |= (1 << PD2);
	DDRD |= (1 << PD6);
	PORTD &= ~(1 << PD6);
	DDRB |= (1 << PB1);
	PORTB &= ~(1 << PB1);
}

void configurarTimers(void) {
	TCCR1A = 0;
	TCCR1B = 0;
	OCR1A = 0;
}

void fijarFrecuenciaMelodia(uint16_t frecuencia) {
	if (frecuencia == 0) {
		TCCR1B = 0;
		TCCR1A &= ~(1 << COM1A0);
		PORTB &= ~(1 << PB1);
		return;
	}

	uint16_t limite = (F_CPU / (2UL * 64UL * frecuencia)) - 1;
	OCR1A = limite;

	TCCR1A = (1 << COM1A0);
	TCCR1B = (1 << WGM12) | (1 << CS11) | (1 << CS10);
}

void apagarBuzzers(void) {
	TCCR1B = 0;
	TCCR1A &= ~(1 << COM1A0);
	PORTB &= ~(1 << PB1);

	TCCR2B = 0;
	TCCR2A &= ~((1 << COM2B1) | (1 << COM2B0));
	PORTD &= ~(1 << PD3);
}

void sonarColor(int color) {
	switch (color) {
		case 0:
			fijarFrecuenciaMelodia(330);
			break;
		case 1:
			fijarFrecuenciaMelodia(440);
			break;
		case 2:
			fijarFrecuenciaMelodia(262);
			break;
		case 3:
			fijarFrecuenciaMelodia(392);
			break;
		default:
			apagarBuzzers();
			break;
	}
}

void prenderColor(int color) {
	uint8_t r = 0;
	uint8_t g = 0;
	uint8_t b = 0;

	switch (color) {
		case 0:
			r = 255;
			g = 255;
			break;
		case 1:
			b = 255;
			break;
		case 2:
			g = 255;
			break;
		case 3:
			r = 255;
			break;
		default:
			limpiarMatriz();
			return;
	}

	for (int fila = 0; fila < 8; fila++) {
		for (int columna = 0; columna < 8; columna++) {
			int ledIndex = fila * 8 + columna;
			int iluminar = 0;

			switch (color) {
				case 0:
					if (fila < 4) iluminar = 1;
					break;
				case 1:
					if (fila >= 4) iluminar = 1;
					break;
				case 2:
					if (columna < 4) iluminar = 1;
					break;
				case 3:
					if (columna >= 4) iluminar = 1;
					break;
			}

			if (iluminar) {
				setLedRGB(ledIndex, r, g, b);
			} else {
				setLedRGB(ledIndex, 0, 0, 0);
			}
		}
	}

	show(leds);
}

void adc_init(void) {
	ADMUX = (1 << REFS0);
	ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

uint16_t adc_read(uint8_t channel) {
	ADMUX = (ADMUX & 0xF0) | (channel & 0x0F);
	ADCSRA |= (1 << ADSC);
	while (ADCSRA & (1 << ADSC));

	return ADC;
}

int leerDireccionJoystick(void) {
	uint16_t valorX = adc_read(0);
	uint16_t valorY = adc_read(1);

	if (valorY > ALTO) return 0; // Arriba
	if (valorY < BAJO) return 1; // Abajo
	if (valorX < BAJO) return 2; // Izquierda
	if (valorX > ALTO) return 3; // Derecha

	return -1;
}

int leerColorUsuario(void) {
	int colorSeleccionado = -1;
	int direccionActual;

	while (1) {
		direccionActual = leerDireccionJoystick();

		if (direccionActual != -1 && direccionActual != colorSeleccionado) {
			colorSeleccionado = direccionActual;
			prenderColor(colorSeleccionado);
		}

		if (!(PIND & (1 << PD2))) {
			_delay_ms(30);

			if (!(PIND & (1 << PD2))) {
				if (colorSeleccionado != -1) {
					while (!(PIND & (1 << PD2))) {
						_delay_ms(10);
					}

					_delay_ms(30);
					sonarColor(colorSeleccionado);
					_delay_ms(150);
					apagarBuzzers();
					limpiarMatriz();

					return colorSeleccionado;
				}
			}
		}
	}
}

int decidirColor(void) {
	return rand() % 4;
}

void perder(void) {
	for (int i = 0; i < 4; i++) {
		fijarFrecuenciaMelodia(150);
		prenderColor(3);
		_delay_ms(150);
		apagarBuzzers();
		limpiarMatriz();
		_delay_ms(150);
	}
}

void acertar(void) {
	fijarFrecuenciaMelodia(523);
	_delay_ms(100);
	fijarFrecuenciaMelodia(659);
	_delay_ms(150);
	apagarBuzzers();
	_delay_ms(200);
}

void rondaUno(void) {
	fijarFrecuenciaMelodia(262);
	_delay_ms(100);
	fijarFrecuenciaMelodia(330);
	_delay_ms(100);
	fijarFrecuenciaMelodia(392);
	_delay_ms(100);
	fijarFrecuenciaMelodia(523);
	_delay_ms(200);
	apagarBuzzers();
	_delay_ms(300);
}

void rondaFinal(void) {
	fijarFrecuenciaMelodia(880);
	_delay_ms(150);
	apagarBuzzers();
	_delay_ms(100);
	fijarFrecuenciaMelodia(880);
	_delay_ms(150);
	apagarBuzzers();
}

void ganar(void) {
	uint16_t notas[] = {262, 330, 392, 523, 659, 784};

	for (int i = 0; i < 6; i++) {
		fijarFrecuenciaMelodia(notas[i]);
		prenderColor(i % 4);
		_delay_ms(120);
	}

	apagarBuzzers();
	limpiarMatriz();
}

/* Arriba, Arriba, Abajo, Abajo, Izquierda, Derecha */
const uint8_t codigoKonami[6] = {1, 1, 0, 0, 2, 3};

void modoLibre(uint8_t *cursorX, uint8_t *cursorY, uint8_t *colorCursor) {
	mostrarCursor(*cursorX, *cursorY, *colorCursor);
	uint8_t progresocodigo = 0;

	while (1) {
		int dir = leerDireccionJoystick();

		if (dir != -1) {
			if (dir == 0 && *cursorY > 0) (*cursorY)--;
			if (dir == 1 && *cursorY < 7) (*cursorY)++;
			if (dir == 2 && *cursorX > 0) (*cursorX)--;
			if (dir == 3 && *cursorX < 7) (*cursorX)++;

			mostrarCursor(*cursorX, *cursorY, *colorCursor);

			if (dir == codigoKonami[progresocodigo]) {
				progresocodigo++;

				if (progresocodigo == 6) {
					fijarFrecuenciaMelodia(523);
					_delay_ms(100);
					fijarFrecuenciaMelodia(659);
					_delay_ms(100);
					fijarFrecuenciaMelodia(784);
					_delay_ms(150);
					apagarBuzzers();
					limpiarMatriz();
					return;
				}
			} else {
				if (dir == codigoKonami[0]) {
					progresocodigo = 1;
				} else {
					progresocodigo = 0;
				}
			}

			_delay_ms(150);

			while (leerDireccionJoystick() != -1) {
				_delay_ms(5);
			}
		}

		if (!(PIND & (1 << PD2))) {
			_delay_ms(30);

			if (!(PIND & (1 << PD2))) {
				while (!(PIND & (1 << PD2))) {
					_delay_ms(5);
				}

				_delay_ms(30);

				uint8_t nuevoColor;
				do {
					nuevoColor = rand() % 4;
				} while (nuevoColor == *colorCursor);

				*colorCursor = nuevoColor;
				mostrarCursor(*cursorX, *cursorY, *colorCursor);
				sonarColor(*colorCursor);
				_delay_ms(150);
				apagarBuzzers();
			}
		}
	}
}