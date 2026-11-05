/* TWI Interface with LCD using PCF8574 IC */

/* 	
	PCF8574 has 8-Bit Port for I/O operation 
	P0 = RS;		//--- Register Select
	P1 = RW;		//--- Read / Write Operation Select
	P2 = EN;		//--- Latch to Data Register Enable Pin
	P3 = Backlight;	//--- LCD Backlight Control
	P4 = D4;		//--- LCD pin D4
	P5 = D5;		//--- LCD pin D5
	P6 = D6;		//--- LCD pin D6
	P7 = D7;		//--- LCD pin D7	
*/
#ifndef TWI_LCD_H_
#define TWI_LCD_H_

void PCF8574_write(unsigned char x);
void twi_lcd_4bit_send(unsigned char x);
void twi_lcd_cmd(const unsigned char x);
void twi_lcd_dwr(unsigned char x);
void twi_lcd_msg(const char *c);
void twi_lcd_clear();
void twi_lcd_init();
static void twi_lcd_linea(const char *texto);
#endif