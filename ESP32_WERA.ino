  /*
    WERA - ESP32 unified sketch (FIXED)
    - UART2 voice module (DFRobot DF2301Q) via Serial2 (RX=16, TX=17)
    - Fix pin conflicts, setup ordering, syntax, LCD prints, prefs init
  */

  #include <WiFi.h>
  #include <WebServer.h>
  #include <DNSServer.h>
  #include <Preferences.h>
  #include "time.h"
  #include <Wire.h>
  #include <ESP32SharpIR.h>
  #include <ESP32Servo.h>
  #include <MPU6050_tockn.h>
  #include <Adafruit_Sensor.h>
  #include <LiquidCrystal_I2C.h>

  // Firebase v1 for ESP32
  #include <Firebase_ESP_Client.h>
  #include <addons/TokenHelper.h>
  #include <addons/RTDBHelper.h>

  // Library modul gravity : voice recognition
  #include <DFRobot_DF2301Q.h>
  // Konstruktor tetap di-declare dengan Serial 1 object reference
  DFRobot_DF2301Q_UART asr(&Serial1, 16, 17);

  // ============== CONFIG ==============
  #define FIREBASE_HOST "https://wera-iot-default-rtdb.asia-southeast1.firebasedatabase.app"
  #define FIREBASE_AUTH "VW3RzQN8pYrn1sXzdhh4SBLO1Cmk3zat7Kku09Gx"
  
  // ======= AP MODE =======
  WebServer server(80);
  DNSServer dnsServer;
  const byte DNS_PORT = 53;
  bool apMode = false;
  bool checkInternet(){
      WiFiClient client;
      return client.connect("www.google.com", 80);
    }

  // ======= Preferences =======
  Preferences prefs;         // untuk kalibrasi / board size
  Preferences preferences;   // khusus WiFi credentials

  String saved_ssid = "";
  String saved_password = "";
  const char *PREF_NAMESPACE = "wera_prefs";
  const char *PREF_KEY_CALIB_DONE = "cal_done";
  const char *PREF_KEY_BOARD_W = "board_w";
  const char *PREF_KEY_BOARD_H = "board_h";
  const char *PREF_KEY_STRIP_CM = "strip_w";
  bool calibrationRequired = false;

  //TIMER
  unsigned long bootMillis = 0;
  unsigned long operationStartMillis = 0;
  int operationCycleCount = 0;
  unsigned long lastADCRead = 0;
  unsigned long lastLCDUpdate = 0;
  unsigned long lastFirebaseSend = 0;
  unsigned long lastConnCheck = 0;
  unsigned long lastPrefsSave = 0;

  // AP default credentials
  const char* AP_SSID = "WERA_setup";
  const char* AP_PASS = "12345678";

  // Pins (sesuaikan jika perlu)
  ESP32SharpIR sensor1( ESP32SharpIR::GP2Y0A21YK0F, 35);
  ESP32SharpIR sensor2( ESP32SharpIR::GP2Y0A21YK0F, 32);
  ESP32SharpIR sensor3( ESP32SharpIR::GP2Y0A21YK0F, 33);

  // NOTE: change batt ADC pin to avoid conflict with IR_KANAN(33)
  const int PIN_BATT_ADC = 36; // pastikan board support ADC pada pin ini

  const int PIN_SERVO_LEFT = 13;
  const int PIN_SERVO_RIGHT = 4;

  // ================= BATTERY MONITOR CONFIG =================
  const float VOLTAGE_DIVIDER_FACTOR = 5.003f;  // faktor divider hasil ukur (6.78V / 1.356V)
  const float ADC_VREF_CAL = 3.71f;             // hasil kalibrasi ADC agar cocok dengan multimeter
  const int ADC_BITS = 4095;
  const float BATT_MIN_VOLTAGE = 6.0;
  const float BATT_MAX_VOLTAGE = 7.8f;
  float lastVbat = 0.0;
  const float VBAT_TOLERANCE = 0.05;
  
  // averaging parameter
  const int BATT_SAMPLES = 25;                  // jumlah sample untuk averaging
  

  // fungsi pembacaan stabil
  float readBatteryVoltage() {
    uint32_t sum = 0;
    for (int i = 0; i < BATT_SAMPLES; i++) {
      sum += analogRead(PIN_BATT_ADC);
      delayMicroseconds(400);
    }
    float raw = sum / (float)BATT_SAMPLES;
    float vadc = (raw / ADC_BITS) * ADC_VREF_CAL;
    float vbat_now = vadc * VOLTAGE_DIVIDER_FACTOR;

    // ====== Deadband filter ======
  if (fabs(vbat_now - lastVbat) < VBAT_TOLERANCE) {
    vbat_now = lastVbat;  // jangan ubah kalau selisih kecil
  } else {
    lastVbat = vbat_now;  // simpan kalau ada perubahan nyata
  }

  vbat_now = 0.8f * lastVbat + 0.2f * vbat_now;
  lastVbat = vbat_now;

  return vbat_now;
  }

  // Time Config
  const char* ntpServer = "pool.ntp.org";
  const long gmtOffset_sec = 7 * 3600; //GMT +7 (WIB)
  const int daylightOffset_sec = 0;
  char timeString[25];
  String getTimeNow(){
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)){
      return "00-00-0000 00:00:00";
    }
    strftime(timeString, sizeof(timeString), "%d-%m-%Y %H:%M:%S", &timeinfo);
    return String(timeString);
  }

  // Servo microseconds default (empiris harus dikalibrasi)
  int SERVO_STOP_LEFT = 1525;
  int SERVO_STOP_RIGHT = 1530;
  int SERVO_FORWARD_LEFT = 2400;
  int SERVO_BACK_LEFT = 544;
  int SERVO_FORWARD_RIGHT = 544;
  int SERVO_BACK_RIGHT = 2400;

  // Movement parameters
  float stripWidthCm = 24.0;
  const int DELAY_AFTER_STRIP_MS = 1000;
  const int DIST_FRONT_THRESHOLD_CM = 10;
  const int DIST_SIDE_TURN_THRESHOLD_CM = 20;

  const int ANGLE_CORRECTION_THRESHOLD = 10;
  const int TURN_DELAY_MS = 800;
  const int ADVANCE_DELAY_MS = 600;

  // Device
  String deviceID = "WERA001";
  
  //Firebase Path
  String FIREBASE_TELEMETRY_PATH = "/rtdb_devices/" + deviceID + "/telemetry";
  String FIREBASE_STATUS_PATH    = "/rtdb_devices/" + deviceID + "/telemetry/status";
  String FIREBASE_ROOT_PATH = "/rtdb_devices/" + deviceID;
  String FIREBASE_CONSTAT_PATH = "/rtdb_devices/" +deviceID + "/connection_status";

  //LCD I2C
  LiquidCrystal_I2C lcd(0x27, 16, 2); // alamat 0x27 atau 0x3F tergantung modul

  // Firebase objects
  FirebaseData fbdo;
  FirebaseData streampath;
  FirebaseAuth auth;
  FirebaseConfig config;

  // Hardware objects
  MPU6050 mpu6050(Wire);
  Servo servoLeft;
  Servo servoRight;

  // State
  bool isCalibrating = false;
  bool isOperating = false;
  unsigned long lastTelemetryMillis = 0;
  const unsigned long TELEMETRY_INTERVAL_MS = 2000;
  float boardWidthCm = 240.0;
  float boardHeightCm = 120.0;
  float startAngle = 0.0;
  bool debugEnabled = true;
  
  // Forward declarations
  void runZigZagOnce();
  void performTurnAndAdvance(int passNumber);
  void doCalibration();
  void saveBoardSize(float w, float h, float strip);
  void loadBoardSize();
  void saveCalibrationDone(bool v);
  bool getCalibrationDone();
  int batteryPercent(float voltage);
  void sendTelemetry();
  void updateLCD(float voltage, bool operating);
  void onShutdown();

  // ================= UTILITY =================
  void saveWiFi(String ssid, String pass) {
    preferences.begin("wifi", false);
    preferences.putString("ssid", ssid);
    preferences.putString("password", pass);
    preferences.end();
  }

  void loadWiFi() {
    preferences.begin("wifi", true);
    saved_ssid = preferences.getString("ssid", "");
    saved_password = preferences.getString("password", "");
    preferences.end();
  }

  float vbat = readBatteryVoltage();
  int percent = batteryPercent(vbat);

  // ================= SERVOS =================
