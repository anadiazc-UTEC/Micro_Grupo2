#define F_CPU 16000000UL
#define LCD_ADDR 0x27

#include <avr/io.h>
#include <util/delay.h>

int acompanamiento1[] = {}; // Contiene la lista de frecuencias necesarias para el acompanamiento de la cancion 1
int duracionesAc1[] = {}; // Contiene la lista de duraciónes de cada nota en milisegundos para el acompañamiento 1

int melodia1[] = {}; // Contiene la lista de frecuencias necesaria para la melodia de la cancion 1
int duracionesMe1[] = {}; // Contiene la lista de duraciónes de cada nota en milisegundos para la melodia 1

//Guarda el largo de las melodias y acompañamientos con el fin de saber cuando se termina la canción cuando esta se esté reproduciendo.
int cantidadElementosAc1 = sizeof(acompanamiento1) / sizeof(acompanamiento1[0]);
int cantidadElementosMe1 = sizeof(melodia1) / sizeof(melodia1[0]);


int acompanamiento2[] = {}; // Contiene la lista de frecuencias necesarias para el acompanamiento de la cancion 2
int duracionesAc2[] = {}; // Contiene la lista de duraciónes de cada nota en milisegundos para el acompañamiento 2

int melodia2[] = {}; // Contiene la lista de frecuencias necesaria para la melodia de la cancion 2
int duracionesMe2[] = {}; // Contiene la lista de duraciónes de cada nota en milisegundos para la melodia 2

//Guarda el largo de las melodias y acompañamientos con el fin de saber cuando se termina la canción cuando esta se esté reproduciendo.
int cantidadElementosAc2 = sizeof(acompanamiento2) / sizeof(acompanamiento2[0]);
int cantidadElementosMe2 = sizeof(melodia2) / sizeof(melodia2[0]);


int acompanamiento3[] = {}; // Contiene la lista de frecuencias necesarias para el acompanamiento de la cancion 3
int duracionesAc3[] = {}; // Contiene la lista de duraciónes de cada nota en milisegundos para el acompañamiento 3

int melodia3[] = {}; // Contiene la lista de frecuencias necesaria para la melodia de la cancion 3
int duracionesMe3[] = {}; // Contiene la lista de duraciónes de cada nota en milisegundos para la melodia 3

//Guarda el largo de las melodias y acompañamientos con el fin de saber cuando se termina la canción cuando esta se esté reproduciendo.
int cantidadElementosAc3 = sizeof(acompanamiento3) / sizeof(acompanamiento3[0]);
int cantidadElementosMe3 = sizeof(melodia3) / sizeof(melodia3[0]);

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
void configurarTimers(void){
	TCCR1A = 0;
	TCCR1B = 0;
	OCR1A = 0;

	TCCR2A = 0;
	TCCR2B = 0;
	OCR2A = 0;
	OCR2B = 0;
}
	
