#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --- OpenWeatherMap API ---
#define OWM_API_KEY          "1bef650d2c6ea7a91f58252948c2d325"
#define OWM_CITY             "Nakhon Si Thammarat"
#define OWM_COUNTRY          "TH"
#define WEATHER_INTERVAL_MS  (2UL * 60UL * 1000UL)

// --- Telegram Bot ---
#define TG_BOT_TOKEN  "8948698437:AAETguWxKsAg0ILJ8ANmZVGpHOsIs5-dLoY"   // เช่น 123456789:ABCdef...
#define TG_CHAT_ID    "7745779456"     // เช่น 123456789 หรือ -100123456789
#define AQI_ALERT_THRESHOLD   3          // แจ้งเตือนเมื่อ AQI >= ค่านี้
#define PM25_ALERT_THRESHOLD  35.0f      // แจ้งเตือนเมื่อ PM2.5 >= ค่านี้ (µg/m³)

// --- OLED ---
#define OLED_SDA     21
#define OLED_SCL     22
#define OLED_ADDRESS 0x3C
#define SCREEN_W     128
#define SCREEN_H     64

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

#define DEBOUNCE_MS          20
#define WIFI_RESET_HOLD_MS   5000

// ─── Structs ──────────────────────────────────────────────
struct SwitchState {
  uint8_t pin;
  bool lastRaw;
  bool stable;
  unsigned long lastChangeTime;
};

struct RelayState {
  uint8_t pin;
  bool on;
};

struct WeatherData {
  float temp;
  int   humidity;
  int   aqi;
  float pm25;
  bool  valid;
};

// ─── Globals ──────────────────────────────────────────────
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

WeatherData weather = {0, 0, 0, 0, false};
unsigned long lastWeatherFetch = 0;
bool lastAqiAlertSent = false;   // ป้องกันส่งซ้ำขณะ AQI ยังสูงอยู่

Adafruit_SSD1306 display(SCREEN_W, SCREEN_H, &Wire, -1);

// ─── Telegram Send ────────────────────────────────────────
void sendTelegram(const String& msg) {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  String url = String("https://api.telegram.org/bot") + TG_BOT_TOKEN + "/sendMessage";

  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  // escape ข้อความเป็น JSON
  JsonDocument doc;
  doc["chat_id"] = TG_CHAT_ID;
  doc["text"]    = msg;
  doc["parse_mode"] = "HTML";

  String body;
  serializeJson(doc, body);

  int code = http.POST(body);
  if (code == HTTP_CODE_OK) {
    Serial.println("[Telegram] ส่งสำเร็จ");
  } else {
    Serial.printf("[Telegram] Error: %d\n", code);
  }
  http.end();
}

// ─── OLED Helpers ─────────────────────────────────────────
void oledMsg(const char* line1, const char* line2 = "", const char* line3 = "") {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 10); display.print(line1);
  display.setCursor(0, 26); display.print(line2);
  display.setCursor(0, 42); display.print(line3);
  display.display();
}