void stopMotors() {
  servoLeft.writeMicroseconds(SERVO_STOP_LEFT);
  servoRight.writeMicroseconds(SERVO_STOP_RIGHT);
  delay(200);
}

void driveForward() {
  servoLeft.writeMicroseconds(SERVO_FORWARD_LEFT);
  servoRight.writeMicroseconds(SERVO_FORWARD_RIGHT);
}

void driveBackward() {
  servoLeft.writeMicroseconds(SERVO_BACK_LEFT);
  servoRight.writeMicroseconds(SERVO_BACK_RIGHT);
}

void spinLeftInPlace() {
  servoLeft.writeMicroseconds(SERVO_BACK_LEFT);
  servoRight.writeMicroseconds(SERVO_FORWARD_RIGHT);
}

void spinRightInPlace() {
  servoLeft.writeMicroseconds(SERVO_FORWARD_LEFT);
  servoRight.writeMicroseconds(SERVO_BACK_RIGHT);
}

void correctDirection() {
  mpu6050.update();
  float currentAngle = mpu6050.getAngleZ();
  float angleDiff = currentAngle - startAngle;
  while (angleDiff > 180) angleDiff -= 360;
  while (angleDiff < -180) angleDiff += 360;
  if (abs(angleDiff) > ANGLE_CORRECTION_THRESHOLD) {
    if (angleDiff > 0) {
      servoLeft.writeMicroseconds(SERVO_STOP_LEFT);
      servoRight.writeMicroseconds(SERVO_FORWARD_RIGHT);
    } else {
      servoLeft.writeMicroseconds(SERVO_FORWARD_LEFT);
      servoRight.writeMicroseconds(SERVO_STOP_RIGHT);
    }
      delay(50);
    }
}

