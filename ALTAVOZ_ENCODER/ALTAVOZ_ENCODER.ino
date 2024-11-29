#include <driver/dac.h>
#include <driver/timer.h>
#include <math.h>

// Pines
const dac_channel_t dacPin = DAC_CHANNEL_1; // DAC1 corresponde a GPIO25
const int clkPin = 32;                      // CLK del encoder (GPIO32)
const int dtPin = 33;                       // DT del encoder (GPIO33)
const int buttonPin = 13;                   // Pulsador para alternar delta de frecuencia (GPIO13)

// Parámetros de la señal senoidal
const int puntos = 50;              // Número de puntos de la tabla
const int amplitud = 127;           // Amplitud de la señal (máx. 127)
const int offset = 128;             // Offset para centrar la onda

// Tabla precomputada para la onda senoidal
int senoTabla[puntos];
volatile int indice = 0;

// Configuración del temporizador
hw_timer_t* timer = NULL;

// Frecuencia inicial de la onda
volatile float frecuencia = 100.0;

// Delta de frecuencia y su nivel actual
volatile float deltaFrecuencia = 10.0;
const float deltaValores[] = {100.0, 100.0, 100.0, 100.0}; // Niveles de delta
int deltaIndex = 2; // Comienza en el nivel de 10.0

void IRAM_ATTR onTimer() {
  dac_output_voltage(dacPin, senoTabla[indice]);
  indice = (indice + 1) % puntos;
}

void setup() {
  Serial.begin(9600);
  dac_output_enable(dacPin);

  // Configurar pines del encoder
  pinMode(clkPin, INPUT);
  pinMode(dtPin, INPUT);

  // Configurar el pulsador con resistencia PullUp interna
  pinMode(buttonPin, INPUT_PULLUP);

  // Precomputar la tabla de valores seno
  for (int i = 0; i < puntos; i++) {
    senoTabla[i] = offset + amplitud * sin(2 * PI * i / puntos);
  }

  // Configurar temporizador de hardware
  timer = timerBegin(0, 80, true); // Prescaler 80 (1 µs por tick)
  timerAttachInterrupt(timer, &onTimer, true);
  actualizarFrecuencia(frecuencia); // Configuración inicial
  timerAlarmEnable(timer);          // Activar alarma del temporizador
}

void loop() {
  static int lastClkState = HIGH;
  static int lastButtonState = HIGH;
  static unsigned long lastDebounceTime = 0;
  const unsigned long debounceDelay = 50; // Tiempo de debounce para el botón

  // Leer estado actual del CLK
  int currentClkState = digitalRead(clkPin);

  // Detectar flanco descendente del CLK
  if (currentClkState == LOW && lastClkState == HIGH) {
    // Leer estado del DT para determinar dirección
    if (digitalRead(dtPin) == HIGH) {
      frecuencia += deltaFrecuencia; // Incrementar frecuencia
    } else {
      frecuencia -= deltaFrecuencia; // Reducir frecuencia
    }

    // Limitar frecuencia a un rango útil
    if (frecuencia < 100.0) frecuencia = 100.0;
    if (frecuencia > 2000.0) frecuencia = 1000.0;

    actualizarFrecuencia(frecuencia);

    // Mostrar la frecuencia actual por consola
    
    Serial.println(frecuencia);
  }

  lastClkState = currentClkState;

  // Leer estado actual del botón
  int buttonState = digitalRead(buttonPin);

  // Detectar flanco descendente con debounce
  if (buttonState == LOW && lastButtonState == HIGH && (millis() - lastDebounceTime) > debounceDelay) {
    // Cambiar al siguiente nivel de delta
    deltaIndex = (deltaIndex + 1) % 4; // Ciclar entre 0, 1, 2, 3
    deltaFrecuencia = deltaValores[deltaIndex];

    // // Mostrar el delta actual en consola
    // Serial.print("Delta de frecuencia cambiado a: ");
    // Serial.println(deltaFrecuencia);

    lastDebounceTime = millis(); // Actualizar el tiempo de debounce
  }

  lastButtonState = buttonState;
}

void actualizarFrecuencia(float frecuencia) {
  float periodo = 1.0 / (frecuencia * puntos);  // Periodo total del ciclo
  int tiempoPorPunto = (periodo * 1e6);         // Tiempo por punto en microsegundos

  timerAlarmWrite(timer, tiempoPorPunto, true); // Configurar temporizador con el nuevo valor
}
