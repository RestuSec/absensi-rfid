# absensi-rfid

RFID attendance system built on **ESP8266 + M5StickC Plus2**.

Tap an RFID card → read its UID → show **HADIR (green)** or **GAGAL (red)** on the M5StickC display with a buzzer beep.

## Architecture

```
RFID card ──► RC522 ──► ESP8266 (NodeMCU) reads UID
                          │  UART (TX → G26)
                          ▼
                     M5StickC Plus2 (display HADIR/GAGAL + buzzer)
```

- **ESP8266 (NodeMCU)** is the brain: reads the RC522 and sends the UID over hardware UART.
- **M5StickC Plus2** is the display: shows green/red screen + buzzer beep.

## Wiring

### RC522 → ESP8266 (NodeMCU)

| RC522 pin | → | NodeMCU pin |
|-----------|---|-------------|
| SDA (SS)  | → | D8 |
| SCK       | → | D5 |
| MOSI      | → | D7 |
| MISO      | → | D6 |
| RST       | → | D3 |
| 3.3V      | → | 3V |
| GND       | → | G |

### ESP8266 → M5StickC Plus2

| ESP8266 | → | M5StickC |
|---------|---|----------|
| TX      | → | G26 (Serial2 RX) |
| RX      | → | G0  (Serial2 TX) |
| GND     | → | GND |

> Note: TX/RX must be crossed (TX→RX, RX→TX). A shared GND is required.

## Firmware

- [`firmware/absensi_esp8266/absensi_esp8266.ino`](firmware/absensi_esp8266/absensi_esp8266.ino) — reads the RC522, connects WiFi, pushes the UID to Google Sheets, and sends the result to the M5 over `Serial1`.
- [`firmware/m5_display/m5_display.ino`](firmware/m5_display/m5_display.ino) — listens on `Serial2` and renders the result on screen + buzzer.

### Libraries

- ESP8266: [MFRC522](https://github.com/miguelbalboa/rfid)
- M5StickC Plus2: M5StickCPlus2 / M5Unified / M5GFX

### Build (arduino-cli)

```sh
arduino-cli compile --fqbn esp8266:esp8266:nodemcuv2 firmware/absensi_esp8266
arduino-cli compile --fqbn esp32:esp32:m5stack_stickc_plus2 firmware/m5_display
```

## Behavior

- **Blue** standby: "TAP KARTU..."
- **Green** "HADIR" + long beep: card read OK (shows UID)
- **Red** "GAGAL" + double beep: read failed

## Standalone WiFi version (no laptop, no M5StickC)

**`firmware/absensi_esp8266_oled_wifi/absensi_esp8266_oled_wifi.ino`** runs the whole
thing on the ESP8266 alone: reads the RC522, shows the result on a **SSD1306
OLED**, and sends every tap to a **Google Sheet** over WiFi. No laptop, no
battery-hungry M5StickC — just the ESP8266 + a cheap OLED.

### Wiring

| OLED (SSD1306 I2C) | → | ESP8266 (NodeMCU) |
|---|---|---|
| VCC | → | 3V |
| GND | → | G |
| SCL | → | D1 |
| SDA | → | D2 |

RC522 wiring is unchanged (SDA→D8, SCK→D5, MOSI→D7, MISO→D6, RST→D3, 3.3V, GND).

### Setup Google Sheet (once)

1. Open `sheets.new`, make a spreadsheet.
2. **Extensions → Apps Script** → paste the code from `tools/gas_attendance.gs`.
3. **Deploy → New deployment → Web app**: Execute as *Me*, Who has access
   *Anyone* → Deploy. Copy the `/exec` URL and paste it into `APPS_SCRIPT_URL`
   in the firmware.
4. Register names by editing the `DAFTAR` array in the firmware
   (`{"19:79:F4:1F", "Budi"}`).

### WiFi (WiFiManager)

On first boot the ESP8266 creates its own hotspot **`AbsensiRFID`**. Connect
with your phone, enter your WiFi SSID/password — it is saved permanently.
To switch WiFi later, **hold the FLASH button for 3 seconds** to reset the
stored network and reconfigure.

### Build

```sh
arduino-cli compile --fqbn esp8266:esp8266:nodemcuv2 firmware/absensi_esp8266_oled_wifi
arduino-cli upload --fqbn esp8266:esp8266:nodemcuv2 --port COM3 firmware/absensi_esp8266_oled_wifi
```

Libraries: MFRC522, Adafruit SSD1306, Adafruit GFX, WiFiManager.

## Notes

- The RC522 only reads **13.56 MHz (Mifare/ISO14443A)** cards. 125 kHz tags will not work.
- If the reader is intermittent, power the RC522 from a stable 3.3 V source.

## Tools: PC logging dashboard

`tools/attendance_log.py` reads the UID stream over USB serial and runs a
small local dashboard in your browser. From there you can view attendance
live, edit or delete names, and export to Excel.

### Install

```sh
pip install pyserial openpyxl
```

### Run

```sh
cd tools
python attendance_log.py            # opens the dashboard in Chrome
python attendance_log.py --no-browser   # run without opening the browser
python attendance_log.py --list         # print registered names, then exit
```

The dashboard is served at `http://localhost:8080`:

- **Tap a card** → it appears in the table (auto-refreshes every 5 s).
- **✏️ pencil** next to a name → rename that card (saved to `roster.csv`).
- **🗑️ trash** next to a row → delete that row (clean up before export).
- **Export Excel** → downloads a styled `attendance.xlsx`.

### Registering cards

Tap an unknown card, note its `UID`, then click the **✏️** next to it and
type the name. It is saved to `roster.csv` and remembered on next launch.

### Options

- `DEDUP = False` (in `attendance_log.py`) logs every tap — good for testing.
  Set it to `True` for production so each person registers one check-in per day.