// ================= CALIBRATION =================
void doCalibration() {
  Serial.println("=== AUTO BOARD CALIBRATION (FULL SENSOR) START ===");

  isCalibrating = true;
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Sedang");
  lcd.setCursor(0,1);
  lcd.print("Kalibrasi...");

  float maxWidth = 0;
  float maxHeight = 0;

  // ---- Scan Width pakai IR kiri + kanan ----
  float leftDistStart = sensor1.getDistanceFloat();
  Serial.printf("Left distance start: %.1f cm\n", leftDistStart);

  driveForward();
  while (true) {
    float rightDist = sensor3.getDistanceFloat();
    if (rightDist < 12.0) { // ketemu dinding kanan
      stopMotors();
      float rightDistEnd = rightDist;
      maxWidth = leftDistStart + rightDistEnd + stripWidthCm;
      Serial.printf("Right wall detected, Width ≈ %.1f cm\n", maxWidth);
      break;
    }
      delay(50);
    }
  delay(500);

  // putar 90° ke bawah
  spinRightInPlace(); delay(600); stopMotors(); delay(300);

  // ---- Scan Height pakai IR depan ----
  Serial.println("Scanning Height...");
  driveForward();
  while (true) {
    float dist = sensor2.getDistanceFloat();
    if (dist < 12.0) { // tembok bawah ketemu
      stopMotors();
      maxHeight = stripWidthCm; // NOTE: sangat kasar, adjust strategy jika perlu
      Serial.printf("Bottom wall detected, Height ≈ %.1f cm\n", maxHeight);
      break;
    }
    delay(50);
  }
  delay(500);

  // ---- Balik ke kiri (pakai IR kiri) ----
  spinLeftInPlace(); delay(600); stopMotors(); delay(300);
  driveForward();
  while (true) {
    float leftDist = sensor1.getDistanceFloat();
    if (leftDist < 12.0) {
      stopMotors();
      Serial.println("Back to left wall.");
      break;
    }
    delay(50);
  }
  delay(500);

  // ---- Balik ke atas (pakai IR depan) ----
  spinLeftInPlace(); delay(600); stopMotors(); delay(300);
  driveForward();
  while (true) {
    float dist = sensor2.getDistanceFloat();
    if (dist < 12.0) {
      stopMotors();
      Serial.println("Back to top wall.");
      break;
    }
    delay(50);
  }
  delay(500);

  // putar balik ke kanan (posisi awal)
  spinRightInPlace(); delay(1200); stopMotors();

  // ---- Simpan hasil ----
  if (maxWidth > 0 && maxHeight > 0) {
    boardWidthCm = maxWidth;
    boardHeightCm = maxHeight;
    saveBoardSize(boardWidthCm, boardHeightCm, stripWidthCm);
    saveCalibrationDone(true);

    Serial.printf("AUTO CALIB DONE -> Width=%.1f cm, Height=%.1f cm\n", boardWidthCm, boardHeightCm);

    // ==== Kirim hasil kalibrasi ke Firebase ====
    FirebaseJson json;
    json.set("device_id", deviceID);
    json.set("status", "Calibrated");
    json.set("board_width", boardWidthCm);
    json.set("board_height", boardHeightCm);
    json.set("strip_width", stripWidthCm);
    json.set("timestamp", millis());

    if (Firebase.RTDB.setJSON(&fbdo, FIREBASE_TELEMETRY_PATH.c_str(), &json)) {
      Serial.println("Calibration data uploaded to Firebase!");
    } else {
      Serial.println("Failed to upload calibration data: " + fbdo.errorReason());
    }

    //Update LCD
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Kalibrasi Selesai");
    lcd.setCursor(0,1);
    lcd.print("W:" + String((int)boardWidthCm) + " H:" + String((int)boardHeightCm));
    delay(2000);

    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Kembali");
    lcd.setCursor(0,1);
    lcd.print("Posisi Awal");
    delay(2000);
  } else {
    Serial.println("Calibration failed, values not saved.");
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Kalibrasi Gagal");
  }
  isCalibrating = false;
  Serial.println("=== AUTO BOARD CALIBRATION COMPLETE ===");
  }

// ================= PREFERENCES =================
void saveCalibrationDone(bool v) {
  prefs.begin(PREF_NAMESPACE, false);
  prefs.putBool(PREF_KEY_CALIB_DONE, v);
  prefs.end();
}


bool getCalibrationDone() {
  return true;
}

void saveBoardSize(float w, float h, float strip) {
  prefs.putFloat(PREF_KEY_BOARD_W, w);
  prefs.putFloat(PREF_KEY_BOARD_H, h);
  prefs.putFloat(PREF_KEY_STRIP_CM, strip);
}

void loadBoardSize() {
  boardWidthCm = prefs.getFloat(PREF_KEY_BOARD_W, boardWidthCm);
  boardHeightCm = prefs.getFloat(PREF_KEY_BOARD_H, boardHeightCm);
  stripWidthCm = prefs.getFloat(PREF_KEY_STRIP_CM, stripWidthCm);
}

void onShutdown(){
  String lastTime = prefs.getString("last_time", "unknown");

  // Hitung uptime dari selisih millis()
  unsigned long uptimeMillis = millis() - bootMillis;
  unsigned long seconds = uptimeMillis / 1000;
  unsigned long hours = seconds / 3600;
  seconds %= 3600;
  unsigned long minutes = seconds / 60;
  seconds %= 60;

  char uptimeStr[25];
  sprintf(uptimeStr, "%02lu jam %02lu menit %02lu detik", hours, minutes, seconds);

  // Simpan ke Firebase
  Firebase.RTDB.setString(&fbdo, (FIREBASE_ROOT_PATH + "/last_shutdown").c_str(), lastTime);
  Firebase.RTDB.setString(&fbdo, (FIREBASE_ROOT_PATH + "/uptime").c_str(), uptimeStr);

  Serial.println("🕒 Waktu hidup terakhir disimpan:");
  Serial.println("Last shutdown: " + lastTime);
  Serial.println("Uptime: " + String(uptimeStr));
}

// ================= MAIN SETUP & LOOP =================
void handleRoot() {
  String html = R"rawliteral(
    <!DOCTYPE html><html><head><meta charset="UTF-8">
    <title>Setup WiFi</title></head><body>
    <h2>Masukkan WiFi</h2>
    <form action="/setwifi" method="GET">
      SSID: <input name="ssid"><br>
      Password: <input name="pass"><br>
      <input type="submit" value="Simpan">
    </form>
    </body></html>
  )rawliteral";
  server.send(200, "text/html", html);
}

void handleSetWiFi() {
  if (server.hasArg("ssid") && server.hasArg("pass")) {
    saved_ssid = server.arg("ssid");
    saved_password = server.arg("pass");
    saveWiFi(saved_ssid, saved_password);

    String html = R"rawliteral(
      <!DOCTYPE html>
      <html><head><meta charset="UTF-8"><title>WiFi Saved</title>
      <meta http-equiv="refresh" content="2;url=wera://success"></head><body>
      <h2>✅ WiFi berhasil disimpan</h2><p>ESP32 akan restart...</p></body></html>
    )rawliteral";
    server.send(200, "text/html", html);
    delay(1500);
    ESP.restart();
  } else {
    server.send(400, "text/plain", "SSID & Password tidak ditemukan!");
  }
}

void notifyAppResetStatus(const String &status) {
  FirebaseJson json;
  json.set("device_id", deviceID);
  json.set("reset_status", status);
  json.set("timestamp", millis());

  if (Firebase.RTDB.setJSON(&fbdo, ("/rtdb_devices/" + deviceID + "/reset_status").c_str(), &json)) {
    Serial.println("✅ Reset status dikirim: " + status);
  } else {
    Serial.println("❌ Gagal kirim reset status: " + fbdo.errorReason());
  }
}

