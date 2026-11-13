#include "protocolo.h"
#include <stddef.h>

// Definición del tipo de puntero a función para los estados del parser

typedef bool (*EstadoParser_t)(uint8_t byte_recibido, TramaBT_t *trama_out);

static uint8_t  cmd_temp = 0;
static uint8_t  d1_temp  = 0;
static uint8_t  d2_temp  = 0;
static uint8_t  chk_temp = 0;
static uint32_t ultimo_byte_ms = 0;
static uint16_t errores_protocolo = 0;     // contador de tramas descartadas

static bool esp_inicio(uint8_t b, TramaBT_t *t);
static bool esp_cmd(uint8_t b, TramaBT_t *t);
static bool esp_d1(uint8_t b, TramaBT_t *t);
static bool esp_d2(uint8_t b, TramaBT_t *t);
static bool esp_chk(uint8_t b, TramaBT_t *t);
static bool esp_fin(uint8_t b, TramaBT_t *t);

static EstadoParser_t estado_parser = esp_inicio;

// Estado 1: esperar el delimitador de inicio ($)
static bool esp_inicio(uint8_t b, TramaBT_t *t) {
    (void)t;
    if (b == TRAMA_START) {
        estado_parser = esp_cmd;
    }
    return false;
}

// Estado 2: byte de comando
static bool esp_cmd(uint8_t b, TramaBT_t *t) {
    (void)t;
    cmd_temp = b;
    estado_parser = esp_d1;
    return false;
}

// Estado 3: primer byte de dato
static bool esp_d1(uint8_t b, TramaBT_t *t) {
    (void)t;
    d1_temp = b;
    estado_parser = esp_d2;
    return false;
}

// Estado 4: segundo byte de dato
static bool esp_d2(uint8_t b, TramaBT_t *t) {
    (void)t;
    d2_temp = b;
    estado_parser = esp_chk;
    return false;
}

// Estado 5: checksum
static bool esp_chk(uint8_t b, TramaBT_t *t) {
    (void)t;
    chk_temp = b;
    estado_parser = esp_fin;
    return false;
}

// Estado 6: delimitador de fin (#) y validacion
static bool esp_fin(uint8_t b, TramaBT_t *t) {
    t->comando  = cmd_temp;
    t->dato1    = d1_temp;
    t->dato2    = d2_temp;
    t->checksum = chk_temp;

    if (b != TRAMA_STOP) {
        // Trama mal delimitada (se perdio un byte). Si el byte que llego es un '$',
        // probablemente es el inicio de la trama siguiente: no lo descartamos.
        t->valida = false;
        t->error  = NACK_INCOMPLETA;
        errores_protocolo++;
        estado_parser = (b == TRAMA_START) ? esp_cmd : esp_inicio;
        return true;
    }

    if ((uint8_t)(cmd_temp ^ d1_temp ^ d2_temp) == chk_temp) {
        t->valida = true;
        t->error  = 0;
    } else {
        t->valida = false;
        t->error  = NACK_CHECKSUM;
        errores_protocolo++;
    }

    estado_parser = esp_inicio;
    return true;
}

// Procesa un byte. Devuelve true cuando se completo una trama (valida o no).
bool protocolo_procesar_byte(char byte_recibido, uint32_t ahora_ms, TramaBT_t *trama_out) {
    ultimo_byte_ms = ahora_ms;
    if (estado_parser == NULL) {
        estado_parser = esp_inicio;
    }
    return estado_parser((uint8_t)byte_recibido, trama_out);
}

// Si quedo una trama a medias y no llegan bytes hace mas de TRAMA_TIMEOUT_MS,
// se descarta. Devuelve true si descarto una trama incompleta.
bool protocolo_verificar_timeout(uint32_t ahora_ms) {
    if (estado_parser != esp_inicio && (ahora_ms - ultimo_byte_ms) > TRAMA_TIMEOUT_MS) {
        estado_parser = esp_inicio;
        errores_protocolo++;
        return true;
    }
    return false;
}

// Volver a esperar inicio (por ejemplo, al reactivar la USART tras APAGADO)
void protocolo_reiniciar(void) {
    estado_parser = esp_inicio;
}

uint16_t protocolo_errores(void) {
    return errores_protocolo;
}

// Construir y enviar una trama completa: $ CMD D1 D2 CHK #
void protocolo_enviar_trama(uint8_t comando, uint8_t dato1, uint8_t dato2) {
    uint8_t chk = (uint8_t)(comando ^ dato1 ^ dato2);

    bluetooth_transmit(TRAMA_START);
    bluetooth_transmit((char)comando);
    bluetooth_transmit((char)dato1);
    bluetooth_transmit((char)dato2);
    bluetooth_transmit((char)chk);
    bluetooth_transmit(TRAMA_STOP);
}
