#ifndef BLUETOOTH_H
#define BLUETOOTH_H

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <stdint.h>
#include <stdbool.h>

void bluetooth_init(void);
bool bluetooth_pendiente(void);
char bluetooth_receive(void);
void bluetooth_transmit(char data);
void bluetooth_print(const char *str);
void bluetooth_print_p(const char *str);
void bluetooth_esperar_tx(void);

#endif
