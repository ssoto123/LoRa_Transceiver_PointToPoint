# 📡 Comunicación LoRa Punto a Punto (P2P) - Protocolo Manual

![Status](https://img.shields.io/badge/Status-Educational-success)
![Platform](https://img.shields.io/badge/Platform-ESP32_Heltec_V2-blue)
![LoRa](https://img.shields.io/badge/Protocol-LoRa_P2P-orange)

Este repositorio contiene el código fuente para las prácticas de laboratorio de las asignaturas de: **Sistemas de Sensores** , **Tec. Inalámbricas** e **Internet de las Cosas**, de ITSOEH y la Maestría en Internet de las Cosas por la UAEH.

El proyecto implementa un transceptor LoRa básico que simula manualmente funciones de la **Capa de Enlace de Datos**, permitiendo el envío y recepción de paquetes estructurados entre dos nodos ESP32 sin depender de protocolos de alto nivel como LoRaWAN.

## 🎯 Objetivos de Aprendizaje

1.  **Encapsulamiento de Datos:** Comprender cómo se construye una trama de datos (Header + Payload).
2.  **Manejo de Tiempos No Bloqueante:** Implementación de `millis()` para multitarea (escuchar y esperar al mismo tiempo).
3.  **Control de Flujo:** Implementación básica de CSMA/CA (Carrier Sense Multiple Access with Collision Avoidance) usando *Jitter* aleatorio.
4.  **Filtrado de Direcciones:** Lógica de software para aceptar o descartar paquetes según el destinatario.

## 🛠️ Requisitos de Hardware

* 2x Tarjetas de desarrollo **Heltec WiFi LoRa 32 V2**.
* Cable Micro-USB.
* (Opcional) Sensor DHT11 o DHT22.

## 💻 Requisitos de Software y Librerías

⚠️ **IMPORTANTE:** Este código está diseñado para funcionar con versiones específicas de las librerías para asegurar la compatibilidad en el laboratorio.

| Componente | Librería / Gestor | Versión Requerida |
| :--- | :--- | :--- |
| **Board Manager** | Heltec ESP32 Dev-Boards | **1.1.5** (Estricto) |
| **Librería LoRa** | Integrada en Heltec.h | N/A |
| **Sensor (Opcional)** | DHT Sensor Library | Última estable |

> **Nota:** No actualizar la librería de las tarjetas Heltec a la versión 2.0+ o 3.0+ sin modificar el código, ya que la definición de pines y objetos cambia drásticamente.

## 📦 Estructura del Paquete LoRa

A diferencia de un `Serial.print` simple, en este código construimos un paquete byte por byte para garantizar la integridad:

```text
+---------+---------+---------+---------+----------------------+
|  BYTE 0 |  BYTE 1 |  BYTE 2 |  BYTE 3 |       BYTES 4...n    |
+---------+---------+---------+---------+----------------------+
| Destino | Remite  |   ID    | Longitud|   PAYLOAD (Mensaje)  |
+---------+---------+---------+---------+----------------------+
     ^         ^         ^         ^               ^
     |         |         |         |               |__ Datos del sensor
     |         |         |         |__ Tamaño del mensaje para checksum
     |         |         |__ Contador para detectar paquetes perdidos
     |         |__ ¿Quién envía? (Ej. 0xC1)
     |__ ¿Para quién es? (Ej. 0xD3)

## ⚙️ Configuración para la Práctica

Para probar la comunicación entre dos estudiantes (Equipo A y Equipo B), se deben configurar las direcciones en el código antes de subirlo:

**Estudiante A (Nodo 1):**

```cpp
byte dir_local   = 0xC1; 
byte dir_destino = 0xD3;
```

**Estudiante B (Nodo 2):**
```cpp
byte dir_local   = 0xD3; 
byte dir_destino = 0xC1;
```