void doReset(){
  Serial.println("=== FACTORY RESET DIPANGGIL ===");
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Factory Reset");
  lcd.setCursor(0,1);
  lcd.print("Please reconnect");

  // Hapus WiFi credentials
  preferences.begin("wifi", false);
  preferences.clear();
  preferences.end();

  // Hapus data kalibrasi
  prefs.begin("wera_prefs", false);
  prefs.clear();
  prefs.end();

  WiFi.disconnect(true, true);
  WiFi.mode(WIFI_OFF);
  delay(100);

  WiFi.softAP(AP_SSID, AP_PASS);
  IPAddress apIP = WiFi.softAPIP();
  dnsServer.start(DNS_PORT, "*", apIP);

  server.on("/", handleRoot);
  server.on("/setwifi", handleSetWiFi);
  server.begin();
    
  Serial.println("Semua preferences terhapus. AP Mode aktif lagi!");
  Serial.printf("AP SSID: %s | PASS: %s\n", AP_SSID, AP_PASS);
  Serial.printf("AP IP: %s\n", apIP.toString().c_str());

  delay(1500);
  ESP.restart();
}

void doResetPref(){
  notifyAppResetStatus("Reset dimulai");
  Firebase.RTDB.setString(&fbdo, FIREBASE_STATUS_PATH.c_str(), "Resetting...");
  delay(1000);
  Firebase.RTDB.setString(&fbdo, FIREBASE_STATUS_PATH.c_str(), "Reset Done");
  notifyAppResetStatus("Reset Done");

  prefs.begin("wera_prefs", false);
  prefs.clear();
  prefs.end();
    
  delay(1500);
  ESP.restart();
}

void doResetWiFi(){
  Serial.println("Fungsi Reset WiFi dipanggil..");
  preferences.begin("wifi", false);
  preferences.clear();
  preferences.end();

  WiFi.disconnect(true, true);
  WiFi.mode(WIFI_OFF);
  delay(100);

  WiFi.softAP(AP_SSID, AP_PASS);
  IPAddress apIP = WiFi.softAPIP();
  dnsServer.start(DNS_PORT, "*", apIP);

  server.on("/", handleRoot);
  server.on("/setwifi", handleSetWiFi);
  server.begin();
}

void updateWiFiFromFirebase(const String &newSSID, const String &newPASS) {
  Serial.println("📡 Update WiFi dari Firebase diterima!");
  Serial.println("SSID baru: " + newSSID);
  Serial.println("PASS baru: " + newPASS);

  // Simpan ke Preferences
  preferences.begin("wifi", false);
  preferences.putString("ssid", newSSID);
  preferences.putString("password", newPASS);
  preferences.end();

  // Update status Firebase
  Firebase.RTDB.setString(&fbdo, FIREBASE_STATUS_PATH.c_str(), "Updating WiFi...");

  // Disconnect dan connect ulang
  WiFi.disconnect(true, true);
  delay(1000);
  WiFi.begin(newSSID.c_str(), newPASS.c_str());

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
    delay(500);
    Serial.print(".");
  }


  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ Terhubung ke WiFi baru: " + newSSID);
    Firebase.RTDB.setString(&fbdo, FIREBASE_STATUS_PATH.c_str(), "Connected");

    // 🧹 Hapus node WiFi dari Firebase biar gak update terus
    String wifiPath = "/rtdb_devices/" + deviceID + "/wifi";
    if (Firebase.RTDB.deleteNode(&fbdo, wifiPath.c_str())) {
      Serial.println("✅ Node WiFi dihapus dari RTDB setelah update!");
    } else {
      Serial.println("❌ Gagal hapus node WiFi: " + fbdo.errorReason());
    }

    delay(1000);
    ESP.restart();
  } else {
    Serial.println("\n❌ Gagal konek ke WiFi baru");
    Firebase.RTDB.setString(&fbdo, FIREBASE_STATUS_PATH.c_str(), "WiFi Failed");
  }

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("WiFi terhubung ke");
  lcd.setCursor(0,1);
  lcd.print(newSSID);
}

// ================= STATE HANDLER =================
void startOperation() {
  if (isOperating) {
    Serial.println("⚠️ Sudah dalam mode operasi, abaikan start baru");
    return;
  }
  if (!getCalibrationDone()) {
    Serial.println("❌ Tidak bisa mulai operasi sebelum kalibrasi!");
    lcd.clear();
    lcd.setCursor(0,0); lcd.print("Kalibrasi Dulu!");
    lcd.setCursor(0,1); lcd.print("Ucap: 'Kalibrasi'");
    Firebase.RTDB.setString(&fbdo, FIREBASE_STATUS_PATH.c_str(), "Need Calibration");
    return;
  }

  isOperating = true;
  operationStartMillis = millis();
  operationCycleCount++;
  String startTime = getTimeNow();

  // ID unik tiap siklus
  char cycleID[20];
  sprintf(cycleID, "cycle_%04d", operationCycleCount);

  // Simpan ke Firebase
  String cyclePath = FIREBASE_ROOT_PATH + "/operation_cycles/" + String(cycleID);
  Firebase.RTDB.setString(&fbdo, cyclePath + "/start_time", startTime);
  Firebase.RTDB.setString(&fbdo, cyclePath + "/status", "Running");

  Firebase.RTDB.setString(&fbdo, FIREBASE_STATUS_PATH.c_str(), "Operating");

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Operasi dimulai");
  lcd.setCursor(0,1);
  lcd.print(startTime);

  Serial.printf("🟢 Operasi #%d dimulai pada %s\n", operationCycleCount, startTime.c_str());
  updateLCD(readBatteryVoltage(), isOperating);
}


