/**
 * Absensi RFID -> Google Sheets
 * Deploy as Web App: Run as "Me", Access "Anyone".
 * ESP8266 POSTs JSON {"uid":"...","name":"..."} to the deployed URL.
 */

function doPost(e) {
  var lock = LockService.getScriptLock();
  lock.waitLock(10000);
  try {
    var sheet = SpreadsheetApp.getActiveSpreadsheet().getActiveSheet();
    if (sheet.getLastRow() === 0) {
      sheet.appendRow(["Waktu", "UID", "Nama"]);
    }
    var body = JSON.parse(e.postData.contents);
    sheet.appendRow([new Date(), body.uid || "", body.name || ""]);
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