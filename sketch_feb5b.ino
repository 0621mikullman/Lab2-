// *****************************************************************************
// ESP32 GPIO, Timer Interrupt, and Debounce Demonstration
//
// - Built-in LED blinks 1s ON / 1s OFF
// - Push button toggles external LED ON/OFF
// - Button debounced using 1 ms hardware timer interrupt
// - Serial output reports LED state
//
// Hardware matches provided schematic:
// - Button: GPIO17, external 4.7k pull-down, active HIGH
// - External LED: GPIO15 through 220 ohm resistor to GND
//
// Debounce library:
// https://github.com/brooksbUWO/Debounce
// *****************************************************************************

#include <Arduino.h>
#include <debounce.h>

// ------------------------- GPIO Definitions -------------------------
const uint8_t LED_EXT = 15;     // External LED (D1)
const uint8_t BUTTON  = 17;     // Push button (S1)

// ------------------------- Built-in LED Blink -----------------------
const uint16_t BLINK_INTERVAL_MS = 1000;
unsigned long lastBlinkTime = 0;
bool builtinLedState = false;

// ------------------------- External LED -----------------------------
bool extLedState = false;

// ------------------------- Debounce --------------------------------
// Button is ACTIVE HIGH (per schematic)
bool pressedLogicLevel = HIGH;
Debounce myButton(BUTTON, pressedLogicLevel);

// ------------------------- Hardware Timer ---------------------------
hw_timer_t *timer0 = NULL;
portMUX_TYPE timer0mux = portMUX_INITIALIZER_UNLOCKED;
volatile bool flagTimer0 = false;

// Timer ISR (fires every 1 ms)
void IRAM_ATTR timerISR0()
{
  portENTER_CRITICAL_ISR(&timer0mux);
  flagTimer0 = true;
  portEXIT_CRITICAL_ISR(&timer0mux);
}

void setup()
{
  Serial.begin(115200);

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LED_EXT, OUTPUT);
  pinMode(BUTTON, INPUT);   // External pull-down resistor used

  digitalWrite(LED_BUILTIN, LOW);
  digitalWrite(LED_EXT, LOW);

  // Hardware timer setup
  // 80 prescaler → 1 MHz timer clock (1 µs per tick)
  timer0 = timerBegin(0, 80, true);
  timerAttachInterrupt(timer0, &timerISR0, true);

  // Interrupt every 1 ms
  timerAlarmWrite(timer0, 1000, true);
  timerAlarmEnable(timer0);

  Serial.println("System initialized.");
  Serial.println("Built-in LED blinking. Button toggles external LED.");
}

void loop()
{
  // ---------------- Built-in LED Blink ----------------
  unsigned long now = millis();
  if (now - lastBlinkTime >= BLINK_INTERVAL_MS)
  {
    lastBlinkTime = now;
    builtinLedState = !builtinLedState;
    digitalWrite(LED_BUILTIN, builtinLedState);
  }

  // ---------------- Debounce Update -------------------
  if (flagTimer0)
  {
    portENTER_CRITICAL(&timer0mux);
    flagTimer0 = false;
    portEXIT_CRITICAL(&timer0mux);

    myButton.update();   // sample button every 1 ms
  }

  // ---------------- Button Action ---------------------
  if (myButton.isPressed())
  {
    extLedState = !extLedState;
    digitalWrite(LED_EXT, extLedState);

    Serial.print("External LED is ");
    Serial.println(extLedState ? "ON" : "OFF");
  }
}