void stopOperation() {
  if (!isOperating) {
    Serial.println("⚠️ Sudah berhenti, abaikan stop baru");
    return;
  }

  isOperating = false;
  stopMotors();

  unsigned long elapsed = millis() - operationStartMillis;
  unsigned long seconds = elapsed / 1000;
  unsigned long hours = seconds / 3600;
  seconds %= 3600;
  unsigned long minutes = seconds / 60;
  seconds %= 60;

  char durationStr[30];
  sprintf(durationStr, "%02lu jam %02lu menit %02lu detik", hours, minutes, seconds);

  String endTime = getTimeNow();

  // ID cycle aktif
  char cycleID[20];
  sprintf(cycleID, "cycle_%04d", operationCycleCount);

  // Simpan ke Firebase
  String cyclePath = FIREBASE_ROOT_PATH + "/operation_cycles/" + String(cycleID);
  Firebase.RTDB.setString(&fbdo, cyclePath + "/end_time", endTime);
  Firebase.RTDB.setString(&fbdo, cyclePath + "/duration", durationStr);
  Firebase.RTDB.setString(&fbdo, cyclePath + "/status", "Completed");

  Firebase.RTDB.setString(&fbdo, FIREBASE_STATUS_PATH.c_str(), "Idle");

  // LCD info
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Operasi Selesai");
  lcd.setCursor(0,1);
  lcd.print(String(hours) + "j" + String(minutes) + "m" + String(seconds) + "d");
  
  Serial.printf("🔵 Operasi #%d selesai: %s\nDurasi: %s\n", 
                operationCycleCount, endTime.c_str(), durationStr);

  updateLCD(readBatteryVoltage(), isOperating);
}

// ================= FIREBASE CALLBACKS & TELEMETRY =================
void streamCallback(FirebaseStream data) {
  String path = data.dataPath();
  String dtype = data.dataType();
  Serial.printf("📡 Stream event -> path: %s | type: %s | payload: %s\n",
                 path.c_str(), dtype.c_str(), data.payload().c_str());

  // 🔹 RESET STREAM
  if (path.endsWith("/reset")) {
    if (dtype == "boolean" && data.boolData()) {
      Firebase.RTDB.setBool(&fbdo, (FIREBASE_ROOT_PATH + "/reset_status/reset").c_str(), false);
      Serial.println("🧹 Firebase: Reset pref triggered!");
      doResetPref();
    }
  }

  else if (path.endsWith("/reset_factory")){
    if (dtype == "boolean" && data.boolData()){
      Firebase.RTDB.setBool(&fbdo, (FIREBASE_ROOT_PATH + "/reset_status/reset_factory").c_str(), false);
      Serial.println("🧹 Firebase: Reset Factory triggered!");
      doReset();
    }
  }
  
  // 🔹 WIFI STREAM
  else if (path.endsWith("/wifi")) {
  if (dtype == "json") {
    FirebaseJson wifiData;
    wifiData.setJsonData(data.payload());

    FirebaseJsonData ssidData, passData;
    wifiData.get(ssidData, "ssid");
    wifiData.get(passData, "pass");

    String newSSID = ssidData.stringValue;
    String newPASS = passData.stringValue;

    updateWiFiFromFirebase(newSSID, newPASS);
  } 
  else if (path.endsWith("/pass") && dtype == "string") {
    String newSSID = data.stringData();
    Firebase.RTDB.getString(&fbdo, ("/rtdb_devices/" + deviceID + "/wifi/pass").c_str());
    String newPASS = fbdo.stringData();
    updateWiFiFromFirebase(newSSID, newPASS);
  } 
  else if (path.endsWith("/ssid") && dtype == "string") {
    String newPASS = data.stringData();
    Firebase.RTDB.getString(&fbdo, ("/rtdb_devices/" + deviceID + "/wifi/ssid").c_str());
    String newSSID = fbdo.stringData();
    updateWiFiFromFirebase(newSSID, newPASS);
  }
}

  // 🔹 COMMAND STREAM
  else if (path.endsWith("/command")) {
    if (dtype == "string") {
      String cmd = data.stringData();
      cmd.trim(); cmd.toLowerCase();
      Serial.println("🔥 Command diterima dari Firebase: " + cmd);

      if (cmd == "start") {
        startOperation();
      } else if (cmd == "stop") {
        stopOperation();
      } else if (cmd == "calibrate") {
        doCalibration();
      } else if (cmd == "reset") {
        doReset();
      }
    }
  }
}

void streamTimeoutCallback(bool timeout) {
  if (timeout) {
    Serial.println("Firebase stream timeout, will attempt reconnect.");
  }
}

int batteryPercent(float voltage) {
  float diff = BATT_MAX_VOLTAGE - BATT_MIN_VOLTAGE;
  if (diff <= 0.0) return 0; // 🚨 hindari divide by zero
  if (voltage <= BATT_MIN_VOLTAGE) return 0;
  if (voltage >= BATT_MAX_VOLTAGE) return 100;
  return (int)((voltage - BATT_MIN_VOLTAGE) * 100 / diff);
}

void sendTelemetry() {
  if (millis() - lastTelemetryMillis < TELEMETRY_INTERVAL_MS) return;
  lastTelemetryMillis = millis();

  float frontDist = sensor2.getDistanceFloat();
  float rightDist = sensor3.getDistanceFloat();
  float leftDist = sensor1.getDistanceFloat();
  mpu6050.update();
  float yaw = mpu6050.getAngleZ();
  float ax = mpu6050.getAccX();
  float ay = mpu6050.getAccY();
  float az = mpu6050.getAccZ();
  float gx = mpu6050.getGyroX();
  float gy = mpu6050.getGyroY();
  float gz = mpu6050.getGyroZ();

  String statusStr;
  if (isCalibrating) {
    statusStr = "Calibrating";
  } else if (vbat < BATT_MIN_VOLTAGE) {
    statusStr = "Low Battery";
  } else if (isOperating) {
    statusStr = "Operating";
  } else {
    statusStr = "Idle";
}

  FirebaseJson json;
  json.set("timestamp", millis());
  json.set("device_id", deviceID);
  json.set("status", statusStr);
  json.set("battery_percent", percent);
  json.set("battery_voltage", vbat);
  json.set("front_distance_cm", frontDist);
  json.set("right_distance_cm", rightDist);
  json.set("left_distance_cm", leftDist);
  json.set("yaw_angle", yaw);
  json.set("acc_x", ax);
  json.set("acc_y", ay);
  json.set("acc_z", az);
  json.set("gyro_x", gx);
  json.set("gyro_y", gy);
  json.set("gyro_z", gz);
  json.set("is_operating", isOperating);
  json.set("board_width", boardWidthCm);
  json.set("board_height", boardHeightCm);

  if (Firebase.RTDB.setJSON(&fbdo, FIREBASE_TELEMETRY_PATH.c_str(), &json)) {
    if (debugEnabled){
    Serial.printf("Telemetry -> %s\n", FIREBASE_TELEMETRY_PATH.c_str());
    Serial.printf("  Battery: %.2fV (%d%%)\n", vbat, percent);
    Serial.printf("  IR Front: %.1f cm | Left: %.1f cm | Right: %.1f cm\n", frontDist, leftDist, rightDist);
    Serial.printf("  MPU Yaw: %.1f°\n", yaw);
    Serial.printf("  Acc (X,Y,Z): %.2f, %.2f, %.2f g\n", ax, ay, az);
    Serial.printf("  Gyro (X,Y,Z): %.2f, %.2f, %.2f °/s\n", gx, gy, gz);
    }

    // Update node utama untuk monitoring aplikasi
    Firebase.RTDB.setString(&fbdo, FIREBASE_STATUS_PATH.c_str(), statusStr);
  } else {
    Serial.println("Failed send telemetry: " + fbdo.errorReason());
  }
}

