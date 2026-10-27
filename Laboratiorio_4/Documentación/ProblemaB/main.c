#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include <stdbool.h>
#include <xc.h>

#define BOTON_PIN	PIND2
#define DHT11_PIN	PIND3
#define LDR_PIN		PINC0
#define POT_PIN		PINC1
#define LDR_UMBRAL	500
#define TEMP_UMBRAL	25
#define MS_PISTON	1000
#define PCF8574		0x27
#define ACK_OK		0x06
#define ACK_ERR		0x15

#define CMD_STOP			0x00
#define CMD_START			0x01
#define CMD_SET_VELOCIDAD	0x02
#define CMD_SET_PISTON		0x03
#define CMD_EMERGENCIA		0x04

static volatile uint32_t ms_trans = 0;				// milisegundos transcurridos
static volatile uint32_t ms_trans_btn = 0;			// milisegundos transcurridos al momento de que se apreto el boton
static volatile uint32_t ms_piston_activado = 0;	// milisegundos transcurridos al momento de activar el piston
static volatile uint32_t ms_lcd = 0;				// milisegundos transcurridos al momento de acutalizar el lcd

static volatile bool boton_presionado = false;

static bool hay_objeto = false;
static bool ack_error = false;
static uint16_t adc_ldr = 0x0000;
static uint16_t cant_piezas = 0x0000;
static uint8_t temp_actual = 0x00;
static uint8_t pwm_actual = 0x00;
static uint8_t pwm_nuevo = 0x00;

static void st_detenido(void);
static void st_detectar(void);
static void st_piston(void);
static void st_emergencia(void);

static void (*estado)(void);

void timer0_init(void) {
	TCCR0A	= (1 << WGM01);				//modo CTC
	OCR0A	= 249;						//prescaler de 64 (16MHz / (64 * (249 + 1)) = 1000Hz)
	TIMSK0	|= (1 << OCIE0A);			//Habilita la interrupcion por comparacion A del Timer 0
	TCCR0B	= (1 << CS01) | (1 << CS00);//prescaler a 64
}

//interrupcion del timer
ISR(TIMER0_COMPA_vect) {
	ms_trans++;
}

void external_int0_init(void) {
	EICRA	|= (1 << ISC01);	//configura INT0 para dispararse por flanco de bajada
	EICRA	&= ~(1 << ISC00);	
	EIMSK	|= (1 << INT0);		//habilita interrupcion externa 
}

//interrupcion del boton
ISR(INT0_vect) {
	if (ms_trans - ms_trans_btn > 200 && !(PIND & (1 << BOTON_PIN))) {
		boton_presionado = true;
		ms_trans_btn = ms_trans;
	}
}

void ADC_init(void) {
	ADMUX	|= (1 << REFS0);	// voltaje de referencia con capacitor AREF
	ADCSRA	|= (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); //habilita ADC con preescaler 128
}

uint16_t ADC_read(uint8_t puerto) {

	ADMUX = (ADMUX & 0xF0) | (puerto & 0x0F);	//selecciona que pin leer
	ADCSRA |= (1 << ADSC);						//inicia nueva lectura
	while (ADCSRA & (1 << ADSC));				//espera hasta tener lectura

	return ADC;
}

void I2C_init(void) {
	TWBR = 72;
	TWSR = 0x00;
}

//inicia comunicacion i2c
void I2C_start(void) {
	TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);	
	while (!(TWCR & (1 << TWINT)));
}

//envia datos por i2c
void I2C_write(uint8_t data) {
	TWDR = data;
	TWCR = (1 << TWINT) | (1 << TWEN);
	while (!(TWCR & (1 << TWINT)));
}

//finaliza comunicacion idc
void I2C_stop(void) {
	TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);
}

//manda medio byte al lcd, por usar 4 pines en i2c
void LCD_send_nibble(uint8_t nibble, uint8_t rs) {
	
	uint8_t data = nibble | rs | 0x08;
	I2C_start();
	I2C_write(PCF8574 << 1);
	I2C_write(data | (1 << 2));
	I2C_write(data & ~(1 << 2));
	I2C_stop();
}

//envia un byte al lcd
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

void actualizar_lcd(void) {
	char str[17];
	snprintf(str, sizeof(str), "P:%02d T:%02dC M:%03d", cant_piezas, temp_actual, (pwm_actual*100/255));
	
	if (ack_error) {
		LCD_send_string("ERRor: SPI ACK", str);
	} else if (estado == st_detenido) {
		LCD_send_string("ESTADO: STOP", str);
	} else if (estado == st_detectar) {
		LCD_send_string("ESTADO: MARCHA", str);
	} else if (estado == st_piston) {
		LCD_send_string("ESTADO: PISTON", str);
	} else if (estado == st_emergencia) {
		LCD_send_string("ESTADO: MAX TEMP", str);
	}
}

