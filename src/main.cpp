#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --- WiFi ---
#define WIFI_SSID     "MyHome_2.4G"
#define WIFI_PASSWORD "0939391546"

// --- OpenWeatherMap API ---
#define OWM_API_KEY          "1bef650d2c6ea7a91f58252948c2d325"
#define OWM_CITY             "Nakhon Si Thammarat"
#define OWM_COUNTRY          "TH"
#define WEATHER_INTERVAL_MS  (2UL * 60UL * 1000UL)

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

#define DEBOUNCE_MS 20

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

Adafruit_SSD1306 display(SCREEN_W, SCREEN_H, &Wire, -1);

// ─── OLED Draw ────────────────────────────────────────────
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
    // Row 1: Temp & Humidity
    display.setCursor(0, 12);
    display.print("T:");
    display.print(weather.temp, 1);
    display.print("\xF8""C");   // degree symbol

    display.setCursor(68, 12);
    display.print("H:");
    display.print(weather.humidity);
    display.print("%");

    // Row 2: AQI & PM2.5
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

  // ── Relay section label ──
  display.setCursor(0, 37);
  display.print("RELAY:");

  // ── Relay status boxes ──
  // แต่ละกล่อง: R1 x=0, R2 x=43, R3 x=86  ที่ y=48
  const char* rLabel[] = {"R1", "R2", "R3"};
  for (int i = 0; i < 3; i++) {
    int bx = i * 43;
    if (relay[i].on) {
      // กล่องขาว = ON
      display.fillRoundRect(bx, 47, 40, 14, 3, SSD1306_WHITE);
      display.setTextColor(SSD1306_BLACK);
      display.setCursor(bx + 3, 50);
      display.print(rLabel[i]);
      display.print(":ON");
      display.setTextColor(SSD1306_WHITE);
    } else {
      // กล่องเส้นขอบ = OFF
      display.drawRoundRect(bx, 47, 40, 14, 3, SSD1306_WHITE);
      display.setCursor(bx + 2, 50);
      display.print(rLabel[i]);
      display.print(":--");
    }
  }

  display.display();
}

// ─── Relay Toggle ─────────────────────────────────────────
void toggleRelay(RelayState &r) {
  r.on = !r.on;
  digitalWrite(r.pin, r.on ? RELAY_ON : RELAY_OFF);
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
    Serial.printf("  NO₂         : %.1f µg/m³\n", no2);
    Serial.printf("  O₃          : %.1f µg/m³\n", o3);
    Serial.println("================================================");
  } else {
    Serial.printf("[AQI] HTTP error: %d\n", code);
  }
  http.end();

  drawOLED();
}

// ─── Setup ────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);

  Wire.begin(OLED_SDA, OLED_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println("[OLED] ไม่พบจอ — ตรวจสอบการต่อสาย");
  } else {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(20, 20);
    display.print("Connecting WiFi...");
    display.display();
  }

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

  for (int i = 0; i < 3; i++) {
    pinMode(sw[i].pin, INPUT);
  }

  drawOLED();           // แสดง "Fetching weather..." บน OLED
  fetchWeather();       // ดึงข้อมูลทันที
  lastWeatherFetch = millis();
}

// ─── Loop ─────────────────────────────────────────────────
void loop() {
  unsigned long now = millis();

  // --- Switch debounce & relay toggle ---
  bool relayChanged = false;
  for (int i = 0; i < 3; i++) {
    bool raw = digitalRead(sw[i].pin);

    if (raw != sw[i].lastRaw) {
      sw[i].lastRaw = raw;
      sw[i].lastChangeTime = now;
    }

    if ((now - sw[i].lastChangeTime) >= DEBOUNCE_MS && raw != sw[i].stable) {
      sw[i].stable = raw;
      if (sw[i].stable == LOW) {
        toggleRelay(relay[i]);
        relayChanged = true;
        Serial.printf("SW%d กด -> Relay%d %s\n", i + 1, i + 1, relay[i].on ? "ON" : "OFF");
      }
    }
  }

  if (relayChanged) drawOLED();  // อัปเดต OLED ทันทีเมื่อ relay เปลี่ยน

  // --- Weather fetch ทุก 2 นาที ---
  if (now - lastWeatherFetch >= WEATHER_INTERVAL_MS) {
    lastWeatherFetch = now;
    fetchWeather();
  }
}
