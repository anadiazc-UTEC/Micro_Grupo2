#define F_CPU 16000000UL
#define LCD_ADDR 0x27

#include <avr/io.h>
#include <util/delay.h>


//inicializa la comunicacion I2C
void I2C_init(void) {
	TWBR = 72; // Configura SCL a ~100 kHz con F_CPU = 16 MHz
	TWSR = 0x00;
}

//inicia la comunicacion I2C
void I2C_start(void) {
	TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
	while (!(TWCR & (1 << TWINT)));
}

//envia un dato mediante la comunicacion I2C
void I2C_write(uint8_t data) {
	TWDR = data;
	TWCR = (1 << TWINT) | (1 << TWEN);
	while (!(TWCR & (1 << TWINT)));
}

//finaliza la comunicación I2C
void I2C_stop(void) {
	TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);
}

//envia un nibble al LCD mediante I2C
void lcd_send_nibble(uint8_t nibble, uint8_t rs) {
	// P3 = Backlight (0x08), P2 = Enable (1 << 2), P0 = RS (rs)
	uint8_t data = nibble | rs | 0x08;
	
	I2C_start();
	I2C_write(LCD_ADDR << 1);
	I2C_write(data | (1 << 2));  // Pulse Enable HIGH
	I2C_write(data & ~(1 << 2)); // Pulse Enable LOW
	I2C_stop();
}

//envia un byte al LCD dividido en dos nibbles
void lcd_send_byte(uint8_t byte, uint8_t rs) {
	lcd_send_nibble(byte & 0xF0, rs);
	lcd_send_nibble((byte << 4) & 0xF0, rs);
}

//envia un comando al LCD
void lcd_send_command(uint8_t cmd) {
	lcd_send_byte(cmd, 0);
	_delay_ms(2);
}

//envia un dato al LCD para su visualizacion
void lcd_send_data(uint8_t data) {
	lcd_send_byte(data, 1);
	_delay_us(50);
}

//inicializa el LCD y configura su modo de funcionamiento
void lcd_init(void) {
	I2C_init();
	_delay_ms(50);
	
	// Secuencia de inicialización en modo 4 bits
	lcd_send_nibble(0x30, 0);
	_delay_ms(5);
	lcd_send_nibble(0x30, 0);
	_delay_us(200);
	lcd_send_nibble(0x30, 0);
	_delay_ms(1);
	lcd_send_nibble(0x20, 0); // Establecer modo 4 bits
	_delay_ms(1);

	// Configuración de pantalla
	lcd_send_command(0x28); // 2 líneas, matriz 5x8
	lcd_send_command(0x0C); // Pantalla encendida, cursor apagado
	lcd_send_command(0x01); // Limpiar pantalla
	_delay_ms(2);
	lcd_send_command(0x06); // Incrementar cursor a la derecha
}

//muestra una cadena de caracteres en el LCD
void lcd_write_string(const char *str) {
	while (*str) {
		lcd_send_data(*str++);
	}
}

//limpia la pantalla del LCD
void lcd_clear(void) {
	lcd_send_command(0x01);
	_delay_ms(2);
}

// CANCIÓN 1: At Doom's gate

//Lista de frecuencias para melodia y acompañamiento junto a sus duraciones

uint16_t melodia1[] = {
    165, 165, 330, 165, 165, 294, 165, 165, 262, 165, 165, 247, 165, 165, 233, 247,
    165, 165, 330, 165, 165, 294, 165, 165, 262, 165, 165, 233, 0,

    165, 165, 330, 165, 165, 294, 165, 165, 262, 165, 165, 247, 165, 165, 233, 247,
    165, 165, 330, 165, 165, 294, 165, 165, 262, 165, 165, 233, 0,

    165, 165, 330, 165, 165, 294, 165, 165, 262, 165, 165, 247, 165, 165, 233, 247,
    165, 165, 330, 165, 165, 294, 165, 165, 262, 165, 165, 233, 0,

    165, 165, 330, 165, 165, 294, 165, 165, 262, 165, 165, 247, 165, 165, 233, 247,
    208, 247, 330, 392, 440, 494, 587, 659, 494, 587, 659, 0,

    165, 165, 330, 165, 165, 294, 165, 165, 262, 165, 165, 247, 165, 165, 233, 247,
    165, 165, 330, 165, 165, 294, 165, 165, 262, 165, 165, 233, 0,

    165, 165, 330, 165, 165, 294, 165, 165, 262, 165, 165, 247, 165, 165, 233, 247,
    208, 247, 330, 392, 440, 494, 587, 659, 494, 587, 659, 659
};

