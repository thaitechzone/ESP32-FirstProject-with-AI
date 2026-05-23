# ESP32 DevKit V2 Board - รายละเอียด

## ข้อมูลทั่วไป
บอร์ด ESP32 DevKit V2 เป็นแพลตฟอร์มพัฒนาสำหรับ ESP32 ซึ่งเป็นไมโครคอนโทรลเลอร์แบบ SoC ที่ปรับปรุงเพื่อการเชื่อมต่ออินเทอร์เน็ต มีหน่วยความจำขนาดใหญ่ และความสามารถในการประมวลผลที่ทรงพลัง

## สเปซิฟิเคชัน (Specifications)

### CPU และหน่วยความจำ
- **Processor**: Xtensa 32-bit LX6 dual-core CPU
  - Core 0: up to 160 MHz
  - Core 1: up to 160 MHz
- **SRAM**: 520 KB
- **Flash**: 4 MB (สามารถขยายได้)
- **ROM**: 448 KB

### การเชื่อมต่อ (Connectivity)
- **Wi-Fi**: 802.11 b/g/n (2.4 GHz)
  - AP mode, STA mode
  - Power saving mode
- **Bluetooth**: v4.2 BR/EDR and BLE
- **USB-UART**: CH340 (ใช้สำหรับ Serial Communication)

### พอร์ตและอินเทอร์เฟส
- **GPIO Pins**: 34 pins
  - Digital I/O: 28 pins
  - Analog Input (ADC): 10 pins
  - Analog Output (DAC): 2 pins
- **SPI**: 3 (one dedicated for flash)
- **I2C**: 2
- **UART**: 3
- **PWM**: 16 channels
- **CAN**: 1

### พิน Input/Output
- **Voltage**: 3.3V logic levels
- **Current per pin**: up to 12 mA

### โมดูลเสริม
- **RTC** (Real Time Clock)
- **ADC** (Analog-to-Digital Converter)
- **DAC** (Digital-to-Analog Converter)
- **Capacitive touch sensing**
- **Temperature sensor**
- **Hall effect sensor**

## Pinout แผนภาพ

```
┌─────────────────────────────────────────┐
│        ESP32 DevKit V2 Board            │
├─────────────────────────────────────────┤
│ USB-C/Micro USB                         │
│                                         │
│  3V3  GND  TX  RX  D4  D5  D18 D19      │ (top header)
│  5V   GND  D23 D22 D1  D3  D21 GND      │
│  GND  D36 D39 D34 D35 D32 D33 D25      │
│  GND  D4  D2  D15 D4  D5  D18 D19      │
│  D27 D26 D25 D33 D32 D31 D30 D29      │ (bottom header)
│  EN  SVP D35 D34 D39 D36 GND  GND      │
└─────────────────────────────────────────┘
```

### เพิ่มเติมสำหรับรายละเอียด Pinout ที่สำคัญ

| Pin | ชื่อ | ประเภท | คำอธิบาย |
|-----|------|--------|----------|
| GND | Ground | Power | Voltage reference point |
| 3V3 | 3.3V | Power | 3.3V power supply |
| 5V | 5V | Power | 5V power supply |
| D0-D39 | GPIO | Digital I/O | General Purpose Input/Output |
| TX | Transmit | UART | Serial transmission |
| RX | Receive | UART | Serial reception |
| EN | Enable | Control | Reset/Enable pin |
| SVP | Sense VP | ADC | Analog input |

## วิธีการติดตั้ง/ใช้งาน

### 1. ติดตั้ง Driver
- ดาวน์โหลด CH340 driver จากหรือติดตั้ง driver UART ที่เหมาะสม

### 2. ตั้งค่า Arduino IDE / PlatformIO
```ini
[env:esp32]
platform = espressif32
board = esp32doit-devkit-v1
framework = arduino
monitor_speed = 115200
```

