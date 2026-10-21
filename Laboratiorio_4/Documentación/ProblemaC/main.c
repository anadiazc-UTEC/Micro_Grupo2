/*
PINOUT:
PD0	Bluetooth TX
PD1	Bluetooth RX
PD2	Sensor presencia
PD3	Buzzer
PD4 LED alarma
PD5 LED armado
PC0 LDR

COMANDOS:
'A'	Armar
'D'	Desarmar
'S'	Silenciar
'F'	Forzar alarma
'M'	Manual	
'O' Automatico
*/

#define F_CPU 16000000UL
#include <xc.h>
#include <util/delay.h>
#include <avr/io.h>
#include <avr/pgmspace.h>

#define REC_GPIO_DELAY 26
#define SEND_GPIO_DELAY 25.275

#define TX PIND0
#define RX PIND1
#define PRESENCIA PIND2
#define BUZZER PIND3
#define LED_ALARMA PIND4
#define LED_ACTIVO PIND5

void UART_init(void){
	UBRR0H = (103 >> 8);
	UBRR0L = 103;
	UCSR0B = (1 << RXEN0) | (1 << TXEN0);
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void ADC_init(void) {
	ADMUX |= (1 << REFS0);
	ADCSRA |= (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
	ADCSRA |= (1 << ADEN);
}

void perifericos_init(void){
	DDRD |= (1 << BUZZER) | (1 << LED_ALARMA) | (1 << LED_ACTIVO);
	DDRD &= ~(1 << PRESENCIA);
	PORTD &= ~((1 << BUZZER) | (1 << LED_ALARMA) | (1 << LED_ACTIVO));
}


char UART_receiveChar(void){
	while (!(UCSR0A & (1 << RXC0)));
	return UDR0;
}

void UART_transmit(char data) {
	while (!(UCSR0A & (1 << UDRE0)));
	UDR0 = data;
}

void USART_SendString(const char *str) {
	char c;
	while ((c = pgm_read_byte(str++))) {
		UART_transmit(c);
	}
}

uint16_t ADC_read(void) {

	ADMUX = (ADMUX & 0xF0) | (PINC0 & 0x0F);
	ADCSRA |= (1 << ADSC);
	while (ADCSRA & (1 << ADSC));

	return ADC;
}

void init_all(void){
	UART_init();
	ADC_init();
	perifericos_init();
}
int main(void)
{
	init_all();
    while(1)
    {
		
    }
}