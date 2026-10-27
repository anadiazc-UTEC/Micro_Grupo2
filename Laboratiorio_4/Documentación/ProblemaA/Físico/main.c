#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <avr/eeprom.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


#include "spi.h"
#include "rc522.h"

// Configuracion de Pines
#define LED_GREEN_PIN PD2 // LED verde para indicar operaciones exitosas
#define LED_RED_PIN   PD3 //LED rojo para errores o bloqueos
#define BUZZER_PIN    PD4 // salida directa al modulo buzzer de 3 pines

#define EEPROM_LLAVE 0x58 // verificar si los datos de las 3 tarjetas ya existen en EEPROM

// Estados de la Maquina
typedef enum { //definimos los distintos estados por los que pasa el cajero
    ESTADO_ESPERANDO_TARJETA, // en espera de que acerquen un tag RFID
    ESTADO_INGRESO_PIN, //solicitud y validacion de clave de acceso
    ESTADO_MENU_PRINCIPAL, // opciones disponibles en pantalla
    ESTADO_RETIRO, // aca procesamos las extracciones
    ESTADO_DEPOSITO, // procesamiento de ingreso de fondos
    ESTADO_SALDO // consulta de monto disponible
} EstadoCajero; //tipo personalizado para controlar la maquina de estados

EstadoCajero estado_actual = ESTADO_ESPERANDO_TARJETA; // iniciamos directo pidiendo tarjeta
EstadoCajero estado_anterior = (EstadoCajero)-1; // valor invalido para forzar la primera actualizacion de pantalla

// Estructura de Cuenta
typedef struct { // datos agrupados pertenecientes a una cuenta
    uint8_t uid[4]; // arreglo de 4 bytes con el identificador unico RFID
    char pin[5]; // clave de 4 digitos
    uint32_t saldo; // monto guardado en la cuenta
} Cuenta; // alias para definir variables del tipo estructura

// Variables en EEPROM
Cuenta EEMEM cuentas_guardadas[3]; // guardamos las tres cuentas
uint8_t EEMEM eeprom_flag_inicializada; // bandera en EEPROM para evitar sobrescribir datos al reiniciar

uint8_t cuenta_actual_index = 0; // indice para saber que cuenta esta activa


// CONTROLADOR I2C

#define PCF8574_ADDR 0x4E // direccion I2C del integrado PCF8574 del display (0x27 << 1)

void i2c_init(void) { // rutina de configuracion del bus I2C
    TWSR = 0x00; // prescaler en 0
    TWBR = 72; // ajustamos el reloj a 100 kHz aproximadamente
}

uint8_t i2c_start(void) { // condicion de inicio en la linea I2C con tiempo limite
    TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN); // manda la senal de Start
    uint16_t timer = 1000;
    while (!(TWCR & (1<<TWINT))) { // esperamos confirmacion del hardware
        if (--timer == 0) return 0; // evita colgarse en bucle infinito si la pantalla no responde
    }
    return 1;
}

void i2c_stop(void) { // condicion de parada del bus I2C
    TWCR = (1<<TWINT) | (1<<TWSTO) | (1<<TWEN); // libera el bus
}

uint8_t i2c_write(uint8_t data) { // transmite un byte individual por I2C con timeout
    TWDR = data; // cargamos el dato al registro
    TWCR = (1<<TWINT) | (1<<TWEN); // inicia transmision
    uint16_t timer = 1000;
    while (!(TWCR & (1<<TWINT))) { // aguardamos hasta terminar el envio
        if (--timer == 0) return 0; // retorna 0 si falla la linea
    }
    return 1;
}

void lcd_send_nibble(uint8_t nibble, uint8_t rs) { // envia 4 bits al display ya que opera en modo nibble
    uint8_t data = (nibble & 0xF0) | (rs ? 0x01 : 0x00) | 0x08; // armamos la trama con datos y bits de control
    if (!i2c_start()) return; // abre comunicacion
    i2c_write(PCF8574_ADDR); // enviamos la direccion del expansor
    i2c_write(data | 0x04); // generamos el pulso de Enable en alto
    _delay_us(1); // tiempo de estabilizacion
    i2c_write(data & ~0x04); // bajamos Enable para fijar el dato
    i2c_stop(); // cerramos la transaccion
    _delay_us(50); // espera breve para procesamiento
}