uint16_t duracionesMe1[] = {
    136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136,
    136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 273, 136,

    136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136,
    136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 273, 136,

    136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136,
    136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 273, 136,

    136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136,
    136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 273, 136,

    136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136,
    136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 273, 136,

    136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136,
    136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 273, 545
};

uint16_t acompanamiento1[] = {
    82, 0, 82, 0, 82, 0, 82, 0,
    82, 0, 82, 0, 82, 0, 82, 0,
    82, 0, 82, 0, 82, 0, 82, 0,
    82, 0, 82, 0, 82, 0, 82, 0,

    82, 82, 82, 82, 82, 82, 82, 82,
    82, 82, 82, 82, 82, 82, 82, 82,
    82, 82, 82, 82, 82, 82, 82, 82,
    82, 82, 82, 82, 82, 82, 82, 82
};

uint16_t duracionesAc1[] = {
    273, 1909, 273, 1909, 273, 1909, 273, 1909,
    273, 1909, 273, 1909, 273, 1909, 273, 1909,

    273, 273, 273, 273, 273, 273, 273, 273,
    273, 273, 273, 273, 273, 273, 273, 273,
    273, 273, 273, 273, 273, 273, 273, 273,
    273, 273, 273, 273, 273, 273, 273, 273
};

//Calculo de cantidad de elementos de melodia y acompañamiento 
int cantidadElementosAc1 = sizeof(acompanamiento1) / sizeof(acompanamiento1[0]);
int cantidadElementosMe1 = sizeof(melodia1) / sizeof(melodia1[0]);


// CANCIÓN 2: El rap de Fernanfloo

//Lista de frecuencias para melodia y acompañamiento junto a sus duraciones

uint16_t melodia2[] = {
    0, 0, 330, 392, 440, 466, 494,
    330, 0, 330, 392, 440, 466, 494,
    330, 0, 330, 392, 440, 466, 494,
    523, 494, 392, 330,

    330, 0, 330, 392, 440, 466, 494,
    330, 0, 330, 392, 440, 466, 494,
    330, 0, 330, 392, 440, 494, 523, 587,
    659, 784, 659, 784,
    880, 0, 784, 659, 587, 523
};

uint16_t duracionesMe2[] = {
    285, 142, 142, 142, 142, 142, 142,
    285, 142, 142, 142, 142, 142, 142,
    285, 142, 142, 142, 142, 142, 142,
    285, 285, 285, 285,

    285, 142, 142, 142, 142, 142, 142,
    285, 142, 142, 142, 142, 142, 142,
    285, 142, 142, 142, 142, 142, 142, 142,
    285, 285, 285, 285,
    285, 142, 142, 142, 285, 285
};

uint16_t acompanamiento2[] = {
    0, 165, 165, 131, 247, 165, 165, 131, 247
};

uint16_t duracionesAc2[] = {
    1140, 1140, 1140, 1140, 1140, 1140, 1140, 1140, 1140
};

//Calculo de cantidad de elementos de melodia y acompañamiento 
int cantidadElementosAc2 = sizeof(acompanamiento2) / sizeof(acompanamiento2[0]);
int cantidadElementosMe2 = sizeof(melodia2) / sizeof(melodia2[0]);


// CANCIÓN 3: Plantas vs Zombies

//Lista de frecuencias para melodia y acompañamiento junto a sus duraciones

uint16_t melodia3[] = {
    523, 523, 494, 523, 440, 349,
    440, 349, 523, 349,

    523, 523, 494, 523, 440, 349,
    440, 349, 523, 349,

    523, 523, 494, 523, 440, 349,
    440, 349, 523, 349,

    392, 440, 392, 440, 349, 262,
    247, 262, 349, 440,
    554, 587, 698, 750,

    523, 523, 494, 523, 440, 349
};

