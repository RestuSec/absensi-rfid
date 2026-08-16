#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <ESP8266HTTPClient.h>
#include <SPI.h>
#include <MFRC522.h>

// NodeMCU 1.0 pin SPI: D8=SS, D5=SCK, D7=MOSI, D6=MISO
#define SS_PIN   D8
#define RST_PIN  D3
MFRC522 rfid(SS_PIN, RST_PIN);

#define FLASH_BTN 0  // tombol FLASH (GPIO0) buat reset WiFi

// Google Apps Script (ganti kalau perlu)
const char* APPS_SCRIPT_URL =
  "https://script.google.com/macros/s/AKfycbz1DvCbmSLYbtUqEI7DoqfGE_zwGzkllWD7mOIykJ4XZvH0Z1slZadkwTQ2GBcGB3hl/exec";

// UID -> Nama (tambahin siswa di sini)
struct Siswa { const char* uid; const char* nama; };
const Siswa DAFTAR[] = {
  {"19:79:F4:1F", "Budi"},
  {"F9:04:21:20", "Rehan"},
};
const int N_SISWA = sizeof(DAFTAR) / sizeof(DAFTAR[0]);

const char* namaUntukUID(const String& uid) {
  for (int i = 0; i < N_SISWA; i++)
    if (uid == String(DAFTAR[i].uid)) return DAFTAR[i].nama;
  return "";
}

// Kirim ke Google Sheets (write terjadi server-side walau dapat redirect)
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
  Serial.begin(115200);   // USB debug
  Serial1.begin(115200);  // kirim ke M5StickC (display)
  pinMode(FLASH_BTN, INPUT_PULLUP);

  // WiFi via WiFiManager (hotspot "AbsensiRFID")
  WiFiManager wm;
  if (!wm.autoConnect("AbsensiRFID")) {
    Serial.println("Gagal konek WiFi, restart.");
    ESP.restart();
  }
  Serial.print("WiFi OK, IP: ");
  Serial.println(WiFi.localIP());

  SPI.begin();
  rfid.PCD_Init();
  rfid.PCD_SetAntennaGain(MFRC522::RxGain_max);
  rfid.PCD_AntennaOn();
  Serial1.println("SIAP");
  Serial.println("ESP8266 RC522 + WiFi siap!");
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

  // Kirim ke M5StickC buat display: HADIR/GAGAL + UID
  Serial1.println("UID:" + uid);
  Serial1.println(nama.length() > 0 ? "HADIR" : "BARU");

  // Kirim ke Google Sheets
  bool ok = kirimKeSheets(uid, nama);
  Serial.println("UID: " + uid + " Nama: " + nama + " -> " + (ok ? "OK" : "GAGAL"));
  Serial1.println(ok ? "TERKIRIM" : "GAGALKIRIM");

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
  delay(400);
}