/*
 * ---------------------------------------------------------------------------
 * ASIGNATURA: SISTEMAS DE SENSORES | MAESTRÍA IOT
 * PLATAFORMA: Heltec WiFi LoRa 32 V2
 * * OBJETIVO: 
 * Entender cómo se "encapsula" y "desencapsula" información manualmente
 * para enviarla por radiofrecuencia. Simulamos una capa de enlace de datos.
 * ---------------------------------------------------------------------------
 */

#include "heltec.h"

// ==========================================
// 1. CONFIGURACIÓN DE RADIO (CAPA FÍSICA)
// ==========================================
// 915E6 equivale a 915,000,000 Hz. Es la frecuencia libre ISM para América.
#define BAND    915E6  

// Spreading Factor (SF): Imaginen esto como la velocidad del habla.
// SF bajo (7) = Hablar rápido (menos alcance, más datos).
// SF alto (12) = Hablar muy lento y claro (máximo alcance, pocos datos).
byte spread_factor = 8; 

// ==========================================
// 2. IDENTIDAD Y DIRECCIONAMIENTO (EL "SOBRE")
// ==========================================
// Cada dispositivo necesita un "Nombre" único en la red.
byte dir_local   = 0xC1; // ESTE dispositivo (Mi dirección)
byte dir_destino = 0xD3; // EL OTRO dispositivo (A quién le escribo)

// Variables de control de flujo
byte id_msjLoRa  = 0;   // Contador secuencial (0, 1, 2...) para detectar si se pierden mensajes.
String paqueteEnv= "";  // Aquí guardaremos los datos del sensor listos para enviar.

// ==========================================
// 3. VARIABLES DE RECEPCIÓN (BUFFER)
// ==========================================
// Estas variables almacenan temporalmente lo que llega por el aire.
byte dir_envio  = 0;    // ¿A quién iba dirigido el mensaje que capté?
byte dir_remite = 0;    // ¿Quién lo envió? (Remitente)
String paqueteRcb = ""; // El mensaje real (Payload)
byte   paqRcb_ID  = 0;  // ID del mensaje recibido
byte   paqRcb_Estado = 0; // Bandera de estado: 0=Nada, 1=OK, 2=Error Longitud, 3=Error Dirección

// ==========================================
// 4. TEMPORIZACIÓN (EVITAR COLISIONES)
// ==========================================
boolean serial_msj = true; // Para depurar en pantalla del PC
String sensorEstado = "ON"; // Variable global que simula el valor actual del sensor

long tiempo_antes     = 0;
long tiempo_intervalo = 6000; // Intervalo base de envío: 6 segundos.
// CSMA/CA Simplificado: Agregamos un tiempo aleatorio (0-3s) extra.
// Esto evita que si dos nodos encienden a la vez, se interfieran eternamente.
long tiempo_espera = tiempo_intervalo + random(3000);

#define LED_PIN 25 // LED integrado para feedback visual

void setup() {
  // Inicializamos todo el hardware de la placa Heltec
  // Argumentos: Display, LoRa, Serial, PABOOST, BAND
  // PABOOST: Amplificador de potencia (true para máximo alcance, consume más batería)
  Heltec.begin(true /*Display*/, true /*LoRa*/, serial_msj /*Serial*/, true /*PABOOST*/, BAND);
  
  // Configuración visual (OLED)
  Heltec.display->init();
  Heltec.display->flipScreenVertically();
  Heltec.display->setFont(ArialMT_Plain_10);
  Heltec.display->clear();
  Heltec.display->drawString(0, 0, "Laboratorio LoRa");
  Heltec.display->drawString(0, 10, "Iniciando Rx...");
  Heltec.display->display();

  LoRa.setSpreadingFactor(spread_factor);
  
  // ¡IMPORTANTE! Ponemos la radio en modo "Escucha Continua".
  // Sin esto, el chip LoRa se duerme y no recibe nada.
  LoRa.receive(); 
  
  pinMode(LED_PIN, OUTPUT);
}