// ================= ZIGZAG ALGORITHM (FIXED STOP) =================
void runZigZagOnce() {
  Serial.println(F("Starting ZigZag erasing pattern..."));
  mpu6050.update();
  startAngle = mpu6050.getAngleZ();

  int passes = (int)ceil(boardHeightCm / stripWidthCm);
  Serial.println("Calculated passes needed: " + String(passes));

  for (int pass = 0; pass < passes && isOperating; pass++) {
    Serial.println("=== PASS " + String(pass + 1) + " of " + String(passes) + " ===");
    unsigned long passStartTime = millis();
    driveForward();

    while (isOperating) {
      yield();  // biar loop gak blocking
      if (!isOperating) { // 🔹 kalau stopOperation() dipanggil
        Serial.println("🟥 Operation aborted by user!");
        stopMotors();
        return;
      }

      // Safety voltage check
      if (vbat < BATT_MIN_VOLTAGE || vbat > BATT_MAX_VOLTAGE) {
        Serial.println("⚠️ BATTERY ALERT: " + String(vbat) + "V - Stopping!");
        stopMotors();
        isOperating = false;
        return;
      }

      float frontDist = sensor2.getDistanceFloat();
      if (frontDist < DIST_FRONT_THRESHOLD_CM) {
        Serial.println("Reached board edge, front distance: " + String(frontDist));
        stopMotors();
        delay(200);
        break;
      }

      correctDirection();

      if (abs(mpu6050.getAngleZ() - startAngle) <= ANGLE_CORRECTION_THRESHOLD) {
        driveForward();
      }

      if (millis() - passStartTime > 120000) {
        Serial.println("⏱ Timeout per pass - stopping motors");
        stopMotors();
        break;
      }

      delay(50);
    }

    if (!isOperating) {
      stopMotors();
      Serial.println("🟥 Operation stopped mid-pass");
      return;
    }

    delay(DELAY_AFTER_STRIP_MS);
    if (pass == passes - 1) break;

    performTurnAndAdvance(pass);
  }

  stopMotors();
  Serial.println("✅ ZigZag pattern completed!");
  Firebase.RTDB.setString(&fbdo, FIREBASE_STATUS_PATH.c_str(), "Completed");
  isOperating = false;
}

// ==================== TURN & ADVANCE (v2 - with STOP check) ====================
void performTurnAndAdvance(int passNumber) {
  if (!isOperating) return;
  Serial.println("Performing 90° zigzag turn...");

  bool turnRight = (passNumber % 2 == 0);
  mpu6050.update();
  float initialAngle = mpu6050.getAngleZ();

  // === STEP 1: Rotasi 90° ke arah bawah ===
  float targetAngle1 = initialAngle + (turnRight ? 90.0f : -90.0f);
  while (isOperating) {
    yield();
    if (!isOperating) { stopMotors(); return; }

    mpu6050.update();
    float current = mpu6050.getAngleZ();
    float diff = fabs(current - targetAngle1);
    if (diff < 5.0f) break;

    if (turnRight) spinRightInPlace();
    else spinLeftInPlace();
    delay(50);
  }
  stopMotors();
  if (!isOperating) return;
  delay(200);

  // === STEP 2: Maju sedikit untuk turun satu strip ===
  Serial.println("Advancing one strip downward...");
  driveForward();
  delay(ADVANCE_DELAY_MS);
  stopMotors();
  if (!isOperating) return;
  delay(200);

  // === STEP 3: Rotasi 90° lagi untuk arah balik ===
  mpu6050.update();
  float targetAngle2 = targetAngle1 + (turnRight ? 90.0f : -90.0f);
  while (isOperating) {
    yield();
    if (!isOperating) { stopMotors(); return; }

    mpu6050.update();
    float current = mpu6050.getAngleZ();
    float diff = fabs(current - targetAngle2);
    if (diff < 5.0f) break;

    if (turnRight) spinRightInPlace();
    else spinLeftInPlace();
    delay(50);
  }
  stopMotors();
  if (!isOperating) return;
  delay(200);

  // === STEP 4: Update arah baru ===
  mpu6050.update();
  startAngle = mpu6050.getAngleZ();
  Serial.printf("✅ Turn complete, facing new direction (%.2f°)\n", startAngle);
}