### 3. Upload โปรแกรม
- เชื่อมต่อบอร์ดผ่าน USB
- กดปุ่ม Flash/Boot สั้นๆ
- ใช้คำสั่ง Upload ใน IDE

## ส่วนประกอบหลัก

1. **ESP32 Chip** - Microcontroller หลัก
2. **CH340 USB-UART Bridge** - สำหรับการสื่อสาร Serial
3. **Crystal Oscillator** - 40 MHz clock
4. **Voltage Regulator** - LDO 3.3V
5. **Reset Button** - เพื่อ reset บอร์ด
6. **Boot Button** - สำหรับ Firmware upload
7. **LED Indicator** - ตัวบ่งชี้พลังงาน
8. **Header Pins** - สำหรับเชื่อมต่ออุปกรณ์ภายนอก

## การเชื่อมต่ออุปกรณ์ภายนอก

### LED (ไฟ LED)
```
LED Anode (+) ──[330Ω resistor]── GPIO Pin
LED Cathode (-) ── GND
```

### Push Button (ปุ่มกด)
```
Button One Side ── GPIO Pin
Button Other Side ── GND (or 3.3V with pull-up)
```

### Sensor (เซนเซอร์)
- I2C: SDA (GPIO 21), SCL (GPIO 22)
- SPI: MOSI (GPIO 23), MISO (GPIO 19), SCK (GPIO 18), CS (GPIO 5)

### Relay Module (โมดูลรีเลย์)
**หมายเหตุ**: ทั้งหมด 3 รีเลย์ใช้แบบ **Active Low** (ต้อง LOW เพื่อ ON/ปิด)

| Relay | GPIO Pin | ประเภท | คำอธิบาย |
|-------|----------|--------|----------|
| Relay 1 | GPIO 17 | Active Low | เปิดรีเลย์เมื่อ GPIO17 = LOW |
| Relay 2 | GPIO 16 | Active Low | เปิดรีเลย์เมื่อ GPIO16 = LOW |
| Relay 3 | GPIO 4 | Active Low | เปิดรีเลย์เมื่อ GPIO4 = LOW |

**วิธีการควบคุม**:
```cpp
// เปิด Relay (Active Low → ต้องส่ง LOW)
digitalWrite(17, LOW);   // Relay 1 ON
digitalWrite(16, LOW);   // Relay 2 ON
digitalWrite(4, LOW);    // Relay 3 ON

// ปิด Relay
digitalWrite(17, HIGH);  // Relay 1 OFF
digitalWrite(16, HIGH);  // Relay 2 OFF
digitalWrite(4, HIGH);   // Relay 3 OFF
```

**การเชื่อมต่อ**:
```
Relay Module JD-VCC ── 5V Power Supply
Relay Module GND ── GND
Relay Module IN1 ── GPIO 17 (with resistor 10kΩ)
Relay Module IN2 ── GPIO 16 (with resistor 10kΩ)
Relay Module IN3 ── GPIO 4 (with resistor 10kΩ)
```

**ตัวอย่าง Setup**:
```cpp
#include <Arduino.h>

const int RELAY1_PIN = 17;
const int RELAY2_PIN = 16;
const int RELAY3_PIN = 4;

void setup() {
  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  pinMode(RELAY3_PIN, OUTPUT);
  
  // เริ่มต้นปิด (HIGH = OFF สำหรับ Active Low)
  digitalWrite(RELAY1_PIN, HIGH);
  digitalWrite(RELAY2_PIN, HIGH);
  digitalWrite(RELAY3_PIN, HIGH);
}

void loop() {
  // เปิด Relay 1 สำหรับ 2 วินาที
  digitalWrite(RELAY1_PIN, LOW);
  delay(2000);
  digitalWrite(RELAY1_PIN, HIGH);
  delay(1000);
}
```

