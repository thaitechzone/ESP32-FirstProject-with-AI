#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <time.h>
#include <PubSubClient.h>

// ─── NTP ──────────────────────────────────────────────────
#define NTP_SERVER  "pool.ntp.org"
#define TZ_OFFSET   25200   // UTC+7 Asia/Bangkok

// ─── OpenWeatherMap ───────────────────────────────────────
#define OWM_API_KEY          "1bef650d2c6ea7a91f58252948c2d325"
#define OWM_CITY             "Nakhon Si Thammarat"
#define OWM_COUNTRY          "TH"
#define WEATHER_INTERVAL_MS  (2UL * 60UL * 1000UL)

// ─── Telegram Bot ─────────────────────────────────────────
#define TG_BOT_TOKEN          "8948698437:AAETguWxKsAg0ILJ8ANmZVGpHOsIs5-dLoY"
#define TG_CHAT_ID            "7745779456"
#define AQI_ALERT_THRESHOLD   3
#define PM25_ALERT_THRESHOLD  35.0f

// ─── MQTT (HiveMQ Free) ───────────────────────────────────
#define MQTT_BROKER    "broker.hivemq.com"
#define MQTT_PORT      1883
#define BOARD_ID       "esp32_nst_01"     // <-- เปลี่ยนให้ unique ต่อบอร์ด

// Topic schema: esp32/{BOARD_ID}/telemetry/<data>
//               esp32/{BOARD_ID}/control/relay/{1|2|3}
#define TOPIC_BASE              "esp32/" BOARD_ID
#define TOPIC_STATUS            TOPIC_BASE "/telemetry/status"
#define TOPIC_WEATHER           TOPIC_BASE "/telemetry/weather"
#define TOPIC_RELAY             TOPIC_BASE "/telemetry/relay"
#define TOPIC_CTRL_RELAY_1      TOPIC_BASE "/control/relay/1"
#define TOPIC_CTRL_RELAY_2      TOPIC_BASE "/control/relay/2"
#define TOPIC_CTRL_RELAY_3      TOPIC_BASE "/control/relay/3"

#define MQTT_RECONNECT_MS  5000   // retry ถ้าหลุด

// ─── OLED ─────────────────────────────────────────────────
#define OLED_SDA     21
#define OLED_SCL     22
#define OLED_ADDRESS 0x3C
#define SCREEN_W     128
#define SCREEN_H     64

// ─── Relay (Active Low) ───────────────────────────────────
#define RELAY1 17
#define RELAY2 16
#define RELAY3 4

#define RELAY_ON  LOW
#define RELAY_OFF HIGH

// ─── Switch (Active Low, External Pull-up) ────────────────
#define SW1 34
#define SW2 35
#define SW3 32

#define DEBOUNCE_MS         20
#define WIFI_RESET_HOLD_MS  5000

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
  float pm10;
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

WeatherData weather = {0, 0, 0, 0, 0, false};
unsigned long lastWeatherFetch = 0;
unsigned long lastClockDraw    = 0;
unsigned long lastMqttRetry    = 0;
bool lastAqiAlertSent = false;

Adafruit_SSD1306 display(SCREEN_W, SCREEN_H, &Wire, -1);
WiFiClient       wifiClient;
PubSubClient     mqtt(wifiClient);

// ─── Forward declarations ──────────────────────────────────
void drawOLED();
void publishRelayState();

// ─── MQTT Publish helpers ──────────────────────────────────
void mqttPublishWeather() {
  if (!mqtt.connected()) return;

  JsonDocument doc;
  doc["board_id"]  = BOARD_ID;
  doc["city"]      = OWM_CITY;
  doc["temp"]      = weather.temp;
  doc["humidity"]  = weather.humidity;
  doc["aqi"]       = weather.aqi;
  doc["pm25"]      = weather.pm25;
  doc["pm10"]      = weather.pm10;

  char buf[256];
  serializeJson(doc, buf);
  mqtt.publish(TOPIC_WEATHER, buf, true);  // retained
  Serial.println("[MQTT] Published weather");
}

