#include <M5StickCPlus2.h>

#define RX_PIN G26   // terima dari ESP8266 TX
#define TX_PIN G0

String uid = "";

void bipBerhasil() { M5.Speaker.tone(1000, 300); delay(300); }
void bipGagal()    { M5.Speaker.tone(500, 150); delay(200); M5.Speaker.tone(500, 150); delay(150); }

void tampilTeks(uint16_t warna, const char* judul, const String& sub) {
  M5.Lcd.fillScreen(warna);
  M5.Lcd.setTextColor(TFT_WHITE);
  M5.Lcd.setTextSize(3);
  M5.Lcd.setCursor(20, 30);
  M5.Lcd.println(judul);
  if (sub.length() > 0) {
    M5.Lcd.setTextSize(1);
    M5.Lcd.setCursor(10, 90);
    M5.Lcd.println(sub);
  }
}

void tampilHijau(String uid)   { tampilTeks(TFT_GREEN, "HADIR", "UID: " + uid); bipBerhasil(); }
void tampilBaru(String uid)    { tampilTeks(TFT_ORANGE, "KARTU", "UID BARU: " + uid); bipGagal(); }
void tampilGagalKirim()        { tampilTeks(TFT_RED, "GAGAL", "KIRIM DATA"); bipGagal(); }
void tampilStandby() {
  M5.Lcd.fillScreen(TFT_BLUE);
  M5.Lcd.setTextColor(TFT_WHITE);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(10, 50);
  M5.Lcd.println("TAP KARTU...");
}

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);
  M5.Speaker.begin();
  M5.Lcd.setRotation(3);
  tampilStandby();
  Serial2.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);
}

void loop() {
  M5.update();

  if (Serial2.available()) {
    String line = Serial2.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) return;

    if (line.startsWith("UID:")) {
      uid = line.substring(4);
    } else if (line == "HADIR") {
      tampilHijau(uid);
    } else if (line == "BARU") {
      tampilBaru(uid);
    } else if (line == "TERKIRIM") {
      delay(800);
      tampilStandby();
    } else if (line == "GAGALKIRIM") {
      tampilGagalKirim();
    }
    Serial.println("Dari ESP8266: " + line);
  }
}
