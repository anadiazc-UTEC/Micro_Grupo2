#include "bluetooth.h"

#define BT_BUF_N 64 // potencia de 2: el indice da la vuelta con una mascara

static volatile uint8_t bt_buf[BT_BUF_N];
static volatile uint8_t bt_head = 0;		// lo escribe la ISR
static volatile uint8_t bt_tail = 0;		// lo escribe el programa principal
static volatile uint8_t bt_tx_pendiente = 0;

ISR(USART_RX_vect) {
	uint8_t c = UDR0;
	uint8_t next = (uint8_t)((bt_head + 1) & (BT_BUF_N - 1));
	if (next != bt_tail) {		// si el buffer esta lleno, se descarta el byte
		bt_buf[bt_head] = c;
		bt_head = next;
	}
}

// Inicializar el puerto serie USART del ATmega328P a 9600 baudios (F_CPU = 16 MHz)
void bluetooth_init(void) {

	// Valor de UBRR0 para 9600 bps con F_CPU = 16 MHz:
	// UBRR0 = (16000000 / (16 * 9600)) - 1 = 103.16 -> 103
	uint16_t ubrr_val = 103;

	UBRR0H = (uint8_t)(ubrr_val >> 8);
	UBRR0L = (uint8_t)(ubrr_val);

	// Vaciar el buffer (por ejemplo, al reactivar la USART al salir de APAGADO)
	bt_head = 0;
	bt_tail = 0;
	bt_tx_pendiente = 0;

	// Habilitar recepcion, transmision e interrupcion de recepcion 
	UCSR0B = (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0);

	// Configurar formato de trama: 8 bits de datos, 1 bit de parada, sin paridad (8N1)
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

// Hay bytes recibidos en el buffer sin procesar
bool bluetooth_pendiente(void) {
	return bt_head != bt_tail;
}

// Sacar un byte del buffer (llamar solo si bluetooth_pendiente() es true)
char bluetooth_receive(void) {
	uint8_t c;
	if (bt_head == bt_tail) {
		return 0;
	}
	c = bt_buf[bt_tail];
	bt_tail = (uint8_t)((bt_tail + 1) & (BT_BUF_N - 1));
	return (char)c;
}

// Transmitir un caracter enviandolo al registro UDR0
void bluetooth_transmit(char data) {

	// Si la USART esta apagada por PRR no se transmite (evita quedar colgado)
	if (PRR & (1 << PRUSART0)) {
		return;
	}

	// Esperar a que el bufer de transmision este listo para recibir nuevos datos
	while (!(UCSR0A & (1 << UDRE0)));

	// Borrar TXC0 para saber cuando termina este byte.
	// FE0, DOR0 y UPE0 deben escribirse en 0; U2X0 y MPCM0 se conservan.
	UCSR0A = (uint8_t)((UCSR0A & ((1 << U2X0) | (1 << MPCM0))) | (1 << TXC0));

	UDR0 = data;
	bt_tx_pendiente = 1;
}

// Esperar a que el ultimo byte termine de salir por el cable.
// Escribir en UDR0 tarda 1 ms desde que se cambia hasta que se ejecuta. Hay que
// llamar a esto antes de apagar la USART con PRR, o la ultima trama se corta.
void bluetooth_esperar_tx(void) {
	if (!bt_tx_pendiente) {
		return;
	}
	if (PRR & (1 << PRUSART0)) {
		bt_tx_pendiente = 0;
		return;
	}
	while (!(UCSR0A & (1 << TXC0)));
	bt_tx_pendiente = 0;
}

// Transmitir una cadena de caracteres desde la memoria RAM
void bluetooth_print(const char *str) {
	while (*str != '\0') {
		bluetooth_transmit(*str++);
	}
}

// Transmitir una cadena de caracteres almacenada en memoria Flash (PROGMEM)
void bluetooth_print_p(const char *str) {
	char c;
	while ((c = pgm_read_byte(str++))) {
		bluetooth_transmit(c);
	}
}
