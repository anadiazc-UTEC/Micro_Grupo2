#ifndef PROTOCOLO_H
#define PROTOCOLO_H

#include <stdint.h>
#include <stdbool.h>

// [ $ ][ CMD ][ D1 ][ D2 ][ CHK ][ # ]      CHK = CMD ^ D1 ^ D2

#define TRAMA_START         '$'     // 0x24
#define TRAMA_STOP          '#'     // 0x23
#define TRAMA_TIMEOUT_MS    20UL    // una trama tarda ~6,25 ms a 9600 baud

// Codigos de error que viajan en el NACK: N(cmd_recibido, codigo)
#define NACK_CHECKSUM       1
#define NACK_DESCONOCIDO    2
#define NACK_DATO           3
#define NACK_EMERGENCIA     4       // rechazado: hay una emergencia activa
#define NACK_INCOMPLETA     5       // falta el STOP o se corto la trama (timeout)

typedef struct {
    uint8_t comando;
    uint8_t dato1;
    uint8_t dato2;
    uint8_t checksum;
    bool    valida;
    uint8_t error;      // NACK_X cuando valida == false
} TramaBT_t;

bool protocolo_procesar_byte(char byte_recibido, uint32_t ahora_ms, TramaBT_t *trama_out);
bool protocolo_verificar_timeout(uint32_t ahora_ms);
void protocolo_reiniciar(void);
uint16_t protocolo_errores(void);
void protocolo_enviar_trama(uint8_t comando, uint8_t dato1, uint8_t dato2);

#endif
