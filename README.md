<p align="center">
  <img src="https://avatars.githubusercontent.com/u/177526496?v=4" width="120" style="border-radius:50%;box-shadow:0 4px 20px rgba(0,0,0,.15)">
</p>

<h1 align="center">Absensi RFID</h1>

<p align="center">
  <b>Sistem absensi otomatis berbasis RFID untuk ESP8266</b><br>
  <sub>Dashboard web modern · Export Excel · WiFi Manager · NTP Sync</sub>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/Platform-ESP8266-000?style=flat-square&logo=espressif" alt="ESP8266">
  <img src="https://img.shields.io/badge/Language-C++-00599C?style=flat-square&logo=cplusplus" alt="C++">
  <img src="https://img.shields.io/badge/IDE-Arduino-00979D?style=flat-square&logo=arduino" alt="Arduino">
  <img src="https://img.shields.io/badge/License-MIT-green?style=flat-square" alt="License">
  <img src="https://img.shields.io/badge/Version-2.0-blue?style=flat-square" alt="Version">
</p>

---

## Fitur

| Fitur | Deskripsi |
|-------|-----------|
| **RFID Absen** | Tempel kartu → data otomatis tercatat |
| **Dashboard Web** | UI modern, responsive, ringan (~950 bytes CSS) |
| **Kelola Siswa** | Tambah / edit / hapus data siswa |
| **Export Excel** | Download data absen sebagai HTML table (buka di Excel) |
| **No Urut** | Auto-increment nomor urut, persisten di JSON |
| **NTP Sync** | Jam real-time, timeout 10 detik (gak hang) |
| **WiFi Manager** | Konfigurasi WiFi tanpa reset ESP |
| **Anti Double-Tap** | Debounce 1.5 detik, kartu gak kebaca dua kali |
| **JSON Persistent** | Data selamat di LittleFS, survive reboot |
| **Corrupt-Proof** | JSON file dicek, kalau rusak di-skip (gak boot loop) |

## Hardware

```
┌─────────────────┐      ┌─────────────────┐
│   ESP8266       │      │   MFRC522       │
│  (D1 Mini)      │      │  RFID Reader    │
│                 │      │                 │
│  D8 ────────────┼──────┼── SDA           │
│  D5 ────────────┼──────┼── SCK           │
│  D7 ────────────┼──────┼── MOSI          │
│  D6 ────────────┼──────┼── MISO          │
│  D3 ────────────┼──────┼── RST           │
│  GND ───────────┼──────┼── GND           │
│  3.3V ──────────┼──────┼── 3.3V          │
│                 │      │                 │
│  GPIO2 ──[LED]──┼──┐   │                 │
│                 │  │   │                 │
└─────────────────┘  │   └─────────────────┘
                     │
                    GND
```

| MFRC522 Pin | ESP8266 Pin |
|-------------|-------------|
| SDA | D8 |
| SCK | D5 |
| MOSI | D7 |
| MISO | D6 |
| RST | D3 |
| GND | GND |
| 3.3V | 3.3V |
| LED | GPIO2 (onboard) |

## Instalasi

### 1. Install Arduino IDE

Download dari [arduino.cc](https://www.arduino.cc/en/software)

### 2. Install ESP8266 Core

**File → Preferences → Additional Board Manager URLs:**

```
https://arduino.esp8266.com/stable/package_esp8266com_index.json
```

**Tools → Board → Board Manager → cari "esp8266" → Install**

### 3. Install Library

**Sketch → Include Library → Manage Libraries → install:**

- `MFRC522` by GithubCommunity
- `ArduinoJson` by Benoit Blanchon
- `WiFiManager` by tzapu

### 4. Upload Firmware

1. Buka `firmware/absensi_rfid.ino`
2. **Tools → Board → LOLIN(WEMOS) D1 R2 & mini**
3. **Tools → Port → COM3** (atau COM yang terdeteksi)
4. Klik **Upload** (→ icon)

### 5. Konfigurasi WiFi

1. Nyalain ESP8266
2. Cari WiFi **`Absensi-RFID`** di HP/laptop
3. Halaman WiFi Manager otomatis muncul
4. Pilih WiFi → masukin password → **Save**
5. ESP restart → hubung ke WiFi yang dipilih

### 6. Buka Dashboard

Buka Serial Monitor (115200 baud) → lihat IP address.

Buka di browser: `http://<IP_ADDRESS>`

## Cara Pakai

```
1. Buka Dashboard    →  http://192.168.x.x
2. Tab "Siswa"       →  Daftarkan kartu (UID + Nama + Kelas)
3. Tempel Kartu      →  Absen otomatis tercatat
4. Tab "Absen"       →  Lihat semua data kehadiran
5. Export Excel      →  Download data sebagai file Excel
```

### Cek UID Kartu

Tempel kartu di reader → lihat di Serial Monitor:

```
[RFID] AB:CD:EF:12
```

UID itulah yang dimasukkan saat mendaftarkan siswa.

## Struktur Folder

```
absensi-rfid/
├── firmware/
│   └── absensi_rfid.ino      ← Firmware utama
├── .gitignore
└── README.md
```

## Teknologi

| Komponen | Tech |
|----------|------|
| Microcontroller | ESP8266 (Wemos D1 Mini) |
| RFID Reader | MFRC522 (13.56MHz) |
| Web Server | ESP8266WebServer |
| File System | LittleFS |
| JSON | ArduinoJson |
| Time Sync | NTP (pool.ntp.org) |
| WiFi Config | WiFiManager |

## Lisensi

MIT License

---

<p align="center">
  <b>RestuSec</b> · <a href="https://github.com/RestuSec">GitHub</a>
</p>
