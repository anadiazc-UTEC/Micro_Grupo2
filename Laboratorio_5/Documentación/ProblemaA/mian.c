/*
 * Lab5ParteA.c
 *
 * Created: 28/10/2026 17:15:18
 * Author : anidi
 */

#define F_CPU 16000000UL

#define DHT11_PIN PD4
#define DHT11_PORT PORTD
#define DHT11_DDR DDRD
#define DHT11_PINR PIND

#define LED_PIN PB0
#define BOTON_EMERGENCIA_PIN PD3
#define BUZZER_PIN PB1

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <stdio.h>
#define PCF8574 0x27

#include "dht11.h"
#include "dht11.c"
#include "twi_lcd.h"

// Variables globales de control de tiempo y eventos
// SE DECLARAN 'volatile' PORQUE:
// Se comparten entre el flujo principal y Rutinas de Servicio de Interrupción.
// 'volatile' evita que el compilador optimice estas variables guardándolas en registros
// y fuerce la lectura/escritura directa desde RAM cada vez que se consultan o modifican.

volatile uint32_t inactividad_ms = 0; // Contador de tiempo sin actividad en ms
volatile uint8_t evento_pulsador = 0; // Flag para indicar pulsación de botón
volatile uint8_t evento_emergencia = 0; // Flag para indicar solicitud de emergencia
volatile uint8_t evento_buzzer = 0; // Flag para encender el buzzer

volatile uint32_t ms_ticks = 0; 
volatile uint8_t flag_led = 0;
volatile uint8_t flag_lcd = 0;
volatile uint8_t flag_adc = 0;

uint8_t estadoLED = 0;

volatile uint16_t contador_lcd = 0;
volatile uint16_t contador_led = 0;
volatile uint8_t flag_leer_sensores = 0;

// Variables para lecturas
uint16_t valor_adc = 0;
uint8_t humedad = 0;
uint8_t temperatura = 0;
uint8_t dht_valido = 0;


// Sirve para medir intervalos más largos sin bloquear la ejecución con delays.
volatile uint16_t timer1_ticks = 0; // Contador de interrupciones del Timer1 (cada 100 ms). La usamos para medir intervalos mas largos
									// contando cuantas veces se ejecuta la interrupción, ejemplo: si se ejecutó 20 veces (20 x 100ms = 2s) transcurrieron 2 segundos

// Usamos la segunda forma presentada en la diapositiva de maquinas de estados, punteros a funcion
static void (*estado)(void); // Puntero a la función que representa el estado actual

static void st_activo(void);
static void st_standby(void);
static void st_apagado(void);
static void st_seguro(void);

static uint32_t leer_inactividad(void)
{
	uint32_t valor;

	valor = inactividad_ms;

	return valor;
}


// Reiniciar el contador de inactividad.
static void reiniciar_inactividad(void)
{
	inactividad_ms = 0;
}

// Configuración de puertos
void configurarPuertos(void) {

	// Configurar LED (PB0) y Buzzer (PB1) como salidas
	DDRB |= (1 << PB0) | (1 << PB1);
	PORTB &= ~((1 << PB0) | (1 << PB1));

	// Configurar PD2 (INT0) y PD3 (INT1) como entradas con pull-up activado
	DDRD &= ~((1 << PD2) | (1 << PD3));
	PORTD |= (1 << PD2) | (1 << PD3);

	// Configurar disparo por flanco de bajada en INT0 e INT1
	EICRA |= (1 << ISC01) | (1 << ISC11);
	EICRA &= ~((1 << ISC00) | (1 << ISC10));

	// Limpiar posibles interrupciones pendientes
	EIFR = (1 << INTF0) | (1 << INTF1);

	// Habilitar interrupciones externas INT0 e INT1
	EIMSK |= (1 << INT0) | (1 << INT1);
}


// Interrupción INT0 (Pulsador general / Buzzer)
ISR(INT0_vect) {

	static uint32_t ultima_pulsacion = 0;
	uint32_t ahora = ms_ticks;

	// Antirrebote de 50 ms
	if ((ahora - ultima_pulsacion) >= 50) {
		evento_pulsador = 1;
		evento_buzzer = 1;
		ultima_pulsacion = ahora;
	}
}