void publishRelayState() {
  if (!mqtt.connected()) return;

  JsonDocument doc;
  doc["board_id"]  = BOARD_ID;
  doc["relay1"]    = relay[0].on;
  doc["relay2"]    = relay[1].on;
  doc["relay3"]    = relay[2].on;

  char buf[128];
  serializeJson(doc, buf);
  mqtt.publish(TOPIC_RELAY, buf, true);  // retained
  Serial.println("[MQTT] Published relay state");
}

void mqttPublishStatus(const char* status) {
  if (!mqtt.connected()) return;

  JsonDocument doc;
  doc["board_id"] = BOARD_ID;
  doc["status"]   = status;
  doc["ip"]       = WiFi.localIP().toString();

  struct tm t;
  if (getLocalTime(&t)) {
    char timeBuf[20];
    strftime(timeBuf, sizeof(timeBuf), "%d/%m/%Y %H:%M:%S", &t);
    doc["time"] = timeBuf;
  }

  char buf[200];
  serializeJson(doc, buf);
  mqtt.publish(TOPIC_STATUS, buf, true);
  Serial.printf("[MQTT] Status: %s\n", status);
}

// ─── MQTT Callback (รับคำสั่ง control) ───────────────────
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String topicStr(topic);
  String msg;
  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];
  msg.trim();
  msg.toUpperCase();

  Serial.printf("[MQTT] Received [%s]: %s\n", topic, msg.c_str());

  int relayIdx = -1;
  if      (topicStr == TOPIC_CTRL_RELAY_1) relayIdx = 0;
  else if (topicStr == TOPIC_CTRL_RELAY_2) relayIdx = 1;
  else if (topicStr == TOPIC_CTRL_RELAY_3) relayIdx = 2;

  if (relayIdx < 0) return;

  bool newState = false;
  if      (msg == "ON"  || msg == "1" || msg == "TRUE")  newState = true;
  else if (msg == "OFF" || msg == "0" || msg == "FALSE") newState = false;
  else return;  // payload ไม่รู้จัก

  if (relay[relayIdx].on != newState) {
    relay[relayIdx].on = newState;
    digitalWrite(relay[relayIdx].pin, newState ? RELAY_ON : RELAY_OFF);
    Serial.printf("[MQTT] Relay%d -> %s\n", relayIdx + 1, newState ? "ON" : "OFF");

    publishRelayState();
    drawOLED();

    // แจ้ง Telegram
    String tgMsg = String("📡 <b>[MQTT] Relay") + (relayIdx + 1) + "</b> ";
    tgMsg += newState ? "เปิด (ON) ✅" : "ปิด (OFF) ⛔";
    tgMsg += "\n<i>สั่งผ่าน MQTT</i>";
    // (sendTelegram เรียกหลัง declare)
  }
}

// ─── MQTT Connect ─────────────────────────────────────────
void mqttConnect() {
  if (WiFi.status() != WL_CONNECTED) return;

  // Client ID unique ต่อบอร์ด
  String clientId = String("ESP32_") + BOARD_ID + "_" + String(random(0xFFFF), HEX);

  Serial.printf("[MQTT] Connecting as %s ...\n", clientId.c_str());

  // LWT (Last Will Testament) — แจ้งเมื่อบอร์ดหลุด
  if (mqtt.connect(clientId.c_str(),
                   nullptr, nullptr,           // no auth on HiveMQ free
                   TOPIC_STATUS, 0, true,      // LWT topic, QoS, retain
                   "{\"status\":\"offline\"}")) {
    Serial.println("[MQTT] Connected");

    // Subscribe control topics
    mqtt.subscribe(TOPIC_CTRL_RELAY_1);
    mqtt.subscribe(TOPIC_CTRL_RELAY_2);
    mqtt.subscribe(TOPIC_CTRL_RELAY_3);

    mqttPublishStatus("online");
    publishRelayState();
  } else {
    Serial.printf("[MQTT] Failed rc=%d — retry in %ds\n",
                  mqtt.state(), MQTT_RECONNECT_MS / 1000);
  }
}

