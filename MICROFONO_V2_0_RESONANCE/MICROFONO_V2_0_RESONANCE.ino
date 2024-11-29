#include <LiquidCrystal_I2C.h>
#include <Wire.h>

// Configura LCD con dirección 0x27, 16 columnas y 2 filas
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Pines del micrófono y LED
const int micPin = 36;         // Pin analógico para el micrófono
const int ledPin = 2;          // LED para indicar resonancia
const unsigned long intervaloMuestreo = 50; // Intervalo de muestreo en microsegundos
unsigned long ultimoTiempoMuestreo = 0;      // Último tiempo de lectura del micrófono

// Pines y UART para la comunicación
const int RXD1 = 16; // Pin RX de UART1
const int TXD1 = 17; // Pin TX de UART1

// Variables para la frecuencia
float frecuenciaActual = -1;  // Última frecuencia capturada
bool actualizarLCD = false;   // Bandera para indicar si actualizar la pantalla
bool enResonancia = false;    // Bandera para indicar si hay resonancia
unsigned long inicioAlarma = 0; // Tiempo de inicio de la alarma
bool estadoLed = false;       // Estado actual del LED

void setup() {
  lcd.init();           // Inicializa la pantalla LCD
  lcd.backlight();      // Activa la luz de fondo
  pinMode(ledPin, OUTPUT);  // Configura el LED como salida

  // Encabezado en la pantalla LCD
  lcd.setCursor(0, 0);
  lcd.print("Frecuencia (Hz):");

  Serial.begin(9600);   // UART0 para monitor serial
  Serial1.begin(9600, SERIAL_8N1, RXD1, TXD1); // Configura UART1
  lcd.setCursor(0, 1);
  lcd.print("Esperando datos");
}

void loop() {
  // Detener la lectura y actualización si está en resonancia
  if (!enResonancia) {
    // **Leer datos de la UART1**
    if (Serial1.available() > 0) {
      String input = Serial1.readStringUntil('\n'); // Leer hasta nueva línea
      float nuevaFrecuencia = input.toFloat();      // Convertir a número flotante

      if (abs(nuevaFrecuencia - frecuenciaActual) > 0.1) {
        frecuenciaActual = nuevaFrecuencia;
        actualizarLCD = true;
      }
    }

    // **Actualizar la pantalla LCD**
    if (actualizarLCD) {
      actualizarLCD = false;
      lcd.setCursor(0, 1);
      lcd.print("                "); // Limpiar la línea
      lcd.setCursor(0, 1);
      lcd.print(frecuenciaActual, 2); // Mostrar frecuencia con 2 decimales
      lcd.print(" Hz");
    }

    // **Leer valor del micrófono y detectar resonancia**
    unsigned long tiempoActual = micros();
    if (tiempoActual - ultimoTiempoMuestreo >= intervaloMuestreo) {
      ultimoTiempoMuestreo = tiempoActual;
      int valorMic = analogRead(micPin);
      Serial.print(4095);
      Serial.print(" ");
      Serial.print(0);
      Serial.print(" ");
      Serial.println(valorMic);

      if (valorMic >= 2600) {
        alarmaResonancia();  // Activar alarma de resonancia
      }
    }
  }

  // **Parpadeo no bloqueante del LED durante resonancia**
  if (enResonancia) {
    unsigned long tiempoAlarma = millis() - inicioAlarma;
    if (tiempoAlarma < 5000) { // Parpadeo durante 5 segundos
      if (millis() % 500 < 250) {
        if (!estadoLed) {
          digitalWrite(ledPin, HIGH);
          estadoLed = true;
        }
      } else {
        if (estadoLed) {
          digitalWrite(ledPin, LOW);
          estadoLed = false;
        }
      }
    } else {
      enResonancia = false;
      digitalWrite(ledPin, LOW);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Frecuencia (Hz):");
      actualizarLCD = true;
    }
  }
}

void alarmaResonancia() {
  enResonancia = true;
  inicioAlarma = millis(); // Establecer inicio del parpadeo

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Resuena:");
  lcd.setCursor(0, 1);
  lcd.print(frecuenciaActual, 2);
  lcd.print(" Hz");
}
