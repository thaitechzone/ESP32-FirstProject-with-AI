#include <Arduino.h>
#include <WiFi.h>

// --- WiFi ---
#define WIFI_SSID "MyHome_2.4G"
#define WIFI_PASSWORD "0939391546"

// --- Relay (Active Low) ---
#define RELAY1 17
#define RELAY2 16
#define RELAY3 4

#define RELAY_ON  LOW
#define RELAY_OFF HIGH

// --- Switch (Active Low, External Pull-up) ---
#define SW1 34
#define SW2 35
#define SW3 32

#define DEBOUNCE_MS 20

struct SwitchState {
  uint8_t pin;
  bool lastRaw;       // สถานะ raw ล่าสุดที่อ่านได้
  bool stable;        // สถานะที่ผ่าน debounce แล้ว
  unsigned long lastChangeTime;
};

struct RelayState {
  uint8_t pin;
  bool on;            // true = ON, false = OFF
};

SwitchState sw[3] = {
  {SW1, HIGH, HIGH, 0},
  {SW2, HIGH, HIGH, 0},
  {SW3, HIGH, HIGH, 0},
};

RelayState relay[3] = {
  {RELAY1, false},
  {RELAY2, false},
  {RELAY3, false},
};

void toggleRelay(RelayState &r) {
  r.on = !r.on;
  digitalWrite(r.pin, r.on ? RELAY_ON : RELAY_OFF);
}

void setup() {
  Serial.begin(115200);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.printf("Connecting to WiFi: %s", WIFI_SSID);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.printf("\nWiFi connected — IP: %s\n", WiFi.localIP().toString().c_str());

  for (int i = 0; i < 3; i++) {
    pinMode(relay[i].pin, OUTPUT);
    digitalWrite(relay[i].pin, RELAY_OFF);
  }

  // GPIO34/35 เป็น Input Only, ใช้ INPUT เพราะมี External Pull-up แล้ว
  for (int i = 0; i < 3; i++) {
    pinMode(sw[i].pin, INPUT);
  }
}

void loop() {
  unsigned long now = millis();

  for (int i = 0; i < 3; i++) {
    bool raw = digitalRead(sw[i].pin);

    // รีเซ็ตนาฬิกาทุกครั้งที่สัญญาณเปลี่ยน
    if (raw != sw[i].lastRaw) {
      sw[i].lastRaw = raw;
      sw[i].lastChangeTime = now;
    }

    // ยืนยันเมื่อสัญญาณ stable ครบ DEBOUNCE_MS
    if ((now - sw[i].lastChangeTime) >= DEBOUNCE_MS && raw != sw[i].stable) {
      sw[i].stable = raw;
      if (sw[i].stable == LOW) {  // Active Low — กด = LOW
        toggleRelay(relay[i]);
        Serial.printf("SW%d กด -> Relay%d %s\n", i + 1, i + 1, relay[i].on ? "ON" : "OFF");
      }
    }
  }
}
