import serial
import threading
import time
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation

PUERTOSERIAL = 'COM3'  

BAUDRATE = 9600        

MAX_PUNTOS = 100
ejeX = []
datosSetpoint = []
datosLdr = []
datosPot = []
datosPwm = []

ejecutando = True

def lecturaSerial():
    global ejecutando, datosSetpoint, datosLdr, datosPot, datosPwm, ejeX
    
    try:

        ser = serial.Serial(PUERTOSERIAL, BAUDRATE, timeout=1)
        time.sleep(1.5)

        contador = 0
        while ejecutando:
            if ser.in_waiting > 0:
                linea = ser.readline().decode('utf-8', errors='ignore').strip()
                
                partes = linea.split(',')
                if len(partes) == 5:
                    try:
                        sp = int(partes[0])
                        ldr = int(partes[1])
                        pot = int(partes[2])
                        pwm = int(partes[3])

                        contador += 1
                        ejeX.append(contador)
                        datosSetpoint.append(sp)
                        datosLdr.append(ldr)
                        datosPot.append(pot)
                        datosPwm.append(pwm)

                        # Limitar tamaño de buffers
                        if len(ejeX) > MAX_PUNTOS:
                            ejeX.pop(0)
                            datosSetpoint.pop(0)
                            datosLdr.pop(0)
                            datosPot.pop(0)
                            datosPwm.pop(0)

                    except ValueError:
                        #ignora textos
                        pass

    except serial.SerialException as e:
        print(f"Error en puerto serial: {e}")
        ejecutando = False

fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 8), sharex=True)
fig.canvas.manager.set_window_title('Monitoreo del Sistema de Iluminación')

linea_sp, = ax1.plot([], [], 'r--', label='Setpoint (Deseado)', linewidth=1.5)
linea_ldr, = ax1.plot([], [], 'g-', label='LDR (Iluminación Real)', linewidth=2)
linea_pot, = ax1.plot([], [], 'b-.', label='Potenciómetro (Posición Mecanismo)', linewidth=1.5)

ax1.set_ylabel('Lectura ADC (0 - 1023)')
ax1.set_ylim(-10, 1050)
ax1.legend(loc='upper right')
ax1.grid(True)
ax1.set_title('Respuesta del Sistema y Posición Angular')

linea_pwm, = ax2.plot([], [], 'm-', label='PWM Aplicado al Motor', linewidth=1.5)
ax2.set_xlabel('N° de Muestra')
ax2.set_ylabel('PWM (0 - 255)')
ax2.set_ylim(-5, 265)
ax2.legend(loc='upper right')
ax2.grid(True)

def actualizar(frame):
    if len(ejeX) > 0:
        ax1.set_xlim(ejeX[0], ejeX[-1] + 1)
        
        linea_sp.set_data (ejeX, datosSetpoint)
        linea_ldr.set_data(ejeX, datosLdr)
        linea_pot.set_data(ejeX, datosPot)
        linea_pwm.set_data(ejeX, datosPwm)

    return linea_sp, linea_ldr, linea_pot, linea_pwm

hilo = threading.Thread(target=lecturaSerial, daemon=True)
hilo.start()

ani = FuncAnimation(fig, actualizar, interval=100, cache_frame_data=False)

plt.tight_layout()
plt.show()

ejecutando = False