/*
colores a detectar

PINOUT
A5 SCL
A4 SLA
PIN 9 servo
PIN 3 ROJO
PIN 5 VERDE
PIN 6 AZUL
*/
#define PCF8574	0x27
#define F_CPU 16000000UL
#include <xc.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

struct plastico{
	uint16_t max_color[3];
	uint16_t min_color[3];
} ;
struct plastico plastico_rojo;
struct plastico plastico_verde;
struct plastico plastico_amarillo;
struct plastico plastico_azul;

char color;

void plasticos_init(void){
	// Rojo
	plastico_rojo.max_color[0] = 255;
	plastico_rojo.max_color[1] = 0;
	plastico_rojo.max_color[2] = 0;

	plastico_rojo.min_color[0] = 200;
	plastico_rojo.min_color[1] = 0;
	plastico_rojo.min_color[2] = 0;

	// Verde
	plastico_verde.max_color[0] = 0;
	plastico_verde.max_color[1] = 255;
	plastico_verde.max_color[2] = 0;

	plastico_verde.min_color[0] = 0;
	plastico_verde.min_color[1] = 200;
	plastico_verde.min_color[2] = 0;

	// Amarillo
	plastico_amarillo.max_color[0] = 255;
	plastico_amarillo.max_color[1] = 255;
	plastico_amarillo.max_color[2] = 0;

	plastico_amarillo.min_color[0] = 200;
	plastico_amarillo.min_color[1] = 200;
	plastico_amarillo.min_color[2] = 0;

	// Azul
	plastico_azul.max_color[0] = 0;
	plastico_azul.max_color[1] = 0;
	plastico_azul.max_color[2] = 255;

	plastico_azul.min_color[0] = 0;
	plastico_azul.min_color[1] = 0;
	plastico_azul.min_color[2] = 200;
}

void UART_init(void){
	UBRR0H = (103 >> 8);
	UBRR0L = 103;
	UCSR0B = (1 << RXEN0) | (1 << TXEN0);
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

char UART_receiveChar(void){
	while (!(UCSR0A & (1 << RXC0)));
	return UDR0;
}

void ADC_init(void) {
	ADMUX |= (1 << REFS0);
	ADCSRA |= (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
	ADCSRA |= (1 << ADEN);
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

void LCD_send_string(const char *str) {
	while (*str) {
		LCD_data(*str++);
	}
}

uint16_t ADC_read(void) {

	ADMUX = (ADMUX & 0xF0) | (PINC0 & 0x0F);
	ADCSRA |= (1 << ADSC);
	while (ADCSRA & (1 << ADSC));

	return ADC;
}

void init_leds(void){
	DDRD |= (1 << PIND3) | (1 << PIND5) | (1 << PIND6);
}
uint16_t leer_color(void){
	uint16_t valor_color = 0;
	for (int i = 0; i <10; i++){
		valor_color += ADC_read();
	}
	return (valor_color / 10);
}

int comparar_plastico(struct plastico plastic, uint16_t rojo, uint16_t verde, uint16_t azul){
	if ((plastic.max_color[0] <= rojo && plastic.min_color[0] >= rojo)
		&& (plastic.max_color[1] <= verde && plastic.min_color[1] >= verde)
		&& (plastic.max_color[2] <= azul && plastic.min_color[2] >= azul)){
			
			return 1;
		}
		
	return 0;
}

char comparar_lectura(uint16_t rojo, uint16_t verde, uint16_t azul){
	
	if (comparar_plastico(plastico_rojo, rojo, verde, azul) == 1){
		return 'R'
	} else if(comparar_plastico(plastico_verde, rojo, verde, azul) == 1){
		return 'G'
	} else if(comparar_plastico(plastico_azul, rojo, verde, azul) == 1){
		return 'B'
	} else if(comparar_plastico(plastico_amarillo, rojo, verde, azul) == 1){
		return 'Y'
	} else {
		return 'X';
	}
}
void iniciar_reconocimiento(void){
	uint16_t valor_luz_rojo, valor_luz_verde, valor_luz_azul;
	PORTD &= ~((1 << PIND3) | (1 << PIND5) | (1 << PIND6));
	//rojo
	PORTD |= (1 << PIND3);
	_delay_ms(100);
	valor_luz_rojo = leer_color();
	PORTD &= ~(1 << PIND3);
	//verde
	PORTD |= (1 << PIND5);
	_delay_ms(100);
	valor_luz_verde = leer_color();
	PORTD &= ~(1 << PIND5);
	//azul
	PORTD |= (1 << PIND6);
	_delay_ms(100);
	valor_luz_azul = leer_color();
	PORTD &= ~(1 << PIND6);
	color = comparar_lectura(valor_luz_rojo, valor_luz_verde, valor_luz_azul);
	
	
	
}

int main(void)
{
	ADC_init();
	LCD_init();
	init_leds();
	UART_init();
	plasticos_init();
	
	while(1)
	{
		char op = UART_receiveChar();
		switch (op){
			case 'I':
				iniciar_reconocimiento();
				break;
		}
		
	}
}