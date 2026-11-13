import serial
import time
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

PUERTO_SERIE = 'COM11'      # <-- Puerto COM del Arduino medidor
BAUD_RATE = 38400           # Debe coincidir con BAUD del firmware
DURACION_CAPTURA_S = 45     # ACTIVO (10 s) -> STANDBY (20 s) -> APAGADO, con margen

VREF = 1.1                  # Referencia interna del ADC (V). Es de 1,0 a 1,2 V segun el chip:
                            # medila con el multimetro en el pin AREF y pone ese valor aca.
R_SHUNT = 1.1               # Resistencia shunt (Ohm). Mejor si la medis con el multimetro.
N_MUESTRAS = 16             # Lecturas sumadas por el firmware en cada muestra
VCC = 5.0                   # Tension en el pin 5V de la placa principal (medila)

datos_tiempo = []
datos_vshunt = []
datos_corriente = []

print(f"Abriendo el puerto {PUERTO_SERIE} a {BAUD_RATE} baudios...")

try:
    # Abrir puerto serie
    ser = serial.Serial()
    ser.port = PUERTO_SERIE
    ser.baudrate = BAUD_RATE
    ser.timeout = 2.0       # Timeout de 2 segundos para no congelar la consola

    # Desactivar DTR/RTS para que el Arduino no se reinicie al abrir el puerto
    ser.dtr = False
    ser.rts = False
    ser.open()

    time.sleep(2)           # Esperar a que se estabilice el puerto
    ser.reset_input_buffer()  # Descartar datos viejos o lineas cortadas

    print("Capturando datos de consumo (Ctrl+C para cancelar)...")

    start_time = time.time()
    muestras_recibidas = 0

    while (time.time() - start_time) < DURACION_CAPTURA_S:
        linea = ser.readline().decode('ascii', errors='ignore').strip()

        if not linea:
            print(" Esperando datos (no se recibe nada por el puerto serie)...")
            continue

        partes = linea.split(',')
        if len(partes) != 2:
            continue        # Linea incompleta (por ejemplo, la primera)

        try:
            t_ms = int(partes[0])
            suma = int(partes[1])
        except ValueError:
            continue

        adc = suma / N_MUESTRAS         # Promedio de las 16 lecturas (con decimales)
        v = adc * VREF / 1024.0         # Tension en el shunt (V)
        i = v / R_SHUNT * 1000.0        # Corriente (mA)

        datos_tiempo.append(t_ms / 1000.0)
        datos_vshunt.append(v)
        datos_corriente.append(i)
        muestras_recibidas += 1

        # Mostrar una muestra cada 0,5 s (imprimir las 100 por segundo frena la lectura)
        if muestras_recibidas % 50 == 0:
            print(f" [{muestras_recibidas}] t={t_ms / 1000.0:.2f} s  ADC={adc:.1f}  "
                  f"Vshunt={v * 1000:.1f} mV  I={i:.1f} mA")

    ser.close()
    print(f"\nCaptura finalizada. Total de muestras: {muestras_recibidas}")

except KeyboardInterrupt:
    print("\nCaptura interrumpida por el usuario.")
    if 'ser' in locals() and ser.is_open:
        ser.close()
except Exception as e:
    print(f"\nError al comunicar con la USART: {e}")

# Procesar y graficar si se recibieron datos
if datos_tiempo:
    df = pd.DataFrame({
        'Tiempo_s': datos_tiempo,
        'Vshunt_V': datos_vshunt,
        'Corriente_mA': datos_corriente
    })

    # Tiempo relativo: que la grafica empiece en 0 s
    df['Tiempo_s'] = df['Tiempo_s'] - df['Tiempo_s'].iloc[0]

    df.to_csv('mediciones_consumo.csv', index=False)
    print("Archivo 'mediciones_consumo.csv' exportado.")

    # Calculos
    dt = np.diff(df['Tiempo_s'], prepend=df['Tiempo_s'].iloc[0])
    carga_total_mAh = (df['Corriente_mA'] * dt).sum() / 3600.0
    corriente_promedio = df['Corriente_mA'].mean()
    potencia_promedio_mW = VCC * corriente_promedio

    print("\n========= RESULTADOS DE CONSUMO =========")
    print(f"Duracion:           {df['Tiempo_s'].iloc[-1]:.2f} s")
    print(f"Corriente minima:   {df['Corriente_mA'].min():.2f} mA")
    print(f"Corriente maxima:   {df['Corriente_mA'].max():.2f} mA")
    print(f"Corriente promedio: {corriente_promedio:.2f} mA")
    print(f"Potencia promedio:  {potencia_promedio_mW:.2f} mW")
    print(f"Carga total:        {carga_total_mAh:.4f} mAh")
    print("=========================================\n")

    # Grafica
    plt.figure(figsize=(10, 5))
    plt.plot(df['Tiempo_s'], df['Corriente_mA'], color='tab:blue', label='Corriente (mA)')
    plt.title('Problema C: Perfil de corriente vs. tiempo')
    plt.xlabel('Tiempo (s)')
    plt.ylabel('Corriente (mA)')
    plt.grid(True, linestyle='--', alpha=0.6)
    plt.legend()
    plt.tight_layout()
    plt.savefig('grafica_corriente_tiempo.png', dpi=300)
    plt.show()
else:
    print("\n[ERROR]: No se recibio ninguna muestra valida. Revisa el cableado TX/GND, "
          "el puerto COM y que los baudios coincidan (38400).")
