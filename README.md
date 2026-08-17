# Absensi RFID

Sistem absensi berbasis RFID untuk ESP8266 (Wemos D1 Mini) + MFRC522.

## Fitur

- Absen otomatis dengan kartu RFID
- Dashboard web (modern, responsive)
- Kelola data siswa (tambah/edit/hapus)
- Export data absen ke Excel (HTML table)
- Auto-increment nomor urut absen
- NTP time sync (jam real-time)
- WiFi Manager (konfigurasi WiFi tanpa reset)
- Anti double-tap (debounce 1.5 detik)
- JSON persisten di LittleFS (survive reboot)

## Hardware

- ESP8266 (Wemos D1 Mini)
- MFRC522 RFID Reader
- LED indicator (GPIO2)

## Wiring

| MFRC522 | ESP8266 |
|---------|---------|
| SDA     | D8      |
| SCK     | D5      |
| MOSI    | D7      |
| MISO    | D6      |
| RST     | D3      |
| GND     | GND     |
| 3.3V    | 3.3V    |

## Upload

1. Install Arduino IDE + ESP8266 core
2. Install library: `MFRC522`, `ArduinoJson`, `WiFiManager`
3. Buka `firmware/absensi_rfid.ino`
4. Pilih board: **LOLIN(WEMOS) D1 R2 & mini**
5. Upload

## Pakai

1. Nyalain ESP8266 → cari WiFi **"Absensi-RFID"**
2. Konfigurasi WiFi di browser
3. Buka IP yang muncul di Serial Monitor
4. Daftarkan siswa (UID + nama + kelas)
5. Tempel kartu RFID → absen otomatis
