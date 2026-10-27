#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <stdio.h>

#define MPU6050_ADDR 0x68

float accelXRaw, accelYRaw, accelZRaw;

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

int main(void) {
	uart_init(103);
	i2cInit();
	mpu6050Init();

	while (1) {
		mpu6050LeerAcelerometro();
		uart_print("Probando datos MPU\r\n");
		_delay_ms(200);
	}

	return 0;
}