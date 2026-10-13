
#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define POT_MIN 100
#define POT_MAX 923
#define PWM_MIN 100
#define ZONA_MUERTA 5

int setPoint = 0;

uint16_t adc_ldr = 0;
uint16_t adc_pot = 0;

uint8_t pwm_val = 0;
int sentido = 0; // 0 = detenido, 1 = izquierda, 2 = derecha


void configurarPuertos(void) {
	// PC0 = LDR, PC1 = potenciometro
	DDRC &= ~((1 << PC0) | (1 << PC1));
	PORTC &= ~((1 << PC0) | (1 << PC1));

	// PD4 = IN1, PD5 = IN2, PD6 = OC0A (PWM)
	DDRD |= (1 << PD4) | (1 << PD5) | (1 << PD6);

	PORTD &= ~((1 << PD4) | (1 << PD5) | (1 << PD6));
}


void configurarUART(void) {
	// UART a 9600 baudios con F_CPU = 16 MHz
	UBRR0H = 0;
	UBRR0L = 103;

	UCSR0B = (1 << RXEN0) | (1 << TXEN0);
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}


void escribirUART(char dato) {
	while (!(UCSR0A & (1 << UDRE0)));
	UDR0 = dato;
}


void escribirUARTString(char *texto) {
	while (*texto) {
		escribirUART(*texto++);
	}
}


char leerUART(void) {
	while (!(UCSR0A & (1 << RXC0)));
	return UDR0;
}


void leerCadenaUART(char *datoRecibido, int tamano) {
	int i = 0;

	while (i < tamano - 1) {
		char dato = leerUART();

		if (dato == '\n' || dato == '\r') {
			break;
		}

		datoRecibido[i++] = dato;
	}

	datoRecibido[i] = '\0';
}


void elegirSetPoint(void) {
	char datoRecibido[10];

	escribirUARTString("Ingrese SetPoint (0-1023):\r\n");

	leerCadenaUART(datoRecibido, sizeof(datoRecibido));

	setPoint = atoi(datoRecibido);

	if (setPoint < 0) setPoint = 0;
	if (setPoint > 1023) setPoint = 1023;
}


void adc_init(void) {
	ADMUX = (1 << REFS0);

	ADCSRA = (1 << ADEN) |
	(1 << ADPS2) |
	(1 << ADPS1) |
	(1 << ADPS0);

	DIDR0 |= (1 << ADC0D) | (1 << ADC1D);
}


uint16_t adc_read(uint8_t canal) {
	ADMUX = (ADMUX & 0xF0) | (canal & 0x07);

	ADCSRA |= (1 << ADSC);

	while (ADCSRA & (1 << ADSC));

	return ADC;
}


void pwm_init(void) {
	// Timer0: Fast PWM no inversor sobre OC0A (PD6)
	TCCR0A = (1 << COM0A1) |
	(1 << WGM01) |
	(1 << WGM00);

	TCCR0B = (1 << CS01) | (1 << CS00);

	OCR0A = 0;
}


void detenerMotor(void) {
	PORTD &= ~((1 << PD4) | (1 << PD5));

	OCR0A = 0;

	pwm_val = 0;
	sentido = 0;
}


uint8_t calcularPWM(int error) {
	long magnitudError = labs((long)error);
	long auxPWM = (magnitudError * 255L) / 1023L;

	if (auxPWM > 255) {
		auxPWM = 255;
	}

	if (auxPWM > 0 && auxPWM < PWM_MIN) {
		auxPWM = PWM_MIN;
	}

	return (uint8_t)auxPWM;
}


int main(void) {
	configurarPuertos();
	configurarUART();
	adc_init();
	pwm_init();

	char informacionEnviar[64];

	// Esperar el setpoint enviado por Python a traves de UART.
	elegirSetPoint();

	while (1) {
		adc_ldr = adc_read(0);
		adc_pot = adc_read(1);

		int error = setPoint - (int)adc_ldr;

		detenerMotor();

		if (error > ZONA_MUERTA) {
			// Movimiento hacia el limite superior.
			if (adc_pot < POT_MAX) {
				PORTD |= (1 << PD4);
				PORTD &= ~(1 << PD5);

				sentido = 1;
				pwm_val = calcularPWM(error);
			}
		}
		else if (error < -ZONA_MUERTA) {
			// Movimiento hacia el limite inferior.
			if (adc_pot > POT_MIN) {
				PORTD &= ~(1 << PD4);
				PORTD |= (1 << PD5);

				sentido = 2;
				pwm_val = calcularPWM(error);
			}
		}

		OCR0A = pwm_val;

		// Formato: setpoint,LDR,potenciometro,PWM,sentido
		snprintf(informacionEnviar, sizeof(informacionEnviar),
		"%d,%u,%u,%u,%d\r\n",
		setPoint,
		(unsigned int)adc_ldr,
		(unsigned int)adc_pot,
		(unsigned int)pwm_val,
		sentido);

		escribirUARTString(informacionEnviar);

		_delay_ms(100);
	}

	return 0;
}