⚠️ **ข้อควรระวัง**:
- Relay ต้องใช้กระแสมากจึงไม่ควรต่อตรงกับ GPIO พอตอง (ต้องใช้ transistor หรือ relay driver)
- ต้องใช้ resistor pull-down สำหรับการป้องกัน
- Relay JD-VCC ต้องเชื่อมต่อกับ 5V เสมอ
- ใช้ Diode protection สำหรับ relay coil

### Switch Module (โมดูลสวิตช์)
**หมายเหตุ**: ทั้งหมด 3 สวิตช์ใช้แบบ **Active Low** (กดปุ่ม = LOW) และมี **External Pull-up**

| Switch | GPIO Pin | ประเภท | คำอธิบาย |
|--------|----------|--------|----------|
| SW1 | GPIO 34 | Active Low + Pull-up | ปุ่มกดเมื่อ GPIO34 = LOW |
| SW2 | GPIO 35 | Active Low + Pull-up | ปุ่มกดเมื่อ GPIO35 = LOW |
| SW3 | GPIO 32 | Active Low + Pull-up | ปุ่มกดเมื่อ GPIO32 = LOW |

**วิธีการอ่านค่า**:
```cpp
// อ่านสถานะปุ่ม
int sw1_state = digitalRead(34);  // LOW = pressed, HIGH = released
int sw2_state = digitalRead(35);  // LOW = pressed, HIGH = released
int sw3_state = digitalRead(32);  // LOW = pressed, HIGH = released

// ตรวจสอบการกดปุ่ม
if (digitalRead(34) == LOW) {
  // SW1 pressed
}
```

**การเชื่อมต่อ**:
```
SW1 Switch One Side ── GPIO 34
SW1 Switch Other Side ── 3.3V (External Pull-up 10kΩ)

SW2 Switch One Side ── GPIO 35
SW2 Switch Other Side ── 3.3V (External Pull-up 10kΩ)

SW3 Switch One Side ── GPIO 32
SW3 Switch Other Side ── 3.3V (External Pull-up 10kΩ)
```

**Wiring Diagram**:
```
3.3V ──[10kΩ]──┬── GPIO 34 (SW1)
               │
SW1 Button ────┘
               
3.3V ──[10kΩ]──┬── GPIO 35 (SW2)
               │
SW2 Button ────┘
               
3.3V ──[10kΩ]──┬── GPIO 32 (SW3)
               │
SW3 Button ────┘

GND ── (common for all switches when pressed)
```

**ตัวอย่าง Setup Code**:
```cpp
#include <Arduino.h>

const int SW1_PIN = 34;
const int SW2_PIN = 35;
const int SW3_PIN = 32;

void setup() {
  // ตั้งค่า pins เป็น INPUT (มี pull-up ภายนอก)
  pinMode(SW1_PIN, INPUT);
  pinMode(SW2_PIN, INPUT);
  pinMode(SW3_PIN, INPUT);
  
  Serial.begin(115200);
  Serial.println("Switch Control System Started");
}

void loop() {
  int sw1 = digitalRead(SW1_PIN);
  int sw2 = digitalRead(SW2_PIN);
  int sw3 = digitalRead(SW3_PIN);
  
  // ตรวจสอบ SW1
  if (sw1 == LOW) {
    Serial.println("SW1 Pressed!");
    delay(50);  // Debounce delay
    while (digitalRead(SW1_PIN) == LOW);
    delay(50);
  }
  
  // ตรวจสอบ SW2
  if (sw2 == LOW) {
    Serial.println("SW2 Pressed!");
    delay(50);
    while (digitalRead(SW2_PIN) == LOW);
    delay(50);
  }
  
  // ตรวจสอบ SW3
  if (sw3 == LOW) {
    Serial.println("SW3 Pressed!");
    delay(50);
    while (digitalRead(SW3_PIN) == LOW);
    delay(50);
  }
  
  delay(10);
}
```

