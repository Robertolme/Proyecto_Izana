# Proyecto_Izana

el primero que nada que se joda caballo fiestero fakiu fakiu fakui

# Adquisición de Señal con ESP32 vía I2S + ADC

Este proyecto permite capturar señales analógicas a alta velocidad usando el ADC interno del ESP32 combinado con el periférico I2S. El objetivo es obtener muestras de hasta **1 millón por segundo (1 MSPS)** y enviarlas por puerto serial para análisis, graficación o visualización en una interfaz web.

## Características

- Lectura de señales analógicas a alta velocidad (1 MSPS)
- Resolución de 9 bits (0 - 511)
- Uso del periférico I2S para mejorar velocidad y rendimiento
- Salida por Serial en formato `signal:<valor>`
- Ideal para construir un osciloscopio casero o logger de señales

## Requisitos

- ESP32 Dev Module (cualquier placa compatible)
- Arduino IDE o PlatformIO
- Puerto GPIO36 como entrada analógica (canal ADC1_CHANNEL_0)

## Archivos importantes

- `main.ino`: Código fuente principal del proyecto
- `README.md`: Este archivo

## ¿Cómo usar?

### En Arduino IDE:

1. Copia el código en un nuevo proyecto `.ino`.
2. Selecciona la placa: **ESP32 Dev Module**
3. Conecta la señal analógica al pin **GPIO36**
4. Carga el programa y abre el **Monitor Serial** a 115200 baudios

## ejemplo de salida 

signal:121
signal:118
signal:130
signal:141


## Notas técnicas
 - El ADC se configura a 9 bits y el I2S extrae los datos en bloques de 16 bits (por hardware).
 - Se usa i2s_read() para leer múltiples muestras de forma eficiente.
 - Se recomienda usar GPIO36 (ADC1_CHANNEL_0) ya que es de los pocos compatibles con I2S.

### 🔍 Diagnóstico rápido
1. ¿Estás usando el pin correcto?
El canal ADC1_CHANNEL_0 corresponde a GPIO36 (VP).

## Asegúrate de que la señal esté conectada a ese pin.

2. ¿La señal está viva?
Si no hay nada conectado, el pin flota (quedará en 0).

Si tienes un potenciómetro, sensor, generador de funciones, etc., conéctalo al GPIO36 y alimenta el circuito.

Señales válidas: 0 V a ~3.3 V (con 11 dB de atenuación llega hasta ~3.6 V máx).

3. ¿Estás usando una fuente de señal compatible?

a) Si estás usando un potenciómetro, conecta:

b) Un extremo a 3.3 V

c) Otro a GND

d) El pin central (wiper) al GPIO36

4. ¿Qué pasa si no conectas nada?
El ADC lee voltaje en el aire → ruido o ceros.

Es normal que leas 0 si el pin está desconectado o aterrizado.

## Prueba sencilla con potenciómetro
Conecta un potenciómetro así:

[3.3V] ─── [ POT ] ─── [GND]
               │
            GPIO36

Verás valores como:

signal:23
signal:88
signal:130
signal:511

A medida que giras el potenciómetro.

