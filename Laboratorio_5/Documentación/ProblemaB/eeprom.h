#ifndef EEPROM_H
#define EEPROM_H

#include <avr/io.h>
#include <stdint.h>

#define EEPROM_TAMANIO  1024u

uint8_t eeprom_leer(uint16_t direccion);
void eeprom_escribir(uint16_t direccion, uint8_t dato);
void eeprom_actualizar(uint16_t direccion, uint8_t dato);

uint16_t eeprom_leer_16(uint16_t direccion);
void eeprom_actualizar_16(uint16_t direccion, uint16_t dato);

#endif