⚠️ **ข้อควรระวัง - Switch**:
- External Pull-up ต้องเชื่อมต่ออยู่แล้ว (ไม่ต้องเปิด Internal Pull-up)
- GPIO 34, 35, 32 เป็น Input-only pins (ไม่มี output capability)
- ต้องใช้ Debouncing เพื่อหลีกเลี่ยง Switch Bounce
- มีค่า Pull-up External 10kΩ

### OLED Display 0.96" (I2C)
**หมายเหตุ**: จอ OLED 0.96 นิ้ว ความละเอียด 128x64 pixels ใช้โปรโตคอล I2C (SSD1306 driver)

| Pin OLED | GPIO ESP32 | คำอธิบาย |
|----------|------------|----------|
| VCC      | 3.3V       | ไฟเลี้ยง 3.3V |
| GND      | GND        | กราวด์ |
| SDA      | GPIO 21    | I2C Data |
| SCL      | GPIO 22    | I2C Clock |

**I2C Address**: `0x3C` (ค่าเริ่มต้น) หรือ `0x3D` (ขึ้นกับ hardware)

**การเชื่อมต่อ**:
```
OLED VCC  ── 3.3V
OLED GND  ── GND
OLED SDA  ── GPIO 21 (SDA)
OLED SCL  ── GPIO 22 (SCL)
```

**Wiring Diagram**:
```
ESP32          OLED 0.96"
3.3V  ─────── VCC
GND   ─────── GND
GPIO21 ─────── SDA
GPIO22 ─────── SCL
```

**Library ที่ใช้** (เพิ่มใน platformio.ini):
```ini
lib_deps =
  adafruit/Adafruit SSD1306@^2.5.7
  adafruit/Adafruit GFX Library@^1.11.9
```

**ตัวอย่าง Setup Code**:
```cpp
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define OLED_ADDRESS  0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup() {
  Wire.begin(21, 22);  // SDA=21, SCL=22

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println("OLED not found!");
    while (true);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Hello ESP32!");
  display.display();
}
```

**Layout หน้าจอในโปรเจคนี้** (128×64 px):
```
┌──────────────────────────────┐  y=0
│▓▓ Nakhon Si Thammarat ▓▓▓▓▓▓│  Title bar (ขาว/ดำ)
│ 23/05/2026  14:35:22         │  y=11  วันที่+เวลา (NTP)
│ T:32.4°C         H:78%      │  y=22  อุณหภูมิ/ความชื้น
│ AQI:2(Good)      PM:12       │  y=33  AQI/PM2.5
│──────────────────────────────│  y=43  divider
│ ╔R1:ON╗  [R2:--]  [R3:--]   │  y=45  Relay status
└──────────────────────────────┘  y=63
```
- นาฬิกาอัปเดตทุก **1 วินาที**
- Relay กล่องขาวทึบ = ON / เส้นขอบ = OFF

⚠️ **ข้อควรระวัง - OLED**:
- ใช้ไฟ 3.3V เท่านั้น (ห้ามต่อ 5V โดยตรง)
- I2C ใช้ GPIO 21 (SDA) และ GPIO 22 (SCL) ร่วมกับอุปกรณ์ I2C ตัวอื่นได้
- ถ้ามีหลาย I2C device ต้องมี I2C address ต่างกัน
- ควรมี Pull-up resistor 4.7kΩ บน SDA และ SCL (บางโมดูลมีในตัวแล้ว)

---

## ข้อมูลการไฟ (Power)

### Power Consumption
- **Active Mode**: ~80-160 mA @ 160 MHz
- **Light Sleep**: ~10 mA
- **Deep Sleep**: ~150 µA

### สำหรับการหา Power Supply
- ใช้ USB adapter 5V/1A ขึ้นไป
- หรือ external regulated 3.3V ที่ให้กระแส 500mA ขึ้นไป

### NTP Server (เวลาจริง Asia/Bangkok)

