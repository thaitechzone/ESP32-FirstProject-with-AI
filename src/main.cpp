#include <Arduino.h>

// Relay pins definition
const int RELAY1_PIN = 17;
const int RELAY2_PIN = 16;
const int RELAY3_PIN = 4;

// Timing configuration
const unsigned long ON_TIME = 5000;    // 5 seconds ON
const unsigned long OFF_TIME = 5000;   // 5 seconds OFF
const unsigned long CYCLE_TIME = ON_TIME + OFF_TIME;  // 10 seconds total cycle

unsigned long lastChangeTime = 0;
bool relayState = false;  // false = OFF (HIGH), true = ON (LOW)

void setup() {
  // Initialize relay pins as outputs
  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  pinMode(RELAY3_PIN, OUTPUT);

  // Initial state: all relays OFF (HIGH for Active Low)
  digitalWrite(RELAY1_PIN, HIGH);
  digitalWrite(RELAY2_PIN, HIGH);
  digitalWrite(RELAY3_PIN, HIGH);

  Serial.begin(115200);
  Serial.println("Relay Control System Started");
  Serial.println("Relay1 (GPIO17), Relay2 (GPIO16), Relay3 (GPIO4)");
  Serial.println("ON: 5s, OFF: 5s");
}

void loop() {
  unsigned long currentTime = millis();
  unsigned long elapsedTime = currentTime - lastChangeTime;

  // Check if it's time to toggle relay state
  if (relayState && elapsedTime >= ON_TIME) {
    // Turn OFF relays (HIGH for Active Low)
    digitalWrite(RELAY1_PIN, HIGH);
    digitalWrite(RELAY2_PIN, HIGH);
    digitalWrite(RELAY3_PIN, HIGH);
    relayState = false;
    lastChangeTime = currentTime;
    Serial.println("Relays OFF");

  } else if (!relayState && elapsedTime >= OFF_TIME) {
    // Turn ON relays (LOW for Active Low)
    digitalWrite(RELAY1_PIN, LOW);
    digitalWrite(RELAY2_PIN, LOW);
    digitalWrite(RELAY3_PIN, LOW);
    relayState = true;
    lastChangeTime = currentTime;
    Serial.println("Relays ON");
  }
}