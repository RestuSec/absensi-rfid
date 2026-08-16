/**
 * Absensi RFID -> Google Sheets (fixed roster, update per siswa)
 * Deploy as Web App: Run as "Me", Access "Anyone".
 * ESP8266 POSTs JSON {"uid":"..."}.
 *
 * Tab "DATA_SISWA" (kolom: UID | Nama | Kelas) = database siswa.
 * Tiap kelas punya tab fixed roster: No | Nama | UID | Status | Jam
 * (diisi awal dengan Status = "Belum"). Tiap tap mencari UID di tab
 * kelasnya, lalu mengubah Status -> "Hadir" dan mengisi Jam.
 * Urutan tap TIDAK mengubah urutan daftar absen.
 */

function dataSiswa(ss, uid) {
  var ds = ss.getSheetByName("DATA_SISWA");
  if (!ds) return null;
  var u = String(uid).trim().toUpperCase();
  var data = ds.getDataRange().getValues();
  for (var i = 1; i < data.length; i++) { // skip header
    if (String(data[i][0]).trim().toUpperCase() == u) {
      return {
        nama: String(data[i][1] || "").trim(),
        kelas: String(data[i][2] || "").trim()
      };
    }
  }
  return null;
}

// Update baris siswa (match by UID di kolom 3) -> Status + Jam
function updateTab(ss, kelas, uid) {
  var tab = ss.getSheetByName(kelas);
  if (!tab) return false;
  var u = String(uid).trim().toUpperCase();
  var values = tab.getDataRange().getValues();
  for (var r = 1; r < values.length; r++) { // skip header
    if (String(values[r][2]).trim().toUpperCase() == u) {
      tab.getRange(r + 1, 4, 1, 2).setValues([["Hadir", new Date()]]);
      return true;
    }
  }
  return false;
}

function cariAtauBuatTab(ss, nama) {
  var tab = ss.getSheetByName(nama);
  if (!tab) tab = ss.insertSheet(nama);
  return tab;
}

// ponytail: fallback append ke UNKNOWN supaya data tidak hilang;
// kalau mau ketat, hapus fallback ini dan biarkan UID tak dikenal diabaikan.
function tampungUnknown(ss, uid, nama) {
  var tab = cariAtauBuatTab(ss, "UNKNOWN");
  if (tab.getLastRow() === 0) tab.appendRow(["No", "Waktu", "UID", "Nama", "Status"]);
  tab.appendRow([tab.getLastRow(), new Date(), uid, nama, "Hadir"]);
}

function doPost(e) {
  var lock = LockService.getScriptLock();
  lock.waitLock(10000);
  try {
    var ss = SpreadsheetApp.getActiveSpreadsheet();
    var body = JSON.parse(e.postData.contents);
    var uid = String(body.uid || "");
    var siswa = dataSiswa(ss, uid);
    if (siswa) {
      var updated = updateTab(ss, siswa.kelas, uid);
      if (!updated) tampungUnknown(ss, uid, siswa.nama);
    } else {
      tampungUnknown(ss, uid, "");
    }
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