void drawOLED() {
  display.clearDisplay();

  // ── Title bar ──
  display.fillRect(0, 0, SCREEN_W, 9, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setTextSize(1);
  display.setCursor(4, 1);
  display.print(OWM_CITY);

  // ── Weather section ──
  display.setTextColor(SSD1306_WHITE);
  if (weather.valid) {
    display.setCursor(0, 12);
    display.print("T:");
    display.print(weather.temp, 1);
    display.print("\xF8""C");

    display.setCursor(68, 12);
    display.print("H:");
    display.print(weather.humidity);
    display.print("%");

    const char* aqiLabel[] = {"", "Good", "Fair", "Mod", "Poor", "VPoor"};
    display.setCursor(0, 23);
    display.print("AQI:");
    display.print(weather.aqi);
    if (weather.aqi >= 1 && weather.aqi <= 5) {
      display.print("(");
      display.print(aqiLabel[weather.aqi]);
      display.print(")");
    }

    display.setCursor(68, 23);
    display.print("PM:");
    display.print(weather.pm25, 1);
  } else {
    display.setCursor(10, 16);
    display.print("Fetching weather...");
  }

  // ── Divider ──
  display.drawLine(0, 34, SCREEN_W - 1, 34, SSD1306_WHITE);

  // ── Relay section ──
  display.setCursor(0, 37);
  display.print("RELAY:");

  const char* rLabel[] = {"R1", "R2", "R3"};
  for (int i = 0; i < 3; i++) {
    int bx = i * 43;
    if (relay[i].on) {
      display.fillRoundRect(bx, 47, 40, 14, 3, SSD1306_WHITE);
      display.setTextColor(SSD1306_BLACK);
      display.setCursor(bx + 3, 50);
      display.print(rLabel[i]);
      display.print(":ON");
      display.setTextColor(SSD1306_WHITE);
    } else {
      display.drawRoundRect(bx, 47, 40, 14, 3, SSD1306_WHITE);
      display.setCursor(bx + 2, 50);
      display.print(rLabel[i]);
      display.print(":--");
    }
  }

  display.display();
}

// ─── Relay Toggle ─────────────────────────────────────────
void toggleRelay(RelayState &r, int index) {
  r.on = !r.on;
  digitalWrite(r.pin, r.on ? RELAY_ON : RELAY_OFF);

  String msg = String("🔌 <b>Relay") + (index + 1) + "</b> ";
  msg += r.on ? "เปิด (ON) ✅" : "ปิด (OFF) ⛔";
  sendTelegram(msg);
}

// ─── Weather Fetch ────────────────────────────────────────
void fetchWeather() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[Weather] WiFi ไม่ได้เชื่อมต่อ");
    return;
  }

  HTTPClient http;
  JsonDocument doc;
  float lat = 0, lon = 0;

  String city = String(OWM_CITY);
  city.replace(" ", "%20");
  String weatherUrl = String("http://api.openweathermap.org/data/2.5/weather?q=") +
                      city + "," + OWM_COUNTRY +
                      "&appid=" + OWM_API_KEY + "&units=metric&lang=th";

  http.begin(weatherUrl);
  int code = http.GET();
  if (code == HTTP_CODE_OK) {
    deserializeJson(doc, http.getString());
    weather.temp     = doc["main"]["temp"];
    weather.humidity = doc["main"]["humidity"];
    float feels      = doc["main"]["feels_like"];
    float windSpeed  = doc["wind"]["speed"];
    const char* desc = doc["weather"][0]["description"];
    lat = doc["coord"]["lat"];
    lon = doc["coord"]["lon"];

    Serial.println("========== สภาพอากาศ นครศรีธรรมราช ==========");
    Serial.printf("  อุณหภูมิ    : %.1f °C (รู้สึกเหมือน %.1f °C)\n", weather.temp, feels);
    Serial.printf("  ความชื้น    : %d %%\n", weather.humidity);
    Serial.printf("  ความเร็วลม  : %.1f m/s\n", windSpeed);
    Serial.printf("  สภาพ        : %s\n", desc);
  } else {
    Serial.printf("[Weather] HTTP error: %d\n", code);
  }
  http.end();

  if (lat == 0 && lon == 0) return;

  String aqiUrl = String("http://api.openweathermap.org/data/2.5/air_pollution?lat=") +
                  String(lat, 4) + "&lon=" + String(lon, 4) + "&appid=" + OWM_API_KEY;

  http.begin(aqiUrl);
  code = http.GET();
  if (code == HTTP_CODE_OK) {
    deserializeJson(doc, http.getString());
    weather.aqi   = doc["list"][0]["main"]["aqi"];
    weather.pm25  = doc["list"][0]["components"]["pm2_5"];
    float pm10    = doc["list"][0]["components"]["pm10"];
    float co      = doc["list"][0]["components"]["co"];
    float no2     = doc["list"][0]["components"]["no2"];
    float o3      = doc["list"][0]["components"]["o3"];
    weather.valid = true;

    const char* aqiLabel[] = {"", "ดี", "พอใช้", "ปานกลาง", "แย่", "แย่มาก"};
    Serial.println("------------- คุณภาพอากาศ (AQI) ---------------");
    Serial.printf("  AQI         : %d (%s)\n", weather.aqi,
                  (weather.aqi >= 1 && weather.aqi <= 5) ? aqiLabel[weather.aqi] : "?");
    Serial.printf("  PM2.5       : %.1f µg/m³\n", weather.pm25);
    Serial.printf("  PM10        : %.1f µg/m³\n", pm10);
    Serial.printf("  CO          : %.1f µg/m³\n", co);
    Serial.printf("  NO2         : %.1f µg/m³\n", no2);
    Serial.printf("  O3          : %.1f µg/m³\n", o3);
    Serial.println("================================================");

    // ── ส่งรายงานอากาศทุก 2 นาที ──
    const char* aqiLabelTH[] = {"", "ดี", "พอใช้", "ปานกลาง", "แย่", "แย่มาก"};
    String weatherMsg =
      String("🌤 <b>สภาพอากาศ ") + OWM_CITY + "</b>\n" +
      "🌡 อุณหภูมิ : <b>" + String(weather.temp, 1) + " °C</b>\n" +
      "💧 ความชื้น : <b>" + weather.humidity + " %</b>\n" +
      "🌬 AQI      : <b>" + weather.aqi +
      (weather.aqi >= 1 && weather.aqi <= 5 ? String(" (") + aqiLabelTH[weather.aqi] + ")" : "") + "</b>\n" +
      "🏭 PM2.5   : <b>" + String(weather.pm25, 1) + " µg/m³</b>\n" +
      "🏭 PM10    : <b>" + String(pm10, 1) + " µg/m³</b>";
    sendTelegram(weatherMsg);

    // ── แจ้งเตือนเมื่อ AQI หรือ PM2.5 เกินค่ากำหนด ──
    bool aqiBad  = weather.aqi  >= AQI_ALERT_THRESHOLD;
    bool pm25Bad = weather.pm25 >= PM25_ALERT_THRESHOLD;

    if ((aqiBad || pm25Bad) && !lastAqiAlertSent) {
      String alertMsg = "⚠️ <b>แจ้งเตือนคุณภาพอากาศ!</b>\n";
      if (aqiBad)  alertMsg += "🔴 AQI: <b>"   + String(weather.aqi)        + "</b> (เกินระดับ " + AQI_ALERT_THRESHOLD + ")\n";
      if (pm25Bad) alertMsg += "🔴 PM2.5: <b>" + String(weather.pm25, 1)    + " µg/m³</b> (เกิน " + String(PM25_ALERT_THRESHOLD, 0) + ")\n";
      alertMsg += "📍 " + String(OWM_CITY);
      sendTelegram(alertMsg);
      lastAqiAlertSent = true;
    }

    // รีเซ็ต flag เมื่อคุณภาพอากาศกลับสู่ปกติ
    if (!aqiBad && !pm25Bad) {
      if (lastAqiAlertSent) {
        sendTelegram("✅ <b>คุณภาพอากาศกลับสู่ปกติ</b>\nAQI: " + String(weather.aqi) +
                     "  PM2.5: " + String(weather.pm25, 1) + " µg/m³");
      }
      lastAqiAlertSent = false;
    }

  } else {
    Serial.printf("[AQI] HTTP error: %d\n", code);
  }
  http.end();

  drawOLED();
}

