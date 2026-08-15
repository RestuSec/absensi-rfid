#include <M5StickCPlus2.h>

#define RX_PIN G26   // terima UID dari ESP8266 TX
#define TX_PIN G0

void bipBerhasil() { M5.Speaker.tone(1000, 300); delay(300); }
void bipGagal()    { M5.Speaker.tone(500, 150); delay(200); M5.Speaker.tone(500, 150); delay(150); }

void tampilHijau(String uid) {
  M5.Lcd.fillScreen(TFT_GREEN);
  M5.Lcd.setTextColor(TFT_WHITE);
  M5.Lcd.setTextSize(3);
  M5.Lcd.setCursor(30, 40);
  M5.Lcd.println("HADIR");
  M5.Lcd.setTextSize(1);
  M5.Lcd.setCursor(10, 90);
  M5.Lcd.println("UID: " + uid);
}

void tampilMerah() {
  M5.Lcd.fillScreen(TFT_RED);
  M5.Lcd.setTextColor(TFT_WHITE);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(20, 50);
  M5.Lcd.println("GAGAL");
}

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
    String uid = Serial2.readStringUntil('\n');
    uid.trim();
    if (uid.length() > 0) {
      Serial.println("Dari ESP8266: " + uid);
      tampilHijau(uid);
      bipBerhasil();
      delay(2000);
      tampilStandby();
    }
  }
}