void lcd_send_byte(uint8_t byte, uint8_t rs) { // mandamos un byte completo dividido en dos partes
    lcd_send_nibble(byte & 0xF0, rs); // cuatro bits superiores
    lcd_send_nibble((byte << 4) & 0xF0, rs); // cuatro bits inferiores
}

void lcd_command(uint8_t cmd) { lcd_send_byte(cmd, 0); } // envio de comandos de control al LCD
void lcd_char(char data) { lcd_send_byte(data, 1); } // escribe un caracter visible en el display

void lcd_init(void) { // inicializacion limpia del LCD 16x2
    i2c_init();
    _delay_ms(50);
    lcd_send_nibble(0x30, 0); _delay_ms(5); // secuencia estandar de inicializacion
    lcd_send_nibble(0x30, 0); _delay_us(150); // segundo paso del reset por software
    lcd_send_nibble(0x30, 0); // confirmacion de modo
    lcd_send_nibble(0x20, 0); // pasamos la pantalla a modo 4 bits

    lcd_command(0x28); // establecemos 2 lineas y fuente 5x8
    lcd_command(0x0C); // encendemos la pantalla sin cursor visible
    lcd_command(0x01); // limpiamos el contenido previo
    _delay_ms(2); // tiempo para completar el borrado
}

void lcd_clear(void) { // borrado completo de la pantalla
    lcd_command(0x01); // comando de limpia
    _delay_ms(2); // pausa requerida por la pantalla
}

void lcd_gotoxy(uint8_t x, uint8_t y) { // nos ubica en la posicion especificada del display
    uint8_t addr = (y == 0) ? (0x80 + x) : (0xC0 + x); // calculo de direccion en memoria DDRAM
    lcd_command(addr); // movemos el cursor
}

void lcd_print(const char* str) { // aca use un bucle para enviar la cadena entera
    while (*str) lcd_char(*str++); // recorre y escribe caracter a caracter
}


//TECLADO MATRICIAL

const char keypad_map[4][4] = {
    {'/', '*', '-', '+'},
    {'3', '6', '9', '='},
    {'2', '5', '8', '0'},
    {'1', '4', '7', 'C'}
};

void keypad_init(void) { // preparacion de puertos para el teclado 4x4
    DDRD |= (1<<PD5) | (1<<PD6) | (1<<PD7); // las primeras 3 filas como salidas
    DDRB |= (1<<PB0); // la cuarta fila como salida
    PORTD |= (1<<PD5) | (1<<PD6) | (1<<PD7); // ponemos las filas en estado alto
    PORTB |= (1<<PB0); // cuarta fila en alto

    DDRC &= ~((1<<PC0) | (1<<PC1) | (1<<PC2) | (1<<PC3)); // columnas configuradas como entradas
    PORTC |= (1<<PC0) | (1<<PC1) | (1<<PC2) | (1<<PC3); // habilitamos pull-up internas
}

char keypad_get_key(void) { // busca entre las filas pulsaciones individuales
    for (uint8_t f = 0; f < 4; f++) { // iteramos por cada fila
        PORTD |= (1<<PD5) | (1<<PD6) | (1<<PD7); // reseteamos salidas a nivel alto
        PORTB |= (1<<PB0);

        if (f == 0) PORTD &= ~(1<<PD5); // bajamos a cero la fila a evaluar
        else if (f == 1) PORTD &= ~(1<<PD6);
        else if (f == 2) PORTD &= ~(1<<PD7);
        else if (f == 3) PORTB &= ~(1<<PB0);

        _delay_us(10);

        for (uint8_t c = 0; c < 4; c++) { // leemos si alguna columna cayo a nivel bajo
            if (!(PINC & (1<<c))) { // detecto tecla presionada
                _delay_ms(20); // retardo antirrebote de bajada
                if (!(PINC & (1<<c))) {
                    char key = keypad_map[f][c]; // obtenemos el caracter correspondiente

                    uint16_t release_timer = 0;
                    while (!(PINC & (1<<c)) && release_timer < 500) { // espera que el usuario suelte el boton
                        _delay_ms(5);
                        release_timer++;
                    }
                    _delay_ms(20); // antirrebote de subida

                    return key; // devolvemos la tecla leida
                }
            }
        }
    }
    return '\0'; // si no hay pulsacion retorna caracter nulo
}