// ─── WiFi Reset Check (กด SW1 ค้าง 5 วินาทีใน setup) ──────
void checkWiFiResetButton() {
  pinMode(SW1, INPUT);

  if (digitalRead(SW1) == HIGH) return;

  Serial.println("[WiFi] SW1 ถูกกด — รอ 5 วินาทีเพื่อ reset WiFi...");
  unsigned long pressStart = millis();

  while (digitalRead(SW1) == LOW) {
    unsigned long held = millis() - pressStart;
    int remaining = 5 - (held / 1000);

    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(10, 8);
    display.print("Hold SW1 to reset");
    display.setCursor(10, 22);
    display.print("WiFi settings...");

    int progress = map(held, 0, WIFI_RESET_HOLD_MS, 0, SCREEN_W - 4);
    display.drawRect(2, 38, SCREEN_W - 4, 10, SSD1306_WHITE);
    display.fillRect(2, 38, progress, 10, SSD1306_WHITE);

    display.setTextSize(2);
    display.setCursor(52, 50);
    display.print(max(remaining, 0));
    display.display();

    if (held >= WIFI_RESET_HOLD_MS) {
      Serial.println("[WiFi] Reset WiFi credentials!");
      oledMsg("WiFi Reset!", "Restarting...", "Connect to AP:");
      delay(1500);

      WiFiManager wm;
      wm.resetSettings();
      delay(500);
      ESP.restart();
    }
  }

  Serial.println("[WiFi] ปล่อยปุ่มก่อนครบ — ยกเลิก reset");
  display.clearDisplay();
  display.display();
}