void I2C_init(void) {
	TWBR = 72; // Configura SCL a ~100 kHz con F_CPU = 16 MHz
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

void lcd_send_nibble(uint8_t nibble, uint8_t rs) {
	// P3 = Backlight (0x08), P2 = Enable (1 << 2), P0 = RS (rs)
	uint8_t data = nibble | rs | 0x08;
	
	I2C_start();
	I2C_write(LCD_ADDR << 1);
	I2C_write(data | (1 << 2));  // Pulse Enable HIGH
	I2C_write(data & ~(1 << 2)); // Pulse Enable LOW
	I2C_stop();
}

void lcd_send_byte(uint8_t byte, uint8_t rs) {
	lcd_send_nibble(byte & 0xF0, rs);
	lcd_send_nibble((byte << 4) & 0xF0, rs);
}

void lcd_send_command(uint8_t cmd) {
	lcd_send_byte(cmd, 0);
	_delay_ms(2);
}

void lcd_send_data(uint8_t data) {
	lcd_send_byte(data, 1);
	_delay_us(50);
}

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

void lcd_write_string(const char *str) {
	while (*str) {
		lcd_send_data(*str++);
	}
}

void lcd_clear(void) {
	lcd_send_command(0x01);
	_delay_ms(2);
}
	
void configurarUART(void) {
	UBRR0H = (unsigned char)(103 >> 8);
	UBRR0L = (unsigned char)(103);

	UCSR0B = (1 << RXEN0) | (1 << TXEN0);
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void escribirUART(char dato){
	while (!(UCSR0A & (1 << UDRE0)));
	UDR0 = dato;
}

void escribirUARTString(char *texto){
	while (*texto){
		escribirUART(*texto++);
	}
}

char leerUART(void){
	while (!(UCSR0A & (1 << RXC0)));
	return UDR0;
}
	
char menuModos(void);
	// Muestra el menú de modo. 
	char opcion = 0;
	// Se queda esperando hasta que presionen P, p, C o c
	int opcion = leerUART;
	// return opcion
	
char menuCanciones(void);
	// Muestra el menú de Cancion
	// Esperar hasta recibir '1', '2' o '3'
	int cancion = leerUART;
	// return cancion

	
void elegirMusica(char cancion);
	// Si cancion == '1' llama a reproducirCancion(acompanamiento1, melodia1, duracionesMe1, duracionesAc1, cantidadElementosMe1, cantidadElementosAc1)
	// Si cancion == '2' llama a reproducirCancion(acompanamiento2, melodia2, duracionesMe2, duracionesAc2, cantidadElementosMe2, cantidadElementosAc2)
	// Si cancion == '3' llama a reproducirCancion(acompanamiento3, melodia3, duracionesMe3, duracionesAc3, cantidadElementosMe3, cantidadElementosAc3)

void reproducirCancion(int acompanamiento[],int melodia[],int duracionesMe[],int duracionesAc[], int cantidadElementosMe, int cantidadElementosAc);
	// Envia a los buzzers la melodia y acompanamiento correspondientes. 

int leerBoton(void){
	int valorBoton = 0; // Ningún botón presionado
	
	if (PINB & (1 << PINB0)) {valorBoton = 1;} // DO (262)
	if (PIND & (1 << PIND2)) {valorBoton = 2;} // RE (294)
	if (PINB & (1 << PINB2)) {valorBoton = 3;} // MI (330)
	if (PINB & (1 << PINB3)) {valorBoton = 4;} // FA (349)
	if (PINB & (1 << PINB4)) {valorBoton = 5;} // SOL (392)
	if (PINB & (1 << PINB5)) {valorBoton = 6;} // LA (440)
	if (PIND & (1 << PIND4)) {valorBoton = 7;} // SI (494)
	if (PIND & (1 << PIND5)) {valorBoton = 8;} // DO ALTO (523)
	
	return valorBoton;
}

void reproducirNotas(int valorBoton);
	// Si valorBoton == 1 reproduce la frec del do (262)
	// Si valorBoton == 2 reproduce la frec del re (294)
	// Si valorBoton == 3 reproduce la frec del mi (330)
	// Si valorBoton == 4 reproduce la frec del fa (349)
	// Si valorBoton == 5 reproduce la frec del sol (392)
	// Si valorBoton == 6 reproduce la frec del la (440)
	// Si valorBoton == 7 reproduce la frec del si (494)
	// Si valorBoton == 8 reproduce la frec del do alto (523)

	
void modoPiano(void);
	// Muestra en el display "Modo Piano"
	while(1){
		// Si desde la PC enviaron 'M' o 'm' para volver al menú sale de modoPiano() y regresa al main()
		// Llama a leerBoton() para saber que boton se esta apretando
		// Si se presiona un boton (valorBoton entre 1 y 8)
			// Llama a reproducirNotas(valorBoton) para reproducir la frecuencia correspondiente
			// Muestra en el display el nombre de la nota
		// Si no se presiona ningun boton (valorBoton == 0)
			// Limpia el display
			// Apaga los buzzers.
		}
	
		
void modoCancion(char cancion);
	// Llama a elegirMusica(cancion)
	// Muestra en el display el nombre de la cancion que se está reproduciendo
	// Al finalizar la canción apaga los buzzers.
	
int main(void)
{
	configurarPuertos();
	configurarTimer();
	configurarLCD();
	configurarUART();

	while (1)
	{
		// Espera a que se seleccione el modo (P/p o C/c)
		char modo = menuModos();
		// Si modo == 'P' llama a modoPiano();
		// Si modo == 'C':
			// Espera que se seleccione la cancion ('1', '2', '3')
			char cancion = menuCanciones();
			// llama a modoCancion y le pasa la canción correspondiente. 
			modoCancion(cancion);
	}

	return 0;
}