void keypad_read_string(char* buffer, uint8_t max_len, uint8_t es_pin) { // captura secuencias de texto/numeros
    uint8_t idx = 0; // posicion dentro del arreglo
    buffer[0] = '\0'; // inicializa como cadena vacia

    while (1) { // ciclo de lectura continuo hasta presionar la tecla de entrada
        char key = keypad_get_key(); // consulta el teclado
        if (key != '\0') { // si detecto una tecla valida
            if (key == '=') { // tecla igual funciona como Enter o Aceptar
                buffer[idx] = '\0'; // terminamos la cadena
                break; // salimos del bucle
                } else if (key == 'C') { // tecla 'C' para borrar
                if (idx > 0) {
                    idx--; // volvemos un espacio
                    buffer[idx] = '\0'; // limpiamos la ultima posicion
                    lcd_gotoxy(idx, 1); // nos movemos en la pantalla
                    lcd_char(' '); // borramos visualmente el caracter
                    lcd_gotoxy(idx, 1); // reubicamos el cursor
                }
                } else if (key >= '0' && key <= '9') { // si es un numero valido
                if (idx < max_len) {
                    buffer[idx] = key; // guardamos en el arreglo
                    lcd_gotoxy(idx, 1); // mostramos el avance en la segunda linea
                    lcd_char(es_pin ? '*' : key); // imprimimos asterisco o el numero real
                    idx++; // avanzamos el indice
                }
            }
        }
    }
}


// INDICADORES LUMINOSOS Y SONOROS

void activar_exito(void) { // senalizacion de accion correcta
    PORTD |= (1<<LED_GREEN_PIN); // encendemos LED verde
    _delay_ms(1000); // permanece encendido un segundo
    PORTD &= ~(1<<LED_GREEN_PIN); // apaga el LED
}

void activar_error(void) { // indicacion luminosa ante un error
    PORTD |= (1<<LED_RED_PIN); // enciende el LED rojo
    _delay_ms(1500); // mantiene la luz 1.5 segundos
    PORTD &= ~(1<<LED_RED_PIN); // apaga el LED rojo
}

void activar_bloqueo_buzzer(void) { // activacion combinada por bloqueo de clave
    PORTD |= (1<<LED_RED_PIN) | (1<<BUZZER_PIN); // encendemos LED rojo y buzzer en simultaneo
    _delay_ms(3000); // duracion del aviso de 3 segundos
    PORTD &= ~((1<<LED_RED_PIN) | (1<<BUZZER_PIN)); // apagamos ambos indicadores
}


//main / maquina estados