void setup() {
  // Serials
  Serial.begin(115200);
  delay(800);
  
  // === ADC setup (pastikan scaling tepat) ===
  pinMode(PIN_BATT_ADC, INPUT);
  analogSetPinAttenuation(PIN_BATT_ADC, ADC_11db); // supaya full-scale 3.3–3.9 V
  analogReadResolution(12);                         // 12-bit (0–4095)

  Serial.println("Battery ADC initialized (ADC_11db, 12-bit)");

  // optional: print debug awal
  Serial.printf("Initial battery read: %.2f V\n", vbat);

  // prefs for calibration (must init before using prefs functions)
  prefs.begin(PREF_NAMESPACE, false);

  Serial.println("\n=== WERA ESP32 Starting ===");
  Wire.begin();
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("WERA Starting...");
  delay(1500);
  lcd.clear();

  // DFRobot voice module init (uses Serial1)
  while (!(asr.begin())){
    Serial.println("komunikasi dengan perangkat gagal, tolong periksa koneksi");
    delay(3000);
  }
  Serial.println("Mulai ok!");

  asr.settingCMD(DF2301Q_UART_MSG_CMD_SET_MUTE, 0);
  asr.settingCMD(DF2301Q_UART_MSG_CMD_SET_WAKE_TIME, 10);

  // load wifi
  loadWiFi();

  // If no credentials -> AP mode for setup
  if (saved_ssid == "" || saved_password == "") {
    apMode = true;
    WiFi.softAP(AP_SSID, AP_PASS);
    IPAddress apIP = WiFi.softAPIP();
    dnsServer.start(DNS_PORT, "*", apIP);

    server.on("/", handleRoot);
    server.on("/setwifi", handleSetWiFi);
    server.begin();
    Serial.println("AP Mode aktif! Connect ke SSID: " + String(AP_SSID));
    Serial.printf("AP IP: %s\n", WiFi.softAPIP().toString().c_str());
    lcd.setCursor(0,0);
    lcd.println("Silahkan scan QR");
    lcd.setCursor(0,1);
    lcd.println("melalui app!! ");
    return;
  }

  // Try connect to WiFi
  WiFi.begin(saved_ssid.c_str(), saved_password.c_str());
  Serial.print("Menghubungkan ke WiFi");
  int timeout = 30;
  while (WiFi.status() != WL_CONNECTED && timeout-- > 0) {
    delay(500);
    Serial.print(".");
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nGagal konek WiFi, masuk AP Mode (clearing prefs and restart)");
    preferences.begin("wifi", false);
    preferences.clear();
    preferences.end();
    delay(500);
    ESP.restart();
  }
  Serial.println("\nTerhubung ke WiFi: " + WiFi.localIP().toString());

  // load calibration data from prefs
  loadBoardSize();

  // initialize hardware
  servoLeft.attach(PIN_SERVO_LEFT);
  servoRight.attach(PIN_SERVO_RIGHT);
  stopMotors();

  mpu6050.begin();
  delay(100);
  mpu6050.calcGyroOffsets(true);
  Serial.println("MPU6050 initialized and calibrated");

  // Firebase init
  config.database_url = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  prefs.begin("wera", false);
  String now = getTimeNow();
  bootMillis = millis();

  // Tampilkan di LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Last Boot:");
  lcd.setCursor(0, 1);
  lcd.print(now);

  // Simpan ke Firebase
  String bootPath = (FIREBASE_ROOT_PATH + "/last_boot");
  if (Firebase.RTDB.setString(&fbdo, bootPath.c_str(), now)) {
    Serial.printf("Boot time tersimpan: %s\n", now.c_str());
  } else {
    Serial.println(fbdo.errorReason());
  }

  prefs.putString("last_time", now);
  esp_register_shutdown_handler(onShutdown);
  
  if(WiFi.status() == WL_CONNECTED){
    Firebase.RTDB.setString(&fbdo, FIREBASE_CONSTAT_PATH.c_str(), "online");
    Serial.println("Device registered di Firebase!");
  } else {
    Firebase.RTDB.setString(&fbdo, FIREBASE_CONSTAT_PATH.c_str(), "offline");
    Serial.println("Device offline (tidak terhubung internet)");
  }

  // Start stream listener for commands
  if (!Firebase.RTDB.beginStream(&streampath, FIREBASE_ROOT_PATH.c_str())) {
  Serial.println("Gagal mulai Stream : " + streampath.errorReason());
} else {
  Firebase.RTDB.setStreamCallback(&streampath, streamCallback, streamTimeoutCallback);
  Serial.println("✅ Stream aktif untuk path: " + FIREBASE_ROOT_PATH);
}

  lastTelemetryMillis = millis();

  // Calibration check
  if (!getCalibrationDone()) {
    calibrationRequired = true;
    Serial.println("\n!!! CALIBRATION REQUIRED !!!");
    Serial.println("Ketik 'calibrate' di Serial Monitor atau ucapkan perintah suara untuk memulai kalibrasi.");

    // Tambahin ke LCD biar user lihat langsung
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Kalibrasi Wajib");
    lcd.setCursor(0,1);
    lcd.print("Ucap: 'Lakukan Kalibrasi'");
  } else {
    Serial.println("System already calibrated");

    // Info di LCD juga
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Kalibrasi OK");
    lcd.setCursor(0,1);
    lcd.print("Siap Operasi");
    delay(2000);
  }

  Serial.println("\n=== WERA Ready for Operation ===");
  Serial.println("Board size: " + String(boardWidthCm) + " x " + String(boardHeightCm) + " cm");
  Serial.println("Strip width: " + String(stripWidthCm) + " cm");
  Serial.println("Send 'start' command via Firebase or Serial to begin erasing");
}

