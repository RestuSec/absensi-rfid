/**
 * Absensi RFID -> Google Sheets
 * Deploy as Web App: Run as "Me", Access "Anyone".
 * ESP8266 POSTs JSON {"uid":"..."}.
 * Tab "DATA_SISWA" = UID | Nama | Kelas (satu-satunya sumber data).
 * Tap → cari UID → tulis ke tab kelas (bikin otomatis kalau belum ada).
 * UID tak dikenal → tulis ke tab UNKNOWN.
 *
 * Buka URL GET sekali buat bersihkan baris sampah di DATA_SISWA.
 */

var SHEET_ID = "1l9SbOloT0Req2wSPOAt1nujk2_urnOuU6ilfAWUFXzM";

function dataSiswa(ss, uid) {
  var ds = ss.getSheetByName("DATA_SISWA");
  if (!ds) return null;
  var u = String(uid).trim().toUpperCase();
  var data = ds.getDataRange().getValues();
  for (var i = 0; i < data.length; i++) {
    var uidCell = String(data[i][0] || "").trim().toUpperCase();
    if (uidCell == "" || uidCell == "UID") continue;
    if (uidCell == u) {
      return {
        nama: String(data[i][1] || "").trim(),
        kelas: String(data[i][2] || "").trim()
      };
    }
  }
  return null;
}

function getOrCreateTab(ss, nama, header) {
  var tab = ss.getSheetByName(nama);
  if (!tab) {
    tab = ss.insertSheet(nama);
    tab.appendRow(header);
  }
  return tab;
}

function doPost(e) {
  var lock = LockService.getScriptLock();
  lock.waitLock(10000);
  try {
    var ss = SpreadsheetApp.openById(SHEET_ID);
    var body = JSON.parse(e.postData.contents);
    var uid = String(body.uid || "").trim().toUpperCase();
    var siswa = dataSiswa(ss, uid);
    if (!siswa) {
      // Unknown → UNKNOWN tab
      var tab = getOrCreateTab(ss, "UNKNOWN", ["No", "Waktu", "UID", "Nama", "Status"]);
      tab.appendRow([tab.getLastRow(), new Date(), uid, "", "Hadir"]);
    } else {
      // Known → class tab
      var hdr = ["No", "Nama", "UID", "Status", "Jam"];
      var tab = getOrCreateTab(ss, siswa.kelas, hdr);
      var values = tab.getDataRange().getValues();
      var found = false;
      for (var r = 1; r < values.length; r++) {
        if (String(values[r][2] || "").trim().toUpperCase() == uid) {
          tab.getRange(r + 1, 4, 1, 2).setValues([["Hadir", new Date()]]);
          found = true;
          break;
        }
      }
      if (!found) {
        tab.appendRow([values.length, siswa.nama, uid, "Hadir", new Date()]);
      }
    }
    return ContentService.createTextOutput("OK")
      .setMimeType(ContentService.MimeType.TEXT);
  } finally {
    lock.releaseLock();
  }
}

// GET → bersihkan baris sampah di DATA_SISWA (timestamp di kolom A)
function doGet() {
  try {
    var ss = SpreadsheetApp.openById(SHEET_ID);
    var ds = ss.getSheetByName("DATA_SISWA");
    if (ds && ds.getLastRow() > 1) {
      var data = ds.getDataRange().getValues();
      var clean = [data[0]];
      for (var i = 1; i < data.length; i++) {
        var cell = String(data[i][0] || "").trim();
        if (cell.match(/^[0-9A-F]{2}(:[0-9A-F]{2})+$/i)) {
          clean.push(data[i]);
        }
      }
      ds.clearContents();
      if (clean.length) ds.getRange(1, 1, clean.length, data[0].length).setValues(clean);
    }
  } catch (e) {}
  return ContentService.createTextOutput("OK").setMimeType(ContentService.MimeType.TEXT);
}