// ─── Telegram Send ────────────────────────────────────────
void sendTelegram(const String& msg) {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  String url = String("https://api.telegram.org/bot") + TG_BOT_TOKEN + "/sendMessage";

  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  JsonDocument doc;
  doc["chat_id"]    = TG_CHAT_ID;
  doc["text"]       = msg;
  doc["parse_mode"] = "HTML";

  String body;
  serializeJson(doc, body);

  int code = http.POST(body);
  Serial.printf("[Telegram] %s\n", code == HTTP_CODE_OK ? "ส่งสำเร็จ" : ("Error: " + String(code)).c_str());
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
  display.setTextSize(1);

  // ── Title bar (y=0) ──
  display.fillRect(0, 0, SCREEN_W, 9, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setCursor(4, 1);
  display.print(OWM_CITY);

  // MQTT dot indicator (ขวาบน — ● = connected, ○ = offline)
  display.fillCircle(SCREEN_W - 4, 4, 3, mqtt.connected() ? SSD1306_BLACK : SSD1306_WHITE);
  if (!mqtt.connected()) display.drawCircle(SCREEN_W - 4, 4, 3, SSD1306_BLACK);

  display.setTextColor(SSD1306_WHITE);

  // ── Row 1 (y=11): Date + Time ──
  struct tm t;
  if (getLocalTime(&t)) {
    char timeBuf[22];
    snprintf(timeBuf, sizeof(timeBuf), "%02d/%02d/%04d %02d:%02d:%02d",
             t.tm_mday, t.tm_mon + 1, t.tm_year + 1900,
             t.tm_hour, t.tm_min, t.tm_sec);
    display.setCursor(0, 11);
    display.print(timeBuf);
  } else {
    display.setCursor(0, 11);
    display.print("Syncing time...");
  }

  // ── Row 2 (y=22): Temp & Humidity ──
  if (weather.valid) {
    display.setCursor(0, 22);
    display.print("T:");
    display.print(weather.temp, 1);
    display.print("\xF8""C");

    display.setCursor(68, 22);
    display.print("H:");
    display.print(weather.humidity);
    display.print("%");

    // ── Row 3 (y=33): AQI & PM2.5 ──
    const char* aqiLabel[] = {"", "Good", "Fair", "Mod", "Poor", "VPoor"};
    display.setCursor(0, 33);
    display.print("AQI:");
    display.print(weather.aqi);
    if (weather.aqi >= 1 && weather.aqi <= 5) {
      display.print("(");
      display.print(aqiLabel[weather.aqi]);
      display.print(")");
    }
    display.setCursor(80, 33);
    display.print("PM:");
    display.print(weather.pm25, 0);
  } else {
    display.setCursor(0, 22);
    display.print("Fetching weather...");
  }

  // ── Divider (y=43) ──
  display.drawLine(0, 43, SCREEN_W - 1, 43, SSD1306_WHITE);

  // ── Row 4 (y=45): Relay boxes ──
  const char* rLabel[] = {"R1", "R2", "R3"};
  for (int i = 0; i < 3; i++) {
    int bx = i * 43;
    if (relay[i].on) {
      display.fillRoundRect(bx, 45, 40, 14, 3, SSD1306_WHITE);
      display.setTextColor(SSD1306_BLACK);
      display.setCursor(bx + 3, 48);
      display.print(rLabel[i]);
      display.print(":ON");
      display.setTextColor(SSD1306_WHITE);
    } else {
      display.drawRoundRect(bx, 45, 40, 14, 3, SSD1306_WHITE);
      display.setCursor(bx + 2, 48);
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

  publishRelayState();

  String tgMsg = String("🔌 <b>Relay") + (index + 1) + "</b> ";
  tgMsg += r.on ? "เปิด (ON) ✅" : "ปิด (OFF) ⛔";
  sendTelegram(tgMsg);
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
    weather.pm10  = doc["list"][0]["components"]["pm10"];
    float co      = doc["list"][0]["components"]["co"];
    float no2     = doc["list"][0]["components"]["no2"];
    float o3      = doc["list"][0]["components"]["o3"];
    weather.valid = true;

    const char* aqiLabel[] = {"", "ดี", "พอใช้", "ปานกลาง", "แย่", "แย่มาก"};
    Serial.println("------------- คุณภาพอากาศ (AQI) ---------------");
    Serial.printf("  AQI         : %d (%s)\n", weather.aqi,
                  (weather.aqi >= 1 && weather.aqi <= 5) ? aqiLabel[weather.aqi] : "?");
    Serial.printf("  PM2.5       : %.1f µg/m³\n", weather.pm25);
    Serial.printf("  PM10        : %.1f µg/m³\n", weather.pm10);
    Serial.printf("  CO          : %.1f µg/m³\n", co);
    Serial.printf("  NO2         : %.1f µg/m³\n", no2);
    Serial.printf("  O3          : %.1f µg/m³\n", o3);
    Serial.println("================================================");

    // ── Publish weather to MQTT ──
    mqttPublishWeather();

    // ── Telegram weather report ──
    const char* aqiLabelTH[] = {"", "ดี", "พอใช้", "ปานกลาง", "แย่", "แย่มาก"};
    String weatherMsg =
      String("🌤 <b>สภาพอากาศ ") + OWM_CITY + "</b>\n" +
      "🌡 อุณหภูมิ : <b>" + String(weather.temp, 1) + " °C</b>\n" +
      "💧 ความชื้น : <b>" + weather.humidity + " %</b>\n" +
      "🌬 AQI      : <b>" + weather.aqi +
      (weather.aqi >= 1 && weather.aqi <= 5 ? String(" (") + aqiLabelTH[weather.aqi] + ")" : "") + "</b>\n" +
      "🏭 PM2.5   : <b>" + String(weather.pm25, 1) + " µg/m³</b>\n" +
      "🏭 PM10    : <b>" + String(weather.pm10, 1) + " µg/m³</b>";
    sendTelegram(weatherMsg);

    // ── AQI/PM2.5 alert ──
    bool aqiBad  = weather.aqi  >= AQI_ALERT_THRESHOLD;
    bool pm25Bad = weather.pm25 >= PM25_ALERT_THRESHOLD;

    if ((aqiBad || pm25Bad) && !lastAqiAlertSent) {
      String alertMsg = "⚠️ <b>แจ้งเตือนคุณภาพอากาศ!</b>\n";
      if (aqiBad)  alertMsg += "🔴 AQI: <b>"   + String(weather.aqi)     + "</b> (เกินระดับ " + AQI_ALERT_THRESHOLD + ")\n";
      if (pm25Bad) alertMsg += "🔴 PM2.5: <b>" + String(weather.pm25, 1) + " µg/m³</b> (เกิน " + String(PM25_ALERT_THRESHOLD, 0) + ")\n";
      alertMsg += "📍 " + String(OWM_CITY);
      sendTelegram(alertMsg);
      lastAqiAlertSent = true;
    }

    if (!aqiBad && !pm25Bad) {
      if (lastAqiAlertSent)
        sendTelegram("✅ <b>คุณภาพอากาศกลับสู่ปกติ</b>\nAQI: " + String(weather.aqi) +
                     "  PM2.5: " + String(weather.pm25, 1) + " µg/m³");
      lastAqiAlertSent = false;
    }

  } else {
    Serial.printf("[AQI] HTTP error: %d\n", code);
  }
  http.end();

  drawOLED();
}

// ─── WiFi Reset Check ─────────────────────────────────────
void checkWiFiResetButton() {
  pinMode(SW1, INPUT);
  if (digitalRead(SW1) == HIGH) return;

  Serial.println("[WiFi] SW1 ถูกกด — รอ 5 วินาทีเพื่อ reset WiFi...");
  unsigned long pressStart = millis();

  while (digitalRead(SW1) == LOW) {
    unsigned long held = millis() - pressStart;

    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(10, 8);  display.print("Hold SW1 to reset");
    display.setCursor(10, 22); display.print("WiFi settings...");

    int progress = map(held, 0, WIFI_RESET_HOLD_MS, 0, SCREEN_W - 4);
    display.drawRect(2, 38, SCREEN_W - 4, 10, SSD1306_WHITE);
    display.fillRect(2, 38, progress, 10, SSD1306_WHITE);

    display.setTextSize(2);
    display.setCursor(52, 50);
    display.print(max((int)(5 - held / 1000), 0));
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

  Serial.println("[WiFi] ปล่อยปุ่มก่อนครบ — ยกเลิก");
  display.clearDisplay();
  display.display();
}

// ─── Setup ────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);

  Wire.begin(OLED_SDA, OLED_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS))
    Serial.println("[OLED] ไม่พบจอ");

  checkWiFiResetButton();

  oledMsg("Connecting WiFi...", "If fail, connect to:", "ESP32-Setup");

  WiFiManager wm;
  wm.setConnectTimeout(20);
  wm.setConfigPortalTimeout(120);
  wm.setAPCallback([](WiFiManager*) {
    oledMsg("WiFi Config Mode", "Connect to WiFi:", ">> ESP32-Setup <<");
  });

  if (!wm.autoConnect("ESP32-Setup")) {
    oledMsg("WiFi Failed!", "Restarting...");
    delay(2000);
    ESP.restart();
  }

  String ip = WiFi.localIP().toString();
  Serial.printf("[WiFi] Connected — IP: %s\n", ip.c_str());
  oledMsg("WiFi Connected!", ip.c_str());

  // ── NTP ──
  configTime(TZ_OFFSET, 0, NTP_SERVER);
  Serial.print("[NTP] Syncing");
  struct tm t;
  int retry = 0;
  while (!getLocalTime(&t) && retry++ < 20) { delay(500); Serial.print("."); }
  if (getLocalTime(&t)) {
    char buf[22]; strftime(buf, sizeof(buf), "%d/%m/%Y %H:%M:%S", &t);
    Serial.printf("\n[NTP] %s\n", buf);
  } else {
    Serial.println("\n[NTP] Sync failed");
  }

  // ── MQTT ──
  mqtt.setServer(MQTT_BROKER, MQTT_PORT);
  mqtt.setCallback(mqttCallback);
  mqtt.setKeepAlive(60);
  mqttConnect();

  // ── Telegram Online ──
  sendTelegram(
    String("✅ <b>ESP32 Online</b>\n") +
    "📡 IP: <b>" + ip + "</b>\n" +
    "📍 " + OWM_CITY + "\n" +
    "🔗 MQTT: <b>" + (mqtt.connected() ? "Connected" : "Offline") + "</b>\n" +
    "🔌 Relay: R1=OFF  R2=OFF  R3=OFF"
  );

  delay(500);

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

  // ── MQTT loop + auto-reconnect ──
  if (WiFi.status() == WL_CONNECTED) {
    if (!mqtt.connected() && now - lastMqttRetry >= MQTT_RECONNECT_MS) {
      lastMqttRetry = now;
      mqttConnect();
    }
    mqtt.loop();
  }

  bool relayChanged = false;

  // SW2, SW3 — toggle Relay2, Relay3
  for (int i = 1; i < 3; i++) {
    bool raw = digitalRead(sw[i].pin);
    if (raw != sw[i].lastRaw) { sw[i].lastRaw = raw; sw[i].lastChangeTime = now; }
    if ((now - sw[i].lastChangeTime) >= DEBOUNCE_MS && raw != sw[i].stable) {
      sw[i].stable = raw;
      if (sw[i].stable == LOW) {
        toggleRelay(relay[i], i);
        relayChanged = true;
        Serial.printf("SW%d กด -> Relay%d %s\n", i+1, i+1, relay[i].on ? "ON" : "OFF");
      }
    }
  }

  // SW1 — toggle Relay1 (กดสั้น)
  {
    bool raw = digitalRead(sw[0].pin);
    if (raw != sw[0].lastRaw) { sw[0].lastRaw = raw; sw[0].lastChangeTime = now; }
    if ((now - sw[0].lastChangeTime) >= DEBOUNCE_MS && raw != sw[0].stable) {
      sw[0].stable = raw;
      if (sw[0].stable == HIGH) {
        if ((now - sw[0].lastChangeTime) < WIFI_RESET_HOLD_MS) {
          toggleRelay(relay[0], 0);
          relayChanged = true;
          Serial.printf("SW1 กด -> Relay1 %s\n", relay[0].on ? "ON" : "OFF");
        }
      }
    }
  }

  if (relayChanged) { drawOLED(); lastClockDraw = now; }

  // Clock tick ทุก 1 วินาที
  if (now - lastClockDraw >= 1000) { lastClockDraw = now; drawOLED(); }

  // Weather ทุก 2 นาที
  if (now - lastWeatherFetch >= WEATHER_INTERVAL_MS) {
    lastWeatherFetch = now;
    fetchWeather();
  }
}
