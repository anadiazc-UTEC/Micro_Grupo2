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
#include <stdio.h>

#define NUM_LEDS 12

typedef struct {
	uint8_t r;
	uint8_t g;
	uint8_t b;
} RGB_t;

struct plastico{
	uint16_t max_color[3];
	uint16_t min_color[3];
};

RGB_t strip[NUM_LEDS];

struct plastico plastico_rojo;
struct plastico plastico_verde;
struct plastico plastico_amarillo;
struct plastico plastico_azul;

char color;

void plasticos_init(void){
	// Rojo
	plastico_rojo.max_color[0] = 474;
	plastico_rojo.max_color[1] = 149;
	plastico_rojo.max_color[2] = 133;

	plastico_rojo.min_color[0] = 412;
	plastico_rojo.min_color[1] = 83;
	plastico_rojo.min_color[2] = 68;

	// Verde
	plastico_verde.max_color[0] = 40;
	plastico_verde.max_color[1] = 177;
	plastico_verde.max_color[2] = 166;

	plastico_verde.min_color[0] = 0;
	plastico_verde.min_color[1] = 117;
	plastico_verde.min_color[2] = 106;

	// Amarillo
	plastico_amarillo.max_color[0] = 501;
	plastico_amarillo.max_color[1] = 525;
	plastico_amarillo.max_color[2] = 588;

	plastico_amarillo.min_color[0] = 441;
	plastico_amarillo.min_color[1] = 465;
	plastico_amarillo.min_color[2] = 528;

	// Azul
	plastico_azul.max_color[0] = 54;
	plastico_azul.max_color[1] = 209;
	plastico_azul.max_color[2] = 292;

	plastico_azul.min_color[0] = 0;
	plastico_azul.min_color[1] = 149;
	plastico_azul.min_color[2] = 232;
}

void ws2812_send_byte(uint8_t byte) {
	for (uint8_t bit = 0; bit < 8; bit++) {
		if (byte & 0x80) {
			PORTB |= (1 << PINB0);
			asm volatile(
			"nop \n\t""nop \n\t""nop \n\t""nop \n\t""nop \n\t"
			"nop \n\t""nop \n\t""nop \n\t""nop \n\t"
			);
			PORTB &= ~(1 << PINB0);
			asm volatile(
			"nop \n\t""nop \n\t""nop \n\t""nop \n\t"
			);
		} else {	
			PORTB |= (1 << PINB0);
			asm volatile(
			"nop \n\t""nop \n\t""nop \n\t""nop \n\t"
			);
			PORTB &= ~(1 << PINB0);
			asm volatile(
			"nop \n\t""nop \n\t""nop \n\t""nop \n\t""nop \n\t"
			"nop \n\t""nop \n\t""nop \n\t""nop \n\t""nop \n\t"
			);
		}
		byte <<= 1;
	}
}
void ws2812_show(void) {
	cli();
	for (uint16_t i = 0; i < NUM_LEDS; i++) {
		ws2812_send_byte(strip[i].g);
		ws2812_send_byte(strip[i].r);
		ws2812_send_byte(strip[i].b);
	}
	sei();
	_delay_us(60);
}
void ws2812_set_pixel(uint16_t index, uint8_t r, uint8_t b, uint8_t g) {
	if (index >= 0 && index < NUM_LEDS) {
		strip[index].r = r;
		strip[index].g = g;
		strip[index].b = b;
	}
}
void ws2812_set_all(uint8_t r, uint8_t g, uint8_t b) {
	for (uint16_t i = 0; i < NUM_LEDS; i++) {
		ws2812_set_pixel(i, r, g, b);
	}
	ws2812_show();
}

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
void UART_transmit(char data) {
	// Esperar a que el buffer de transmisión esté vacío
	while (!(UCSR0A & (1 << UDRE0)));
	
	// Poner el dato en el registro de transmisión
	UDR0 = data;
}
void UART_send_string(const char* str) {
	while (*str) {
		UART_transmit(*str++);
	}
}
void UART_send_rgb(uint16_t r, uint16_t g, uint16_t b) {
	char buffer[40];
	
	// Formato estilo "R:1023 G:512 B:256\r\n"
	snprintf(buffer, sizeof(buffer), "R:%u G:%u B:%u\r\n", r, g, b);
	
	// Si prefieres formato CSV (ej. "1023,512,256\n"), usa:
	// snprintf(buffer, sizeof(buffer), "%u,%u,%u\r\n", r, g, b);
	
	UART_send_string(buffer);
}
// Enviar un número de 16 bits (0 a 65535) en texto ASCII
void UART_send_number(uint16_t num) {
	char buffer[10]; // Suficiente para "65535\r\n\0"
	
	// Convertir el número a texto
	snprintf(buffer, sizeof(buffer), "%u\r\n", num);
	
	// Transmitir carácter por carácter
	UART_send_string(buffer);
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
	LCD_command(0x01);
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
	
	DDRB |= (1 << PINB0);
	PORTB &= ~(1 << PINB0);
	
	_delay_ms(10);
	
}
uint16_t leer_color(void){
	uint16_t valor_color = 0;
	uint16_t val = 0;
	for (int i = 0; i <10; i++){
		val = ADC_read();
		valor_color += val;
		//UART_send_number(val);
	}
	return (valor_color / 10);
}