uint16_t duracionesMe3[] = {
    300, 300, 300, 300, 300, 900,
    300, 900, 300, 900,

    300, 300, 300, 300, 300, 900,
    300, 900, 300, 900,

    300, 300, 300, 300, 300, 900,
    300, 900, 300, 900,

    300, 300, 300, 300, 300, 600,
    300, 300, 300, 600,
    300, 300, 300, 600,

    300, 300, 300, 300, 300, 900
};

uint16_t acompanamiento3[] = {
    587, 880, 1046, 880, 587, 880, 1046, 880,
    587, 880, 1046, 880, 587, 880, 1046, 880,
    587, 880, 1046, 880, 587, 880, 1046, 880,
    587, 880, 1046, 880, 587, 880, 1046, 880,
    587, 880, 1046, 880, 587, 880, 1046, 880,

    494, 698, 988, 698, 494, 698, 988, 698,

    587, 880, 1175, 880, 587, 880, 1175, 880,
    587, 880, 1175, 880, 587, 880, 1175, 880
};

uint16_t duracionesAc3[] = {
    300, 300, 300, 300, 300, 300, 300, 300,
    300, 300, 300, 300, 300, 300, 300, 300,
    300, 300, 300, 300, 300, 300, 300, 300,
    300, 300, 300, 300, 300, 300, 300, 300,
    300, 300, 300, 300, 300, 300, 300, 300,

    300, 300, 300, 300, 300, 300, 300, 300,

    300, 300, 300, 300, 300, 300, 300, 300,
    300, 300, 300, 300, 300, 300, 300, 300
};

//Calculo de cantidad de elementos de melodia y acompañamiento 
int cantidadElementosAc3 = sizeof(acompanamiento3) / sizeof(acompanamiento3[0]);
int cantidadElementosMe3 = sizeof(melodia3) / sizeof(melodia3[0]);

//configura los puertos de entrada y salida del sistema
void configurarPuertos(void){
	DDRB = 0x00; // DDRB como ENTRADA para los botones (PB0, PB2, PB3, PB4, PB5)
	DDRB |= (1 << PB1); // PB1 como SALIDA para buzzer de melodía
	
	// PD2, PD4 y PD5 como ENTRADA para botones
	DDRD &= ~((1 << PD2) | (1 << PD4) | (1 << PD5));
	DDRD |= (1 << PD3); // PD3 como SALIDA para el buzzer de acompañamiento
	
	PORTB &= ~(1 << PB1); // Apagamos los buzzers
	PORTD &= ~(1 << PD3); // Apagamos los buzzers

	DDRC = 0x3F; // PC0-PC5 como SALIDA para el display
}

//inicializa los timers
void configurarTimers(void){
	TCCR1A = 0;
	TCCR1B = 0;
	OCR1A = 0;

	TCCR2A = 0;
	TCCR2B = 0;
	OCR2A = 0;
	OCR2B = 0;
}

