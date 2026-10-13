#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdlib.h>

int setPoint = 0;
uint16_t adc_ldr = 0;
uint16_t adc_pot = 0;
uint8_t pwm_val = 0;
int sentido = 0; //0 = Detenido, 1 = Izquierda, 2 = Derecha

void configurarPuertos(void) {
	// Entradas analógicas PC0 (LDR) y PC1 (Potenciómetro)
	DDRC &= ~((1 << PC0) | (1 << PC1));
	
	// Salidas digitales PD4 (IN1), PD5 (IN2) y PD6 (PWM)
	DDRD |= (1 << PD4) | (1 << PD5) | (1 << PD6);
	
	PORTD &= ~((1 << PD4) | (1 << PD5) | (1 << PD6));
}

void configurarUART(void) {
	UBRR0H = (unsigned char)(103 >> 8);
	UBRR0L = (unsigned char)(103);

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
	tamano -= 1; // Ajuste para dejar espacio al terminador nulo '\0'
	
	while (i < tamano) {
		char dato = leerUART();
		if (dato == '\n' || dato == '\r') {
			break;
		}
		datoRecibido[i] = dato;
		i++;
	}
	datoRecibido[i] = '\0';
}

void elegirSetPoint(void) {
	char datoRecibido[10];
	escribirUARTString("\r\n");
	escribirUARTString("Ingrese SetPoint (0-1023): ");
	
	leerCadenaUART(datoRecibido, 10);
	setPoint = atoi(datoRecibido);
}

void adc_init(void) {
	ADMUX = (1 << REFS0);
	ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

uint16_t adc_read(uint8_t canal) {
	ADMUX = (ADMUX & 0xF0) | (canal & 0x07);
	ADCSRA |= (1 << ADSC);
	while (ADCSRA & (1 << ADSC));
	return ADC;
}

void pwm_init(void) {
	// Timer0 en PD6 (OC0A) en modo Fast PWM
	TCCR0A = (1 << COM0A1) | (1 << WGM01) | (1 << WGM00);
	TCCR0B = (1 << CS01) | (1 << CS00);
	OCR0A = 0;
}

int main(void) {
	configurarPuertos();
	configurarUART();
	adc_init();
	pwm_init();

	char informacionEnviar[64];

	// Espera inicial por el setpoint
	elegirSetPoint();

	while (1) {
		// Lectura de los dos canales del ADC
		adc_ldr = adc_read(0); // PC0 (LDR)
		adc_pot = adc_read(1); // PC1 (Potenciómetro)

		// Cálculo del error según diagrama de flujo
		int error = setPoint - adc_ldr;

		if (error > 0) {
			sentido = 1; // Giro Izquierda
			PORTD |= (1 << PD4);
			PORTD &= ~(1 << PD5);
			
			long auxPWM = (error * 255) / 1023;
			if (auxPWM > 255) {
				auxPWM = 255;
			}
			pwm_val = auxPWM;
		}
		if (error < 0) {
			sentido = 2; // Giro Derecha
			PORTD &= ~(1 << PD4);
			PORTD |= (1 << PD5);
			
			long auxPWM = ((-error) * 255) / 1023;
			if (auxPWM > 255) {
				auxPWM = 255;
			}
			pwm_val = auxPWM;
		}
		if (error == 0) {
			sentido = 0; // Detenido
			PORTD &= ~((1 << PD4) | (1 << PD5));
			pwm_val = 0;
		}

		OCR0A = pwm_val;

		// Telemetría formateada: Setpoint, LDR, Pot, PWM, Sentido (0, 1 o 2)
		sprintf(informacionEnviar, "%d,%d,%d,%d,%d\r\n", setPoint, adc_ldr, adc_pot, pwm_val, sentido);
		escribirUARTString(informacionEnviar);

		_delay_ms(100);
	}

	return 0;
}