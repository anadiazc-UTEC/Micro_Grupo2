#define F_CPU 16000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <util/delay.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <stdio.h>

#define LED_PIN 6
#define MPU6050_ADDR 0x68

#define EEPROM_OFFSET_X 0x10
#define EEPROM_OFFSET_Y 0x14
#define EEPROM_MAGIC_ADDR 0x18
#define EEPROM_MAGIC_VAL 0x42

#define NUM_LEDS 64

uint8_t leds[NUM_LEDS][3];

float accelXRaw, accelYRaw, accelZRaw;

float offsetRoll = 0.0f;
float offsetPitch = 0.0f;

//configuracion del puerto serial uart
void uart_init(unsigned int ubrr) {
	UBRR0H = (unsigned char)(ubrr>>8);
	UBRR0L = (unsigned char)ubrr;
	UCSR0B = (1<<TXEN0) | (1<<RXEN0);
	UCSR0C = (1<<UCSZ01) | (1<<UCSZ00);
}

//recive caracter por puerto serial
char uart_receive(void) {
	while (!(UCSR0A & (1<<RXC0)));
	return UDR0;
}

//envia caracter por puerto serial
void uart_send(char c) {
	while (!(UCSR0A & (1<<UDRE0)));
	UDR0 = c;
}

//envia una cadena de texto completa por puerto serial
void uart_print(const char *s) {
	while (*s) uart_send(*s++);
}

//imprime byte en formato hexadecimal
void uart_print_hex(uint8_t val) {
	char buf[6];
	sprintf(buf, "0x%02X", val);
	uart_print(buf);
}

//imprime un arreglo de bytes en hexadecimal
void uart_print_hex_array(const uint8_t *arr, uint8_t len) {
	for (uint8_t i = 0; i < len; i++) {
		uart_print_hex(arr[i]);
		uart_send(' ');
	}
	uart_print("\r\n");
}

//envia número entero en texto por puerto serial
void transmitirEntero(int16_t numero) {
	char buf[7];
	itoa(numero, buf, 10);
	uart_print(buf);
}

//inicialización del BUS i2c
void i2cInit(void) {
	TWSR = 0x00;
	TWBR = 72;
	TWCR = (1 << TWEN);
}

//condición de inicio en el BUS i2c
void i2cStart(void) {
	TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
	while (!(TWCR & (1 << TWINT)));
}

//condición de parada en el bus i2c
void i2cStop(void) {
	TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);
	_delay_us(100);
}

//escritura de un byte en el bus i2c
void i2cWrite(uint8_t data) {
	TWDR = data;
	TWCR = (1 << TWINT) | (1 << TWEN);
	while (!(TWCR & (1 << TWINT)));
}

//envia un byte sobre el BUS i2c
void i2cSendByte(uint8_t data) {
	i2cWrite(data);
}

//lectura de un byte por i2c con confirmación ack
uint8_t i2cReadAck(void) {
	TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWEA);
	while (!(TWCR & (1 << TWINT)));
	return TWDR;
}

//lectura de un byte por i2c sin confirmación nack
uint8_t i2cReadNack(void) {
	TWCR = (1 << TWINT) | (1 << TWEN);
	while (!(TWCR & (1 << TWINT)));
	return TWDR;
}

//escritura de un byte en la eeprom externa por i2c
void eepromWrite(uint8_t address, uint8_t data) {
	uint8_t devAddr = 0xA0;
	i2cStart();
	i2cSendByte(devAddr);
	i2cSendByte(address);
	i2cSendByte(data);
	i2cStop();
	_delay_ms(5);
}

//lectura de un byte de la eeprom externa por i2c
uint8_t eepromRead(uint8_t address) {
	uint8_t data;
	uint8_t devAddrWrite = 0xA0;
	uint8_t devAddrRead = 0xA1;
	i2cStart();
	i2cSendByte(devAddrWrite);
	i2cSendByte(address);
	i2cStart();
	i2cSendByte(devAddrRead);
	data = i2cReadNack();
	i2cStop();
	return data;
}

//inicialización y activación del sensor mpu6050
void mpu6050Init(void) {
	_delay_ms(150);
	i2cStart();
	i2cWrite(MPU6050_ADDR << 1);
	i2cWrite(0x6B);
	i2cWrite(0x00);
	i2cStop();
}

//lectura de los ejes del acelerómetro desde el mpu6050
void mpu6050LeerAcelerometro(void) {
	i2cStart();
	i2cWrite(MPU6050_ADDR << 1);
	i2cWrite(0x3B);
	i2cStart();
	i2cWrite((MPU6050_ADDR << 1) | 1);
	int16_t ax = (i2cReadAck() << 8) | i2cReadAck();
	int16_t ay = (i2cReadAck() << 8) | i2cReadAck();
	int16_t az = (i2cReadAck() << 8) | i2cReadNack();
	i2cStop();
	accelXRaw = (float)ax;
	accelYRaw = (float)ay;
	accelZRaw = (float)az;
}