void loop() {
  // -------------------------------------------------------
  // BLOQUE 1: TRANSMISIÓN (TX) - ¿Es hora de enviar?
  // LÓGICA DE TRANSMISIÓN (TX)
  // ----------------------------------------
  // Usamos millis() en lugar de delay() para no bloquear el procesador
  // y poder recibir mensajes mientras esperamos para enviar.
  if ((millis() - tiempo_antes) >= tiempo_espera) {
    
    // 1. Leemos el sensor (Simulado o Real)
    sensor_revisa(); 
    paqueteEnv = sensorEstado; // Copiamos el valor a la variable de envío
    
    // 2. Llamamos a la función que construye el paquete y lo lanza al aire
    // Pasamos: (A quién, De parte de quién, ID del mensaje, Datos)
    envia_lora(dir_destino, dir_local, id_msjLoRa, paqueteEnv);
    
    // 3. Preparamos el siguiente ciclo
    id_msjLoRa++; // Aumentamos contador ID
    if (serial_msj) {
      Serial.print("--> Enviando Paquete ID: "); Serial.println(id_msjLoRa-1);
    }
    
    tiempo_antes = millis(); // Reset del cronómetro
    tiempo_espera = tiempo_intervalo + random(3000); // Nuevo tiempo aleatorio
    
    // Parpadeo largo = He transmitido
    digitalWrite(LED_PIN, HIGH); delay(100); digitalWrite(LED_PIN, LOW);
  }

  // -------------------------------------------------------
  // BLOQUE 2: RECEPCIÓN (RX) - ¿Ha llegado algo?
  // -------------------------------------------------------
  // parsePacket() revisa el buffer del chip LoRa. 
  // Retorna el tamaño del paquete en bytes si hay algo, o 0 si está vacío.
  int packetSize = LoRa.parsePacket();
  
  if (packetSize) {
    // 1. Procesamos los bytes crudos
    recibe_lora(packetSize);
    
    // 2. Solo si el estado es 1 (Mensaje válido y para mí) lo mostramos
    if (serial_msj && paqRcb_Estado == 1) {
      Serial.print("Recibido de 0x"); Serial.print(dir_remite, HEX);
      Serial.print(": "); Serial.println(paqueteRcb);
      
      // Actualizamos pantalla OLED
      Heltec.display->clear();
      Heltec.display->drawString(0, 0, "RX de: 0x" + String(dir_remite, HEX));
      Heltec.display->drawString(0, 15, "Dato: " + paqueteRcb);
      Heltec.display->drawString(0, 30, "RSSI: " + String(LoRa.packetRssi())); // Potencia señal
      Heltec.display->display();
    }
    
    // Parpadeo corto = He recibido
    digitalWrite(LED_PIN, HIGH); delay(50); digitalWrite(LED_PIN, LOW);
  }
  
  delay(10); // Pequeña respiración para el procesador
}

/*
 * ---------------------------------------------------------------------------
 * FUNCIÓN: envia_lora
 * EXPLICACIÓN: Construye el "Sobre" byte por byte.
 * Estructura del Paquete: [DESTINO][REMITENTE][ID][LARGO][MENSAJE]
 * ---------------------------------------------------------------------------
 */
void envia_lora(byte destino, byte remite, byte paqueteID, String paquete) {
  
  // CSMA Básico: Esperar si la radio está ocupada transmitiendo algo más
  while (LoRa.beginPacket() == 0) { delay(10); } 
  
  LoRa.beginPacket(); // Iniciamos el buffer de transmisión
  
  // --- CABECERA (HEADER) ---
  LoRa.write(destino);     // Byte 1: Dirección del destinatario
  LoRa.write(remite);      // Byte 2: Mi dirección (para que sepan quién soy)
  LoRa.write(paqueteID);   // Byte 3: ID único del mensaje
  LoRa.write((byte)paquete.length()); // Byte 4: ¿Cuánto mide el mensaje?
  
  // --- CARGA ÚTIL (PAYLOAD) ---
  LoRa.print(paquete);     // El mensaje en sí (ej: "25.4 C")
  
  LoRa.endPacket();        // ¡Fuego! Envía los datos por la antena.
  
  // CRÍTICO: Al terminar de enviar, la radio se apaga. 
  // Debemos volver a ponerla a escuchar inmediatamente.
  LoRa.receive(); 
}

/*
 * ---------------------------------------------------------------------------
 * FUNCIÓN: recibe_lora
 * EXPLICACIÓN: Lee y verifica el paquete byte por byte.
 * Debe leerse en el MISMO ORDEN en que se escribió.
 * ---------------------------------------------------------------------------
 */
void recibe_lora(int tamano) {
  if (tamano == 0) return; // Seguridad
  
  paqueteRcb = ""; // Limpiamos variable anterior
  
  // 1. Leemos la CABECERA
  dir_envio  = LoRa.read(); // Byte 1: ¿Para quién es esto?
  dir_remite = LoRa.read(); // Byte 2: ¿Quién lo mandó?
  paqRcb_ID  = LoRa.read(); // Byte 3: ID del mensaje
  byte len   = LoRa.read(); // Byte 4: Longitud que dice tener el mensaje
  
  // 2. Leemos la CARGA ÚTIL (El resto de bytes)
  while (LoRa.available()) { 
    paqueteRcb += (char)LoRa.read(); // Concatenamos char a String
  }
  
  // 3. VERIFICACIÓN DE INTEGRIDAD (Checksum simple)
  // ¿La longitud recibida coincide con la que decía la cabecera?
  if (len != paqueteRcb.length()) { 
    paqRcb_Estado = 2; // Error: Paquete corrupto
    return; 
  }
  
  // 4. FILTRADO DE DIRECCIÓN (Address Filtering)
  // Si (No es para mí) Y (No es un mensaje global 0xFF)... Lo descarto.
  if (dir_envio != dir_local && dir_envio != 0xFF) { 
    paqRcb_Estado = 3; // Ignorado
    return; 
  }
  
  paqRcb_Estado = 1; // ¡Paquete Aceptado!
}

/*
 * ---------------------------------------------------------------------------
 * FUNCIÓN: sensor_revisa
 * EXPLICACIÓN: Aquí se obtiene el dato. Actualmente es una simulación.
 * Cambia "ON" a "OFF" y viceversa cada vez que se llama.
 * ---------------------------------------------------------------------------
 */
void sensor_revisa() {
  if (sensorEstado == "ON") {
    sensorEstado = "OFF";
  } else {
    sensorEstado = "ON";
  }
}