ESP32 ซิงค์เวลาจาก NTP Server ผ่าน Internet หลัง WiFi connect ใช้ `time.h` built-in ไม่ต้องติดตั้ง library เพิ่ม

| ค่า | รายละเอียด |
|-----|-----------|
| NTP Server | `pool.ntp.org` |
| Timezone | Asia/Bangkok (UTC+7) |
| UTC Offset | `25200` วินาที |
| DST Offset | `0` (ไทยไม่มี DST) |
| อัปเดต OLED | ทุก 1 วินาที |

**การตั้งค่าใน Code**:
```cpp
#define NTP_SERVER  "pool.ntp.org"
#define TZ_OFFSET   25200   // 7 * 3600

// เรียกใน setup() หลัง WiFi connected
configTime(TZ_OFFSET, 0, NTP_SERVER);

// อ่านเวลา
struct tm t;
if (getLocalTime(&t)) {
  // t.tm_hour, t.tm_min, t.tm_sec, t.tm_mday, t.tm_mon+1, t.tm_year+1900
}
```

**รูปแบบที่แสดงบน OLED** (แถวที่ 2 ของหน้าจอ):
```
23/05/2026  14:35:22
```

⚠️ **ข้อควรระวัง - NTP**:
- ต้องเชื่อมต่อ Internet ได้จึงจะ sync ได้
- หลัง sync ครั้งแรก ESP32 เก็บเวลาใน RTC ภายในตัว
- ถ้า sync ไม่ได้ภายใน 10 วินาที OLED แสดง `Syncing time...`

---

### Telegram Bot Notification

ใช้ Telegram Bot API ผ่าน HTTPS เพื่อส่งการแจ้งเตือนจาก ESP32 โดยไม่ต้องติดตั้ง library เพิ่ม (ใช้ HTTPClient + ArduinoJson ที่มีอยู่แล้ว)

**เหตุการณ์ที่แจ้งเตือน**:

| เหตุการณ์ | ข้อความ |
|----------|--------|
| บอร์ด Online | `✅ ESP32 Online` + IP Address |
| Relay ON/OFF | `🔌 Relay1 เปิด (ON) ✅` |
| อัปเดตอากาศ | Temp, Hum, AQI, PM2.5, PM10 ทุก 2 นาที |
| AQI/PM2.5 เกิน | `⚠️ แจ้งเตือนคุณภาพอากาศ!` |
| อากาศกลับปกติ | `✅ คุณภาพอากาศกลับสู่ปกติ` |

**การตั้งค่าใน Code**:
```cpp
#define TG_BOT_TOKEN         "your_bot_token"  // จาก @BotFather
#define TG_CHAT_ID           "your_chat_id"    // Chat ID ของคุณ
#define AQI_ALERT_THRESHOLD  3                 // AQI >= ค่านี้ = แจ้งเตือน
#define PM25_ALERT_THRESHOLD 35.0f             // PM2.5 >= ค่านี้ = แจ้งเตือน
```

**วิธีสร้าง Bot**:
1. คุยกับ **@BotFather** → `/newbot` → รับ Token
2. ส่งข้อความให้ Bot แล้วเปิด `https://api.telegram.org/bot<TOKEN>/getUpdates` เพื่อหา Chat ID

**ฟังก์ชัน sendTelegram()**:
```cpp
void sendTelegram(const String& msg) {
  HTTPClient http;
  String url = "https://api.telegram.org/bot" + String(TG_BOT_TOKEN) + "/sendMessage";
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  // ส่ง JSON: chat_id, text, parse_mode: HTML
  http.POST(body);
  http.end();
}
```

⚠️ **ข้อควรระวัง - Telegram**:
- Bot Token และ Chat ID เป็นข้อมูลลับ — ไม่ควร commit ขึ้น git สาธารณะ
- Telegram API ใช้ HTTPS — ESP32 ต้องเชื่อมต่อ Internet ได้
- Rate limit: 30 msg/วินาที (โปรเจคนี้ส่งน้อยมาก ไม่มีปัญหา)

