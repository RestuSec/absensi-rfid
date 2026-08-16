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

// Update baris siswa (match by UID di kolom 3) -> Status + Jam
function updateTab(ss, kelas, uid) {
  var tab = ss.getSheetByName(kelas);
  if (!tab) return false;
  var u = String(uid).trim().toUpperCase();
  var values = tab.getDataRange().getValues();
  for (var r = 0; r < values.length; r++) {
    var cell = String(values[r][2] || "").trim().toUpperCase();
    if (cell == "" || cell == "UID") continue;  // skip header/kosong
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

// Diagnostik: catat tiap POST + hasil ke tab DEBUG (hapus fungsi ini nanti)
function logDebug(ss, pesan) {
  var tab = cariAtauBuatTab(ss, "DEBUG");
  tab.appendRow([new Date(), pesan]);
}

// Diagnostik: dump isi tab apa aja (hapus nanti)
function dumpTab(ss, nama) {
  var tab = ss.getSheetByName(nama);
  if (!tab) return "TAB '" + nama + "' TIDAK ADA";
  var data = tab.getDataRange().getValues();
  var out = [];
  for (var i = 0; i < data.length; i++) {
    out.push("baris" + (i + 1) + "='" + String(data[i][0]) + "'|'" + String(data[i][1]) + "'|'" + String(data[i][2]) + "'|'" + String(data[i][3]) + "'|'" + String(data[i][4]) + "'");
  }
  return out.join(" # ");
}

// Diagnostik: dump isi tab DATA_SISWA biar keliatan perbedaan UID (hapus nanti)
function dumpDataSiswa(ss) {
  var ds = ss.getSheetByName("DATA_SISWA");
  if (!ds) return "DATA_SISWA TIDAK ADA";
  var data = ds.getDataRange().getValues();
  var out = [];
  for (var i = 0; i < data.length; i++) {
    out.push("baris" + (i + 1) + "='" + String(data[i][0]) + "'|'" + String(data[i][1]) + "'|'" + String(data[i][2]) + "'");
  }
  return out.join(" # ");
}

function doPost(e) {
  var lock = LockService.getScriptLock();
  lock.waitLock(10000);
  try {
    var ss = SpreadsheetApp.openById(SHEET_ID);
    var body = JSON.parse(e.postData.contents);
    var uid = String(body.uid || "");
    var siswa = dataSiswa(ss, uid);
    var pesan = "UID='" + uid + "' dataSiswa=" + (siswa ? ("KETEMU nama='" + siswa.nama + "' kelas='" + siswa.kelas + "'") : "NULL");
    if (siswa) {
      var updated = updateTab(ss, siswa.kelas, uid);
      pesan += " updateTab=" + (updated ? "SUKSES" : "GAGAL (cek baris " + siswa.nama + " di tab '" + siswa.kelas + "')");
      pesan += " || DUMP " + siswa.kelas + ": " + dumpTab(ss, siswa.kelas);
      if (!updated) tampungUnknown(ss, uid, siswa.nama);
    } else {
      pesan += " -> cek isi sel UID di tab DATA_SISWA";
      pesan += " || DUMP DATA_SISWA: " + dumpDataSiswa(ss);
      tampungUnknown(ss, uid, "");
    }
    logDebug(ss, pesan);
    return ContentService.createTextOutput("OK")
      .setMimeType(ContentService.MimeType.TEXT);
  } catch (err) {
    logDebug(ss, "ERROR: " + err);
    throw err;
  } finally {
    lock.releaseLock();
  }
}

function doGet() {
  // Buka URL ini di browser untuk tes: harus balas "OK".
  return ContentService.createTextOutput("OK");
}