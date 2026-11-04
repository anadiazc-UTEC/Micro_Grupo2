#ifndef DHT11_H_
#define DHT11_H_
#define F_CPU 16000000UL
#include <util/delay.h>

// Inicializar pin del DHT11
void dht11_init(void);
/*
 * Esperar a que la entrada deje de tener el nivel indicado.
 * El limite evita que un DHT desconectado congele todo el programa.
 */
static uint8_t dht_esperar_cambio(uint8_t nivel, uint16_t limite_us);
// Leer datos del DHT11
uint8_t dht11_read(uint8_t *humidity, uint8_t *temperature);
#endif