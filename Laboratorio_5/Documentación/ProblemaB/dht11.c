
// Info importante para entender el funcionamiento del codigo:
// DHT11:
// Gracias a la sección 5.3 del datasheet del DHT11(https://www.mouser.com/datasheet/2/758/DHT11-Technical-Data-Sheet-Translated-Version-1143054.pdf)
// podemos saber que una vez que el DHT detecta la señal de inicio envía una señal de respuesta de nivel de voltaje bajo de 80 µs,
// luego envía una señal de voltaje alto durante otros 80 µs
// La señal baja (0) dura entre 26 y 28 µs, mientras que la alta dura 70 µs
// Checksum = Humedad Entera + Humedad Decimal + Temp Entera + Temp Decimal
// El DHT11 transmite exactamente 40 bits (5 bytes en total):
// Byte 0: Parte entera de la Humedad
// Byte 1: Parte decimal de la Humedad
// Byte 2: Parte entera de la Temperatura
// Byte 3: Parte decimal de la Temperatura
// Byte 4: Checksum (Suma de comprobación)

#include "dht11.h"
#include <xc.h>

// Inicializar pin del DHT11
void dht11_init() {

	// Inicialmente poner el pin como entrada
	DHT11_DDR &= ~(1 << DHT11_PIN);

	// Activar resistencia pull-up
	DHT11_PORT |= (1 << DHT11_PIN);
}

/*
 * Esperar a que la entrada deje de tener el nivel indicado.
 * El limite evita que un DHT desconectado congele todo el programa.
 */

static uint8_t dht_esperar_cambio(uint8_t nivel, uint16_t limite_us)
{
    while (limite_us--) {

        uint8_t actual = (DHT11_PINR & (1 << DHT11_PIN)) ? 1 : 0;

        if (actual != nivel) {
            return 1;
        }

        _delay_us(1);
    }

    return 0;
}


// Leer datos del DHT11
uint8_t dht11_read(uint8_t *humidity, uint8_t *temperature) {

    uint8_t data[5] = {0, 0, 0, 0, 0};
    uint8_t checksum;

    // Enviar señal de inicio al DHT11
    DHT11_DDR |= (1 << DHT11_PIN);
    DHT11_PORT &= ~(1 << DHT11_PIN);
    _delay_ms(18);

    // Poner pin a HIGH
    DHT11_PORT |= (1 << DHT11_PIN);
    _delay_us(30);

    // Configurar pin como entrada
    DHT11_DDR &= ~(1 << DHT11_PIN);

    // Verificar respuesta del DHT11:
    // LOW 80 us, HIGH 80 us y luego comienzan los bits.
    if (!dht_esperar_cambio(1, 120)) return 0;
    if (!dht_esperar_cambio(0, 120)) return 0;
    if (!dht_esperar_cambio(1, 120)) return 0;

    // Leer los 5 bytes del sensor correspondientes a su humedad,
    // temperatura (Partes decimales y enteras), y a su checksum.
    for (uint8_t i = 0; i < 5; i++) {

        for (uint8_t j = 0; j < 8; j++) {

            // Cada bit empieza con una señal LOW y luego pasa a HIGH.
            if (!dht_esperar_cambio(0, 100)) return 0;

            _delay_us(35);

            data[i] <<= 1;

            // Si la señal sigue en HIGH despues de 35 us, el bit es 1.
            if (DHT11_PINR & (1 << DHT11_PIN)) {

                data[i] |= 1;

                // Esperar a que termine el pulso HIGH.
                if (!dht_esperar_cambio(1, 100)) return 0;
            }

            // Si era cero, a los 35 us ya estamos en el LOW siguiente.
        }
    }

    // Cálculo del Checksum: suma de los primeros 4 bytes recibidos
    checksum = data[0] + data[1] + data[2] + data[3];

    // Si la checksum calculada es diferente a la checksum del sensor, hubo un error.
    if (checksum != data[4]) {
        return 0;
    }

    // Asignación de valor mediante puntero
    *humidity = data[0]; // Parte entera de la humedad
    *temperature = data[2]; // Parte entera de la temperatura

    return 1; // Éxito
}