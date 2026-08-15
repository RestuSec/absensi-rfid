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

- [`firmware/esp8266/absensi_esp8266.ino`](firmware/esp8266/absensi_esp8266.ino) — reads the RC522 and sends the UID over `Serial1`.
- [`firmware/m5stick/m5_display.ino`](firmware/m5stick/m5_display.ino) — listens on `Serial2` and renders the result on screen + buzzer.

### Libraries

- ESP8266: [MFRC522](https://github.com/miguelbalboa/rfid)
- M5StickC Plus2: M5StickCPlus2 / M5Unified / M5GFX

### Build (arduino-cli)

```sh
arduino-cli compile --fqbn esp8266:esp8266:nodemcuv2 firmware/esp8266
arduino-cli compile --fqbn esp32:esp32:m5stack_stickc_plus2 firmware/m5stick
```

## Behavior

- **Blue** standby: "TAP KARTU..."
- **Green** "HADIR" + long beep: card read OK (shows UID)
- **Red** "GAGAL" + double beep: read failed

## Notes

- The RC522 only reads **13.56 MHz (Mifare/ISO14443A)** cards. 125 kHz tags will not work.
- If the reader is intermittent, power the RC522 from a stable 3.3 V source.