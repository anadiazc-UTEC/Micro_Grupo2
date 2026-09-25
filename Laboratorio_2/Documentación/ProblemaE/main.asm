.include "m328pdef.inc"

.def temp = r16
.def estado = r17
.org 0x0000
	rjmp RESET
.org 0x000A		;PCINT2 (PORTD)
	rjmp ISR_OBSTACULO
RESET:
	ldi temp,	high(RAMEND)
	out SPH,	temp
	ldi temp,	low(RAMEND)
	out SPL,	temp

	ldi temp,	(1<<PINB0) | (1<<PINB1) | (1<<PINB2)
	out DDRB,	temp
	ldi temp,	0x00
	out PORTB,	temp

	ldi temp,	0x00
	out DDRD,	temp
	ldi temp,	0x00
	out PORTD,	temp

	ldi temp,	LOW(103)
	sts UBRR0L,	temp
	ldi temp,	HIGH(103)
	sts UBRR0H,	temp

	ldi temp,	(1<<TXEN0)
	sts UCSR0B,	temp

	ldi temp,	(1<<UCSZ01) | (1<<UCSZ00)
	sts UCSR0C,	temp

	ldi temp,	(1<<PCIE2)
	sts PCICR,	temp
	ldi temp,	(1<<PCINT22)
	sts PCMSK2,	temp

start:
    rjmp start