// Interrupción externa INT1 (se dispara al presionar el botón de emergencia en PD3)
ISR(INT1_vect) {

	static uint32_t ultima_pulsacion = 0;
	uint32_t ahora = ms_ticks;

	// Antirrebote de 50 ms
	if ((ahora - ultima_pulsacion) >= 50) {
		evento_emergencia = 1;
		ultima_pulsacion = ahora;
	}
}

// Configurar Timer0 en modo CTC con interrupciones cada 1ms (f_CPU = 16 MHz)
void timer0_ctc_init(void) {

	TCCR0A = (1 << WGM01); // Modo CTC
	OCR0A = 249; // Valor de comparación para 1ms
	TIMSK0 = (1 << OCIE0A); // Habilitar interrupción por comparación A
	TCCR0B = (1 << CS01) | (1 << CS00); // Preescaler 64 e inicio del timer
}


ISR(TIMER0_COMPA_vect) {

	ms_ticks++;
	inactividad_ms += 1; // Incrementa cada 1 milisegundo

	contador_led += 1;
	contador_lcd += 1;

	if (contador_led >= 200) {
		flag_led = 1;
		contador_led = 0; // Cada 200ms
	}

	if (contador_lcd >= 500) {
		flag_lcd = 1;
		contador_lcd = 0; // Cada 500ms
	}
}


// Configurar Timer1 en modo CTC con interrupciones cada 100ms
void timer1_ctc_init(void) {

	TCCR1A = 0; // Bits WGM11 y WGM10 en 0
	OCR1A = 6249; // Valor de comparación para 100ms exactos (16MHz y Preescaler 256)
	TIMSK1 = (1 << OCIE1A); // Habilitar interrupción por comparación A
	TCCR1B = (1 << WGM12) | (1 << CS12); // Modo CTC (WGM12=1) y Preescaler 256
}


// Interrupción del Timer1 (se ejecuta cada 100 ms)
ISR(TIMER1_COMPA_vect) {

	timer1_ticks++;
	flag_adc = 1;

	if (timer1_ticks >= 20) { // 20 * 100ms = 2000ms = 2 segundos

		flag_leer_sensores = 1; // El DHT11 requiere al menos 2 segundos entre lecturas
		timer1_ticks = 0;
	}
}


// Función para ejecutar las tareas periódicas
void procesar_tareas(void) {

	// Lectura del ADC (cada 100 ms)
	if (flag_adc) {
		flag_adc = 0;
		valor_adc = 0;
	}

	// Lectura del sensor de temperatura (cada 2 s)
	if (flag_leer_sensores) {
		flag_leer_sensores = 0;
		dht_valido = dht11_read(&humedad, &temperatura);
	}

	// Actualización del LCD (cada 500 ms)
	if (flag_lcd) {

		flag_lcd = 0;

		// Actualizar los valores solamente cuando el sistema está ACTIVO
		if (estado == st_activo) {
			char buffer[17];

			if (dht_valido) {
				snprintf(buffer, sizeof(buffer), "ADC:%4u T:%2uC", valor_adc, temperatura);
			}
			else {
				snprintf(buffer, sizeof(buffer), "ADC:%4u T:ERROR", valor_adc);
			}

			twi_lcd_linea(buffer);
		}
	}

	// Activación del Buzzer por evento del pulsador
	if (evento_buzzer) {

		PORTB |= (1 << BUZZER_PIN);
		_delay_ms(100);
		PORTB &= ~(1 << BUZZER_PIN);

		evento_buzzer = 0;
	}
}


