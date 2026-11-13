/*
 * Lab5ParteB.c
 *
 * Created: 28/10/2026 17:15:18
 * Author : anidi
 *
 * Descripción: Implementación del control de firmware para ATmega328P.
 * Integra planificación cooperativa, FSM mediante punteros a función,
 * lectura de DHT11, ADC, interfaz LCD I2C, gestión de modos Sleep (IDLE y POWER-DOWN),
 * control mediante registro PRR, supervisión/despertador por Watchdog Timer (WDT)
 * y protocolo de comunicación Bluetooth (USART a 9600 bps).
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
#include <util/atomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#define PCF8574 0x27

#include "dht11.h"
#include "dht11.c"
#include "adc.h"
#include "adc.c"
#include "twi_lcd.h"
#include "twi_lcd.C"
#include "bluetooth.h"
#include "bluetooth.c"
#include "wdt.h"
#include "wdt.c"
#include "protocolo.h"
#include "protocolo.c"
#include "eeprom.h"
#include "eeprom.c"

#define EE_DIR_ULTIMA_ALARMA	0x00
#define EE_DIR_CONTADOR_ALARMAS	0x01
#define DIR_EEPROM_REINICIO		0x02

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

// Registro de alarmas (comando 'L'): 
// 1 Emergencia por boton
// 2 Emergencia por app, 
// 3 reinicio por WDT
// 4 reinicio por falta de energia
uint8_t ultima_alarma = 0;

uint8_t contador_alarmas = 0;
uint8_t evento_actividad_bt = 0;

volatile uint8_t alarma_pendiente = 0;

// Sirve para medir intervalos más largos sin bloquear la ejecución con delays.
volatile uint16_t timer1_ticks = 0; // Contador de interrupciones del Timer1 (cada 100 ms). La usamos para medir intervalos mas largos
                                    // contando cuantas veces se ejecutó la interrupción, ejemplo: si se ejecutó 20 veces (20 x 100ms = 2s) transcurrieron 2 segundos

// Usamos la segunda forma presentada en la diapositiva de maquinas de estados, punteros a funcion
static void (*estado)(void); // Puntero a la función que representa el estado actual

static void st_activo(void);
static void st_standby(void);
static void st_apagado(void);
static void st_seguro(void);

static uint32_t leer_inactividad(void)
{
    uint32_t valor;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        valor = inactividad_ms;
    }

    return valor;
}


// Reiniciar el contador de inactividad.
static void reiniciar_inactividad(void)
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        inactividad_ms = 0;
    }
}

static void registrar_alarma(uint8_t codigo)
{
	ultima_alarma = codigo;
	if (contador_alarmas < 254){
        contador_alarmas++;
    }
	eeprom_actualizar(EE_DIR_ULTIMA_ALARMA, ultima_alarma);
	eeprom_actualizar(EE_DIR_CONTADOR_ALARMAS, contador_alarmas);
}

static void cargar_alarmas(void)
{
	ultima_alarma = eeprom_leer(EE_DIR_ULTIMA_ALARMA);
	contador_alarmas = eeprom_leer(EE_DIR_CONTADOR_ALARMAS);
    // Reiniciar por overflow
	if (ultima_alarma == 0xFF){
        ultima_alarma = 0;
    }
	if (contador_alarmas == 0xFF){
        contador_alarmas = 0;
    }
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

    // Antirrebote de 50 ms (o bypass directo si el Timer0 está deshabilitado por PRR en el modo APAGADO)
    if ((PRR & (1 << PRTIM0)) || ((ahora - ultima_pulsacion) >= 50)) {
        evento_pulsador = 1;
        evento_buzzer = 1;
        ultima_pulsacion = ahora;
    }
}


// Interrupción externa INT1 (se dispara al presionar el botón de emergencia en PD3)
ISR(INT1_vect) {

    static uint32_t ultima_pulsacion = 0;
    uint32_t ahora = ms_ticks;

    // Antirrebote de 50 ms (o bypass directo si el Timer0 está deshabilitado por PRR en el modo APAGADO)
    if ((PRR & (1 << PRTIM0)) || ((ahora - ultima_pulsacion) >= 50)) {
        if (!evento_emergencia){
            alarma_pendiente = 1;
        }
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
	if (PRR & (1 << PRTIM0)) {
		inactividad_ms += 100; // Sumo manualmente los ms si el timer esta apagado
	}

    if (timer1_ticks >= 20) { // 20 * 100ms = 2000ms = 2 segundos

        flag_leer_sensores = 1; // El DHT11 requiere al menos 2 segundos entre lecturas
        timer1_ticks = 0;
    }
}


// Interrupción del Watchdog Timer 
ISR(WDT_vect) {
    flag_wdt_despertar = 1;
	
	if (estado != st_apagado) {
		// Escribimos un código secreto (0xAA) en la posición 0x02 de la EEPROM
		eeprom_actualizar(DIR_EEPROM_REINICIO, 0xAA);
		
		// Forzamos un bucle para que el hardware ejecute el reset final
		while(1);
	}
}


// Procesar tramas recibidas desde la interfaz Bluetooth (HC-05 / USART)
void procesar_comandos_bluetooth(void) {

    uint32_t ahora = ms_ticks;

    if (!bluetooth_pendiente()) {
        // Trama a medias y más de 20 ms inactivo se descarta
        if (protocolo_verificar_timeout(ahora)){
            protocolo_enviar_trama('N', 0, NACK_INCOMPLETA);
        }
        return;
    }

    char c = bluetooth_receive();
    TramaBT_t trama;

    // Si se detecta y valida una trama completa mediante el parser FSM
    if (protocolo_procesar_byte(c, ahora, &trama)) {

        if (!trama.valida) {
            protocolo_enviar_trama('N', trama.comando, trama.error); // NACK con el código de error
            return;
        }

        // Las consultas (E, S, L) no son actividad: si no la app impediria pasar a STANDBY
        if (trama.comando != 'E' && trama.comando != 'S' && trama.comando != 'L') {
            reiniciar_inactividad();
        }

        // Interpretación de comandos 
        if (trama.comando == 'E') { // Consultar Estado actual
            if (estado == st_activo){
                protocolo_enviar_trama('E', 'A', 0);
            } else if (estado == st_standby) {
                protocolo_enviar_trama('E', 'S', 0);
            }else if (estado == st_apagado) {
                protocolo_enviar_trama('E', 'P', 0);
            }else if (estado == st_seguro) {
                protocolo_enviar_trama('E', 'X', 0);
            }
        }
        else if (trama.comando == 'S') { // Solicitar lecturas de sensores
            protocolo_enviar_trama('T', temperatura, dht_valido);
            protocolo_enviar_trama('D', (uint8_t)(valor_adc >> 8), (uint8_t)(valor_adc & 0xFF));
        } 
        else if (trama.comando == 'L') { // Consultar alarmas
            protocolo_enviar_trama('L', ultima_alarma, contador_alarmas);
        }
        else if (trama.comando == 'C') { // Transición de estado guiada por software
            if (estado == st_seguro) { // La app no puede anular una emergencia
                protocolo_enviar_trama('N', 'C', NACK_EMERGENCIA);
                return;
            }
            if (trama.dato1 == '0' || trama.dato1 == 'A') { //Activo
                wdt_configurar_reset_2s();
                wdt_configurar_prr_activo(); // Restaurar periféricos en PRR para permitir comunicación TWI
                twi_init();
                twi_lcd_init();
                twi_lcd_linea("ACTIVO");
                estado = st_activo;
                protocolo_enviar_trama('K', 'C', 0); // ACK comando aceptado
            }
            else if (trama.dato1 == '1' || trama.dato1 == 'S') { // Stand by
                wdt_configurar_reset_2s();
                wdt_configurar_prr_activo(); // Asegurar TWI activo antes de escribir en la LCD
                twi_init();
                twi_lcd_init();
                twi_lcd_linea("ESPERANDO");
                wdt_configurar_prr_standby(); // Aplicar ahorro Standby tras actualizar LCD
                estado = st_standby;
                protocolo_enviar_trama('K', 'C', 0); // ACK comando aceptado
            }
            else if (trama.dato1 == '2' || trama.dato1 == 'P') { // Apagar
                wdt_configurar_prr_activo();
                twi_init();                  
                twi_lcd_init();
                twi_lcd_linea("APAGADO");
                PCF8574_write(0x00); // Apagar retroiluminación de la LCD
                protocolo_enviar_trama('K', 'C', 0); // Responder y avisar ANTES de apagar la USART
                protocolo_enviar_trama('V', 1, 'P'); // Notificar el apagado para no confundir con perdida de conexion
                bluetooth_esperar_tx();              // Esperar a que salga el último byte
                wdt_configurar_prr_apagado();
                estado = st_apagado;
            }
            else {
                protocolo_enviar_trama('N', 'C', NACK_DATO); // Dato invalido
            }
        }
        else if (trama.comando == 'B') { // Buzzer pedido desde la app
            if (estado == st_seguro) { // Enviar solo cuando no hay emergencia
				protocolo_enviar_trama('N', 'B', NACK_EMERGENCIA);
			} else { 
				evento_buzzer = 1;
				evento_actividad_bt = 1;
				protocolo_enviar_trama('K', 'B', 0); //Buzzer aceptado
			}
        }
        else if (trama.comando == 'X') { // Solicitud de Emergencia por app
            if (!evento_emergencia){
                registrar_alarma(2);
            }
            evento_emergencia = 1;
            protocolo_enviar_trama('K', 'X', 0); // Emergencia confirmada
        }
        else if (trama.comando == 'F') { // PRUEBA B WDT: Simular Cuelgue de CPU
            protocolo_enviar_trama('K', 'F', 0); // Confirma prueba WDT
            while(1); // Entra en bucle infinito intencional. Provoca el Reset a los 2s.
        }
        else {
            protocolo_enviar_trama('N', trama.comando, NACK_DESCONOCIDO); // Comando desconocido
        }
    }
}


// Función para ejecutar las tareas periódicas
void procesar_tareas(void) {

    // Alimentar el Watchdog Timer durante la ejecución normal para evitar el reset (Prueba B)
    wdt_reset();
	if (alarma_pendiente) {          // alarma de emergencia marcada por la ISR de INT1
		alarma_pendiente = 0;
		registrar_alarma(1);
	}
    // Lectura del ADC (cada 100 ms) - Solo cuando el sistema está ACTIVO para evitar congelar la CPU con PRADC
    if (flag_adc) {
        flag_adc = 0;
        if (estado == st_activo) {
            valor_adc = adc_read(0);
        }
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

    // Procesamiento no bloqueante de comandos recibidos por Bluetooth
    procesar_comandos_bluetooth();
}


// Estado ACTIVO: Ejecuta las tareas periódicas y mantiene disponibles los periféricos
static void st_activo(void) {

    // Configuración del registro PRR para el estado ACTIVO (Periféricos esenciales habilitados)
    wdt_configurar_prr_activo();

    // Transición: Cualquier estado -> Estado seguro (Solicitud de emergencia)
    if (evento_emergencia) {

        PORTB &= ~(1 << LED_PIN);
        estadoLED = 0;
        
        twi_lcd_linea("EMERGENCIA");
        
        estado = st_seguro; // Transición al estado SEGURO
        return;
    }

    // Si ocurre un evento de actividad del usuario, se reinicia el contador de inactividad
    if (evento_pulsador || evento_actividad_bt) {

        reiniciar_inactividad();
        evento_pulsador = 0;
		evento_actividad_bt = 0;
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
        
        wdt_configurar_prr_activo();
        twi_init();
        twi_lcd_init();
        twi_lcd_linea("ESPERANDO");
        wdt_configurar_prr_standby();

        estado = st_standby; // Transición al estado STANDBY
    }
}


// Estado STANDBY: Muestra el mensaje "Esperando" y reduce actividad
static void st_standby(void) {

    // Transición: Cualquier estado -> Estado seguro (Solicitud de emergencia)
    if (evento_emergencia) {
        
        wdt_configurar_prr_activo(); // Restaurar registro PRR antes de reiniciar TWI
        twi_init();                  // Reinicializar registros del hardware TWI/I2C
        twi_lcd_init();              // Reinicializar TWI/LCD tras salir de ahorro de energía
        twi_lcd_linea("EMERGENCIA");

        estado = st_seguro; // Transición al estado SEGURO
        return;
    }

    // Transición: STANDBY a ACTIVO (Pulsación de botón o evento válido)
    if (evento_pulsador || evento_actividad_bt) {

        reiniciar_inactividad();
        evento_pulsador = 0;
        evento_actividad_bt = 0;
		
        wdt_configurar_prr_activo(); // Restaurar registro PRR antes de reiniciar TWI
        twi_init();                  // Reinicializar registros del hardware TWI/I2C
        twi_lcd_init();              // Reinicializar TWI/LCD tras salir de ahorro de energía
        twi_lcd_linea("ACTIVO");
        
        estado = st_activo; // Transición al estado ACTIVO
        return;
    }

    // Transición: STANDBY a APAGADO (30 segundos sin actividad acumulados desde el último evento)
    if (leer_inactividad() >= 30000UL) {
        
        wdt_configurar_prr_activo(); // Restaurar PRR/TWI antes de escribir en LCD
        twi_init();                  // Reinicializar registros del hardware TWI/I2C
        twi_lcd_init();
        twi_lcd_linea("APAGADO");
        _delay_ms(100);
        PCF8574_write(0x00); // Apagar retroiluminación de la LCD
        protocolo_enviar_trama('V', 1, 'P'); // Avisar a la app antes de apagar la USART
        bluetooth_esperar_tx();              // Esperar a que salga el último byte
        wdt_configurar_prr_apagado();
        estado = st_apagado; // Transición al estado APAGADO
        return;
    }

    // Apagar el LED mientras se encuentra en STANDBY
    flag_led = 0;
    PORTB &= ~(1 << LED_PIN);
    estadoLED = 0;

    // Ajustar registro PRR para ahorro intermedio en STANDBY antes de dormir
    wdt_configurar_prr_standby();

    // Entrar en modo SLEEP_MODE_IDLE entre tareas pendientes para ahorrar energía
    entrar_modo_idle();
}


// Estado APAGADO: Deshabilita periféricos no necesarios
static void st_apagado(void) {

    // Apagar el LED mientras se encuentra en APAGADO
    flag_led = 0;
    PORTB &= ~(1 << LED_PIN);
    estadoLED = 0;
    
    // Transición: Cualquier estado -> Estado seguro (Solicitud de emergencia)
    if (evento_emergencia) {
        
		wdt_configurar_reset_2s();
        wdt_configurar_prr_activo(); // Restaurar registro PRR antes de reinicializar el TWI/LCD
        twi_init();                  // Reinicializar registros del hardware TWI/I2C
        twi_lcd_init();
        bluetooth_init();            // Reinicializar la USART para poder responder en SEGURO

        twi_lcd_linea("EMERGENCIA");
        
        estado = st_seguro; // Transición al estado SEGURO
        return;
    }

    // Transición: APAGADO a ACTIVO
    if (evento_pulsador) {

        reiniciar_inactividad();
        evento_pulsador = 0;
        
        wdt_configurar_reset_2s();   // Restaurar WDT en Modo Reset (Prueba B)
        wdt_configurar_prr_activo(); // Restaurar registro PRR antes de reinicializar el TWI/LCD
        twi_init();                  // Reinicializar registros del hardware TWI/I2C
        twi_lcd_init();              // Reinicializar el expansor y controlador LCD
        bluetooth_init();            // Reinicializar la interfaz serie USART
        adc_init();                  // Reinicializar el ADC
        
        twi_lcd_linea("ACTIVO");
        
        estado = st_activo; // Transición al estado ACTIVO
        return;
    }

    // PRUEBA A WDT: Despertador periódico cada 8 segundos en Power-Down
    if (flag_wdt_despertar) {
        flag_wdt_despertar = 0;

        wdt_configurar_prr_activo();
        adc_init();
        bluetooth_init();

        uint16_t temp_adc = adc_read(0);
        protocolo_enviar_trama('V', 4, (uint8_t)(temp_adc >> 2));
        bluetooth_esperar_tx();

        wdt_configurar_prr_apagado();
    }

    // Configuración de ahorro profundo mediante registro PRR en APAGADO
    wdt_configurar_prr_apagado();

    // Configurar WDT en Modo Interrupción 8s para la Prueba A
    wdt_configurar_interrupcion_8s();

    // PROCEDIMIENTO ESTRICTO PARA DESPERTAR DE POWER-DOWN
    // En Power-Down, el detector de flancos no funciona porque el reloj de la CPU está apagado.
    // Para que los botones puedan despertar al microcontrolador, debemos configurarlos por "Nivel Bajo".
    EIMSK &= ~((1 << INT0) | (1 << INT1));   // Apagar interrupciones temporalmente para evitar disparos falsos
    EICRA &= ~((1 << ISC01) | (1 << ISC11)); // Configurar el disparo por Nivel Bajo (borrar los bits de flanco)
    EIFR = (1 << INTF0) | (1 << INTF1);      // Limpiar cualquier bandera de interrupción pendiente
    EIMSK |= (1 << INT0) | (1 << INT1);      // Volver a encender las interrupciones externas

    // Entrar en modo SLEEP_MODE_PWR_DOWN (Bajo consumo profundo)
    entrar_modo_power_down(); 
    // La CPU se detiene aquí. Cuando el pin baje a 0V, el micro se despertará asíncronamente.

    // AL DESPERTAR DEL POWER-DOWN
    // Restaurar inmediatamente el comportamiento original (disparo por Flanco de bajada)
    EIMSK &= ~((1 << INT0) | (1 << INT1));   // Apagar interrupciones temporalmente
    EICRA |= (1 << ISC01) | (1 << ISC11);    // Restaurar la configuración de flanco de bajada
    EIFR = (1 << INTF0) | (1 << INTF1);      // Limpiar cualquier bandera pendiente
    EIMSK |= (1 << INT0) | (1 << INT1);      // Volver a encender interrupciones para el funcionamiento normal
}


// Estado SEGURO: Recuperación y detención de actuadores ante solicitud de emergencia
static void st_seguro(void) {

    // Apagar LED para garantizar estado seguro de actuadores
    flag_led = 0;
    PORTB &= ~(1 << LED_PIN);
    PORTB |= (1 << BUZZER_PIN);
    
    estadoLED = 0;

    if ((PIND & (1 << BOTON_EMERGENCIA_PIN)) && evento_pulsador) {

        evento_emergencia = 0;
        evento_pulsador = 0;

        PORTB &= ~(1 << BUZZER_PIN);
        reiniciar_inactividad();
        
        wdt_configurar_prr_activo(); // Restaurar registro PRR antes de escribir en TWI/LCD
        twi_init();                  // Reinicializar registros del hardware TWI/I2C
        twi_lcd_init();
        twi_lcd_linea("ACTIVO");
        
        estado = st_activo; // Transición al estado ACTIVO
    }
}


// función main
int main(void)
{
	// Configuración de puertos.
	configurarPuertos();

	// Inicialización de la gestión de energía y lectura de causa de reset por WDT
	wdt_manager_init();
	cargar_alarmas();
	if (eeprom_leer(DIR_EEPROM_REINICIO) == 0xAA) {
		causa_ultimo_reset = RESET_WATCHDOG;
		eeprom_actualizar(DIR_EEPROM_REINICIO, 0x00);   // Borramos la bandera para el próximo reinicio normal
	}
	
	// Inicialización de periféricos
	adc_init();
	dht11_init();
	timer0_ctc_init();
	timer1_ctc_init();
	twi_init();
	twi_lcd_init();
	bluetooth_init();
	
	// Configurar Watchdog Timer en Modo Reset a ~2s para supervisión contra bloqueos (Prueba B)
	wdt_configurar_reset_2s();

	// Verificar si el inicio actual proviene de un reset provocado por el Watchdog Timer
	if (causa_ultimo_reset == RESET_WATCHDOG) {
		registrar_alarma(3);    // Enviar registro reinicio
		protocolo_enviar_trama('V', 3, 0); 
	}else {
		registrar_alarma(4); //reinicio por perdida de energia
	}

	// Definición del estado inicial
	estado = st_activo; // Estado inicial: ACTIVO

	sei(); // Habilita interrupciones globales obligatorias para los timers
	
	while (1)
	{
		procesar_tareas();    // Ejecutar las tareas periódicas
		estado();           // Ejecuta la función del estado actual
	}
}