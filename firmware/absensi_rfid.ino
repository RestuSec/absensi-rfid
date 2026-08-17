#include <SPI.h>
#include <MFRC522.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <time.h>

#define SS_PIN  D8
#define RST_PIN D3
#define LED_PIN 2

MFRC522 rfid(SS_PIN, RST_PIN);
ESP8266WebServer server(80);

// ─── STRUCT ─────────────────────────────────────────────────────────────────

struct Siswa {
  String uid;
  String nama;
  String kelas;
};

struct Absen {
  int no;
  String uid;
  String nama;
  String kelas;
  String waktu;
};

#define MAX_SISWA 100
#define MAX_ABSEN 200

Siswa daftarSiswa[MAX_SISWA];
Absen daftarAbsen[MAX_ABSEN];
int jumlahSiswa = 0;
int jumlahAbsen = 0;
int noUrutAbsen = 0; // ponytail: counter persisten, reset if MAX_ABSEN > ~2M

// ─── LITTLEFS ───────────────────────────────────────────────────────────────

void simpanSiswa() {
  File f = LittleFS.open("/siswa.json", "w");
  if (!f) return;
  DynamicJsonDocument doc(4096);
  JsonArray arr = doc.to<JsonArray>();
  for (int i = 0; i < jumlahSiswa; i++) {
    JsonObject obj = arr.createNestedObject();
    obj["uid"]   = daftarSiswa[i].uid;
    obj["nama"]  = daftarSiswa[i].nama;
    obj["kelas"] = daftarSiswa[i].kelas;
  }
  serializeJson(doc, f);
  f.close();
}

void muatSiswa() {
  if (!LittleFS.exists("/siswa.json")) return;
  File f = LittleFS.open("/siswa.json", "r");
  if (!f) return;
  DynamicJsonDocument doc(4096);
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err) { Serial.println("[FS] siswa.json corrupt, skip"); return; }
  jumlahSiswa = 0;
  for (JsonObject obj : doc.as<JsonArray>()) {
    if (jumlahSiswa >= MAX_SISWA) break;
    daftarSiswa[jumlahSiswa].uid   = obj["uid"].as<String>();
    daftarSiswa[jumlahSiswa].nama  = obj["nama"].as<String>();
    daftarSiswa[jumlahSiswa].kelas = obj["kelas"].as<String>();
    jumlahSiswa++;
  }
}

void simpanAbsen() {
  File f = LittleFS.open("/absen.json", "w");
  if (!f) return;
  DynamicJsonDocument doc(12288);
  JsonObject root = doc.to<JsonObject>();
  root["counter"] = noUrutAbsen;
  JsonArray arr = root.createNestedArray("data");
  for (int i = 0; i < jumlahAbsen; i++) {
    JsonObject obj = arr.createNestedObject();
    obj["no"]    = daftarAbsen[i].no;
    obj["uid"]   = daftarAbsen[i].uid;
    obj["nama"]  = daftarAbsen[i].nama;
    obj["kelas"] = daftarAbsen[i].kelas;
    obj["waktu"] = daftarAbsen[i].waktu;
  }
  serializeJson(doc, f);
  f.close();
}

void muatAbsen() {
  if (!LittleFS.exists("/absen.json")) return;
  File f = LittleFS.open("/absen.json", "r");
  if (!f) return;
  DynamicJsonDocument doc(12288);
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err) { Serial.println("[FS] absen.json corrupt, skip"); return; }
  noUrutAbsen = doc["counter"] | 0;
  jumlahAbsen = 0;
  for (JsonObject obj : doc["data"].as<JsonArray>()) {
    if (jumlahAbsen >= MAX_ABSEN) break;
    daftarAbsen[jumlahAbsen].no    = obj["no"].as<int>();
    daftarAbsen[jumlahAbsen].uid   = obj["uid"].as<String>();
    daftarAbsen[jumlahAbsen].nama  = obj["nama"].as<String>();
    daftarAbsen[jumlahAbsen].kelas = obj["kelas"].as<String>();
    daftarAbsen[jumlahAbsen].waktu = obj["waktu"].as<String>();
    jumlahAbsen++;
  }
}

// ─── HELPER ─────────────────────────────────────────────────────────────────

String getWaktu() {
  time_t now = time(nullptr);
  struct tm* t = localtime(&now);
  char buf[30];
  sprintf(buf, "%02d/%02d/%04d %02d:%02d:%02d",
    t->tm_mday, t->tm_mon+1, t->tm_year+1900,
    t->tm_hour, t->tm_min, t->tm_sec);
  return String(buf);
}

Siswa* cariSiswa(String uid) {
  for (int i = 0; i < jumlahSiswa; i++) {
    if (daftarSiswa[i].uid == uid) return &daftarSiswa[i];
  }
  return nullptr;
}

