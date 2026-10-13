import serial
import time
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation

#guarda el puerto serie que se va a utilizar
PUERTOSERIAL = "COM11"

#define la velocidad de comunicacion con el microcontrolador
BAUDRATE = 9600

#define la cantidad maxima de muestras que se muestran en la grafica
MAX_PUNTOS = 100

#pide el setpoint hasta que se ingrese un valor valido
while True:
    try:
        SETPOINT = int(input("Ingrese el setpoint (0-1023): "))

        if 0 <= SETPOINT <= 1023:
            break

        print("El setpoint debe estar entre 0 y 1023.")

    except ValueError:
        print("Ingrese un numero entero valido.")


#guarda los numeros de las muestras recibidas
ejeX = []

#guarda los valores del setpoint recibidos
datosSetpoint = []

#guarda las lecturas del sensor de luz
datosLdr = []

#guarda las lecturas del potenciometro
datosPot = []

#guarda los valores del pwm del motor
datosPwm = []

#cuenta la cantidad de muestras recibidas
contador = 0

try:
    #abre el puerto serie para comunicarse con el microcontrolador
    ser = serial.Serial(
        PUERTOSERIAL,
        BAUDRATE,
        timeout=0.05
    )

    #espera a que el microcontrolador termine de iniciarse
    time.sleep(2)

    #limpia los datos anteriores que quedaron en el puerto
    ser.reset_input_buffer()

    #envia el setpoint al microcontrolador
    ser.write(f"{SETPOINT}\n".encode("ascii"))

    #espera a que se termine de enviar el setpoint
    ser.flush()

    #muestra el setpoint que se envio
    print(f"Setpoint enviado: {SETPOINT}")

    #avisa que comenzo la recepcion de datos
    print("Recibiendo datos del ATmega328P...")

except serial.SerialException as e:
    #muestra un mensaje si no se puede abrir el puerto
    print(f"No se pudo abrir el puerto serie: {e}")

    #finaliza el programa si ocurre un error de comunicacion
    raise SystemExit


#crea las dos graficas en una misma ventana
fig, (ax1, ax2) = plt.subplots(
    2, 1, figsize=(10, 8), sharex=True
)

#coloca el nombre de la ventana
fig.canvas.manager.set_window_title(
    "Monitoreo del Sistema de Iluminacion"
)

#crea la linea que representa el setpoint
linea_sp, = ax1.plot(
    [], [], "r--",
    label="Setpoint deseado",
    linewidth=1.5
)

#crea la linea que representa la lectura del sensor de luz
linea_ldr, = ax1.plot(
    [], [], "g-",
    label="LDR (iluminacion real)",
    linewidth=2
)

#crea la linea que representa la lectura del potenciometro
linea_pot, = ax1.plot(
    [], [], "b-.",
    label="Potenciometro",
    linewidth=1.5
)

#coloca el nombre del eje vertical de la primera grafica
ax1.set_ylabel("Lectura ADC (0-1023)")

#define los limites del eje vertical de la primera grafica
ax1.set_ylim(-10, 1050)

#muestra los nombres de las lineas de la grafica
ax1.legend(loc="upper right")

#agrega una cuadrilla para facilitar la lectura de los valores
ax1.grid(True)

#coloca el titulo de la primera grafica
ax1.set_title("Respuesta del sistema y posicion")


#crea la linea que representa el pwm aplicado al motor
linea_pwm, = ax2.plot(
    [], [], "m-",
    label="PWM aplicado al motor",
    linewidth=1.5
)

#coloca el nombre del eje horizontal de la segunda grafica
ax2.set_xlabel("Numero de muestra")

#coloca el nombre del eje vertical de la segunda grafica
ax2.set_ylabel("PWM (0-255)")

#define los limites del eje vertical de la segunda grafica
ax2.set_ylim(-5, 265)

#muestra el nombre de la linea de la grafica
ax2.legend(loc="upper right")

#agrega una cuadrilla para facilitar la lectura de los valores
ax2.grid(True)


#lee los datos recibidos y actualiza las graficas
def actualizar(frame):
    #permite modificar el contador de muestras
    global contador

    #lee todas las lineas que estan esperando en el puerto
    while ser.in_waiting > 0:

        #lee una linea y la convierte en texto
        linea = ser.readline().decode(
            "utf-8", errors="ignore"
        ).strip()

        #separa los valores recibidos usando las comas
        partes = linea.split(",")

        #ignora los mensajes que no tienen cinco datos
        if len(partes) != 5:
            continue

        try:
            #obtiene el setpoint enviado por el microcontrolador
            sp = int(partes[0])

            #obtiene la lectura del sensor de luz
            ldr = int(partes[1])

            #obtiene la lectura del potenciometro
            pot = int(partes[2])

            #obtiene el valor del pwm del motor
            pwm = int(partes[3])

            #obtiene el sentido de giro del motor
            sentido = int(partes[4])

        except ValueError:
            #ignora los datos que no se pueden convertir a numeros
            continue

        #aumenta el contador cada vez que llega una muestra valida
        contador += 1

        #guarda el numero de la muestra
        ejeX.append(contador)

        #guarda el setpoint recibido
        datosSetpoint.append(sp)

        #guarda la lectura del sensor de luz
        datosLdr.append(ldr)

        #guarda la lectura del potenciometro
        datosPot.append(pot)

        #guarda el valor del pwm recibido
        datosPwm.append(pwm)

        #elimina las muestras mas antiguas cuando se supera el limite
        if len(ejeX) > MAX_PUNTOS:
            ejeX.pop(0)
            datosSetpoint.pop(0)
            datosLdr.pop(0)
            datosPot.pop(0)
            datosPwm.pop(0)

    #actualiza las graficas cuando hay datos disponibles
    if ejeX:

        #ajusta el eje horizontal para mostrar las muestras recientes
        ax1.set_xlim(max(0, ejeX[0] - 1), ejeX[-1] + 1)

        #actualiza la linea del setpoint
        linea_sp.set_data(ejeX, datosSetpoint)

        #actualiza la linea del sensor de luz
        linea_ldr.set_data(ejeX, datosLdr)

        #actualiza la linea del potenciometro
        linea_pot.set_data(ejeX, datosPot)

        #actualiza la linea del pwm del motor
        linea_pwm.set_data(ejeX, datosPwm)

    #devuelve las lineas que deben actualizarse en la ventana
    return linea_sp, linea_ldr, linea_pot, linea_pwm


#actualiza las graficas cada cien milisegundos
ani = FuncAnimation(
    fig,
    actualizar,
    interval=100,
    cache_frame_data=False
)

#acomoda los elementos para que se vean correctamente
plt.tight_layout()

try:
    #muestra la ventana con las graficas
    plt.show()

finally:
    #cierra el puerto serie al terminar el programa
    ser.close()

    #avisa que se cerro la comunicacion
    print("Puerto serie cerrado.")