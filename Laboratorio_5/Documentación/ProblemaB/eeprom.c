#include "eeprom.h"

static void eeprom_esperar(void)
{
    while (EECR & (1 << EEPE));
}

uint8_t eeprom_leer(uint16_t direccion)
{
    if (direccion >= EEPROM_TAMANIO) {
        return 0xFF; // fuera de rango: valor de EEPROM borrada
    }

    eeprom_esperar(); // Esperar a que no haya una escritura en curso.
    EEAR = direccion; // Cargar la dirección en EEAR.
    EECR |= (1 << EERE); // Poner EERE en 1 para leer.
    return EEDR; // El dato queda en EEDR.
}

void eeprom_escribir(uint16_t direccion, uint8_t dato)
{
	uint8_t sreg;

	if (direccion >= EEPROM_TAMANIO) {
		return;
	}

	eeprom_esperar(); // Esperar a que no haya una escritura en curso
	EEAR = direccion;
	EEDR = dato; 
	EECR &= ~((1 << EEPM1) | (1 << EEPM0)); // Modo borrar y escribir en una sola operación

	sreg = SREG; // Guardar el estado de las interrupciones
	cli(); // EEPE debe ponerse en 1 dentro de los 4 ciclos siguientes a EEMPE
	EECR |= (1 << EEMPE); // Habilita la escritura
	EECR |= (1 << EEPE); // Inicia la escritura
	
	SREG = sreg; // Restaurar las interrupciones como estaban
}

void eeprom_actualizar(uint16_t direccion, uint8_t dato)
{
    if (eeprom_leer(direccion) != dato) {
        eeprom_escribir(direccion, dato);
    }
}

uint16_t eeprom_leer_16(uint16_t direccion)
{
    uint16_t bajo = eeprom_leer(direccion);
    uint16_t alto = eeprom_leer(direccion + 1);
    return (uint16_t)((alto << 8) | bajo);
}

void eeprom_actualizar_16(uint16_t direccion, uint16_t dato)
{
    eeprom_actualizar(direccion, (uint8_t)(dato & 0xFF));
    eeprom_actualizar(direccion + 1, (uint8_t)(dato >> 8));
}