void blinkLED(int n, int ms) {
  for (int i = 0; i < n; i++) {
    digitalWrite(LED_PIN, LOW); delay(ms);
    digitalWrite(LED_PIN, HIGH);
    if (i < n-1) delay(ms);
  }
}

// ─── HTML DASHBOARD ─────────────────────────────────────────────────────────

String htmlHeader(String judul) {
  return R"(<!DOCTYPE html><html><head><meta charset='UTF-8'>
<meta name='viewport' content='width=device-width,initial-scale=1'>
<title>Absensi RFID</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;background:#f1f5f9;color:#1e293b}
a{color:#3b82f6;text-decoration:none}
nav{background:#0f172a;padding:12px 20px;display:flex;gap:16px;align-items:center}
nav .brand{color:#fff;font-size:16px;font-weight:700;margin-right:auto}
nav a{color:#cbd5e1;font-size:13px}
.wrap{max-width:960px;margin:24px auto;padding:0 16px}
h2{font-size:18px;font-weight:700;margin-bottom:16px}
.card{background:#fff;border-radius:10px;box-shadow:0 1px 3px rgba(0,0,0,.08);overflow:hidden}
table{width:100%;border-collapse:collapse;font-size:14px}
th{background:#0f172a;color:#fff;padding:10px 14px;text-align:left;font-weight:600;font-size:13px}
td{padding:9px 14px;border-bottom:1px solid #e2e8f0}
.btn{display:inline-block;padding:8px 16px;font-size:13px;font-weight:600;border:none;border-radius:8px;text-decoration:none}
.btn-primary{background:#0f172a;color:#fff}
.btn-danger{background:#ef4444;color:#fff}
.btn-sm{padding:4px 10px;font-size:12px}
label{display:block;font-size:13px;font-weight:600;color:#64748b;margin-bottom:4px}
input{padding:8px 12px;font-size:14px;border:1px solid #e2e8f0;border-radius:8px;width:100%;background:#fff}
input:focus{outline:none;border-color:#3b82f6}
.form-row{display:flex;gap:10px;margin-bottom:12px}
.form-row>*{flex:1}
.msg{padding:10px 14px;margin-bottom:14px;font-size:13px;background:#ecfdf5;color:#065f46;border-radius:10px;border-left:4px solid #10b981}
@media(max-width:640px){
  nav{padding:10px 14px}
  .form-row{flex-direction:column}
  table{font-size:13px}
  th,td{padding:7px 10px}
}
</style></head><body>
<nav>
<span class='brand'>Absensi RFID</span>
<a href='/'>Absen</a>
<a href='/siswa'>Siswa</a>
<a href='/export'>Export</a>
<a href='/hapus-semua' onclick='return confirm("Hapus semua data absen?")'>Hapus Semua</a>
</nav>
<div class='wrap'>
<h2>)" + judul + R"(</h2>)";
}

String htmlFooter() {
  return "</div></body></html>";
}

// ─── ROUTES ─────────────────────────────────────────────────────────────────

void handleRoot() {
  String html = htmlHeader("Data Absensi");
  html += "<div style='display:flex;gap:8px;margin-bottom:14px'>";
  html += "<a href='/export' class='btn btn-primary'>Export Excel</a>";
  html += "<a href='/hapus-semua' class='btn btn-danger' onclick='return confirm(\"Hapus semua data absen?\")'>Hapus Semua</a>";
  html += "</div>";
  html += "<div class='card'><table><tr><th>No</th><th>Nama</th><th>Kelas</th><th>UID</th><th>Waktu</th><th>Aksi</th></tr>";
  for (int i = 0; i < jumlahAbsen; i++) {
    html += "<tr><td>" + String(daftarAbsen[i].no) + "</td>";
    html += "<td>" + daftarAbsen[i].nama + "</td>";
    html += "<td>" + daftarAbsen[i].kelas + "</td>";
    html += "<td>" + daftarAbsen[i].uid + "</td>";
    html += "<td>" + daftarAbsen[i].waktu + "</td>";
    html += "<td><a href='/hapus?i=" + String(i) + "' class='btn btn-danger btn-sm' onclick='return confirm(\"Hapus?\")'>Hapus</a></td></tr>";
  }
  if (jumlahAbsen == 0) html += "<tr><td colspan='6' style='text-align:center;color:#64748b;padding:32px'>Belum ada data absensi</td></tr>";
  html += "</table></div>";
  html += htmlFooter();
  server.send(200, "text/html", html);
}

void handleSiswa() {
  String msg = "";
  if (server.hasArg("aksi")) {
    String aksi = server.arg("aksi");
    if (aksi == "tambah") {
      String uid  = server.arg("uid"); uid.toUpperCase(); uid.trim();
      String nama = server.arg("nama"); nama.trim();
      String kelas = server.arg("kelas"); kelas.trim();
      if (uid.length() && nama.length() && kelas.length() && jumlahSiswa < MAX_SISWA) {
        bool ada = false;
        for (int i = 0; i < jumlahSiswa; i++) {
          if (daftarSiswa[i].uid == uid) { ada = true; break; }
        }
        if (!ada) {
          daftarSiswa[jumlahSiswa].uid   = uid;
          daftarSiswa[jumlahSiswa].nama  = nama;
          daftarSiswa[jumlahSiswa].kelas = kelas;
          jumlahSiswa++;
          simpanSiswa();
          msg = "Siswa berhasil ditambahkan.";
        } else msg = "UID sudah terdaftar.";
      }
    } else if (aksi == "edit") {
      int idx = server.arg("idx").toInt();
      String nama  = server.arg("nama"); nama.trim();
      String kelas = server.arg("kelas"); kelas.trim();
      if (idx >= 0 && idx < jumlahSiswa) {
        daftarSiswa[idx].nama  = nama;
        daftarSiswa[idx].kelas = kelas;
        simpanSiswa();
        msg = "Data siswa diperbarui.";
      }
    } else if (aksi == "hapus") {
      int idx = server.arg("idx").toInt();
      if (idx >= 0 && idx < jumlahSiswa) {
        for (int i = idx; i < jumlahSiswa-1; i++) daftarSiswa[i] = daftarSiswa[i+1];
        jumlahSiswa--;
        simpanSiswa();
        msg = "Siswa dihapus.";
      }
    }
  }

  String html = htmlHeader("Data Siswa");
  if (msg.length()) html += "<div class='msg'>" + msg + "</div>";

  html += R"(<div class='card' style='padding:20px'>
<form method='POST'>
<input type='hidden' name='aksi' value='tambah'>
<div class='form-row'>
<div><label>UID</label><input name='uid' placeholder='Contoh: 19:79:F4:1F'></div>
<div><label>Nama</label><input name='nama' placeholder='Nama siswa'></div>
<div><label>Kelas</label><input name='kelas' placeholder='Contoh: TKJ 2'></div>
</div>
<button type='submit' class='btn btn-primary'>Tambah Siswa</button>
</form></div><br>)";

  html += "<div class='card'><table><tr><th>No</th><th>UID</th><th>Nama</th><th>Kelas</th><th>Aksi</th></tr>";
  for (int i = 0; i < jumlahSiswa; i++) {
    html += "<tr><td>" + String(i+1) + "</td>";
    html += "<td>" + daftarSiswa[i].uid + "</td>";
    html += "<td>" + daftarSiswa[i].nama + "</td>";
    html += "<td>" + daftarSiswa[i].kelas + "</td>";
    html += "<td>";
    html += "<a href='/edit-siswa?idx=" + String(i) + "' class='btn btn-primary btn-sm'>Edit</a> ";
    html += "<a href='/siswa?aksi=hapus&idx=" + String(i) + "' class='btn btn-danger btn-sm' onclick='return confirm(\"Hapus?\")'>Hapus</a>";
    html += "</td></tr>";
  }
  if (jumlahSiswa == 0) html += "<tr><td colspan='5' style='text-align:center;color:#64748b;padding:32px'>Belum ada data siswa</td></tr>";
  html += "</table></div>";
  html += htmlFooter();
  server.send(200, "text/html", html);
}

void handleEditSiswa() {
  int idx = server.arg("idx").toInt();
  if (idx < 0 || idx >= jumlahSiswa) { server.sendHeader("Location", "/siswa"); server.send(302); return; }

  if (server.method() == HTTP_POST) {
    daftarSiswa[idx].nama  = server.arg("nama");
    daftarSiswa[idx].kelas = server.arg("kelas");
    simpanSiswa();
    server.sendHeader("Location", "/siswa");
    server.send(302);
    return;
  }

  String html = htmlHeader("Edit Siswa");
  html += "<div class='card' style='padding:20px'>";
  html += "<form method='POST'>";
  html += "<div class='form-row'>";
  html += "<div><label>UID</label><input value='" + daftarSiswa[idx].uid + "' disabled></div>";
  html += "<div><label>Nama</label><input name='nama' value='" + daftarSiswa[idx].nama + "'></div>";
  html += "<div><label>Kelas</label><input name='kelas' value='" + daftarSiswa[idx].kelas + "'></div>";
  html += "</div>";
  html += "<button type='submit' class='btn btn-primary'>Simpan</button> ";
  html += "<a href='/siswa' class='btn btn-primary'>Batal</a>";
  html += "</form></div>";
  html += htmlFooter();
  server.send(200, "text/html", html);
}

void handleHapus() {
  int idx = server.arg("i").toInt();
  if (idx >= 0 && idx < jumlahAbsen) {
    for (int i = idx; i < jumlahAbsen-1; i++) daftarAbsen[i] = daftarAbsen[i+1];
    jumlahAbsen--;
    simpanAbsen();
  }
  server.sendHeader("Location", "/");
  server.send(302);
}

void handleHapusSemua() {
  jumlahAbsen = 0;
  noUrutAbsen = 0;
  simpanAbsen();
  server.sendHeader("Location", "/");
  server.send(302);
}

void handleExport() {
  String out = "<html><head><meta charset='UTF-8'></head><body>";
  out += "<table border='1' cellpadding='6' cellspacing='0' style='border-collapse:collapse;font-family:Arial,sans-serif'>";
  out += "<tr style='background:#0f172a;color:#fff;font-weight:bold'>";
  out += "<td>No</td><td>Nama</td><td>Kelas</td><td>UID</td><td>Waktu</td></tr>";
  for (int i = 0; i < jumlahAbsen; i++) {
    String bg = (i % 2 == 0) ? "#f8fafc" : "#fff";
    out += "<tr style='background:" + bg + "'>";
    out += "<td>" + String(daftarAbsen[i].no) + "</td>";
    out += "<td>" + daftarAbsen[i].nama + "</td>";
    out += "<td>" + daftarAbsen[i].kelas + "</td>";
    out += "<td>" + daftarAbsen[i].uid + "</td>";
    out += "<td>" + daftarAbsen[i].waktu + "</td></tr>";
  }
  if (jumlahAbsen == 0) out += "<tr><td colspan='5' style='text-align:center;color:#999'>Tidak ada data</td></tr>";
  out += "</table></body></html>";
  server.sendHeader("Content-Disposition", "attachment; filename=absensi.html");
  server.send(200, "text/html", out);
}

// ─── SETUP ──────────────────────────────────────────────────────────────────

void setup() {
  Serial.begin(115200);
  delay(100);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

  SPI.begin();
  rfid.PCD_Init();
  Serial.println("[RC522] Ready");

  LittleFS.begin();
  muatSiswa();
  muatAbsen();
  Serial.println("[FS] Siswa: " + String(jumlahSiswa) + ", Absen: " + String(jumlahAbsen));

  WiFiManager wm;
  if (!wm.autoConnect("Absensi-RFID")) ESP.restart();
  Serial.println("[WiFi] " + WiFi.localIP().toString());

  configTime(7 * 3600, 0, "pool.ntp.org");
  Serial.print("[NTP] Sync");
  unsigned long ntpStart = millis();
  while (time(nullptr) < 100000 && millis() - ntpStart < 10000) { delay(500); Serial.print("."); }
  if (time(nullptr) < 100000) Serial.println(" TIMEOUT (lanjut tanpa jam)");
  else Serial.println(" OK");

  server.on("/",            handleRoot);
  server.on("/siswa",       HTTP_GET,  handleSiswa);
  server.on("/siswa",       HTTP_POST, handleSiswa);
  server.on("/edit-siswa",  HTTP_GET,  handleEditSiswa);
  server.on("/edit-siswa",  HTTP_POST, handleEditSiswa);
  server.on("/hapus",       handleHapus);
  server.on("/hapus-semua", handleHapusSemua);
  server.on("/export",      handleExport);
  server.begin();
  Serial.println("[HTTP] Server started");

  blinkLED(3, 150);
}

// ─── LOOP ───────────────────────────────────────────────────────────────────

unsigned long lastRFID = 0;

void loop() {
  server.handleClient();

  if (millis() - lastRFID < 1500) return; // debounce
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) return;
  lastRFID = millis();

  String uid = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (i > 0) uid += ":";
    if (rfid.uid.uidByte[i] < 0x10) uid += "0";
    uid += String(rfid.uid.uidByte[i], HEX);
  }
  uid.toUpperCase();
  Serial.println("[RFID] " + uid);

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();

  String nama  = "UNKNOWN";
  String kelas = "UNKNOWN";
  Siswa* s = cariSiswa(uid);
  if (s) { nama = s->nama; kelas = s->kelas; }

  if (jumlahAbsen < MAX_ABSEN) {
    noUrutAbsen++;
    daftarAbsen[jumlahAbsen].no    = noUrutAbsen;
    daftarAbsen[jumlahAbsen].uid   = uid;
    daftarAbsen[jumlahAbsen].nama  = nama;
    daftarAbsen[jumlahAbsen].kelas = kelas;
    daftarAbsen[jumlahAbsen].waktu = getWaktu();
    jumlahAbsen++;
    simpanAbsen();
    blinkLED(1, 500);
    Serial.println("[ABSEN] #" + String(noUrutAbsen) + " " + nama + " - " + kelas);
  }
}
