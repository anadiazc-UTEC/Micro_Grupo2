#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include <xc.h>

#define BOTON_PIN	PIND2
#define DHT11_PIN	PIND3
#define LDR_PIN		PINC0
#define POT_PIN		PINC1
#define LDR_UMBRAL	400
#define TEMP_UMBRAL	0
#define MS_PISTON	1000
#define PCF8574		0x27

#define CMD_STOP			0x00
#define CMD_START			0x01
#define CMD_SET_VELOCIDAD	0x02
#define CMD_SET_PISTON		0x03
#define CMD_EMERGENCIA		0x04

static void st_detenido(void);
static void st_detectar(void);
static void st_piston(void);
static void st_emergencia(void);

static void (*estado)(void);

static void st_detenido(void) {}

static void st_detectar(void) {}

static void st_piston(void) {}

static void st_emergencia(void) {}

void external_int0_init(void) {
	EICRA	|= (1 << ISC01);
	EICRA	&= ~(1 << ISC00);
	EIMSK	|= (1 << INT0);
}



void ADC_init(void) {
	ADMUX	|= (1 << REFS0);
	ADCSRA	|= (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
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

void spi_init(void) {
	DDRB	|= (1<<PINB2)|(1<<PINB3)|(1<<PINB5);
	DDRB	&= ~(1<<PINB4);
	PORTB	|= (1 << PINB2);
	SPCR	= (1<<SPE) | (1<<MSTR) |(1 << SPR0);
}

uint8_t spi_transfer(uint8_t comando, uint8_t valor) {
	PORTB &= ~(1 << PINB2);
	
	SPDR = comando;
	while(!(SPSR & (1<<SPIF)));
	
	SPDR = valor;
	while(!(SPSR & (1<<SPIF)));
	
	//esperar ACK
	_delay_us(20);
	SPDR = 0x00;
	while(!(SPSR & (1<<SPIF)));
	
	uint8_t res = SPDR;
	PORTB |= (1 << PINB2);
	
	return res;
}

void init_all(void){
	
	DDRD	&= ~(1 << BOTON_PIN);
	PORTD	|= (1 << BOTON_PIN);
	
	external_int0_init();
	ADC_init();
	LCD_init();
	spi_init();
	
	sei();
	
	estado = st_detenido;
	
}

int main(void)
{
	init_all();
    while(1)
    {
		estado();
		
    }
	return 0;
}