#include <SPI.h>
#include <MFRC522.h>

// NodeMCU 1.0 pin SPI: D8=SS, D5=SCK, D7=MOSI, D6=MISO
#define SS_PIN   D8
#define RST_PIN  D3

MFRC522 rfid(SS_PIN, RST_PIN);

void setup() {
  Serial.begin(115200); // monitor debug (USB)
  Serial1.begin(115200); // kirim ke M5StickC (pin TX1)

  SPI.begin();
  rfid.PCD_Init();
  rfid.PCD_SetAntennaGain(MFRC522::RxGain_max);
  rfid.PCD_AntennaOn();
  Serial.println("ESP8266 RC522 siap!");
}

void loop() {
  if (!rfid.PICC_IsNewCardPresent()) return;
  if (!rfid.PICC_ReadCardSerial()) return;

  String uid = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) uid += "0";
    uid += String(rfid.uid.uidByte[i], HEX);
    if (i < rfid.uid.size - 1) uid += ":";
  }
  uid.toUpperCase();

  // Kirim ke M5StickC lewat Serial1
  Serial1.println(uid);
  Serial.println("UID: " + uid); // debug

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
  delay(500);
}