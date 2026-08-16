#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <ESP8266HTTPClient.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ---------- RFID (RC522) ----------
#define SS_PIN   D8
#define RST_PIN  D3
MFRC522 rfid(SS_PIN, RST_PIN);

// ---------- OLED (SSD1306 I2C) ----------
#define OLED_ADDR 0x3C
Adafruit_SSD1306 display(128, 64, &Wire, -1);

// ---------- Tombol FLASH (GPIO0) buat reset WiFi ----------
#define FLASH_BTN 0

// ---------- Google Apps Script (ganti URL ini) ----------
const char* APPS_SCRIPT_URL =
  "https://script.google.com/macros/s/AKfycbz1DvCbmSLYbtUqEI7DoqfGE_zwGzkllWD7mOIykJ4XZvH0Z1slZadkwTQ2GBcGB3hl/exec";

// ---------- Map UID -> Nama (tambahin siswa di sini) ----------
struct Siswa { const char* uid; const char* nama; };
const Siswa DAFTAR[] = {
  {"19:79:F4:1F", "Budi"},
  {"F9:04:21:20", "Rehan"},
};
const int N_SISWA = sizeof(DAFTAR) / sizeof(DAFTAR[0]);

const char* namaUntukUID(const String& uid) {
  for (int i = 0; i < N_SISWA; i++) {
    if (uid == String(DAFTAR[i].uid)) return DAFTAR[i].nama;
  }
  return "";
}

// ---------- Tampilan OLED ----------
void oledStandby() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(8, 24);
  display.println("TAP");
  display.setCursor(8, 46);
  display.println("KARTU...");
  display.display();
}

void oledTampil(const char* baris1, const char* baris2, unsigned long ms) {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 8);
  display.println(baris1);
  display.setTextSize(1);
  display.setCursor(0, 40);
  display.println(baris2);
  display.display();
  delay(ms);
}

// ---------- Kirim ke Google Sheets ----------
bool kirimKeSheets(const String& uid, const String& nama) {
  String body = "{\"uid\":\"" + uid + "\",\"name\":\"" + nama + "\"}";
  WiFiClient client;
  HTTPClient http;
  http.setTimeout(8000);
  http.begin(client, APPS_SCRIPT_URL);
  http.addHeader("Content-Type", "application/json");
  int code = http.POST(body);
  bool ok = (code == 200 || code == 302 || code == 303);
  http.end();
  return ok;
}

void setup() {
  Serial.begin(115200);
  Serial.println("Mulai...");

  pinMode(FLASH_BTN, INPUT_PULLUP);

  // OLED
  Wire.begin(D2, D1);
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("OLED gagal init");
  }
  oledStandby();

  // WiFi via WiFiManager (hotspot "AbsensiRFID")
  WiFiManager wm;
  bool konek = wm.autoConnect("AbsensiRFID");
  if (!konek) {
    Serial.println("Gagal konek WiFi, reset.");
    ESP.restart();
  }
  Serial.print("WiFi OK, IP: ");
  Serial.println(WiFi.localIP());

  // RFID
  SPI.begin();
  rfid.PCD_Init();
  rfid.PCD_SetAntennaGain(MFRC522::RxGain_max);
  rfid.PCD_AntennaOn();
  oledStandby();
}

void loop() {
  // Reset WiFi: tahan tombol FLASH 3 detik
  if (digitalRead(FLASH_BTN) == LOW) {
    unsigned long t0 = millis();
    while (digitalRead(FLASH_BTN) == LOW && millis() - t0 < 3000) delay(10);
    if (millis() - t0 >= 3000) {
      WiFiManager wm;
      wm.resetSettings();
      ESP.restart();
    }
  }

  if (!rfid.PICC_IsNewCardPresent()) return;
  if (!rfid.PICC_ReadCardSerial()) return;

  String uid = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) uid += "0";
    uid += String(rfid.uid.uidByte[i], HEX);
    if (i < rfid.uid.size - 1) uid += ":";
  }
  uid.toUpperCase();

  String nama = namaUntukUID(uid);
  if (nama.length() > 0) {
    oledTampil("HADIR", ("UID: " + uid).c_str(), 1500);
  } else {
    oledTampil("KARTU", "UID BARU", 1000);
  }

  // Kirim ke Sheets
  oledTampil("KIRIM...", uid.c_str(), 800);
  bool ok = kirimKeSheets(uid, nama);
  oledTampil(ok ? "TERKIRIM" : "ERROR", uid.c_str(), 1200);

  Serial.println("UID: " + uid + " Nama: " + nama + " -> " + (ok ? "OK" : "GAGAL"));

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
  oledStandby();
  delay(300);
}