# ESP32 Smart Relay & Weather Monitor

โปรเจค IoT บน ESP32 สำหรับควบคุม Relay 3 ตัวด้วยปุ่มกด แสดงสภาพอากาศและคุณภาพอากาศแบบ Real-time จาก OpenWeatherMap บนจอ OLED 0.96 นิ้ว และแจ้งเตือนผ่าน Telegram Bot

---

## สารบัญ

- [ฟีเจอร์หลัก](#ฟีเจอร์หลัก)
- [อุปกรณ์ที่ใช้](#อุปกรณ์ที่ใช้)
- [การเชื่อมต่อวงจร](#การเชื่อมต่อวงจร)
- [Library ที่ใช้](#library-ที่ใช้)
- [การติดตั้งและเปิดโปรแกรม](#การติดตั้งและเปิดโปรแกรม)
- [การตั้งค่า WiFi ครั้งแรก](#การตั้งค่า-wifi-ครั้งแรก)
- [การ Reset WiFi](#การ-reset-wifi)
- [การตั้งค่า Telegram Bot](#การตั้งค่า-telegram-bot)
- [Layout หน้าจอ OLED](#layout-หน้าจอ-oled)
- [การทำงานของปุ่ม Switch](#การทำงานของปุ่ม-switch)
- [การแจ้งเตือน Telegram](#การแจ้งเตือน-telegram)
- [การตั้งค่าในโปรแกรม](#การตั้งค่าในโปรแกรม)
- [Serial Monitor Output](#serial-monitor-output)
- [โครงสร้างไฟล์](#โครงสร้างไฟล์)
- [ข้อควรระวัง](#ข้อควรระวัง)

---

## ฟีเจอร์หลัก

| ฟีเจอร์ | รายละเอียด |
|--------|-----------|
| ควบคุม Relay | 3 ตัว Toggle ON/OFF ด้วยปุ่ม SW1–SW3 |
| WiFi Manager | ตั้งค่า WiFi ผ่าน Captive Portal — ไม่ต้อง hardcode |
| สภาพอากาศ | อุณหภูมิ, ความชื้น, ความเร็วลม จาก OpenWeatherMap |
| คุณภาพอากาศ | AQI, PM2.5, PM10, CO, NO₂, O₃ |
| อัปเดตอัตโนมัติ | ดึงข้อมูลใหม่ทุก **2 นาที** |
| จอ OLED | แสดงผล 128×64 pixels แบบ I2C (SSD1306) |
| Telegram แจ้งเตือน | Online, Relay ON/OFF, รายงานอากาศ, แจ้งเตือน AQI/PM2.5 |
| WiFi Reset | กด SW1 ค้าง **5 วินาที** ขณะ boot เพื่อล้าง WiFi |
| Debounce | ป้องกัน Switch Bounce ด้วย millis() 20ms |
| Serial Debug | แสดงข้อมูลทั้งหมดผ่าน Serial Monitor 115200 baud |

---

## อุปกรณ์ที่ใช้

| อุปกรณ์ | รุ่น / สเปค |
|--------|------------|
| Microcontroller | ESP32 DevKit V1 (DOIT) |
| จอแสดงผล | OLED 0.96" I2C SSD1306 128×64 px |
| Relay Module | 3-Channel Active Low Relay |
| ปุ่มกด | Tactile Switch × 3 (Active Low + External Pull-up 10kΩ) |
| Power Supply | USB 5V / 1A ขึ้นไป |

---

## การเชื่อมต่อวงจร

### Relay Module (Active Low)

| Relay | GPIO | การทำงาน |
|-------|------|---------|
| Relay 1 | GPIO **17** | LOW = ON, HIGH = OFF |
| Relay 2 | GPIO **16** | LOW = ON, HIGH = OFF |
| Relay 3 | GPIO **4**  | LOW = ON, HIGH = OFF |

```
ESP32 GPIO17 ──[10kΩ]── Relay IN1
ESP32 GPIO16 ──[10kΩ]── Relay IN2
ESP32 GPIO4  ──[10kΩ]── Relay IN3
Relay JD-VCC ── 5V
Relay GND    ── GND
```

### Switch (Active Low + External Pull-up 10kΩ)

| Switch | GPIO | หน้าที่ใน loop | หน้าที่ขณะ boot |
|--------|------|--------------|----------------|
| SW1 | GPIO **34** | Toggle Relay1 (กดสั้น) | กดค้าง 5 วิ = Reset WiFi |
| SW2 | GPIO **35** | Toggle Relay2 | — |
| SW3 | GPIO **32** | Toggle Relay3 | — |

```
3.3V ──[10kΩ]──┬── GPIO34 (SW1)    3.3V ──[10kΩ]──┬── GPIO35 (SW2)
               └── SW1 ── GND                      └── SW2 ── GND

3.3V ──[10kΩ]──┬── GPIO32 (SW3)
               └── SW3 ── GND
```

> GPIO 34 และ 35 เป็น **Input-Only** — ไม่มี Internal Pull-up ต้องใช้ External 10kΩ เสมอ

### OLED 0.96" I2C (SSD1306)

| OLED | GPIO ESP32 | หมายเหตุ |
|------|------------|---------|
| VCC  | 3.3V       | ห้ามต่อ 5V |
| GND  | GND        | |
| SDA  | GPIO **21** | I2C Data |
| SCL  | GPIO **22** | I2C Clock |

> I2C Address: `0x3C` (default)

---

## Library ที่ใช้

| Library | Version | หน้าที่ |
|---------|---------|--------|
| `Arduino.h` | built-in | Arduino core |
| `WiFi.h` | built-in (ESP32) | WiFi stack |
| `WiFiManager` (tzapu) | ^2.0.17 | Captive Portal ตั้งค่า WiFi |
| `HTTPClient.h` | built-in (ESP32) | HTTP GET/POST |
| `ArduinoJson` (bblanchon) | ^7.0.0 | Parse JSON / build JSON |
| `Wire.h` | built-in | I2C protocol |
| `Adafruit SSD1306` | ^2.5.7 | ควบคุม OLED display |
| `Adafruit GFX Library` | ^1.11.9 | Graphics primitives |

> PlatformIO ดาวน์โหลด library ทั้งหมดอัตโนมัติจาก `platformio.ini`

---

## การติดตั้งและเปิดโปรแกรม

### สิ่งที่ต้องติดตั้ง

- [Visual Studio Code](https://code.visualstudio.com)
- **PlatformIO IDE** Extension ใน VS Code
- **CH340 USB Driver** (สำหรับ ESP32 DevKit V1)

### ขั้นตอน

**1. ติดตั้ง PlatformIO IDE**

- เปิด VS Code → `Ctrl+Shift+X`
- ค้นหา `PlatformIO IDE` → Install
- Restart VS Code หลังติดตั้ง

**2. เปิด Project**

```
File → Open Folder → เลือกโฟลเดอร์ ESP32-FirstProject-with-AI
```

**3. แก้ไขค่าตั้งต้นใน `src/main.cpp`**

| บรรทัด | define | ค่าที่ต้องแก้ |
|--------|--------|-------------|
| 11 | `OWM_API_KEY` | API Key จาก openweathermap.org |
| 17 | `TG_BOT_TOKEN` | Token จาก @BotFather |
| 18 | `TG_CHAT_ID` | Chat ID ของคุณ |

**4. Build และ Upload**

- กดปุ่ม **Build** (✓) ที่ Status Bar เพื่อตรวจสอบ
- เชื่อมต่อ ESP32 ผ่าน USB
- กดปุ่ม **Upload** (→) ที่ Status Bar
- shortcut: `Ctrl+Alt+U`

> หาก Upload ไม่ได้ → กดปุ่ม **BOOT** บนบอร์ดค้างไว้ระหว่าง Upload

**5. เปิด Serial Monitor**

- กดไอคอนปลั๊กที่ Status Bar หรือ `Ctrl+Alt+S`
- Baud: **115200**

---

## การตั้งค่า WiFi ครั้งแรก

เมื่อบอร์ดยังไม่มีข้อมูล WiFi บันทึกไว้:

1. OLED แสดง `Connecting WiFi... / Connect to: ESP32-Setup`
2. ESP32 เปิด Access Point ชื่อ **"ESP32-Setup"**
3. เชื่อมต่อ WiFi **"ESP32-Setup"** ด้วยมือถือหรือคอมพิวเตอร์
4. Browser เปิด Captive Portal อัตโนมัติ (หรือเปิด `http://192.168.4.1`)
5. กด **Configure WiFi** → เลือก SSID → ใส่ Password → **Save**
6. ESP32 restart และเชื่อมต่อ WiFi อัตโนมัติ
7. Telegram ได้รับข้อความ `✅ ESP32 Online` พร้อม IP Address

> WiFi credentials บันทึกใน Flash — ไม่หายเมื่อปิดไฟ
> AP Portal หมดเวลาใน **120 วินาที** แล้ว ESP32 จะ restart ใหม่

---

## การ Reset WiFi

ใช้เมื่อต้องการเปลี่ยน WiFi หรือแก้ปัญหาการเชื่อมต่อ:

1. **กด SW1 ค้างไว้ขณะบอร์ดกำลัง boot** (ช่วง setup)
2. OLED แสดง progress bar นับถอยหลัง 5 วินาที:
   ```
   Hold SW1 to reset
   WiFi settings...
   [████████████░░]  2
   ```
3. ค้างครบ **5 วินาที** → ลบ credentials → restart → เปิด AP Portal ใหม่
4. ปล่อยก่อน 5 วินาที → ยกเลิก บูตตามปกติ

---

## การตั้งค่า Telegram Bot

### สร้าง Bot

1. เปิด Telegram → ค้นหา **@BotFather**
2. พิมพ์ `/newbot` → ตั้งชื่อ Bot → รับ **Bot Token**

### หา Chat ID

1. ส่งข้อความอะไรก็ได้ให้ Bot ของคุณ
2. เปิด URL ในเบราว์เซอร์:
   ```
   https://api.telegram.org/bot<TOKEN>/getUpdates
   ```
3. ดูค่า `"id"` ใน `"chat"` — นั่นคือ Chat ID ของคุณ

### ใส่ค่าใน code

```cpp
#define TG_BOT_TOKEN  "123456789:ABCdefGHIjklMNOpqrsTUVwxyz"
#define TG_CHAT_ID    "987654321"
```

---

## Layout หน้าจอ OLED

จอ 128×64 pixels แบ่ง 3 โซน:

```
┌──────────────────────────────┐  ← y=0
│▓▓ Nakhon Si Thammarat ▓▓▓▓▓▓│  Title bar (พื้นขาว, ตัวดำ)
│ T:32.4°C         H:78%      │  ← y=12  อุณหภูมิ / ความชื้น
│ AQI:2(Good)      PM:12.3    │  ← y=23  AQI / PM2.5
│──────────────────────────────│  ← y=34  เส้นแบ่ง
│ RELAY:                       │  ← y=37
│ ╔R1:ON╗  ╔R2:ON╗  [R3:--]   │  ← y=47  กล่อง Relay
└──────────────────────────────┘  ← y=63
```

**สัญลักษณ์ Relay บน OLED:**

| แสดงผล | ความหมาย |
|--------|---------|
| กล่องขาวทึบ `R1:ON` | Relay เปิดอยู่ |
| กล่องเส้นขอบ `R3:--` | Relay ปิดอยู่ |

**ลำดับ Startup:**
```
Connecting WiFi... → WiFi Connected! → Fetching weather... → หน้าหลัก
```

---

## การทำงานของปุ่ม Switch

| ปุ่ม | GPIO | กดสั้น (loop) | กดค้าง 5 วิ (boot เท่านั้น) |
|-----|------|-------------|--------------------------|
| SW1 | 34 | Toggle Relay1 ON↔OFF | Reset WiFi credentials |
| SW2 | 35 | Toggle Relay2 ON↔OFF | — |
| SW3 | 32 | Toggle Relay3 ON↔OFF | — |

- Debounce: **20ms** (millis-based, non-blocking)
- OLED อัปเดตทันทีเมื่อ Relay เปลี่ยน
- Telegram แจ้งเตือนทุกครั้งที่ Relay เปลี่ยนสถานะ

---

## การแจ้งเตือน Telegram

| เหตุการณ์ | ตัวอย่างข้อความ |
|----------|---------------|
| บอร์ด Online | `✅ ESP32 Online` + IP + ตำแหน่ง |
| Relay เปลี่ยน | `🔌 Relay1 เปิด (ON) ✅` หรือ `ปิด (OFF) ⛔` |
| อัปเดตอากาศ (ทุก 2 นาที) | Temp, Hum, AQI, PM2.5, PM10 |
| AQI/PM2.5 เกินค่ากำหนด | `⚠️ แจ้งเตือนคุณภาพอากาศ!` |
| คุณภาพอากาศกลับปกติ | `✅ คุณภาพอากาศกลับสู่ปกติ` |

**ตัวอย่างข้อความ Telegram:**

```
🌤 สภาพอากาศ Nakhon Si Thammarat
🌡 อุณหภูมิ : 32.4 °C
💧 ความชื้น : 78 %
🌬 AQI      : 2 (พอใช้)
🏭 PM2.5   : 12.3 µg/m³
🏭 PM10    : 18.7 µg/m³
```

```
⚠️ แจ้งเตือนคุณภาพอากาศ!
🔴 AQI: 4 (เกินระดับ 3)
🔴 PM2.5: 38.5 µg/m³ (เกิน 35)
📍 Nakhon Si Thammarat
```

**ค่า Threshold ที่แก้ไขได้:**

```cpp
#define AQI_ALERT_THRESHOLD   3      // AQI >= 3 (ปานกลาง) แจ้งเตือน
#define PM25_ALERT_THRESHOLD  35.0f  // PM2.5 >= 35 µg/m³ แจ้งเตือน
```

> แจ้งเตือนซ้ำครั้งเดียวต่อเหตุการณ์ — ไม่สแปม ถ้าอากาศยังแย่จะไม่ส่งซ้ำจนกว่าจะกลับปกติแล้วแย่ใหม่

---

## การตั้งค่าในโปรแกรม

ไฟล์ [src/main.cpp](src/main.cpp) — ค่าทั้งหมดที่ปรับได้:

```cpp
// OpenWeatherMap
#define OWM_API_KEY          "your_api_key"        // API Key
#define OWM_CITY             "Nakhon Si Thammarat"  // ชื่อเมือง (ภาษาอังกฤษ)
#define OWM_COUNTRY          "TH"                   // รหัสประเทศ ISO 3166
#define WEATHER_INTERVAL_MS  (2UL * 60UL * 1000UL) // รอบดึงข้อมูล (ms)

// Telegram
#define TG_BOT_TOKEN          "your_bot_token"     // Token จาก @BotFather
#define TG_CHAT_ID            "your_chat_id"       // Chat ID ของคุณ
#define AQI_ALERT_THRESHOLD   3                    // AQI ขั้นต่ำที่แจ้งเตือน
#define PM25_ALERT_THRESHOLD  35.0f                // PM2.5 (µg/m³) ที่แจ้งเตือน

// OLED
#define OLED_ADDRESS  0x3C   // I2C address (0x3C หรือ 0x3D)

// Switch / Relay
#define DEBOUNCE_MS          20    // debounce time (ms)
#define WIFI_RESET_HOLD_MS   5000  // กด SW1 ค้างนานแค่ไหนเพื่อ reset WiFi (ms)
```

---

## Serial Monitor Output

Baud rate: **115200**

```
[WiFi] Connected — IP: 192.168.1.105
[Telegram] ส่งสำเร็จ
========== สภาพอากาศ นครศรีธรรมราช ==========
  อุณหภูมิ    : 32.4 °C (รู้สึกเหมือน 38.1 °C)
  ความชื้น    : 78 %
  ความเร็วลม  : 2.1 m/s
  สภาพ        : มีเมฆมาก
------------- คุณภาพอากาศ (AQI) ---------------
  AQI         : 2 (พอใช้)
  PM2.5       : 12.3 µg/m³
  PM10        : 18.7 µg/m³
  CO          : 210.5 µg/m³
  NO2         : 5.2 µg/m³
  O3          : 68.4 µg/m³
================================================
SW1 กด -> Relay1 ON
[Telegram] ส่งสำเร็จ
SW2 กด -> Relay2 ON
[Telegram] ส่งสำเร็จ
```

**ตารางระดับ AQI (OpenWeatherMap):**

| ค่า | ระดับ (EN) | ความหมาย |
|-----|-----------|---------|
| 1 | Good | ดี |
| 2 | Fair | พอใช้ |
| 3 | Moderate | ปานกลาง |
| 4 | Poor | แย่ |
| 5 | Very Poor | แย่มาก |

---

## โครงสร้างไฟล์

```
ESP32-FirstProject-with-AI/
├── src/
│   └── main.cpp              # โปรแกรมหลัก (Relay, OLED, Weather, Telegram)
├── platformio.ini            # config board + library dependencies
├── ESP32DevkitBoard.md       # เอกสารอ้างอิง pinout และวงจร
├── README.md                 # ไฟล์นี้
└── .pio/                     # build cache (auto-generated, ไม่ต้อง commit)
    └── libdeps/              # library ที่ดาวน์โหลดอัตโนมัติ
```

---

## ข้อควรระวัง

- **OLED ใช้ 3.3V เท่านั้น** — ต่อ 5V โดยตรงจะเสียหาย
- **GPIO 34, 35 เป็น Input-Only** — ไม่มี Internal Pull-up ต้องต่อ External 10kΩ เสมอ
- **Relay Active Low** — LOW = เปิด, HIGH = ปิด
- **OpenWeatherMap Free Tier** — limit 60 calls/นาที การดึงทุก 2 นาทีใช้ 2 calls/รอบ ปลอดภัย
- **Telegram Rate Limit** — ส่งได้สูงสุด 30 ข้อความ/วินาที โปรแกรมนี้ส่งน้อยมาก ไม่มีปัญหา
- **WiFiManager AP Portal** — หมดเวลา 120 วินาที หากไม่ตั้งค่า ESP32 จะ restart
- **Bot Token และ Chat ID** — เป็นข้อมูลลับ ไม่ควร commit ขึ้น git สาธารณะ

---

## License

โปรเจคนี้พัฒนาเพื่อการศึกษาและใช้งาน IoT บน ESP32