int main(void) { // funcion principal del programa
    spi_init(); // aca configuramos el puerto SPI
    mfrc522_init(); // iniciamos el chip RFID
    lcd_init(); // preparacion del display LCD
    keypad_init(); // activamos las entradas/salidas del teclado

    DDRD |= (1<<LED_GREEN_PIN) | (1<<LED_RED_PIN) | (1<<BUZZER_PIN); // definimos como salidas los pines de los indicadores

    uint8_t flag = eeprom_read_byte(&eeprom_flag_inicializada); // consulta de la bandera en EEPROM
    if (flag != EEPROM_LLAVE) { // si es el primer arranque con los nuevos UIDs
        
        Cuenta cuentas_iniciales[3] = {
            {{0xC3, 0x10, 0x36, 0x16}, "1234", 5000}, // Cuenta 1: Llavero Azul (PIN: 1234)
            {{0xC3, 0x27, 0xFC, 0x28}, "5678", 1500}, // Cuenta 2: Tarjeta Blanca (PIN: 5678)
            {{0xA1, 0x14, 0x4C, 0x1C}, "4321", 3000}  // Cuenta 3: Tercera Tarjeta (PIN: 4321)
        };
        
        eeprom_update_block(&cuentas_iniciales, &cuentas_guardadas, sizeof(cuentas_iniciales)); // escribimos la estructura en EEPROM
        eeprom_write_byte(&eeprom_flag_inicializada, EEPROM_LLAVE); // guardamos la llave de confirmacion
    }

    uint8_t uid_leido[5]; // arreglo para almacenar el identificador de la tarjeta
    char entrada_texto[10]; // bufer donde recibimos los datos ingresados
    uint8_t intentos_pin = 0; // contador de intentos de clave fallidos
    Cuenta cuenta_actual; // estructura temporal para la sesion en curso

    while (1) { // bucle principal infinito
        if (estado_actual != estado_anterior) { // detecta cambios de estado para renovar la pantalla
            lcd_clear(); // borrado de pantalla
            if (estado_actual == ESTADO_ESPERANDO_TARJETA) { // mensaje de espera de tarjeta
                lcd_gotoxy(0, 0);
                lcd_print("  CAJERO ATM  ");
                lcd_gotoxy(0, 1);
                lcd_print("Acerque Tarjeta");
                } else if (estado_actual == ESTADO_INGRESO_PIN) { // pantalla de pedido de PIN
                lcd_gotoxy(0, 0);
                lcd_print("Ingrese PIN (=):");
                } else if (estado_actual == ESTADO_MENU_PRINCIPAL) { // vista del menu con opciones
                lcd_gotoxy(0, 0);
                lcd_print("1:Saldo 2:Retiro");
                lcd_gotoxy(0, 1);
                lcd_print("3:Dep  4:Salir");
            }
            estado_anterior = estado_actual; // actualiza la referencia de estado
        }

        switch (estado_actual) { // evaluacion de la maquina de estados

            case ESTADO_ESPERANDO_TARJETA: // lectura y autenticacion RFID
            {
                uint8_t status;
                uint8_t str[MAX_LEN];

                status = mfrc522_request(PICC_REQIDL, str); // aca consultamos si hay un tag cerca
                if (status == STATUS_OK) { // si respondio una tarjeta
                    status = mfrc522_get_card_id(uid_leido); // obtenemos el ID de 4 bytes
                    if (status == STATUS_OK) { // lectura correcta
                        uint8_t encontrada = 0;

                        for (uint8_t i = 0; i < 3; i++) { // recorremos la lista de cuentas en EEPROM
                            Cuenta temp;
                            eeprom_read_block(&temp, &cuentas_guardadas[i], sizeof(Cuenta));
                            if (memcmp(uid_leido, temp.uid, 4) == 0) { // comparamos identificadores
                                cuenta_actual_index = i; // guardamos el indice encontrado
                                encontrada = 1; // marcamos la coincidencia
                                break;
                            }
                        }

                        if (encontrada) { // si la tarjeta existe en la base de datos
                            activar_exito(); // destello del LED verde
                            intentos_pin = 0; // reseteamos el contador de fallos
                            estado_actual = ESTADO_INGRESO_PIN; // pasamos a pedir la clave
                            } else { // si la tarjeta no coincide
                            lcd_clear();
                            lcd_gotoxy(0, 0);
                            lcd_print("  Tarjeta No");
                            lcd_gotoxy(0, 1);
                            lcd_print("  Registrada!");
                            activar_error(); // aviso con LED rojo
                            estado_anterior = (EstadoCajero)-1; // forzamos refresco del display
                        }
                    }
                }
            }
            break;

            case ESTADO_INGRESO_PIN: // verificacion de la clave del usuario
            keypad_read_string(entrada_texto, 4, 0); // lectura de los 4 digitos mostrando texto ingresado

            eeprom_read_block(&cuenta_actual, &cuentas_guardadas[cuenta_actual_index], sizeof(Cuenta)); // traemos los datos de EEPROM

            if (strcmp(entrada_texto, cuenta_actual.pin) == 0) { // comprobamos si el PIN coincide
                activar_exito(); // confirmacion con LED verde
                estado_actual = ESTADO_MENU_PRINCIPAL; // ingreso concedido
                } else { // clave erronea
                intentos_pin++; // sumamos un intento fallido

                if (intentos_pin >= 3) { // limite de 3 fallos alcanzado
                    lcd_clear();
                    lcd_gotoxy(0, 0);
                    lcd_print("Intentos Maximos");
                    lcd_gotoxy(0, 1);
                    lcd_print("   Bloqueado!");

                    activar_bloqueo_buzzer(); // aca disparamos la alarma sonora por 3 segundos

                    estado_actual = ESTADO_ESPERANDO_TARJETA; // reiniciamos la sesion
                    } else { // si aun restan intentos
                    lcd_clear();
                    lcd_gotoxy(0, 0);
                    lcd_print("PIN Incorrecto!");
                    activar_error(); // solo enciende el LED rojo
                    estado_anterior = (EstadoCajero)-1;
                }
            }
            break;

            case ESTADO_MENU_PRINCIPAL: // atencion de las opciones del usuario
            {
                char opcion = '\0';
                while (opcion < '1' || opcion > '4') { // espera una tecla valida entre 1 y 4
                    opcion = keypad_get_key();
                }

                if (opcion == '1') estado_actual = ESTADO_SALDO;
                else if (opcion == '2') estado_actual = ESTADO_RETIRO;
                else if (opcion == '3') estado_actual = ESTADO_DEPOSITO;
                else if (opcion == '4') estado_actual = ESTADO_ESPERANDO_TARJETA;
            }
            break;

            case ESTADO_SALDO: // despliegue de dinero disponible
            eeprom_read_block(&cuenta_actual, &cuentas_guardadas[cuenta_actual_index], sizeof(Cuenta)); // lectura de datos actualizados
            lcd_clear();
            lcd_gotoxy(0, 0);
            lcd_print("Saldo Actual:");
            lcd_gotoxy(0, 1);

            char str_saldo[16];
            sprintf(str_saldo, "$%lu", cuenta_actual.saldo); // formateo del monto
            lcd_print(str_saldo); // lo enviamos al display

            _delay_ms(3000); // mostramos la cifra durante 3 segundos
            estado_actual = ESTADO_MENU_PRINCIPAL; // retorno al menu
            break;

            case ESTADO_RETIRO: // procesamiento de egreso de dinero
            lcd_clear();
            lcd_gotoxy(0, 0);
            lcd_print("Monto Retiro(=):");

            keypad_read_string(entrada_texto, 6, 0); // leemos la cantidad a extraer
            uint32_t monto_retiro = atol(entrada_texto); // conversion a numero entero

            eeprom_read_block(&cuenta_actual, &cuentas_guardadas[cuenta_actual_index], sizeof(Cuenta));

            if (monto_retiro > 0 && monto_retiro <= cuenta_actual.saldo) { // validacion de fondos
                cuenta_actual.saldo -= monto_retiro; // descontamos el valor
                eeprom_update_block(&cuenta_actual, &cuentas_guardadas[cuenta_actual_index], sizeof(Cuenta)); // actualizamos la EEPROM

                lcd_clear();
                lcd_gotoxy(0, 0);
                lcd_print("Retiro Exitoso!");
                activar_exito(); // senal verde de exito
                } else { // saldo insuficiente o monto cero
                lcd_clear();
                lcd_gotoxy(0, 0);
                lcd_print("     Saldo");
                lcd_gotoxy(0, 1);
                lcd_print(" Insuficiente!");
                activar_error(); // indicador rojo de error
            }
            _delay_ms(2000);
            estado_actual = ESTADO_MENU_PRINCIPAL;
            break;

            case ESTADO_DEPOSITO: // procesamiento de abono de dinero
            lcd_clear();
            lcd_gotoxy(0, 0);
            lcd_print("Monto Dep. (=):");

            keypad_read_string(entrada_texto, 6, 0); // leemos el valor digitado
            uint32_t monto_deposito = atol(entrada_texto); // conversion a variable numerica

            if (monto_deposito > 0) { // comprobamos que sea un monto valido
                eeprom_read_block(&cuenta_actual, &cuentas_guardadas[cuenta_actual_index], sizeof(Cuenta));
                cuenta_actual.saldo += monto_deposito; // sumamos al saldo existente
                eeprom_update_block(&cuenta_actual, &cuentas_guardadas[cuenta_actual_index], sizeof(Cuenta)); // guardamos en memoria

                lcd_clear();
                lcd_gotoxy(0, 0);
                lcd_print("Deposito Exitoso");
                activar_exito(); // LED verde de confirmacion
                } else { // si ingreso cero o nada
                lcd_clear();
                lcd_gotoxy(0, 0);
                lcd_print("Monto Invalido!");
                activar_error(); // aviso de error con LED rojo
            }
            _delay_ms(2000);
            estado_actual = ESTADO_MENU_PRINCIPAL;
            break;
        }
    }
    return 0;
}