//configura la frecuencia de la melodia mediante el Timer1
void fijarFrecuenciaMelodia(int frecuencia){
	if (frecuencia == 0){
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

//configura la frecuencia del acompañamiento mediante el Timer2
void fijarFrecuenciaAcompanamiento(uint16_t frecuencia) {
	if (frecuencia == 0) {
		TCCR2B = 0;
		TCCR2A &= ~((1 << COM2B1) | (1 << COM2B0));
		PORTD &= ~(1 << PD3);
		return;
	}

	uint16_t limite = (F_CPU / (2UL * 256UL * frecuencia)) - 1;

	if (limite > 255) {
		limite = 255;
	}

	OCR2A = limite;
	OCR2B = limite;

	TCCR2A = (1 << WGM21) | (1 << COM2B0);
	TCCR2B = (1 << CS22) | (1 << CS21);
}

//apaga ambos buzzers y detiene los timers
void apagarBuzzers(void) {
	TCCR1B = 0;
	TCCR1A &= ~(1 << COM1A0);
	PORTB &= ~(1 << PB1);

	TCCR2B = 0;
	TCCR2A &= ~((1 << COM2B1) | (1 << COM2B0));
	PORTD &= ~(1 << PD3);
}

//configura la comunicacion UART para transmitir y recibir datos
void configurarUART(void) {
	UBRR0H = (unsigned char)(103 >> 8);
	UBRR0L = (unsigned char)(103);

	UCSR0B = (1 << RXEN0) | (1 << TXEN0);
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

//envia un caracter mediante UART
void escribirUART(char dato){
	while (!(UCSR0A & (1 << UDRE0)));
	UDR0 = dato;
}

//envia una cadena de caracteres mediante UART
void escribirUARTString(char *texto){
	while (*texto){
		escribirUART(*texto++);
	}
}

//recibe un caracter mediante UART
char leerUART(void){
	while (!(UCSR0A & (1 << RXC0)));
	return UDR0;
}

//muestra el menu principal y devuelve el modo seleccionado
char menuModos(void){
	escribirUARTString("\r\n");
	escribirUARTString("===== MENU =====\r\n");
	escribirUARTString("P: Piano\r\n");
	escribirUARTString("C: Cancion\r\n");
	escribirUARTString("Seleccione una opcion: ");

	char opcion;
	do {
		opcion = leerUART();
	} while (opcion != 'P' && opcion != 'p' && opcion != 'C' && opcion != 'c');

	return opcion;
}

//muestra el menu de canciones y devuelve la opcion seleccionada
char menuCanciones(void) {
	lcd_clear();
	lcd_write_string("Modo Canción");
	escribirUARTString("\r\n");
	escribirUARTString("===== CANCIONES =====\r\n");
	escribirUARTString("1: Cancion 1\r\n");
	escribirUARTString("2: Cancion 2\r\n");
	escribirUARTString("3: Cancion 3\r\n");
	escribirUARTString("M: Volver\r\n");
	escribirUARTString("Seleccione una cancion: ");
	
	char cancion;
	do {
		cancion = leerUART();
	} while (cancion != '1' && cancion != '2' && cancion != '3'&& cancion != 'm' && cancion != 'M');

	return cancion;
}

//reproduce la melodia y el acompañamiento de una cancion a la vez
void reproducirCancion(uint16_t acompanamiento[],uint16_t melodia[],uint16_t duracionesMe[],uint16_t duracionesAc[], int cantidadElementosMe, int cantidadElementosAc){
	int indiceMe = 0;
	int indiceAc = 0;
	int tiempoMe = 0;
	int tiempoAc = 0;
	
	fijarFrecuenciaMelodia(melodia[indiceMe]);
	fijarFrecuenciaAcompanamiento(acompanamiento[indiceAc]);
	
	tiempoMe = duracionesMe[indiceMe];
	tiempoAc = duracionesAc[indiceAc];
	
	while (indiceMe < cantidadElementosMe && indiceAc < cantidadElementosAc) {
		_delay_ms(1);
		
		tiempoMe--;
		tiempoAc--;
		
		if (tiempoMe == 0) {
			indiceMe++;
			if (indiceMe < cantidadElementosMe) {
				fijarFrecuenciaMelodia(melodia[indiceMe]);
				tiempoMe = duracionesMe[indiceMe];
			}
		}
		
		if (tiempoAc == 0) {
			indiceAc++;
			if (indiceAc < cantidadElementosAc) {
				fijarFrecuenciaAcompanamiento(acompanamiento[indiceAc]);
				tiempoAc = duracionesAc[indiceAc];
			}
		}
	}
	
	apagarBuzzers();
}

//selecciona y reproduce la cancion correspondiente
void elegirMusica(char cancion){
	if (cancion == '1'){
		reproducirCancion(acompanamiento1, melodia1, duracionesMe1, duracionesAc1, cantidadElementosMe1, cantidadElementosAc1);
	}
	if (cancion == '2'){
		reproducirCancion(acompanamiento2, melodia2, duracionesMe2, duracionesAc2, cantidadElementosMe2, cantidadElementosAc2);
	}
	if (cancion == '3') {
		reproducirCancion(acompanamiento3, melodia3, duracionesMe3, duracionesAc3, cantidadElementosMe3, cantidadElementosAc3);
	}
}

//detecta que boton esta presionado y devuelve el numero de la nota correspondiente
int leerBoton(void){
	int valorBoton = 0; // Ningún botón presionado
	
	if (PINB & (1 << PINB0)) {valorBoton = 1;} // DO (262)      -> PB0
	if (PIND & (1 << PIND2)) {valorBoton = 2;} // RE (294)      -> PD2
	if (PINB & (1 << PINB2)) {valorBoton = 3;} // MI (330)      -> PB2
	if (PINB & (1 << PINB3)) {valorBoton = 4;} // FA (349)      -> PB3
	if (PINB & (1 << PINB4)) {valorBoton = 5;} // SOL (392)     -> PB4
	if (PINB & (1 << PINB5)) {valorBoton = 6;} // LA (440)      -> PB5
	if (PIND & (1 << PIND4)) {valorBoton = 7;} // SI (494)      -> PD4 (Cambiado desde PB6)
	if (PIND & (1 << PIND5)) {valorBoton = 8;} // DO ALTO (523) -> PD5 (Cambiado desde PB7)
	
	return valorBoton;
}

//muestra en el LCD el nombre de la nota correspondiente al boton presionado
void mostrarNota(int valorBoton) {
	if (valorBoton == 1) lcd_write_string("DO");
	if (valorBoton == 2) lcd_write_string("RE");
	if (valorBoton == 3) lcd_write_string("MI");
	if (valorBoton == 4) lcd_write_string("FA");
	if (valorBoton == 5) lcd_write_string("SOL");
	if (valorBoton == 6) lcd_write_string("LA");
	if (valorBoton == 7) lcd_write_string("SI");
	if (valorBoton == 8) lcd_write_string("DO ALTO");
}

//reproduce la nota musical correspondiente al boton presionado
void reproducirNotas(int valorBoton){
	if (valorBoton == 1){fijarFrecuenciaMelodia(262);}
	if (valorBoton == 2){fijarFrecuenciaMelodia(294);}
	if (valorBoton == 3){fijarFrecuenciaMelodia(330);}
	if (valorBoton == 4){fijarFrecuenciaMelodia(349);}
	if (valorBoton == 5){fijarFrecuenciaMelodia(392);}
	if (valorBoton == 6){fijarFrecuenciaMelodia(440);}
	if (valorBoton == 7){fijarFrecuenciaMelodia(494);}
	if (valorBoton == 8){fijarFrecuenciaMelodia(523);}
}

//gestiona el modo piano permitiendo tocar notas
void modoPiano(void){
	lcd_clear();
	lcd_write_string("Modo Piano");
	escribirUARTString("\r\nM: Volver\r\n");
	int valorBoton;
	
	while(1){
		if (UCSR0A & (1 << RXC0)) {
			char dato = UDR0;
			if (dato == 'M' || dato == 'm') {
				PORTD &= ~((1 << PD2) | (1 << PD3));
				TCCR1B = 0;
				return;
			}
		}
		
		valorBoton = leerBoton();
		
		if (valorBoton > 0 && valorBoton < 9) {
			lcd_clear();
			mostrarNota(valorBoton);
			reproducirNotas(valorBoton);
		}
		else {
			apagarBuzzers();
		}
	}
}

//gestiona la reproduccion de la cancion seleccionada y muestra su nombre en el LCD
void modoCancion(char cancion){
	lcd_clear();
	lcd_write_string("Modo Cancion");
	lcd_send_command(0xC0);
	
	if (cancion == '1') {
		lcd_write_string("At Doom's Gate");
	}
	if (cancion == '2') {
		lcd_write_string("El rap de Fernanfloo");
	}
	if (cancion == '3') {
		lcd_write_string("PvZ");
	}
	if (cancion == 'M' || cancion == 'm') {
		PORTD &= ~((1 << PD2) | (1 << PD3));
		TCCR1B = 0;
		return;
	}
	
	elegirMusica(cancion);
	apagarBuzzers();
}

//inicializa el sistema y gestiona el menu principal
int main(void) {
	configurarPuertos();
	configurarTimers();
	lcd_init();
	configurarUART();

	while (1) {
		char modo = menuModos();
		if (modo == 'P' || modo == 'p') {
			modoPiano();
		}
		if (modo == 'C' || modo == 'c') {
			char cancion = menuCanciones();
			modoCancion(cancion);
		}
	}

	return 0;
}