void loop() {
  if (apMode) {
    dnsServer.processNextRequest();
    server.handleClient();
    return;
  }

  unsigned long now = millis();
  
  //cek baterai
  if (now - lastADCRead >= 1000 ){
    lastADCRead = now;
    float newVbat = readBatteryVoltage();
    if(fabs(newVbat - vbat) > 0.03){
      vbat = newVbat;
      percent = batteryPercent(vbat);
    }
  }

  // ====== FIREBASE TELEMETRY (setiap 2s) ======
  if (now - lastFirebaseSend >= 2000) {
    lastFirebaseSend = now;
    static bool fbBusy = false;
    if (!fbBusy) {
      fbBusy = true;
      sendTelemetry();
      yield();  // beri waktu WiFi task
      fbBusy = false;
    }
  }

  // ====== LCD UPDATE (maks 1x per detik) ======
  if (now - lastLCDUpdate >= 1000) {
    lastLCDUpdate = now;
    updateLCD(vbat, isOperating || isCalibrating);
  }

  // ====== CONNECTION STATUS CHECK (10s) ======
  if (now - lastConnCheck >= 10000) {
    lastConnCheck = now;
    String status = (WiFi.status() == WL_CONNECTED) ? "online" : "offline";
    Firebase.RTDB.setString(&fbdo, FIREBASE_CONSTAT_PATH.c_str(), status);
  }

  // ====== SAVE LAST TIME TO PREFS (30s) ======
  if (now - lastPrefsSave >= 30000) {
    lastPrefsSave = now;
    prefs.putString("last_time", getTimeNow());
  }
  
  vbat = readBatteryVoltage();
  percent = batteryPercent(vbat);

    // handle Serial test commands
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    cmd.toLowerCase();    
    if (cmd == "start") {
      startOperation();
      Serial.println("Manual START received.");
    } else if (cmd == "stop") {
      stopOperation();
      Serial.println("Manual STOP received.");
    } else if (cmd == "test") {
      Serial.println("Movement test...");
      driveForward(); delay(5000); stopMotors(); delay(1000);
      spinRightInPlace(); delay(5000); stopMotors(); delay(1000); spinLeftInPlace(); delay(5000); stopMotors(); delay(1000);
      driveBackward(); delay(5000); stopMotors();
      Serial.println("Test done");
    } else if (cmd == "status") {
      Serial.printf("Status - Operating: %s, Battery: %.2fV\n", isOperating ? "YES" : "NO", readBatteryVoltage());
    } else if (cmd == "calibrate") {
      doCalibration();
      Serial.println("Kalibrasi dilakukan....");
    } else if (cmd == "reset"){
      doReset();
      Serial.println("Reset dilakukan....");
    } else if (cmd == "debug on") {
      debugEnabled = true;
      Serial.println("✅ Debug mode ENABLED");
    } else if (cmd == "debug off") {
      debugEnabled = false;
      Serial.println("❌ Debug mode DISABLED");
    }
  }

  uint8_t CMDID = asr.getCMDID();
  switch (CMDID){
    case 5: //Mulai Operasi
      startOperation();
      break;
    case 6: //Stop Operasi
      stopOperation();
      break;
    case 7: //Test Pergerakan
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Test Pergerakan");
      driveForward(); delay(5000); stopMotors(); delay(1000);
      spinRightInPlace(); delay(5000); stopMotors(); delay(1000); spinLeftInPlace(); delay(5000); stopMotors(); delay(1000);
      driveBackward(); delay(5000); stopMotors();
      Serial.println("Test done");
      break;
    case 8: //Cek Status
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("V:" + String(vbat, 2) + "V");  // tampilkan tegangan 2 digit desimal  
      Serial.printf("Status - Operating: %s, Battery: %.2fV\n", isOperating ? "YES" : "NO", percent  + "%");
      delay(5000);
      break;
    case 9: //Lakukan Kalibrasi
      Serial.println("Kalibrasi dilakukan");
      doCalibration();
      break;
    case 10: //Lakukan Reset
      Serial.println("Reset dilakukan");
      doReset();
      break;
    case 11: //Reset WiFi
      Serial.println("WiFi Berhasil direset...");
      doResetWiFi();
      break;
    default:
      if (CMDID != 0){
        Serial.print("CMDID = ");
        Serial.println(CMDID);
      }
  }
  delay(300);

  if (calibrationRequired){
    sendTelemetry();
    updateLCD(vbat, false);
    delay(200);
    return;
  }

  static unsigned long lastCheck = 0;
  if (millis() - lastCheck > 5000) {
    lastCheck = millis();
    if (checkInternet()) {
      Firebase.RTDB.setString(&fbdo, FIREBASE_CONSTAT_PATH.c_str(), "online");
    } else {
      Firebase.RTDB.setString(&fbdo, FIREBASE_CONSTAT_PATH.c_str(), "offline");
    }
  }

 static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate > 10000) {
    lastUpdate = millis();
    String now = getTimeNow();
    prefs.putString("last_time", now);
    Serial.println("Waktu terakhir diperbarui: " + now);
  }

  // ====== BATTERY SAFETY ======
  if (vbat < BATT_MIN_VOLTAGE && isOperating) {
    Serial.println("CRITICAL: Battery low, stopping...");
    stopMotors();
    isOperating = false;

    FirebaseJson lowBat;
    lowBat.set("timestamp", getTimeNow());
    lowBat.set("voltage", vbat);
    Firebase.RTDB.pushJSON(&fbdo, (FIREBASE_ROOT_PATH + "/events/low_battery").c_str(), &lowBat);
  }

  // telemetry
  sendTelemetry();

  //update LCD tampilan status
  updateLCD(vbat, isOperating || isCalibrating);

  // main operation (hanya jalankan sekali tiap start)
  static bool operationStarted = false;
  if (isOperating && !operationStarted) {
    Serial.println("Starting erasing operation...");
    operationStarted = true;
    runZigZagOnce();
    operationStarted = false;
  }


  yield();
}

void updateLCD(float voltage, bool operating){
  static String lastLine1 = "", lastLine2 = "";
  String line1, line2;

  if (isCalibrating) {
    line1 = "Sedang Kalibrasi";
    line2 = "Please wait...";
  } else if (voltage < BATT_MIN_VOLTAGE) {
    line1 = "LOW BATTERY";
    line2 = "Battery: " + String(percent) + "%";
  } else if (operating) {
    line1 = "Status:Operating";
    line2 = "Battery: " + String(percent) + "%";
  } else {
    line1 = "Status: IDLE";
    line2 = "Battery: " + String(percent) + "%";
  }

  if (line1 != lastLine1 || line2 != lastLine2) {
    lcd.clear();
    lcd.setCursor(0,0); lcd.print(line1);
    lcd.setCursor(0,1); lcd.print(line2);
    lastLine1 = line1;
    lastLine2 = line2;
  }
}