---

### MQTT (HiveMQ Free Broker)

ESP32 เชื่อมต่อ MQTT Broker สาธารณะ HiveMQ เพื่อส่ง Telemetry และรับคำสั่ง Control Relay จากระยะไกล ใช้ Library **PubSubClient**

**Broker ที่ใช้**:

| ค่า | รายละเอียด |
|-----|-----------|
| Broker Host | `broker.hivemq.com` |
| Port | `1883` (TCP, ไม่เข้ารหัส) |
| Client ID | `ESP32_<BOARD_ID>_<random hex>` (unique ทุก session) |
| Authentication | ไม่ต้องใช้ Username/Password (Public broker) |

**BOARD_ID** — ค่าคงที่ที่กำหนดใน `#define BOARD_ID "esp32_nst_01"` ใช้เป็น namespace ของ Topic เพื่อป้องกัน topic ชนกันเมื่อมีหลายบอร์ด

---

**Topic Schema**:

```
esp32/<BOARD_ID>/
├── telemetry/
│   ├── status       ← สถานะบอร์ด (online/offline/IP/เวลา)
│   ├── weather      ← ข้อมูลอากาศ (Temp/Hum/AQI/PM2.5/PM10)
│   └── relay        ← สถานะ Relay 1/2/3
└── control/
    └── relay/
        ├── 1        ← สั่ง ON/OFF Relay 1
        ├── 2        ← สั่ง ON/OFF Relay 2
        └── 3        ← สั่ง ON/OFF Relay 3
```

---

**Payload ตัวอย่าง (Telemetry)**:

`esp32/esp32_nst_01/telemetry/status` (retained):
```json
{ "status": "online", "ip": "192.168.1.42", "time": "14:35:22" }
```

`esp32/esp32_nst_01/telemetry/weather`:
```json
{ "temp": 32.4, "humidity": 78, "aqi": 2, "pm25": 12.5, "pm10": 20.1 }
```

`esp32/esp32_nst_01/telemetry/relay` (retained):
```json
{ "relay1": true, "relay2": false, "relay3": false }
```

**LWT (Last Will and Testament)** — เมื่อบอร์ด disconnect กะทันหัน Broker จะส่ง payload นี้แทน:
```json
{ "status": "offline" }
```
Topic LWT: `esp32/esp32_nst_01/telemetry/status`

---

**Control Payload** (ส่งไปที่ `esp32/esp32_nst_01/control/relay/1`):

| Payload | ผล |
|---------|-----|
| `ON` หรือ `1` หรือ `TRUE` | เปิด Relay |
| `OFF` หรือ `0` หรือ `FALSE` | ปิด Relay |

(Case-insensitive — รองรับตัวพิมพ์เล็ก/ใหญ่)

---

**OLED Indicator**: ในแถบชื่อบนสุดของหน้าจอมีจุด `•` แสดงสถานะ MQTT
```
▓▓ Nakhon Si Thammarat • ▓▓▓   ← มีจุด = MQTT connected
▓▓ Nakhon Si Thammarat   ▓▓▓   ← ไม่มีจุด = MQTT disconnected
```

---

**การตั้งค่าใน Code**:
```cpp
#define MQTT_BROKER   "broker.hivemq.com"
#define MQTT_PORT     1883
#define BOARD_ID      "esp32_nst_01"
#define TOPIC_BASE              "esp32/" BOARD_ID
#define TOPIC_STATUS            TOPIC_BASE "/telemetry/status"
#define TOPIC_WEATHER           TOPIC_BASE "/telemetry/weather"
#define TOPIC_RELAY             TOPIC_BASE "/telemetry/relay"
#define TOPIC_CTRL_RELAY_1      TOPIC_BASE "/control/relay/1"
#define TOPIC_CTRL_RELAY_2      TOPIC_BASE "/control/relay/2"
#define TOPIC_CTRL_RELAY_3      TOPIC_BASE "/control/relay/3"
#define MQTT_RECONNECT_MS  5000   // retry interval เมื่อ disconnect
```