int comparar_plastico(struct plastico plastic, uint16_t rojo, uint16_t verde, uint16_t azul){
	/*
	UART_send_string("leido: ");
	UART_send_rgb(rojo, verde, azul);
	UART_send_string("max: ");
	UART_send_rgb(plastic.max_color[0], plastic.max_color[1], plastic.max_color[2]);
	UART_send_string("min: ");
	UART_send_rgb(plastic.min_color[0], plastic.min_color[1], plastic.min_color[2]);
	
	if (plastic.max_color[0] >= rojo && plastic.min_color[0] <= rojo){
		UART_send_string("rojo ok\n");
	}else{
		UART_send_string("rojo no\n");
	}
	if (plastic.max_color[1] >= verde && plastic.min_color[1] <= verde){
		UART_send_string("verde ok\n");
	}else{
		UART_send_string("verde no\n");
	}
	if (plastic.max_color[2] >= azul && plastic.min_color[2] <= azul){
		UART_send_string("azul ok\n");
	}else{
		UART_send_string("azul no\n");
	}
	*/
	if ((plastic.max_color[0] >= rojo && plastic.min_color[0] <= rojo)
		&& (plastic.max_color[1] >= verde && plastic.min_color[1] <= verde)
		&& (plastic.max_color[2] >= azul && plastic.min_color[2] <= azul)){
			
			return 1;
	}
		
	return 0;
}

char comparar_lectura(uint16_t rojo, uint16_t verde, uint16_t azul){
	
	if (comparar_plastico(plastico_rojo, rojo, verde, azul) == 1){
		return 'R';
	} else if(comparar_plastico(plastico_verde, rojo, verde, azul) == 1){
		return 'G';
	} else if(comparar_plastico(plastico_azul, rojo, verde, azul) == 1){
		return 'B';
	} else if(comparar_plastico(plastico_amarillo, rojo, verde, azul) == 1){
		return 'Y';
	} else {
		return 'X';
	}
}
void iniciar_reconocimiento(void){
	uint16_t valor_luz_rojo, valor_luz_verde, valor_luz_azul;
	ws2812_set_all(0, 0, 0);
	_delay_ms(100);
	
	//rojo
	ws2812_set_all(255, 0, 0);
	_delay_ms(1000);
	//UART_send_string("R\n");
	valor_luz_rojo = leer_color();

	//verde
	ws2812_set_all(0, 255, 0);
	_delay_ms(1000);
	//UART_send_string("G\n");
	valor_luz_verde = leer_color();
	
	//azul
	ws2812_set_all(0, 0, 255);
	_delay_ms(1000);
	//UART_send_string("B\n");
	valor_luz_azul = leer_color();
	
	ws2812_set_all(0, 0, 0);
	
	color = comparar_lectura(valor_luz_rojo, valor_luz_verde, valor_luz_azul);
	
	switch (color){
		case 'R':
			LCD_send_string("Rojo");
			mover_servo_grados(0);
			ws2812_set_all(255, 0, 0);
			break;
		case 'G':
			LCD_send_string("Verde");
			mover_servo_grados(45);
			ws2812_set_all(0, 255, 0);
			break;
		case 'B':
			LCD_send_string("Azul");
			mover_servo_grados(90);
			ws2812_set_all(0, 0, 255);
			break;
		case 'Y':
			LCD_send_string("Amarillo");
			mover_servo_grados(135);
			ws2812_set_all(200, 150, 0);
			break;
		case 'X':
			LCD_send_string("Desconocido");
			mover_servo_grados(180);
			ws2812_set_all(255, 255, 255);
			break;
		default:
			LCD_send_string("Desconocido");
			mover_servo_grados(180);
			ws2812_set_all(255, 255, 255);
			break;
	}
	_delay_ms(5000);
	UART_send_rgb(valor_luz_rojo, valor_luz_verde, valor_luz_azul);
	
	ws2812_set_all(0, 0, 0);
	
}

int main(void)
{
	ADC_init();
	LCD_init();
	init_leds();
	UART_init();
	servo_init();
	plasticos_init();
	while(1)
	{
		ws2812_set_all(0, 0, 0);
		LCD_send_string("Esperando...");
		_delay_ms(500);
		mover_servo_grados(180);
		char op = UART_receiveChar();
		switch (op){
			case 'I':
				iniciar_reconocimiento();
				break;
		}
		
	}
}