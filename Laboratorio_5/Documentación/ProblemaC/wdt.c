#include "wdt.h"
#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/interrupt.h>


// Variantes de consumo
// 0: firmware final (Idle en STANDBY + Power-Down en APAGADO + PRR + WDT)
// 1: sin optimizaciones (sin modos sleep y sin PRR)
// 2: solo Idle en STANDBY (sin Power-Down y sin PRR)
// 3: solo PRR (sin modos sleep)
// 4: solo modos sleep (Idle + Power-Down, sin PRR)
#ifndef VARIANTE_CONSUMO
#define VARIANTE_CONSUMO 0
#endif
#define USAR_PRR      (VARIANTE_CONSUMO == 0 || VARIANTE_CONSUMO == 3)
#define USAR_IDLE     (VARIANTE_CONSUMO == 0 || VARIANTE_CONSUMO == 2 || VARIANTE_CONSUMO == 4)
#define USAR_PWR_DOWN (VARIANTE_CONSUMO == 0 || VARIANTE_CONSUMO == 4)

volatile uint8_t flag_wdt_despertar = 0;
CausaReset_t causa_ultimo_reset = RESET_POR_DEFECTO;

// Deshabilitar el Watchdog inmediatamente al encender/reiniciar (Sección .init3 de GCC)
// Evita que el micro entre en bucles infinitos de reset a 16ms tras un desbordamiento del WDT.
void wdt_init_early(void) __attribute__((naked)) __attribute__((section(".init3")));

static uint8_t mcusr_guardado __attribute__((section(".noinit")));

void wdt_init_early(void) {
	mcusr_guardado = MCUSR;
    MCUSR = 0;
    wdt_disable();
}

void wdt_manager_init(void) {
    // Evaluar la máscara del bit WDRF (Bit 3 de MCUSR)
    if (mcusr_guardado & (1 << WDRF)) {
        causa_ultimo_reset = RESET_WATCHDOG; // Asigna correctamente el enum en 1
    } else {
        causa_ultimo_reset = RESET_POR_DEFECTO;
    }
}

void wdt_configurar_reset_2s(void) {
    cli();
    wdt_reset();
    WDTCSR |= (1 << WDCE) | (1 << WDE);
    // Configurar WDE=1, WDIE=1 y bits WDP2:WDP1:WDP0 (0111 = 2.0 segundos nominales según datasheet)
    WDTCSR = (1 << WDIE) | (1 << WDE) | (1 << WDP2) | (1 << WDP1) | (1 << WDP0);
    sei();
}

void wdt_configurar_interrupcion_8s(void) {
    cli();
    wdt_reset();
    WDTCSR |= (1 << WDCE) | (1 << WDE);
    // Configurar WDIE=1, WDE=0 y bits WDP3:WDP0 (1001 = 8.0 segundos nominales)
    WDTCSR = (1 << WDIE) | (1 << WDP3) | (1 << WDP0);
    sei();
}

void wdt_configurar_prr_activo(void) {
    PRR &= ~((1 << PRADC) | (1 << PRUSART0) | (1 << PRTWI) | (1 << PRTIM0) | (1 << PRTIM1));
#if USAR_PRR
	PRR |= (1 << PRSPI) | (1 << PRTIM2);
#endif
	ADCSRA |= (1 << ADEN);
}

void wdt_configurar_prr_standby(void) {
#if !USAR_PRR
	return;
#endif
	ADCSRA &= ~(1 << ADEN);
     PRR |= (1 << PRADC) | (1 << PRTWI) | (1 << PRTIM0) | (1 << PRSPI) | (1 << PRTIM2);
}

void wdt_configurar_prr_apagado(void) {
#if !USAR_PRR
	return;
#endif
	ADCSRA &= ~(1 << ADEN);
    PRR |= (1 << PRADC) | (1 << PRUSART0) | (1 << PRTWI) | (1 << PRTIM0) | (1 << PRTIM1) | (1 << PRSPI) | (1 << PRTIM2);
}

void entrar_modo_idle(void) {
#if !USAR_IDLE
    return;
#endif
    set_sleep_mode(SLEEP_MODE_IDLE);
    sleep_enable();
    sleep_cpu();
    sleep_disable();
}

void entrar_modo_power_down(void) {
#if !USAR_PWR_DOWN
    return;
#endif
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_enable();
    sleep_cpu();
    sleep_disable();
}