**Auto-reconnect**: loop() ตรวจสอบ `mqtt.connected()` ทุก 5 วินาที ถ้าหลุดจะเชื่อมต่อใหม่อัตโนมัติ (non-blocking)

**ทดสอบด้วย MQTT Explorer**:
1. เปิด [MQTT Explorer](https://mqtt-explorer.com/)
2. Host: `broker.hivemq.com`, Port: `1883`
3. Connect → ดู Topic `esp32/esp32_nst_01/#`
4. Publish ไปที่ `esp32/esp32_nst_01/control/relay/1` payload `ON` → Relay 1 เปิด

⚠️ **ข้อควรระวัง - MQTT**:
- HiveMQ Free Broker เป็น **Public** — ใครก็ subscribe/publish ได้ → ไม่ควรส่งข้อมูลลับ
- Client ID ต้องไม่ซ้ำกัน (random hex ช่วยป้องกัน)
- Port 1883 ไม่เข้ารหัส ถ้าต้องการความปลอดภัยให้ใช้ TLS port 8883

---

## ซอฟต์แวร์และไลบรารี่

### Core Libraries (มีมาตามค่าเริ่มต้น)
- Arduino.h
- WiFi.h
- HTTPClient.h
- Wire.h (I2C)
- BLEDevice.h
- SPIFFS.h

### Libraries ที่ใช้ในโปรเจคนี้ (platformio.ini)

| Library | Version | หน้าที่ |
|---------|---------|--------|
| `bblanchon/ArduinoJson` | ^7.0.0 | Parse และสร้าง JSON |
| `adafruit/Adafruit SSD1306` | ^2.5.7 | OLED display driver |
| `adafruit/Adafruit GFX Library` | ^1.11.9 | Graphics primitives |
| `tzapu/WiFiManager` | ^2.0.17 | WiFi Captive Portal |
| `knolleary/PubSubClient` | ^2.8 | MQTT client |

```ini
[env:esp32doit-devkit-v1]
platform = espressif32
board = esp32doit-devkit-v1
framework = arduino
monitor_speed = 115200
lib_deps =
  bblanchon/ArduinoJson@^7.0.0
  adafruit/Adafruit SSD1306@^2.5.7
  adafruit/Adafruit GFX Library@^1.11.9
  tzapu/WiFiManager@^2.0.17
  knolleary/PubSubClient@^2.8
```

## โหมดการสนับสนุน

1. **Development**: เหมาะสำหรับการทดลองและพัฒนา
2. **Production**: สามารถใช้ในโครงการจริงด้วยการป้องกันและการจัดการพลังงาน
3. **IoT Projects**: ดีสำหรับ IoT devices ที่เชื่อมต่ออินเทอร์เน็ต

## ข้อจำกัดและข้อควรระวัง

⚠️ **ข้อควรระวัง**
- GPIO pins ทำงานที่ 3.3V (ต่อ 5V โดยตรงอาจทำให้เสียหายได้)
- มี limited GPIO pins ที่สามารถใช้ได้พร้อมกัน
- ADC มีความแม่นยำ 12-bit
- ไม่สามารถใช้ RF pins (GPIO 6-11) ได้จากชั้นบนของผู้ใช้

## ทรัพยากรและเอกสารอ้างอิง

- [Official ESP32 Documentation](https://docs.espressif.com/)
- [Arduino ESP32 Core](https://github.com/espressif/arduino-esp32)
- [PlatformIO ESP32 Support](https://platformio.org/boards/espressif32)

---
**หมายเหตุ**: เอกสารนี้เป็นข้อมูลอ้างอิงสำหรับ ESP32 DevKit V2 Board สำหรับโปรเจค IoT