// Estado ACTIVO: Ejecuta las tareas periódicas y mantiene disponibles los periféricos
static void st_activo(void) {

	// Transición: Cualquier estado -> Estado seguro (Solicitud de emergencia)
	if (evento_emergencia) {

		PORTB &= ~(1 << LED_PIN);
		estadoLED = 0;
		
		twi_lcd_linea("EMERGENCIA");
		
		estado = st_seguro; // Transición al estado SEGURO
		return;
	}

	// Si ocurre un evento de actividad del usuario, se reinicia el contador de inactividad
	if (evento_pulsador) {

		reiniciar_inactividad();
		evento_pulsador = 0;
	}

	// Parpadeo del LED solamente en el estado ACTIVO
	if (flag_led) {

		flag_led = 0;
		estadoLED = !estadoLED;

		if (estadoLED) {
			PORTB |= (1 << LED_PIN);
		}
		else {
			PORTB &= ~(1 << LED_PIN);
		}
	}

	// Transición: ACTIVO a STANDBY (10 segundos sin actividad)
	if (leer_inactividad() >= 10000UL) {

		PORTB &= ~(1 << LED_PIN);
		estadoLED = 0;
		flag_led = 0;
		
		twi_lcd_linea("ESPERANDO");

		estado = st_standby; // Transición al estado STANDBY
	}
}


// Estado STANDBY: Muestra el mensaje "Esperando" y reduce actividad
static void st_standby(void) {

	// Apagar el LED mientras se encuentra en STANDBY
	flag_led = 0;
	PORTB &= ~(1 << LED_PIN);
	estadoLED = 0;

	// Transición: Cualquier estado -> Estado seguro (Solicitud de emergencia)
	if (evento_emergencia) {
		
		twi_lcd_linea("EMERGENCIA");

		estado = st_seguro; // Transición al estado SEGURO
		return;
	}

	// Transición: STANDBY a ACTIVO (Pulsación de botón o evento válido)
	if (evento_pulsador) {

		reiniciar_inactividad();
		evento_pulsador = 0;
		
		twi_lcd_linea("ACTIVO");
		
		estado = st_activo; // Transición al estado ACTIVO
	}

	// Transición: STANDBY a APAGADO (30 segundos sin actividad acumulados desde el último evento)
	else if (leer_inactividad() >= 30000UL) {
		
		twi_lcd_linea("APAGADO");
		PCF8574_write(0x00);
		estado = st_apagado; // Transición al estado APAGADO
	}
}


// Estado APAGADO: Deshabilita periféricos no necesarios
static void st_apagado(void) {

	// Apagar el LED mientras se encuentra en APAGADO
	flag_led = 0;
	PORTB &= ~(1 << LED_PIN);
	estadoLED = 0;
	
	// Transición: Cualquier estado -> Estado seguro (Solicitud de emergencia)
	if (evento_emergencia) {
		
		twi_lcd_init();

		twi_lcd_linea("EMERGENCIA");
		estado = st_seguro; // Transición al estado SEGURO
		return;
	}

	// Transición: APAGADO a ACTIVO
	if (evento_pulsador) {

		reiniciar_inactividad();
		evento_pulsador = 0;
		twi_lcd_init();
		
		twi_lcd_linea("ACTIVO");
		estado = st_activo; // Transición al estado ACTIVO
	}
}


// Estado SEGURO: Recuperación y detención de actuadores ante solicitud de emergencia
static void st_seguro(void) {

	// Apagar LED para garantizar estado seguro de actuadores
	flag_led = 0;
	PORTB &= ~(1 << LED_PIN);
	PORTB |= (1 << BUZZER_PIN);
	
	estadoLED = 0;
	
	/*
	 * Para salir: soltar el boton de emergencia y pulsar el boton general.
	 * En el codigo original evento_emergencia nunca volvia a cero.
	 */

	if ((PIND & (1 << BOTON_EMERGENCIA_PIN)) && evento_pulsador) {

		evento_emergencia = 0;
		evento_pulsador = 0;

		PORTB &= ~(1 << BUZZER_PIN);
		reiniciar_inactividad();
		
		twi_lcd_linea("ACTIVO");
		
		estado = st_activo; // Transición al estado ACTIVO
	}
}


// función main
int main(void)
{
	// Configuración de puertos.
	configurarPuertos();

	// Inicialización de periféricos
	
	dht11_init();
	timer0_ctc_init();
	timer1_ctc_init();
	twi_init();
	twi_lcd_init();
	// Definición del estado inicial
	estado = st_activo; // Estado inicial: ACTIVO

	sei(); // Habilita interrupciones globales obligatorias para los timers

	while (1)
	{
	  	procesar_tareas();	// Ejecutar las tareas periódicas
		estado();           // Ejecuta la función del estado actual
	}
}
