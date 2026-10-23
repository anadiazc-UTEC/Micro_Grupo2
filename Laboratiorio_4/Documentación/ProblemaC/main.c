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
'O'	Automatico

MODOS:
'A'	Auto
'M' Manual

ESTADOS:
'A'	Armado
'D' Desarmado
'P' Peligro / Alarma activa
'S' Silenciado
*/

#define F_CPU 16000000UL
#define PCF8574	0x27
#include <xc.h>
#include <util/delay.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <stdbool.h>

#define REC_GPIO_DELAY 26
#define SEND_GPIO_DELAY 25.275
#define LDR_MAX_LIGHT 600

#define TX PIND0
#define RX PIND1
#define PIN_PRESENCIA PIND2
#define BUZZER PIND3
#define LED_ALARMA PIND4
#define LED_ACTIVO PIND5

char estado = 'D';
char old_estado = 'D';
char modo = 'M';
bool presencia = false;
bool puerta_abierta = false;
bool old_presencia = false;
bool old_puerta_abierta = false;
bool hubo_cambio = true;

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
	DDRD &= ~(1 << PIN_PRESENCIA);
	PORTD &= ~((1 << BUZZER) | (1 << LED_ALARMA) | (1 << LED_ACTIVO));
}


char UART_receive(void){
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
bool UART_pendiente(void) {
	return (UCSR0A & (1 << RXC0));
}

uint16_t ADC_read(void) {

	ADMUX = (ADMUX & 0xF0) | (PINC0 & 0x0F);
	ADCSRA |= (1 << ADSC);
	while (ADCSRA & (1 << ADSC));

	return ADC;
}
void I2C_init(void) {
	TWBR = 72;
	TWSR = 0x00;
}

void I2C_start(void) {
	TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
	while (!(TWCR & (1 << TWINT)));
}

void I2C_write(uint8_t data) {
	TWDR = data;
	TWCR = (1 << TWINT) | (1 << TWEN);
	while (!(TWCR & (1 << TWINT)));
}

void I2C_stop(void) {
	TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);
}

void LCD_send_nibble(uint8_t nibble, uint8_t rs) {
	
	uint8_t data = nibble | rs | 0x08;
	I2C_start();
	I2C_write(PCF8574 << 1);
	I2C_write(data | (1 << 2));
	I2C_write(data & ~(1 << 2));
	I2C_stop();
}

void LCD_send_byte(uint8_t byte, uint8_t rs) {
	LCD_send_nibble(byte & 0xF0, rs);
	LCD_send_nibble((byte << 4) & 0xF0, rs);
}
void LCD_command(uint8_t cmd) {
	LCD_send_byte(cmd, 0);
	_delay_ms(2);
}

void LCD_data(char data) {
	LCD_send_byte(data, 1);
	_delay_us(50);
}
void LCD_init(void){
	I2C_init();
	_delay_ms(50);
	LCD_send_nibble(0x30, 0);
	_delay_ms(5);
	LCD_send_nibble(0x30, 0);
	_delay_us(200);
	LCD_send_nibble(0x30, 0);
	_delay_ms(1);
	LCD_send_nibble(0x20, 0);
	_delay_ms(1);
	
	LCD_command(0x28);
	LCD_command(0x0C);
	LCD_command(0x01);
	_delay_ms(2);
	LCD_command(0x06);
}

void LCD_send_string(const char *str, const char *str2) {
	LCD_command(0x01);
	LCD_command(0x80);
	while (*str) {
		LCD_data(*str++);
	}
	LCD_command(0xC0);
	while (*str2) {
		LCD_data(*str2++);
	}
}
void init_all(void){
	UART_init();
	ADC_init();
	perifericos_init();
	LCD_init();
	USART_SendString(PSTR("Iniciando sistema.\r\n"));
	USART_SendString(PSTR("Modo manual activado.\r\n"));
	USART_SendString(PSTR("Sistema desarmado.\r\n"));
	USART_SendString(PSTR("A: Armar.\r\n"));
	USART_SendString(PSTR("D: Desarmar.\r\n"));
	USART_SendString(PSTR("S: Silenciar.\r\n"));
	USART_SendString(PSTR("F: Forzar alarma.\r\n"));
	USART_SendString(PSTR("M: Manual.\r\n"));
	USART_SendString(PSTR("O: Automatico.\r\n"));
}

void procesar_comando(char comando){
	switch (comando){
		case 'A':case 'a':		//Armar
			estado = 'A';
			USART_SendString(PSTR("Sistema armado.\r\n"));
			break;
		case 'D':case 'd':		//Desarmar
			estado = 'D';
			USART_SendString(PSTR("Sistema desarmado.\r\n"));
			break;
		case 'S':case 's':		//Silenciar
			if (estado == 'P'){
				estado = 'S';
				USART_SendString(PSTR("Alarma silenciada.\r\n"));
			}else {
				USART_SendString(PSTR("No esta sonanando la alarma. No se puede silenciar.\r\n"));
			}
			break;
		case 'F':case 'f':		//Forzar alarma
			if (modo == 'M'){
				estado = 'P';
				USART_SendString(PSTR("Alarma forzada.\r\n"));
			} else{
				USART_SendString(PSTR("Modo automatico. No se puede forzar alarma.\r\n"));
			}
			break;
		case 'M':case 'm':		//Manual
			modo = 'M';
			USART_SendString(PSTR("Modo manual activado.\r\n"));
			break;
		case 'O':case 'o':		//Automatico
			modo = 'A';
			estado = 'A';
			USART_SendString(PSTR("Modo automatico activado.\r\n"));
			break;
	}
}

