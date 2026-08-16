/**
 * Absensi RFID -> Google Sheets
 * Deploy as Web App: Run as "Me", Access "Anyone".
 * ESP8266 POSTs JSON {"uid":"..."} to the deployed URL.
 *
 * Sheet "Roster" (kolom: UID | Nama | Kelas) menentukan identitas siswa.
 * Tiap tap: cari UID di Roster, tulis baris ke sheet pertama (Absensi).
 * Siswa baru (UID belum ada di Roster) -> Nama & Kelas kosong.
 */

function kelasDariUID(ss, uid) {
  var roster = ss.getSheetByName("Roster");
  if (!roster) return ["", ""];
  var data = roster.getDataRange().getValues();
  for (var i = 1; i < data.length; i++) { // skip header
    if (String(data[i][0]) == String(uid)) {
      return [String(data[i][1] || ""), String(data[i][2] || "")];
    }
  }
  return ["", ""];
}

function doPost(e) {
  var lock = LockService.getScriptLock();
  lock.waitLock(10000);
  try {
    var ss = SpreadsheetApp.getActiveSpreadsheet();
    var sheet = ss.getSheets()[0];
    if (sheet.getLastRow() === 0) {
      sheet.appendRow(["Waktu", "UID", "Nama", "Kelas"]);
    }
    var body = JSON.parse(e.postData.contents);
    var uid = String(body.uid || "");
    var info = kelasDariUID(ss, uid);
    sheet.appendRow([new Date(), uid, info[0], info[1]]);
    return ContentService.createTextOutput("OK")
      .setMimeType(ContentService.MimeType.TEXT);
  } finally {
    lock.releaseLock();
  }
}

function doGet() {
  // Buka URL ini di browser untuk tes: harus balas "OK".
  return ContentService.createTextOutput("OK");
}