#ifndef RC522_H_
#define RC522_H_

#include <avr/io.h>
#include <stdint.h>

// Definición de Pines para ATmega328P
#define SS_PORT  PORTB
#define SS_DDR   DDRB
#define SS_PIN   PB2

#define RST_PORT PORTB
#define RST_DDR  DDRB
#define RST_PIN  PB1

#define SS_LOW()  (SS_PORT &= ~(1 << SS_PIN))
#define SS_HIGH() (SS_PORT |= (1 << SS_PIN))

// Registros MFRC522
#define CommandReg      0x01
#define CommIEnReg      0x02
#define CommIrqReg      0x04
#define DivIrqReg       0x05
#define ErrorReg        0x06
#define Status2Reg      0x08
#define FIFODataReg     0x09
#define FIFOLevelReg    0x0A
#define WaterLevelReg   0x0B
#define ControlReg      0x0C
#define BitFramingReg   0x0D
#define CollReg         0x0E
#define ModeReg         0x11
#define TxControlReg    0x14
#define TxASKReg        0x15
#define RFCfgReg        0x26
#define TModeReg        0x2A
#define TPrescalerReg   0x2B
#define TReloadRegH     0x2C
#define TReloadRegL     0x2D

// Comandos MFRC522
#define PCD_IDLE        0x00
#define PCD_TRANSCEIVE  0x0C
#define PCD_RESETPHASE  0x0F

// Comandos PICC (Tarjeta)
#define PICC_REQIDL     0x26
#define PICC_ANTICOLL   0x93

// Estados de retorno
#define STATUS_OK       0
#define STATUS_ERROR    1
#define MAX_LEN         16

// Funciones públicas
void mfrc522_init(void);
void mfrc522_write(uint8_t reg, uint8_t value);
uint8_t mfrc522_read(uint8_t reg);
uint8_t mfrc522_request(uint8_t reqMode, uint8_t *TagType);
uint8_t mfrc522_get_card_id(uint8_t *CardUID);

#endif /* RC522_H_ */