void actualizar_perifericos(void){
	switch(estado){
		case 'A':
			if (hubo_cambio){
				if (puerta_abierta && presencia){
					USART_SendString(PSTR("ALARMA:OFF; ZONA:PUERTA/PRESENCIA\r\n"));
					LCD_send_string("EST: ARMADO", "AL:OFF/PUER/PRES");
				} else if (puerta_abierta){
					USART_SendString(PSTR("ALARMA:OFF; ZONA:PUERTA\r\n"));
					LCD_send_string("EST: ARMADO", "AL:OFF/PUERTA");
				} else if (presencia){
					USART_SendString(PSTR("ALARMA:OFF; ZONA:PRESENCIA\r\n"));
					LCD_send_string("EST: ARMADO", "AL:OFF/PRESENCIA");
				} else {
					USART_SendString(PSTR("ALARMA:OFF; ZONA:N/A\r\n"));
					LCD_send_string("EST: ARMADO", "AL:OFF/N/A");
				}
			}
			PORTD |= (1 << LED_ACTIVO);
			PORTD &= ~((1 << LED_ALARMA) | (1 << BUZZER));
			break;
		case 'D':
			PORTD &= ~((1 << LED_ACTIVO) | (1 << LED_ALARMA) | (1 << BUZZER));
			LCD_send_string("EST: DESARMADO", " ");
			break;
		case 'P':
			if (hubo_cambio){
				if (puerta_abierta && presencia){
					USART_SendString(PSTR("ALARMA:ON; ZONA:PUERTA/PRESENCIA\r\n"));
					LCD_send_string("EST: ALARMA", "AL:ON/ PUER/PRES");
				} else if (puerta_abierta){
					USART_SendString(PSTR("ALARMA:ON; ZONA:PUERTA\r\n"));
					LCD_send_string("EST: ALARMA", "AL:ON/ PUERTA");
				} else if (presencia){
					USART_SendString(PSTR("ALARMA:ON; ZONA:PRESENCIA\r\n"));
					LCD_send_string("EST: ALARMA", "AL:ON/ PRESENCIA");
				} else {
					USART_SendString(PSTR("ALARMA:ON; ZONA:N/A\r\n"));
					LCD_send_string("EST: ALARMA", "AL:ON/ N/A");
				}
			}
			PORTD |= ((1 << LED_ACTIVO) | (1 << LED_ALARMA) | (1 << BUZZER));
			PORTD &= ~(1 << LED_ACTIVO);
			break;
		case 'S':
			if (hubo_cambio){
				if (puerta_abierta && presencia){
					USART_SendString(PSTR("ALARMA:SILENCIADO; ZONA:PUERTA/PRESENCIA\r\n"));
					LCD_send_string("EST: SILENCIADO", "AL:SIL/PUER/PRES");
				} else if (puerta_abierta){
					USART_SendString(PSTR("ALARMA:SILENCIADO; ZONA:PUERTA\r\n"));
					LCD_send_string("EST: SILENCIADO", "AL:SIL/PUERTA");
				} else if (presencia){
					USART_SendString(PSTR("ALARMA:SILENCIADO; ZONA:PRESENCIA\r\n"));
					LCD_send_string("EST: SILENCIADO", "AL:SIL/PRESENCIA");
				} else {
					USART_SendString(PSTR("ALARMA:SILENCIADO; ZONA:N/A\r\n"));
					LCD_send_string("EST: SILENCIADO", "AL:SIL/N/A");
				}
			}
			PORTD |= (1 << LED_ALARMA);
			PORTD &= ~((1 << LED_ACTIVO) | (1 << BUZZER));
			break;
	}
}

void revisar_sistema(void){
	
	presencia = (PIND & (1 << PIN_PRESENCIA));
	puerta_abierta = (ADC_read() > LDR_MAX_LIGHT);
	hubo_cambio = (presencia != old_presencia) || (puerta_abierta != old_puerta_abierta) || (estado != old_estado);
	
	if ((modo == 'A' && estado == 'A') && (puerta_abierta || presencia)){
		estado = 'P';
		hubo_cambio = true;
	}
	
	actualizar_perifericos();
	
	old_puerta_abierta = puerta_abierta;
	old_presencia = presencia;
	old_estado = estado;
	
}

int main(void)
{
	init_all();
	char comando = ' ';
	while(1)
	{
		if (UART_pendiente()){
			comando = UART_receive();
			procesar_comando(comando);
		}
		revisar_sistema();
		_delay_ms(100);
	}
}