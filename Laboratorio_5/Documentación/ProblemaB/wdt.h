#ifndef WDT_H_
#define WDT_H_

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    RESET_POR_DEFECTO,
    RESET_WATCHDOG
} CausaReset_t;

extern volatile uint8_t flag_wdt_despertar;
extern CausaReset_t causa_ultimo_reset;

void wdt_manager_init(void);
void wdt_configurar_reset_2s(void);
void wdt_configurar_interrupcion_8s(void);

void wdt_configurar_prr_activo(void);
void wdt_configurar_prr_standby(void);
void wdt_configurar_prr_apagado(void);

void entrar_modo_idle(void);
void entrar_modo_power_down(void);
#endif