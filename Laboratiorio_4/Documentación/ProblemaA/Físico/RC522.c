#include "rc522.h"
#include "spi.h"
#include <util/delay.h>

void mfrc522_write(uint8_t reg, uint8_t value) {
    SS_LOW();
    spi_transfer((reg << 1) & 0x7E);
    spi_transfer(value);
    SS_HIGH();
}

uint8_t mfrc522_read(uint8_t reg) {
    uint8_t val;
    SS_LOW();
    spi_transfer(((reg << 1) & 0x7E) | 0x80);
    val = spi_transfer(0x00);
    SS_HIGH();
    return val;
}

void mfrc522_setBitMask(uint8_t reg, uint8_t mask) {
    uint8_t tmp = mfrc522_read(reg);
    mfrc522_write(reg, tmp | mask);
}

void mfrc522_clearBitMask(uint8_t reg, uint8_t mask) {
    uint8_t tmp = mfrc522_read(reg);
    mfrc522_write(reg, tmp & (~mask));
}

void mfrc522_reset(void) {
    mfrc522_write(CommandReg, PCD_RESETPHASE);
    _delay_ms(50);
}

void mfrc522_init(void) {
    SS_DDR |= (1 << SS_PIN);
    RST_DDR |= (1 << RST_PIN);

    // Reset hardware
    RST_PORT &= ~(1 << RST_PIN);
    _delay_ms(10);
    RST_PORT |= (1 << RST_PIN);
    _delay_ms(50);

    SS_HIGH();

    mfrc522_reset();

    // Configuración de temporizadores y modulación a 106 kbps
    mfrc522_write(TModeReg, 0x8D);
    mfrc522_write(TPrescalerReg, 0x3E);
    mfrc522_write(TReloadRegL, 30);
    mfrc522_write(TReloadRegH, 0);

    mfrc522_write(TxASKReg, 0x40);
    mfrc522_write(ModeReg, 0x3D);

    // Ganancia de antena al máximo (48dB)
    mfrc522_write(RFCfgReg, 0x7F);

    // Encender antena
    uint8_t temp = mfrc522_read(TxControlReg);
    if (!(temp & 0x03)) {
        mfrc522_setBitMask(TxControlReg, 0x03);
    }
}

static uint8_t mfrc522_to_card(uint8_t command, uint8_t *sendData, uint8_t sendLen, uint8_t *backData, uint8_t *backLen) {
    uint8_t status = STATUS_ERROR;
    uint8_t irqEn = 0x00;
    uint8_t waitIRq = 0x00;
    uint8_t lastBits;
    uint8_t n;
    uint16_t i;

    if (command == PCD_TRANSCEIVE) {
        irqEn = 0x77;
        waitIRq = 0x30;
    }

    mfrc522_write(CommIEnReg, irqEn | 0x80);
    mfrc522_clearBitMask(CommIrqReg, 0x80);
    mfrc522_setBitMask(FIFOLevelReg, 0x80);

    mfrc522_write(CommandReg, PCD_IDLE);

    for (i = 0; i < sendLen; i++) {
        mfrc522_write(FIFODataReg, sendData[i]);
    }

    mfrc522_write(CommandReg, command);
    if (command == PCD_TRANSCEIVE) {
        mfrc522_setBitMask(BitFramingReg, 0x80);
    }

    i = 2000;
    do {
        n = mfrc522_read(CommIrqReg);
        i--;
    } while ((i != 0) && !(n & 0x01) && !(n & waitIRq));

    mfrc522_clearBitMask(BitFramingReg, 0x80);

    if (i != 0) {
        if (!(mfrc522_read(ErrorReg) & 0x1B)) {
            status = STATUS_OK;
            if (n & irqEn & 0x01) {
                status = STATUS_ERROR;
            }

            if (command == PCD_TRANSCEIVE) {
                n = mfrc522_read(FIFOLevelReg);
                lastBits = mfrc522_read(ControlReg) & 0x07;
                if (lastBits) {
                    *backLen = (n - 1) * 8 + lastBits;
                } else {
                    *backLen = n * 8;
                }

                if (n == 0) n = 1;
                if (n > MAX_LEN) n = MAX_LEN;

                for (i = 0; i < n; i++) {
                    backData[i] = mfrc522_read(FIFODataReg);
                }
            }
        }
    }
    return status;
}

uint8_t mfrc522_request(uint8_t reqMode, uint8_t *TagType) {
    uint8_t status;
    uint8_t backBits;

    mfrc522_write(BitFramingReg, 0x07);
    TagType[0] = reqMode;
    status = mfrc522_to_card(PCD_TRANSCEIVE, TagType, 1, TagType, &backBits);

    if ((status != STATUS_OK) || (backBits != 0x10)) {
        status = STATUS_ERROR;
    }
    return status;
}

uint8_t mfrc522_get_card_id(uint8_t *CardUID) {
    uint8_t status;
    uint8_t backBits;

    mfrc522_write(BitFramingReg, 0x00);
    CardUID[0] = PICC_ANTICOLL;
    CardUID[1] = 0x20;
    status = mfrc522_to_card(PCD_TRANSCEIVE, CardUID, 2, CardUID, &backBits);

    if (status == STATUS_OK) {
        uint8_t checksum = 0;
        for (uint8_t i = 0; i < 4; i++) {
            checksum ^= CardUID[i];
        }
        if (checksum != CardUID[4]) {
            status = STATUS_ERROR;
        }
    }
    return status;
}