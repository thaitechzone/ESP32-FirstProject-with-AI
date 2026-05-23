# ESP32 Smart Relay & Weather Monitor

โปรเจคควบคุม Relay 3 ตัวด้วยปุ่มกด พร้อมแสดงสภาพอากาศและคุณภาพอากาศแบบ Real-time จาก OpenWeatherMap บนจอ OLED 0.96 นิ้ว รองรับการตั้งค่า WiFi แบบไม่ต้อง hardcode ผ่าน WiFiManager

---

## สารบัญ

- [ฟีเจอร์หลัก](#ฟีเจอร์หลัก)
- [อุปกรณ์ที่ใช้](#อุปกรณ์ที่ใช้)
- [การเชื่อมต่อวงจร](#การเชื่อมต่อวงจร)
- [Library ที่ใช้](#library-ที่ใช้)
- [การติดตั้งและเปิดโปรแกรม](#การติดตั้งและเปิดโปรแกรม)
- [การตั้งค่า WiFi ครั้งแรก](#การตั้งค่า-wifi-ครั้งแรก)
- [การ Reset WiFi](#การ-reset-wifi)
- [Layout หน้าจอ OLED](#layout-หน้าจอ-oled)
- [การทำงานของปุ่ม Switch](#การทำงานของปุ่ม-switch)
- [การตั้งค่าในโปรแกรม](#การตั้งค่าในโปรแกรม)
- [Serial Monitor Output](#serial-monitor-output)
- [โครงสร้างไฟล์](#โครงสร้างไฟล์)

---

## ฟีเจอร์หลัก

| ฟีเจอร์ | รายละเอียด |
|--------|-----------|
| ควบคุม Relay | 3 ตัว (Toggle ON/OFF) ด้วยปุ่มกด SW1–SW3 |
| แสดงสภาพอากาศ | อุณหภูมิ, ความชื้น จาก OpenWeatherMap API |
| คุณภาพอากาศ | AQI, PM2.5, PM10, CO, NO₂, O₃ |
| อัปเดตอัตโนมัติ | ดึงข้อมูลใหม่ทุก **2 นาที** |
| จอแสดงผล | OLED 0.96" 128×64 แบบ I2C |
| WiFi Manager | ตั้งค่า WiFi ผ่าน Captive Portal ไม่ต้อง hardcode |
| Debounce | ป้องกัน Switch Bounce ด้วย millis() (20ms) |
| Serial Debug | แสดงข้อมูลทั้งหมดผ่าน Serial Monitor 115200 baud |

---

## อุปกรณ์ที่ใช้

| อุปกรณ์ | รุ่น / สเปค |
|--------|------------|
| Microcontroller | ESP32 DevKit V1 (DOIT) |
| จอแสดงผล | OLED 0.96" I2C SSD1306 128×64 |
| Relay Module | 3-Channel Relay (Active Low) |
| ปุ่มกด | Tactile Switch × 3 (Active Low + External Pull-up 10kΩ) |
| Power Supply | USB 5V / 1A ขึ้นไป |

---

## การเชื่อมต่อวงจร

### Relay Module (Active Low)

| Relay | GPIO | หมายเหตุ |
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

### Switch (Active Low + External Pull-up)

| Switch | GPIO | หน้าที่ |
|--------|------|--------|
| SW1 | GPIO **34** | Toggle Relay1 / Reset WiFi (กด 5 วินาที) |
| SW2 | GPIO **35** | Toggle Relay2 |
| SW3 | GPIO **32** | Toggle Relay3 |

```
3.3V ──[10kΩ]──┬── GPIO34 (SW1)
               └── ปุ่ม SW1 ── GND

3.3V ──[10kΩ]──┬── GPIO35 (SW2)
               └── ปุ่ม SW2 ── GND

3.3V ──[10kΩ]──┬── GPIO32 (SW3)
               └── ปุ่ม SW3 ── GND
```

> **หมายเหตุ**: GPIO 34, 35 เป็น Input-Only pin ไม่มี Internal Pull-up ต้องใช้ External Pull-up เสมอ

### OLED 0.96" I2C (SSD1306)

| OLED Pin | ESP32 GPIO | หมายเหตุ |
|----------|------------|---------|
| VCC | 3.3V | ห้ามต่อ 5V |
| GND | GND | |
| SDA | GPIO **21** | I2C Data |
| SCL | GPIO **22** | I2C Clock |

> I2C Address: `0x3C` (ค่าเริ่มต้น)

---

## Library ที่ใช้

| Library | Version | หน้าที่ |
|---------|---------|--------|
| `Arduino.h` | built-in | Arduino core framework |
| `WiFi.h` | built-in (ESP32) | WiFi connection |
| `HTTPClient.h` | built-in (ESP32) | HTTP GET request |
| `Wire.h` | built-in | I2C communication |
| `bblanchon/ArduinoJson` | ^7.0.0 | Parse JSON จาก OpenWeatherMap |
| `adafruit/Adafruit SSD1306` | ^2.5.7 | ควบคุมจอ OLED |
| `adafruit/Adafruit GFX Library` | ^1.11.9 | Graphics สำหรับ OLED |
| `tzapu/WiFiManager` | ^2.0.17 | Config Portal สำหรับตั้งค่า WiFi |

> PlatformIO จะดาวน์โหลด library เหล่านี้อัตโนมัติจาก `platformio.ini`

---

## การติดตั้งและเปิดโปรแกรม

### ความต้องการของระบบ

- **VS Code** (Visual Studio Code)
- **PlatformIO IDE Extension** (ติดตั้งใน VS Code)
- **Python 3.x** (PlatformIO ต้องการ)
- **CH340 Driver** (สำหรับ ESP32 DevKit V1)

### ขั้นตอนการติดตั้ง

**1. ติดตั้ง VS Code**

ดาวน์โหลดจาก [https://code.visualstudio.com](https://code.visualstudio.com) แล้วติดตั้ง

**2. ติดตั้ง PlatformIO IDE Extension**

- เปิด VS Code
- กด `Ctrl+Shift+X` เปิด Extensions
- ค้นหา `PlatformIO IDE`
- กด **Install** แล้วรอจนเสร็จ (อาจใช้เวลา 2–5 นาที)
- **Restart VS Code** หลังติดตั้งเสร็จ

**3. เปิด Project**

```
File → Open Folder → เลือกโฟลเดอร์ ESP32-FirstProject-with-AI
```

หรือใช้ shortcut: `Ctrl+K` แล้ว `Ctrl+O`

**4. ติดตั้ง Library อัตโนมัติ**

PlatformIO จะดาวน์โหลด library อัตโนมัติเมื่อ build ครั้งแรก
หรือกดปุ่ม **Build** (✓) ที่ Status Bar ด้านล่าง

**5. ตั้งค่า API Key**

เปิดไฟล์ [src/main.cpp](src/main.cpp) แก้ไขบรรทัดที่ 11:

```cpp
#define OWM_API_KEY  "ใส่ API Key ของคุณที่นี่"
```

> สมัคร API Key ฟรีได้ที่ [https://openweathermap.org/api](https://openweathermap.org/api)

**6. Upload โปรแกรม**

- เชื่อมต่อ ESP32 ผ่าน USB
- กดปุ่ม **Upload** (→) ที่ Status Bar ด้านล่าง
- หรือใช้ shortcut: `Ctrl+Alt+U`

> หาก upload ไม่ได้ ให้กดปุ่ม **BOOT** บนบอร์ดค้างไว้ขณะ upload

**7. เปิด Serial Monitor**

- กดปุ่มรูปปลั๊กไฟที่ Status Bar (Serial Monitor)
- หรือใช้ shortcut: `Ctrl+Alt+S`
- Baud rate: **115200**

---

## การตั้งค่า WiFi ครั้งแรก

เมื่อ upload โปรแกรมและเปิดบอร์ดครั้งแรก (ยังไม่มีข้อมูล WiFi บันทึกไว้):

**1.** OLED แสดง:
```
Connecting WiFi...
If fail, connect to:
ESP32-Setup
```

**2.** ESP32 เปิด Access Point ชื่อ **"ESP32-Setup"**

**3.** ใช้มือถือหรือคอมพิวเตอร์ เชื่อมต่อ WiFi ชื่อ **"ESP32-Setup"**

**4.** Browser จะเปิด Captive Portal อัตโนมัติ (หรือเปิด `http://192.168.4.1`)

**5.** กด **"Configure WiFi"** → เลือก SSID → ใส่ Password → กด **"Save"**

**6.** ESP32 จะ restart และเชื่อมต่อ WiFi อัตโนมัติ

> WiFi credentials ถูกบันทึกใน Flash ของ ESP32 ไม่หายเมื่อปิดไฟ

---

## การ Reset WiFi

ใช้เมื่อต้องการเปลี่ยน WiFi หรือแก้ปัญหาการเชื่อมต่อ:

**1.** **กด SW1 ค้างไว้** ขณะที่บอร์ดกำลัง boot (ช่วง setup)

**2.** OLED แสดง progress bar นับถอยหลัง 5 วินาที:
```
Hold SW1 to reset
WiFi settings...
[████████████░░░]  3
```

**3.** ค้างครบ **5 วินาที** → WiFi credentials ถูกลบ → บอร์ด restart → เปิด AP Portal ใหม่

**4.** ถ้า **ปล่อยปุ่มก่อน 5 วินาที** → ยกเลิก boot ปกติ

> AP Portal จะหมดเวลาใน **120 วินาที** หากไม่มีการตั้งค่า ESP32 จะ restart ใหม่

---

## Layout หน้าจอ OLED

จอ OLED ขนาด 128×64 pixels แบ่งเป็น 3 ส่วน:

```
┌──────────────────────────────┐
│▓▓ Nakhon Si Thammarat ▓▓▓▓▓▓│  ← Title bar (พื้นขาว ตัวดำ)
│ T:32.4°C         H:78%      │  ← อุณหภูมิ + ความชื้น
│ AQI:2(Good)      PM:12.3    │  ← AQI + PM2.5 (µg/m³)
│──────────────────────────────│  ← เส้นแบ่ง
│ RELAY:                       │
│ ╔════╗  ╔════╗  [      ]    │  ← R1:ON  R2:ON  R3:--
│ ║R1:ON║  ║R2:ON║  [ R3:-- ] │  ← กล่องขาว=ON, เส้นขอบ=OFF
└──────────────────────────────┘
```

**สัญลักษณ์ Relay:**
- **กล่องขาวทึบ** (R1:ON) = Relay เปิดอยู่
- **กล่องเส้นขอบ** (R3:--) = Relay ปิดอยู่

**ระหว่าง startup:**
```
Connecting WiFi...   →   WiFi Connected!   →   Fetching weather...   →   หน้าหลัก
```

---

## การทำงานของปุ่ม Switch

| ปุ่ม | GPIO | การกดสั้น | การกดค้าง 5 วินาที |
|-----|------|----------|------------------|
| SW1 | 34 | Toggle Relay1 (ON↔OFF) | Reset WiFi credentials |
| SW2 | 35 | Toggle Relay2 (ON↔OFF) | — |
| SW3 | 32 | Toggle Relay3 (ON↔OFF) | — |

- Debounce time: **20ms** (ป้องกัน Switch Bounce)
- OLED อัปเดตทันทีเมื่อ Relay เปลี่ยนสถานะ
- Serial Monitor แสดงสถานะทุกครั้งที่กด

---

## การตั้งค่าในโปรแกรม

เปิดไฟล์ [src/main.cpp](src/main.cpp) แก้ไขค่าต่อไปนี้ตามต้องการ:

```cpp
// --- OpenWeatherMap API ---
#define OWM_API_KEY          "your_api_key_here"   // API Key ของคุณ
#define OWM_CITY             "Nakhon Si Thammarat"  // ชื่อเมือง (ภาษาอังกฤษ)
#define OWM_COUNTRY          "TH"                   // รหัสประเทศ ISO 3166
#define WEATHER_INTERVAL_MS  (2UL * 60UL * 1000UL) // ความถี่ดึงข้อมูล (ms)

// --- OLED ---
#define OLED_ADDRESS  0x3C   // I2C Address (0x3C หรือ 0x3D)

// --- Debounce ---
#define DEBOUNCE_MS         20    // ระยะ debounce (ms)
#define WIFI_RESET_HOLD_MS  5000  // เวลากด SW1 เพื่อ reset WiFi (ms)
```

**ตัวอย่างเปลี่ยนเมือง:**
```cpp
#define OWM_CITY     "Bangkok"
#define OWM_COUNTRY  "TH"
```

---

## Serial Monitor Output

เปิด Serial Monitor ที่ **115200 baud** เพื่อดู debug output:

```
[WiFi] Connected — IP: 192.168.1.105
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
  NO₂         : 5.2 µg/m³
  O₃          : 68.4 µg/m³
================================================
SW1 กด -> Relay1 ON
SW2 กด -> Relay2 ON
SW1 กด -> Relay1 OFF
```

**AQI Index ของ OpenWeatherMap:**
| ค่า | ระดับ | ความหมาย |
|-----|-------|---------|
| 1 | Good | คุณภาพดี |
| 2 | Fair | พอใช้ได้ |
| 3 | Moderate | ปานกลาง |
| 4 | Poor | แย่ |
| 5 | Very Poor | แย่มาก |

---

## โครงสร้างไฟล์

```
ESP32-FirstProject-with-AI/
├── src/
│   └── main.cpp              # โปรแกรมหลักทั้งหมด
├── platformio.ini            # การตั้งค่า PlatformIO และ library
├── ESP32DevkitBoard.md       # เอกสารอ้างอิง pinout และวงจร
├── README.md                 # ไฟล์นี้
└── .pio/                     # โฟลเดอร์ build (auto-generated)
    └── libdeps/              # Library ที่ดาวน์โหลดอัตโนมัติ
```

---

## ข้อควรระวัง

- **ห้ามต่อ OLED กับ 5V** — ใช้ 3.3V เท่านั้น
- **GPIO 34, 35 เป็น Input-Only** — ไม่มี internal pull-up ต้องต่อ external 10kΩ เสมอ
- **Relay ใช้ Active Low** — ส่ง LOW เพื่อเปิด, HIGH เพื่อปิด
- **API Key ฟรี** มี limit 60 calls/นาที — การดึงทุก 2 นาที ใช้ 2 calls ต่อรอบ (Weather + AQI) ปลอดภัย
- **WiFiManager AP Portal** หมดเวลา 120 วินาที หากไม่ตั้งค่า บอร์ดจะ restart ใหม่

---

## License

โปรเจคนี้เป็น Open Source สำหรับการศึกษาและพัฒนา IoT บน ESP32
