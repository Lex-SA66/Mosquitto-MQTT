# Sistema de Telemetría Sísmica IoT con MQTT

## Descripción del Proyecto
Este repositorio contiene la documentación y el código fuente de un proyecto IoT diseñado para comprender y aplicar el protocolo MQTT en arquitecturas de red locales. 

El sistema emula un sismógrafo digital. Consiste en una **ESP32** (Publisher) que lee datos físicos de un acelerómetro MPU6050, procesa las señales mediante un filtro pasa-altas y un algoritmo de "Cruce por Cero" para detectar patrones oscilatorios, y envía estos datos de forma asíncrona a un **Servidor/Broker Mosquitto** alojado en una placa **Arduino UNO Q** (entorno Linux). Finalmente, los datos son consumidos por un **Dashboard Web** (Subscriber) en tiempo real.

---

## Objetivos de Aprendizaje
* Comprender el protocolo MQTT y su aplicación en ecosistemas IoT.
* Instalar, configurar y administrar Mosquitto MQTT en un entorno Linux (Arduino UNO Q / alternativo a Raspberry Pi).
* Programar un nodo Publisher en ESP32 implementando código asíncrono (FreeRTOS) para evitar bloqueos del sistema.
* Demostrar la integración exitosa entre dispositivos embebidos (sensores físicos) y servidores ligeros de mensajería (Dashboard Web).
* Documentar el proceso de despliegue utilizando buenas prácticas en GitHub.

---

## Arquitectura del Sistema
1.  **Broker MQTT:** Placa Arduino UNO Q corriendo Debian/Linux con el servicio `mosquitto` activo en el puerto 1883 (TCP) y 9001 (WebSockets).
2.  **Publisher:** Microcontrolador ESP32 programado en C++ (Arduino IDE) con la librería `PubSubClient`.
3.  **Subscriber (Consola):** Terminal SSH local en la Arduino UNO Q utilizando `mosquitto_sub`.
4.  **Subscriber (Web):** Interfaz HTML5/JS utilizando `Paho MQTT` y `Chart.js` para visualización gráfica y alertas sonoras/vibratorias.

---

## Instrucciones de Instalación y Configuración

### 1. Instalación del broker Mosquitto en Arduino UNO Q
Se accedió a la placa a través de SSH y se ejecutaron los siguientes comandos para actualizar el sistema e instalar el broker:

```bash
# 1. Actualización de repositorios y paquetes
sudo apt update && sudo apt upgrade -y

# 2. Instalación de Mosquitto y las herramientas de cliente
sudo apt install mosquitto mosquitto-clients -y

# 3. Habilitar Mosquitto para ejecutarse como servicio en el arranque
sudo systemctl enable mosquitto

# 4. Iniciar el servicio manualmente por primera vez
sudo systemctl start mosquitto

```

### 2. Prueba Básica del Broker (Local)
Para validar el funcionamiento del broker, se abrieron dos sesiones de terminal simultáneas en el servidor:

Terminal 1 (Subscriber):
Se suscribió a la escucha de un tópico de prueba.

```bash
mosquitto_sub -h localhost -t "sismo/alerta"
```

Terminal 2 (Publisher):

Se envió un mensaje simulando una alerta.

```bash
mosquitto_pub -h localhost -t "sismo/alerta" -m "SISTEMA OPERATIVO"
```

Resultado: El mensaje "SISTEMA OPERATIVO" apareció instantáneamente en la Terminal 1, confirmando la operatividad del broker local.

### 3. Configuración del Publisher en ESP32
Se utilizó Arduino IDE para la programación del firmware.

Se instalaron las librerías PubSubClient (para MQTT) y Wire (para la comunicación I2C con el sensor MPU6050).

Se configuró el código para conectarse a la red WiFi local y posteriormente apuntar a la IP dinámica asignada a la placa Arduino UNO Q.

La ESP32 fue programada para publicar periódicamente en dos tópicos:

sismo/grafica: Telemetría continua de aceleración lineal en el eje X.

sismo/alerta: Envío del payload "PELIGRO" únicamente al detectar más de 8 oscilaciones completas por segundo (criterio de validación sísmica).

(El código fuente se encuentra en la ruta codigo_esp32/sismografo_esp32.ino).

## Evidencias de Funcionamiento
### A. Instalación y Estado del Broker Mosquitto
Comprobación del servicio corriendo en el sistema Linux.

### B. Prueba Básica de Mensajería (Publisher / Subscriber local)
Validación de transmisión de datos en la misma placa utilizando comandos de consola.

### C. Ejecución del Publisher (Logs del ESP32)
Monitor Serial mostrando la conexión asíncrona exitosa y el reconocimiento del algoritmo de sismos.

### D. Interfaz de Suscriptor Final (Dashboard Web)
Recepción e interpretación visual de la telemetría en tiempo real, procesando los tópicos enviados por el ESP32.
