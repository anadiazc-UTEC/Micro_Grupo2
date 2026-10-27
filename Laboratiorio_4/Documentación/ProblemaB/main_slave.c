#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdbool.h>
#include <xc.h>

#define MOTOR_PWM	PIND6
#define MOTOR_IN1	PIND7
#define MOTOR_IN2	PINB0
#define SERVO_PWM	PINB1
#define LED_VERDE	PIND2
#define LED_ROJO	PIND3
#define BUZZER		PIND4

#define CMD_STOP			0x00
#define CMD_START			0x01
#define CMD_SET_VELOCIDAD	0x02
#define CMD_SET_PISTON		0x03
#define CMD_EMERGENCIA		0x04

#define ACK_OK	0x06
#define ACK_ERR	0x15

volatile uint8_t spi_contador = 0;
volatile uint8_t comando = 0;
volatile uint8_t valor = 0;
volatile bool nuevo_comando = true;

void servo_init(void) {
	DDRB |= (1 << PINB1);
	TCCR1A = (1 << COM1A1) | (1 << WGM11);
	TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS11);
	ICR1 = 39999;
}

void mover_servo_grados(uint16_t angulo) {
	// Convierte el angulo (0 a 180) en valor de registro OCR1A (2000 a 4000)
	// 0	-> 1000 (0.5 ms)
	// 90	-> 3000 (1.5 ms)
	// 180	-> 5000 (2.5 ms)
	OCR1A = 1000 + ((uint32_t)angulo * 4000) / 180;
}

void timer0_pwm_init(void) {
	TCCR0A = (1 << COM0A1) | (1 << WGM01) | (1 << WGM00);
	TCCR0B = (1 << CS01) | (1 << CS00);
	OCR0A = 0;
}

void spi_init(void) {
	DDRB |= (1 << PINB4);
	DDRB &= ~((1 << PINB2) | (1 << PINB3) | (1 << PINB5));
	SPCR = (1 << SPE) | (1 << SPIE);
	SPDR = 0x00;
}

ISR(SPI_STC_vect) {
	
	uint8_t dato = SPDR;
	
	if (spi_contador == 0) {
		comando = dato;
		spi_contador = 1;
		SPDR = 0x00;
	} else if (spi_contador == 1) {
		valor = dato;
		spi_contador = 2;
		nuevo_comando = true;
		
		if (comando == CMD_STOP || 
			comando == CMD_START ||
			comando == CMD_SET_PISTON ||
			comando == CMD_SET_VELOCIDAD ||
			comando == CMD_EMERGENCIA){
				
			SPDR = ACK_OK;
		}else {
			SPDR = ACK_ERR;
		}
	} else {
		spi_contador = 0;
		SPDR = 0x00;
	}
}

void establecer_motor(uint8_t enable, uint8_t velocidad) {
	if (enable) {
		PORTD |= (1 << MOTOR_IN1);
		PORTB &= ~(1 << MOTOR_IN2);
		OCR0A = velocidad;
	} else {
		PORTD &= ~(1 << MOTOR_IN1);
		PORTB &= ~(1 << MOTOR_IN2);
		OCR0A = 0;
	}
}


void init_all(void){
	
	DDRD |= (1 << MOTOR_PWM) | (1 << MOTOR_IN1) | (1 << LED_VERDE) | (1 << LED_ROJO) | (1 << BUZZER);
	DDRB |= (1 << MOTOR_IN2) | (1 << SERVO_PWM);

	PORTD &= ~((1 << LED_VERDE) | (1 << LED_ROJO) | (1 << BUZZER));
	
	timer0_pwm_init();
	servo_init();
	spi_init();
	
	sei();
	
}

int main(void)
{
	init_all();
	bool emergencia = false;
    while(1)
    {
		if (nuevo_comando){
			nuevo_comando = false;
			switch (comando){
				case CMD_STOP:
					emergencia = false;
					establecer_motor(0, 0);
					mover_servo_grados(0);
					PORTD &= ~((1 << LED_VERDE) | (1 << BUZZER) | (1 << LED_ROJO));
					break;
				case CMD_START:
					emergencia = false;
					PORTD |= (1 << LED_VERDE);
					PORTD &= ~((1 << LED_ROJO) | (1 << BUZZER));
					establecer_motor(1, OCR0A);
					break;
				case CMD_SET_PISTON:
					if (!emergencia){
						mover_servo_grados(valor);
					}
					break;
				case CMD_SET_VELOCIDAD:
					if (!emergencia) {
						OCR0A = valor;
					}
					break;
				case CMD_EMERGENCIA:
					emergencia = true;
					establecer_motor(0, 0);
					mover_servo_grados(0);
					PORTD &= ~(1 << LED_VERDE);
					break;
			}
		}
		
		if (emergencia) {
			PORTD ^= (1 << LED_ROJO);
			PORTD ^= (1 << BUZZER);
			_delay_ms(150);
		}
    }
}