// ─── Setup ────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);

  Wire.begin(OLED_SDA, OLED_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println("[OLED] ไม่พบจอ — ตรวจสอบการต่อสาย");
  }

  checkWiFiResetButton();

  oledMsg("Connecting WiFi...", "If fail, connect to:", "ESP32-Setup");

  WiFiManager wm;
  wm.setConnectTimeout(20);
  wm.setConfigPortalTimeout(120);

  wm.setAPCallback([](WiFiManager* wm) {
    Serial.println("[WiFi] เปิด Config Portal — SSID: ESP32-Setup");
    oledMsg("WiFi Config Mode", "Connect to WiFi:", ">> ESP32-Setup <<");
  });

  if (!wm.autoConnect("ESP32-Setup")) {
    Serial.println("[WiFi] เชื่อมต่อไม่สำเร็จ — restart");
    oledMsg("WiFi Failed!", "Restarting...");
    delay(2000);
    ESP.restart();
  }

  String ip = WiFi.localIP().toString();
  Serial.printf("[WiFi] Connected — IP: %s\n", ip.c_str());
  oledMsg("WiFi Connected!", ip.c_str());

  // แจ้งเตือน Telegram ว่าบอร์ด online
  sendTelegram(
    String("✅ <b>ESP32 Online</b>\n") +
    "📡 IP: <b>" + ip + "</b>\n" +
    "📍 " + OWM_CITY + "\n" +
    "🔌 Relay: R1=OFF  R2=OFF  R3=OFF"
  );

  delay(1000);

  for (int i = 0; i < 3; i++) {
    pinMode(relay[i].pin, OUTPUT);
    digitalWrite(relay[i].pin, RELAY_OFF);
  }

  for (int i = 0; i < 3; i++) {
    pinMode(sw[i].pin, INPUT);
  }

  drawOLED();
  fetchWeather();
  lastWeatherFetch = millis();
}

// ─── Loop ─────────────────────────────────────────────────
void loop() {
  unsigned long now = millis();

  bool relayChanged = false;

  // SW2, SW3 — toggle Relay2, Relay3
  for (int i = 1; i < 3; i++) {
    bool raw = digitalRead(sw[i].pin);

    if (raw != sw[i].lastRaw) {
      sw[i].lastRaw = raw;
      sw[i].lastChangeTime = now;
    }

    if ((now - sw[i].lastChangeTime) >= DEBOUNCE_MS && raw != sw[i].stable) {
      sw[i].stable = raw;
      if (sw[i].stable == LOW) {
        toggleRelay(relay[i], i);
        relayChanged = true;
        Serial.printf("SW%d กด -> Relay%d %s\n", i + 1, i + 1, relay[i].on ? "ON" : "OFF");
      }
    }
  }

  // SW1 — toggle Relay1 (กดสั้น) / ฟังก์ชัน WiFi reset จัดการใน setup แล้ว
  {
    bool raw = digitalRead(sw[0].pin);
    if (raw != sw[0].lastRaw) {
      sw[0].lastRaw = raw;
      sw[0].lastChangeTime = now;
    }
    if ((now - sw[0].lastChangeTime) >= DEBOUNCE_MS && raw != sw[0].stable) {
      sw[0].stable = raw;
      if (sw[0].stable == HIGH) {
        unsigned long holdTime = now - sw[0].lastChangeTime;
        if (holdTime < WIFI_RESET_HOLD_MS) {
          toggleRelay(relay[0], 0);
          relayChanged = true;
          Serial.printf("SW1 กด -> Relay1 %s\n", relay[0].on ? "ON" : "OFF");
        }
      }
    }
  }

  if (relayChanged) drawOLED();

  // Weather fetch ทุก 2 นาที
  if (now - lastWeatherFetch >= WEATHER_INTERVAL_MS) {
    lastWeatherFetch = now;
    fetchWeather();
  }
}
