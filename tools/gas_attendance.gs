/**
 * Absensi RFID -> Google Sheets (fixed roster, update per siswa)
 * Deploy as Web App: Run as "Me", Access "Anyone".
 * ESP8266 POSTs JSON {"uid":"..."}.
 *
 * DATA_SISWA (UID | Nama | Kelas) = satu-satunya sumber data.
 * Tab kelas dibuat/diperbaiki OTOMATIS dari DATA_SISWA tiap tap:
 *   No | Nama | UID | Status | Jam, semua siswa kelas itu pre-filled
 *   dengan Status "Belum". Tap mengubah Status -> "Hadir" + Jam.
 * Tidak perlu bikin tab kelas manual.
 */

var SHEET_ID = "1l9SbOloT0Req2wSPOAt1nujk2_urnOuU6ilfAWUFXzM";

function dataSiswa(ss, uid) {
  var ds = ss.getSheetByName("DATA_SISWA");
  if (!ds) return null;
  var u = String(uid).trim().toUpperCase();
  var data = ds.getDataRange().getValues();
  for (var i = 0; i < data.length; i++) {
    var uidCell = String(data[i][0] || "").trim().toUpperCase();
    if (uidCell == "" || uidCell == "UID") continue;  // skip header/kosong
    if (uidCell == u) {
      return {
        nama: String(data[i][1] || "").trim(),
        kelas: String(data[i][2] || "").trim()
      };
    }
  }
  return null;
}

// Pastikan tab kelas ada + struktur No|Nama|UID|Status|Jam dengan
// roster pre-filled dari DATA_SISWA (Status "Belum").
// ponytail: rebuild tab = hapus isi lama yang strukturnya salah;
// ceiling: data lama ilang, upgrade: pindahin dulu ke tab ARSIP.
function jaminTabKelas(ss, kelas) {
  var tab = ss.getSheetByName(kelas);
  var ok = false;
  if (tab) {
    var hdr = tab.getRange(1, 1, 1, 5).getValues()[0];
    ok = String(hdr[0]).trim() == "No" && String(hdr[1]).trim() == "Nama" &&
         String(hdr[2]).trim() == "UID" && String(hdr[3]).trim() == "Status" &&
         String(hdr[4]).trim() == "Jam";
  }
  if (!ok) {
    if (!tab) tab = ss.insertSheet(kelas);
    else tab.clearContents();
    tab.getRange(1, 1, 1, 5).setValues([["No", "Nama", "UID", "Status", "Jam"]]);
    var ds = ss.getSheetByName("DATA_SISWA");
    if (ds) {
      var data = ds.getDataRange().getValues();
      var out = [];
      var no = 1;
      for (var i = 0; i < data.length; i++) {
        if (String(data[i][2] || "").trim().toUpperCase() == kelas.toUpperCase()) {
          out.push([no++, String(data[i][1] || "").trim(), String(data[i][0] || "").trim(), "Belum", ""]);
        }
      }
      if (out.length) tab.getRange(2, 1, out.length, 5).setValues(out);
    }
  }
  return tab;
}

// Update baris siswa (match by UID di kolom 3) -> Status + Jam
function updateTab(ss, kelas, uid) {
  var tab = jaminTabKelas(ss, kelas);
  var u = String(uid).trim().toUpperCase();
  var values = tab.getDataRange().getValues();
  for (var r = 1; r < values.length; r++) {
    var cell = String(values[r][2] || "").trim().toUpperCase();
    if (cell == "" || cell == "UID") continue;
    if (cell == u) {
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
    var ss = SpreadsheetApp.openById(SHEET_ID);
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
