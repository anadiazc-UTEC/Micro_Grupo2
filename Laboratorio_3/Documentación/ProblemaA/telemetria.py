import serial
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
import time

# ==========================================
# CONFIGURACIÓN DEL PUERTO SERIE
# ==========================================
# Cambia 'COM3' por el puerto real de tu Arduino / Convertidor USB-UART
PUERTO_SERIE = 'COM3'  
BAUD_RATE = 9600

try:
    ser = serial.Serial(PUERTO_SERIE, BAUD_RATE, timeout=1)
    print(f"Conectado exitosamente a {PUERTO_SERIE}")
except Exception as e:
    print(f"Error al abrir el puerto serie: {e}")
    exit()

# Estructuras de datos para almacenar el historial
tiempos = []
temperaturas = []
rangos_min = []
rangos_max = []
estados_calefactor = []
velocidades_ventilador = []

tiempo_inicio = time.time()

# Configuración de la figura de Matplotlib
fig, (ax_temp, ax_actuadores) = plt.subplots(2, 1, sharex=True, figsize=(10, 7))
fig.canvas.manager.set_window_title('Sistema de Control de Temperatura - Monitoreo en Tiempo Real')

def actualizar_grafica(frame):
    while ser.in_waiting > 0:
        linea = ser.readline().decode('utf-8', errors='ignore').strip()
        
        # Filtrar solo las líneas que contienen la trama "DATA:"
        if linea.startswith("DATA:"):
            try:
                datos = linea.replace("DATA:", "").split(",")
                temp = float(datos[0])
                r_min = float(datos[1])
                r_max = float(datos[2])
                heater = int(datos[3]) * 100       # Convertir a Porcentaje (0% o 100%)
                fan_pwm = (float(datos[4]) / 255.0) * 100 # Convertir PWM 0-255 a Porcentaje (0-100%)

                t_actual = time.time() - tiempo_inicio

                # Guardar en vectores
                tiempos.append(t_actual)
                temperaturas.append(temp)
                rangos_min.append(r_min)
                rangos_max.append(r_max)
                estados_calefactor.append(heater)
                velocidades_ventilador.append(fan_pwm)

            except ValueError:
                pass

    # Mantener solo los últimos 60 segundos en pantalla para dinamismo
    limite_muestras = 60
    t_plot = tiempos[-limite_muestras:]
    temp_plot = temperaturas[-limite_muestras:]
    rmin_plot = rangos_min[-limite_muestras:]
    rmax_plot = rangos_max[-limite_muestras:]
    heat_plot = estados_calefactor[-limite_muestras:]
    fan_plot = velocidades_ventilador[-limite_muestras:]

    if len(t_plot) > 0:
        # -------------------------------------------------------------
        # PANEL 1: TEMPERATURA Y RANGO IDEAL (SETPOINT) JUNTOS
        # -------------------------------------------------------------
        ax_temp.clear()
        
        # Dibujar límites
        ax_temp.plot(t_plot, rmax_plot, 'g--', alpha=0.7, label='Límite Superior Rango')
        ax_temp.plot(t_plot, rmin_plot, 'g--', alpha=0.7, label='Límite Inferior Rango')
        
        # Franja Verde (Rango Ideal de Temperatura)
        ax_temp.fill_between(t_plot, rmin_plot, rmax_plot, color='green', alpha=0.15, label='Rango Ideal (Setpoint)')
        
        # Temperatura Medida Real
        ax_temp.plot(t_plot, temp_plot, 'r-o', linewidth=2, label='Temperatura Medida (°C)')

        ax_temp.set_ylabel('Temperatura (°C)')
        ax_temp.set_title('Evolución de la Temperatura vs Rango Ideal')
        ax_temp.grid(True, linestyle=':', alpha=0.6)
        ax_temp.legend(loc='upper left')

        # -------------------------------------------------------------
        # PANEL 2: ACCIONES DEL SISTEMA (ACTUADORES)
        # -------------------------------------------------------------
        ax_actuadores.clear()
        
        # Calefactor (ON/OFF)
        ax_actuadores.step(t_plot, heat_plot, color='orange', linewidth=2, where='post', label='Calefactor ON/OFF (%)')
        
        # Ventilador (Velocidad PWM %)
        ax_actuadores.plot(t_plot, fan_plot, color='blue', linewidth=2, linestyle='-', label='Ventilador PWM (%)')

        ax_actuadores.set_xlabel('Tiempo (Segundos)')
        ax_actuadores.set_ylabel('Estado / Potencia (%)')
        ax_actuadores.set_ylim(-5, 105)
        ax_actuadores.set_title('Acciones de los Actuadores')
        ax_actuadores.grid(True, linestyle=':', alpha=0.6)
        ax_actuadores.legend(loc='upper left')

# Animación en tiempo real (actualización cada 500 ms)
ani = FuncAnimation(fig, actualizar_grafica, interval=500, cache_frame_data=False)

plt.tight_layout()
plt.show()

ser.close()