#include "adc.h"
#include <avr/io.h>

void adc_init() {

	// Referencia AVcc, canal ADC0
	ADMUX = (1 << REFS0);

	// Habilitar ADC, preescaler de 128
	// 16MHz/128 = 125kHz (dentro de 50-200kHz recomendado)
	ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);

	// Primera conversión descartada para estabilizar el ADC
	ADCSRA |= (1 << ADSC);
	while (ADCSRA & (1 << ADSC));
}


uint16_t adc_read(uint8_t channel) {

	// Seleccionar canal
	ADMUX = (ADMUX & 0xF0) | (channel & 0x0F);

	// Iniciar conversión
	ADCSRA |= (1 << ADSC);

	// Esperar fin de conversión
	while (ADCSRA & (1 << ADSC));

	// Devolver resultado (leer ADCL primero)
	return ADC;
}