void spi_init(void) {
	DDRB	|= (1<<PINB2)|(1<<PINB3)|(1<<PINB5); //configura SS, MOSI, SCK 
	DDRB	&= ~(1<<PINB4); //configura MISO
	PORTB	|= (1 << PINB2); //deselecciona esclavo
	SPCR	= (1<<SPE) | (1<<MSTR) |(1 << SPR0); //marca como maestro, configura la velocidad y habilita la comunicacion
}

void spi_transfer(uint8_t comando, uint8_t valor) {
	PORTB &= ~(1 << PINB2);
	
	SPDR = comando;
	while(!(SPSR & (1<<SPIF)));
	
	SPDR = valor;
	while(!(SPSR & (1<<SPIF)));
	
	//esperar ACK
	_delay_us(20);
	SPDR = 0x00;
	while(!(SPSR & (1<<SPIF)));
	
	ack_error = (ACK_ERR == SPDR);
	PORTB |= (1 << PINB2);
	
}

bool dht11_read(uint8_t *temp) {
	
	uint8_t data[5] = {0};
	uint16_t timeout;
	
	DDRD |= (1 << DHT11_PIN);
	PORTD &= ~(1 << DHT11_PIN);
	_delay_ms(18);
	PORTD |= (1 << DHT11_PIN);
	_delay_us(30);
	DDRD &= ~(1 << DHT11_PIN);
	
	timeout = 1000;
	while (PIND & (1 << DHT11_PIN)) if (--timeout == 0) return false;
	timeout = 1000;
	while (!(PIND & (1 << DHT11_PIN))) if (--timeout == 0) return false;
	timeout = 1000;
	while (PIND & (1 << DHT11_PIN)) if (--timeout == 0) return false;

	for (uint8_t j = 0; j < 5; j++) {
		for (uint8_t i = 0; i < 8; i++) {
			timeout = 1000;
			while (!(PIND & (1 << DHT11_PIN))) if (--timeout == 0) return false;
			_delay_us(30);
			if (PIND & (1 << DHT11_PIN)) {
				data[j] |= (1 << (7 - i));
			}
			timeout = 1000;
			while (PIND & (1 << DHT11_PIN)) if (--timeout == 0) return false;
		}
	}
	
	if ((data[0] + data[1] + data[2] + data[3]) == data[4]) {
		*temp = data[2];
		return true;
	}
	return false;
}

void init_all(void){
	
	DDRD	&= ~(1 << BOTON_PIN);
	PORTD	|= (1 << BOTON_PIN);
	
	external_int0_init();
	timer0_init();
	ADC_init();
	LCD_init();
	spi_init();
	
	sei();
	
	estado = st_detenido;
	
}

static void st_detenido(void) {
	if (boton_presionado) {
		boton_presionado = false;
		if (temp_actual > TEMP_UMBRAL) {
			spi_transfer(CMD_EMERGENCIA, temp_actual);
			estado = st_emergencia;
		} else {
			pwm_actual = pwm_nuevo;
			spi_transfer(CMD_START, 0x00);
			spi_transfer(CMD_SET_VELOCIDAD, pwm_actual);
			estado = st_detectar;
		}
	}
}

static void st_detectar(void) {
	
	if (temp_actual > TEMP_UMBRAL) {
		spi_transfer(CMD_EMERGENCIA, temp_actual);
		estado = st_emergencia;
		return;
	}
	
	if (pwm_nuevo != pwm_actual) {
		pwm_actual = pwm_nuevo;
		spi_transfer(CMD_SET_VELOCIDAD, pwm_actual);
	}
	
	if (boton_presionado) {
		boton_presionado = false;
		spi_transfer(CMD_STOP, 0);
		estado = st_detenido;
		return;
	}

	if (hay_objeto) {
		cant_piezas++;
		spi_transfer(CMD_SET_PISTON, 90);
		ms_piston_activado = ms_trans;
		estado = st_piston;
	}
}

static void st_piston(void) {
	
	if (temp_actual > TEMP_UMBRAL) {
		spi_transfer(CMD_EMERGENCIA, temp_actual);
		estado = st_emergencia;
		return;
	}
	
	if (ms_trans - ms_piston_activado >= MS_PISTON) {
		spi_transfer(CMD_SET_PISTON, 0);
		estado = st_detectar;
	}
	
}

static void st_emergencia(void) {
	if (boton_presionado) {
		boton_presionado = false;
		spi_transfer(CMD_STOP, 0);
		estado = st_detenido;
	}
}

int main(void)
{
	init_all();
	
	uint16_t adc_pot = 0x0000;
	
	uint8_t temp_lectura = 0x00;
	
    while(1)
    {
		adc_pot = ADC_read(POT_PIN);
		pwm_nuevo = (adc_pot >> 2);
		
		adc_ldr = ADC_read(LDR_PIN);
		hay_objeto = (adc_ldr <= LDR_UMBRAL);
		
		if (dht11_read(&temp_lectura)) {
			temp_actual = temp_lectura;
		}
		
		estado();
		
		if (ms_trans - ms_lcd >= 100) {
			ms_lcd = ms_trans;
			actualizar_lcd();
		}
    }
	return 0;
}