//asignacion de colores rgb a un led específico del buffer
void setLedRGB(int ledIndex, uint8_t r, uint8_t g, uint8_t b) {
	if (ledIndex < 0 || ledIndex >= NUM_LEDS) {
		return;
	}
	leds[ledIndex][0] = r;
	leds[ledIndex][1] = g;
	leds[ledIndex][2] = b;
}

//envia un bit individual con protocolo de tiempo ws2812
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
	}
	else {
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

//envia un byte bit a bit a la tira de leds
void ws2812SendByte(uint8_t byte) {
	for (uint8_t i = 0; i < 8; i++) {
		sendBit(byte & (1 << (7 - i)));
	}
}

//configuracion del pin de datos de la matriz led
void inicializarMatriz(void) {
	DDRD |= (1 << LED_PIN);
}

//transmision serie del buffer de datos a los leds
void ws2812SendBuffer(uint8_t *buffer, uint16_t length) {
	uint8_t sreg = SREG;
	cli();
	while (length--) {
		ws2812SendByte(*buffer++);
	}
	_delay_us(60);
	SREG = sreg;
}

//ordenamiento de formato grb y actualización de la matriz
void show(uint8_t colors[NUM_LEDS][3]) {
	uint8_t grbBuffer[NUM_LEDS * 3];
	for (int i = 0; i < NUM_LEDS; i++) {
		grbBuffer[i * 3 + 0] = colors[i][1];
		grbBuffer[i * 3 + 1] = colors[i][0];
		grbBuffer[i * 3 + 2] = colors[i][2];
	}
	ws2812SendBuffer(grbBuffer, NUM_LEDS * 3);
}

//apagado general de todos los leds de la matriz
void limpiarMatriz(void) {
	for (int i = 0; i < NUM_LEDS; i++) {
		setLedRGB(i, 0, 0, 0);
	}
	show(leds);
}

//configuracion de direcciones de pines para puertos e/s
void configurarPuertos(void) {
	DDRD &= ~((1 << PD2) | (1 << PD3));
	PORTD |= (1 << PD2) | (1 << PD3);
	DDRB |= (1 << PB1);
	PORTB &= ~(1 << PB1);
}

//inicialización de temporizadores del sistema
void configurarTimers(void) {
	TCCR1A = 0;
	TCCR1B = 0;
	OCR1A = 0;
}

//generación de frecuencia para el tono del buzzer
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

//desactivación de salidas de sonido de los buzzers
void apagarBuzzers(void) {
	TCCR1B = 0;
	TCCR1A &= ~(1 << COM1A0);
	PORTB &= ~(1 << PB1);
	TCCR2B = 0;
	TCCR2A &= ~((1 << COM2B1) | (1 << COM2B0));
	PORTD &= ~(1 << PD3);
}

//lectura de parámetros de calibración desde la eeprom interna
void cargarEEPROM(void) {
	uint8_t flag = eeprom_read_byte((uint8_t*)EEPROM_MAGIC_ADDR);
	if (flag == EEPROM_MAGIC_VAL) {
		eeprom_read_block(
		(void*)&offsetRoll,
		(const void*)EEPROM_OFFSET_X,
		sizeof(float)
		);
		eeprom_read_block((void*)&offsetPitch,(const void*)EEPROM_OFFSET_Y,sizeof(float));
	}
	else {
		offsetRoll = 0.0f;
		offsetPitch = 0.0f;
	}
}

//almacenamiento de valores de offset en eeprom interna
void guardarEEPROM(float offX, float offY) {
	offsetRoll = offX;
	offsetPitch = offY;
	eeprom_update_block(
	(const void*)&offsetRoll,
	(void*)EEPROM_OFFSET_X,
	sizeof(float)
	);
	eeprom_update_block(
	(const void*)&offsetPitch,
	(void*)EEPROM_OFFSET_Y,
	sizeof(float)
	);
	eeprom_update_byte(
	(uint8_t*)EEPROM_MAGIC_ADDR,
	EEPROM_MAGIC_VAL
	);
}

int main(void) {
	configurarPuertos();
	configurarTimers();
	uart_init(103);
	i2cInit();
	mpu6050Init();
	inicializarMatriz();
	limpiarMatriz();
	cargarEEPROM();

	while (1) {
		_delay_ms(100);
	}

	return 0;
}