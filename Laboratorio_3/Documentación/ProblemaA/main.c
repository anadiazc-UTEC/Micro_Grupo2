#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>

#define HEATER_PIN PB0   
#define FAN_PIN    PD6   
#define DHT_PIN    PD2   

#define LCD_ADDR   0x4E

// CONTROL I2C
void I2C_Init(void) {
    TWBR = 72; 
    TWSR = 0;
}
void I2C_Start(void) {
    TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
    while (!(TWCR & (1 << TWINT)));
}
void I2C_Stop(void) {
    TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);
}
void I2C_Write(uint8_t data) {
    TWDR = data;
    TWCR = (1 << TWINT) | (1 << TWEN);
    while (!(TWCR & (1 << TWINT)));
}

// CONTROL PANTALLA LCD I2C
void LCD_Write_I2C(uint8_t data) {
    I2C_Start(); I2C_Write(LCD_ADDR); I2C_Write(data); I2C_Stop();
}
void LCD_Pulse_Enable(uint8_t data) {
    LCD_Write_I2C(data | 0x04); _delay_us(1);
    LCD_Write_I2C(data & ~0x04); _delay_us(50);
}
void LCD_Send(uint8_t value, uint8_t mode) {
    uint8_t highnib = (value & 0xF0) | mode | 0x08; 
    uint8_t lownib  = ((value << 4) & 0xF0) | mode | 0x08;
    LCD_Write_I2C(highnib); LCD_Pulse_Enable(highnib);
    LCD_Write_I2C(lownib); LCD_Pulse_Enable(lownib);
}
void LCD_Cmd(uint8_t cmd) { LCD_Send(cmd, 0x00); }
void LCD_Char(uint8_t data) { LCD_Send(data, 0x01); }

void LCD_Init(void) {
    I2C_Init();
    _delay_ms(50);
    LCD_Write_I2C(0x38); LCD_Pulse_Enable(0x38); _delay_ms(5);
    LCD_Write_I2C(0x38); LCD_Pulse_Enable(0x38); _delay_ms(1);
    LCD_Write_I2C(0x38); LCD_Pulse_Enable(0x38); _delay_ms(1);
    LCD_Write_I2C(0x28); LCD_Pulse_Enable(0x28); _delay_ms(1);
    LCD_Cmd(0x28); LCD_Cmd(0x0C); LCD_Cmd(0x06); LCD_Cmd(0x01);
    _delay_ms(2);
}

void LCD_SetCursor(uint8_t row, uint8_t col) {
    uint8_t pos[] = {0x00, 0x40};
    LCD_Cmd(0x80 | (pos[row] + col));
}
void LCD_Print(const char* str) {
    while (*str) LCD_Char(*str++);
}

// UART
void UART_Init(unsigned int baud) {
    unsigned int ubrr = F_CPU / 16 / baud - 1;
    UBRR0H = (unsigned char)(ubrr >> 8);
    UBRR0L = (unsigned char)ubrr;
    UCSR0B = (1 << TXEN0) | (1 << RXEN0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}
void UART_SendString(const char* str) {
    while (*str) {
        while (!(UCSR0A & (1 << UDRE0)));
        UDR0 = *str++;
    }
}

int main(void) {
    UART_Init(9600);
    LCD_Init();
    
    LCD_SetCursor(0, 0);
    LCD_Print("Iniciando...    ");
    UART_SendString("Sistema Inicializado\r\n");

    while (1) {
        _delay_ms(100);
    }
    return 0;
}