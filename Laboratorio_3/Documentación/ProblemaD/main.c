
#include <avr/io.h>
#include <util/delay.h>

int acompanamiento1[] = {}; // Contiene la lista de frecuencias necesarias para el acompanamiento de la cancion 1
int duracionesAc1[] = {}; // Contiene la lista de duraciónes de cada nota en milisegundos para el acompañamiento 1

int melodia1[] = {}; // Contiene la lista de frecuencias necesaria para la melodia de la cancion 1
int duracionesMe1[] = {}; // Contiene la lista de duraciónes de cada nota en milisegundos para la melodia 1

//Guarda el largo de las melodias y acompañamientos con el fin de saber cuando se termina la canción cuando esta se esté reproduciendo.
int cantidadElementosAc1 = sizeof(acompanamiento1) / sizeof(acompanamiento1[0]);
int cantidadElementosMe1 = sizeof(melodia1) / sizeof(melodia1[0]);


int acompanamiento2[] = {}; // Contiene la lista de frecuencias necesarias para el acompanamiento de la cancion 2
int duracionesAc2[] = {}; // Contiene la lista de duraciónes de cada nota en milisegundos para el acompañamiento 2

int melodia2[] = {}; // Contiene la lista de frecuencias necesaria para la melodia de la cancion 2
int duracionesMe2[] = {}; // Contiene la lista de duraciónes de cada nota en milisegundos para la melodia 2

//Guarda el largo de las melodias y acompañamientos con el fin de saber cuando se termina la canción cuando esta se esté reproduciendo.
int cantidadElementosAc2 = sizeof(acompanamiento2) / sizeof(acompanamiento2[0]);
int cantidadElementosMe2 = sizeof(melodia2) / sizeof(melodia2[0]);


int acompanamiento3[] = {}; // Contiene la lista de frecuencias necesarias para el acompanamiento de la cancion 3
int duracionesAc3[] = {}; // Contiene la lista de duraciónes de cada nota en milisegundos para el acompañamiento 3

int melodia3[] = {}; // Contiene la lista de frecuencias necesaria para la melodia de la cancion 3
int duracionesMe3[] = {}; // Contiene la lista de duraciónes de cada nota en milisegundos para la melodia 3

//Guarda el largo de las melodias y acompañamientos con el fin de saber cuando se termina la canción cuando esta se esté reproduciendo.
int cantidadElementosAc3 = sizeof(acompanamiento3) / sizeof(acompanamiento3[0]);
int cantidadElementosMe3 = sizeof(melodia3) / sizeof(melodia3[0]);

void configurarPuertos(void);
	// Configuracion de puertos
	
void configurarTimer(void);
	// Configuracion del timer para las frecuencias
	
void configurarLCD(void);
	// configuracion del Display
	
void configurarUART(void);
	// Configuracion del UART
	
char leerUART(void);
	// Lee el UART y lo guarda en una variable
	// return variableDelUART
	
char menuModos(void);
	// Muestra el menú de modo. 
	char opcion = 0;
	// Se queda esperando hasta que presionen P, p, C o c
	int opcion = leerUART;
	// return opcion
	
char menuCanciones(void);
	// Muestra el menú de Cancion
	// Esperar hasta recibir '1', '2' o '3'
	int cancion = leerUART;
	// return cancion

	
void elegirMusica(char cancion);
	// Si cancion == '1' llama a reproducirCancion(acompanamiento1, melodia1, duracionesMe1, duracionesAc1, cantidadElementosMe1, cantidadElementosAc1)
	// Si cancion == '2' llama a reproducirCancion(acompanamiento2, melodia2, duracionesMe2, duracionesAc2, cantidadElementosMe2, cantidadElementosAc2)
	// Si cancion == '3' llama a reproducirCancion(acompanamiento3, melodia3, duracionesMe3, duracionesAc3, cantidadElementosMe3, cantidadElementosAc3)

void reproducirCancion(int acompanamiento[],int melodia[],int duracionesMe[],int duracionesAc[], int cantidadElementosMe, int cantidadElementosAc);
	// Envia a los buzzers la melodia y acompanamiento correspondientes. 

int leerBoton(void);
// Lee el estado de los pines y lo compara con el valor que tendrian para cada nota, guarda en valorBoton un valor del 1 al 8 dependiendo de la nota (1 = do, 2 = re... 8 = do alto)
// Si no se presiona ningun boton guarda 0
// return valorBoton

void reproducirNotas(int valorBoton);
	// Si valorBoton == 1 reproduce la frec del do (262)
	// Si valorBoton == 2 reproduce la frec del re (294)
	// Si valorBoton == 3 reproduce la frec del mi (330)
	// Si valorBoton == 4 reproduce la frec del fa (349)
	// Si valorBoton == 5 reproduce la frec del sol (392)
	// Si valorBoton == 6 reproduce la frec del la (440)
	// Si valorBoton == 7 reproduce la frec del si (494)
	// Si valorBoton == 8 reproduce la frec del do alto (523)

	
void modoPiano(void);
	// Muestra en el display "Modo Piano"
	while(1){
		// Si desde la PC enviaron 'M' o 'm' para volver al menú sale de modoPiano() y regresa al main()
		// Llama a leerBoton() para saber que boton se esta apretando
		// Si se presiona un boton (valorBoton entre 1 y 8)
			// Llama a reproducirNotas(valorBoton) para reproducir la frecuencia correspondiente
			// Muestra en el display el nombre de la nota
		// Si no se presiona ningun boton (valorBoton == 0)
			// Limpia el display
			// Apaga los buzzers.
		}
	
		
void modoCancion(char cancion);
	// Llama a elegirMusica(cancion)
	// Muestra en el display el nombre de la cancion que se está reproduciendo
	// Al finalizar la canción apaga los buzzers.
	
int main(void)
{
	configurarPuertos();
	configurarTimer();
	configurarLCD();
	configurarUART();

	while (1)
	{
		// Espera a que se seleccione el modo (P/p o C/c)
		char modo = menuModos();
		// Si modo == 'P' llama a modoPiano();
		// Si modo == 'C':
			// Espera que se seleccione la cancion ('1', '2', '3')
			char cancion = menuCanciones();
			// llama a modoCancion y le pasa la canción correspondiente. 
			modoCancion(cancion);
	}

	return 0;
}