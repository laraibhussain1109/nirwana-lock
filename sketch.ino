#include <WiFi.h>
#include <WebServer.h>
#include <WiFiManager.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <Adafruit_Fingerprint.h>
#include <HardwareSerial.h>
#include <Preferences.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

Preferences prefs;

// =====================================================
// LCD
// =====================================================

LiquidCrystal_I2C lcd(0x27, 16, 2);

// =====================================================
// FINGERPRINT
// =====================================================

HardwareSerial fingerSerial(2);

Adafruit_Fingerprint finger(&fingerSerial);

// =====================================================
// WEB SERVER
// =====================================================

WebServer server(80);

// =====================================================
// PINS
// =====================================================

#define BUZZER 4

#define MOTOR1 15
#define MOTOR2 19

#define LIMIT_OPEN 5
#define LIMIT_CLOSE 23

#define ADMIN_BUTTON 14

// =====================================================
// KEYPAD
// =====================================================

const byte ROWS = 4;
const byte COLS = 3;

char keys[ROWS][COLS] =
{
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};

byte rowPins[ROWS] = {27,13,32,25};

byte colPins[COLS] = {26,12,33};

Keypad keypad = Keypad(
makeKeymap(keys),
rowPins,
colPins,
ROWS,
COLS
);

// =====================================================
// VARIABLES
// =====================================================


// CLOUD CONTROL (PythonAnywhere)
String cloudHost = "https://truecollabs.pythonanywhere.com";
String cloudDeviceId = "esp32-gate-01";
String cloudToken = "CHANGE_ME_TOKEN";
unsigned long lastCloudPoll = 0;
unsigned long lastHeartbeat = 0;

void cloudHeartbeat();
void cloudPollCommand();

String password = "123456";

String entered = "";

String gateStatus = "LOCKED";

String fingerStatus = "READY";

bool fingerGateMode = false;

unsigned long fingerModeStart = 0;

int id = 0;

// =====================================================

void beep(int t)
{
  digitalWrite(BUZZER, HIGH);

  delay(t);

  digitalWrite(BUZZER, LOW);
}

// =====================================================

void lcdFast(String l1, String l2 = "")
{
  static String old1 = "";
  static String old2 = "";

  if(old1 == l1 && old2 == l2)
  return;

  old1 = l1;
  old2 = l2;

  lcd.clear();

  lcd.setCursor(0,0);
  lcd.print(l1);

  lcd.setCursor(0,1);
  lcd.print(l2);
}

// =====================================================

void motorStop()
{
  digitalWrite(MOTOR1, LOW);

  digitalWrite(MOTOR2, LOW);
}

// =====================================================

void gateOpen()
{
  gateStatus = "OPENING";

  lcdFast("GATE OPENING");

  digitalWrite(MOTOR1, HIGH);

  digitalWrite(MOTOR2, LOW);

  unsigned long t = millis();

  while(digitalRead(LIMIT_OPEN)==HIGH)
  {
    yield();

    server.handleClient();

    delay(1);

    if(millis()-t > 15000)
    break;
  }

  motorStop();

  gateStatus = "OPEN";

  lcdFast("GATE OPEN");

  beep(100);
}

// =====================================================

void gateClose()
{
  gateStatus = "CLOSING";

  lcdFast("GATE CLOSING");

  digitalWrite(MOTOR1, HIGH);

  digitalWrite(MOTOR2, LOW);

  unsigned long t = millis();

  while(digitalRead(LIMIT_CLOSE)==HIGH)
  {
    yield();

    server.handleClient();

    delay(1);

    if(millis()-t > 15000)
    break;
  }

  motorStop();

  gateStatus = "LOCKED";

  lcdFast("GATE LOCKED");

  beep(100);
}

// =====================================================

void openCycle()
{
  gateOpen();

  delay(5000);

  gateClose();
}

// =====================================================

void wifiSetup()
{
  WiFiManager wm;

  lcdFast("CONNECT WIFI");

  bool res =
  wm.autoConnect("ESP32_GATE");

  if(res)
  {
    lcdFast(
    "IP ADDRESS",
    WiFi.localIP().toString()
    );

    Serial.println(WiFi.localIP());

    delay(3000);
  }
}

// =====================================================

void addCORS()
{
  server.sendHeader(
  "Access-Control-Allow-Origin",
  "*"
  );
}

// =====================================================

void handleStatus()
{
  addCORS();

  String json = "{";

  json += "\"gate\":\"" + gateStatus + "\",";
  json += "\"password\":\"" + password + "\",";
  json += "\"finger\":\"" + fingerStatus + "\"";

  json += "}";

  server.send(
  200,
  "application/json",
  json
  );
}

// =====================================================

void apiOpen()
{
  addCORS();

  gateOpen();

  server.send(
  200,
  "application/json",
  "{\"status\":\"opened\"}"
  );
}

// =====================================================

void apiClose()
{
  addCORS();

  gateClose();

  server.send(
  200,
  "application/json",
  "{\"status\":\"closed\"}"
  );
}

// =====================================================

void apiPassword()
{
  addCORS();

  if(server.hasArg("pass"))
  {
    String p =
    server.arg("pass");

    if(p.length()==6)
    {
      password = p;

      prefs.putString(
      "pass",
      password
      );

      lcdFast(
      "PASSWORD",
      "UPDATED"
      );

      beep(200);

      server.send(
      200,
      "application/json",
      "{\"password\":\"updated\"}"
      );

      delay(2000);

      lcdFast("ENTER PASS");

      return;
    }
  }

  server.send(
  400,
  "application/json",
  "{\"error\":\"invalid\"}"
  );
}

// =====================================================
// ENROLL FINGER
// =====================================================

uint8_t getFingerprintEnroll()
{
  int p = -1;

  uint8_t check =
  finger.loadModel(id);

  if(check == FINGERPRINT_OK)
  {
    fingerStatus = "ALREADY USED";

    lcdFast(
    "ID ALREADY",
    "USED"
    );

    beep(1000);

    delay(2500);

    fingerStatus = "READY";

    lcdFast("READY");

    return check;
  }

  fingerStatus = "PLACE FINGER";

  lcdFast(
  "PLACE FINGER",
  "ID:"+String(id)
  );

  beep(100);

  delay(2500);

  unsigned long t = millis();

  // =====================================================
  // FIRST SCAN
  // =====================================================

  while (p != FINGERPRINT_OK)
  {
    p = finger.getImage();

    yield();

    server.handleClient();

    delay(5);

    if(p == FINGERPRINT_NOFINGER)
    {
      delay(250);
      continue;
    }

    if(p == FINGERPRINT_IMAGEFAIL)
    {
      lcdFast(
      "FINGER ERROR",
      "PLACE PROPER"
      );

      delay(2500);

      lcdFast(
      "PLACE FINGER",
      "ID:"+String(id)
      );

      p = -1;

      continue;
    }

    if(millis()-t > 30000)
    {
      lcdFast("TIMEOUT");

      delay(2000);

      lcdFast("READY");

      return p;
    }
  }

  lcdFast(
  "PROCESSING",
  "PLEASE WAIT"
  );

  delay(1200);

  p = finger.image2Tz(1);

  if (p != FINGERPRINT_OK)
  {
    lcdFast(
    "BAD FINGER",
    "TRY AGAIN"
    );

    delay(2500);

    lcdFast("READY");

    return p;
  }

  // =====================================================
  // REMOVE FINGER
  // =====================================================

  lcdFast(
  "REMOVE",
  "YOUR FINGER"
  );

  beep(100);

  delay(3000);

  p = finger.getImage();

  t = millis();

  while (p != FINGERPRINT_NOFINGER)
  {
    p = finger.getImage();

    yield();

    server.handleClient();

    delay(200);

    if(millis()-t > 15000)
    {
      lcdFast(
      "REMOVE ERROR",
      "TRY AGAIN"
      );

      delay(2500);

      lcdFast("READY");

      return p;
    }
  }

  // =====================================================
  // SECOND SCAN
  // =====================================================

  lcdFast(
  "PLACE AGAIN",
  "SAME FINGER"
  );

  beep(100);

  delay(2500);

  p = -1;

  t = millis();

  while (p != FINGERPRINT_OK)
  {
    p = finger.getImage();

    yield();

    server.handleClient();

    delay(5);

    if(p == FINGERPRINT_NOFINGER)
    {
      delay(250);
      continue;
    }

    if(p == FINGERPRINT_IMAGEFAIL)
    {
      lcdFast(
      "FINGER ERROR",
      "PLACE PROPER"
      );

      delay(2500);

      lcdFast(
      "PLACE AGAIN",
      "SAME FINGER"
      );

      p = -1;

      continue;
    }

    if(millis()-t > 30000)
    {
      lcdFast("TIMEOUT");

      delay(2000);

      lcdFast("READY");

      return p;
    }
  }

  lcdFast(
  "VERIFYING",
  "PLEASE WAIT"
  );

  delay(1500);

  p = finger.image2Tz(2);

  if (p != FINGERPRINT_OK)
  {
    lcdFast(
    "BAD FINGER",
    "TRY AGAIN"
    );

    delay(2500);

    lcdFast("READY");

    return p;
  }

  delay(700);

  p = finger.createModel();

  if (p != FINGERPRINT_OK)
  {
    lcdFast(
    "NOT MATCHED",
    "TRY AGAIN"
    );

    delay(3000);

    lcdFast("READY");

    return p;
  }

  delay(700);

  p = finger.storeModel(id);

  if (p == FINGERPRINT_OK)
  {
    lcdFast(
    "FINGER STORED",
    "ID:"+String(id)
    );

    beep(600);

    delay(3500);
  }
  else
  {
    lcdFast(
    "STORE FAILED",
    "TRY AGAIN"
    );

    delay(2500);
  }

  lcdFast("READY");

  return p;
}

// =====================================================
// DELETE FINGER
// =====================================================

uint8_t deleteFingerprint(uint8_t id)
{
  uint8_t p = -1;

  lcdFast(
  "DELETING",
  "ID:"+String(id)
  );

  delay(1500);

  p = finger.deleteModel(id);

  if (p == FINGERPRINT_OK)
  {
    lcdFast(
    "DELETE",
    "SUCCESS"
    );

    beep(300);

    delay(2500);
  }
  else
  {
    lcdFast(
    "DELETE FAIL"
    );

    delay(2500);
  }

  lcdFast("READY");

  return p;
}

// =====================================================
// NON BLOCKING FINGER GATE WITH TIMEOUT
// =====================================================

void checkFingerprintGate()
{
  static unsigned long lastScan = 0;

  if(!fingerGateMode)
  return;

  // =====================================================
  // AUTO EXIT
  // =====================================================

  if(millis() - fingerModeStart > 10000)
  {
    fingerGateMode = false;

    fingerStatus = "TIMEOUT";

    lcdFast(
    "FINGER TIMEOUT"
    );

    delay(1000);

    fingerStatus = "READY";

    lcdFast("READY");

    return;
  }

  // =====================================================
  // SCAN SPEED
  // =====================================================

  if(millis() - lastScan < 300)
  return;

  lastScan = millis();

  yield();

  server.handleClient();

  int p = finger.getImage();

  // =====================================================
  // NO FINGER
  // =====================================================

  if(p == FINGERPRINT_NOFINGER)
  return;

  // =====================================================
  // IMAGE FAIL
  // =====================================================

  if(p == FINGERPRINT_IMAGEFAIL)
  {
    fingerStatus = "IMAGE FAIL";

    lcdFast(
    "NO FINGER",
    "TRY AGAIN"
    );

    beep(100);

    delay(500);

    lcdFast(
    "PLACE FINGER"
    );

    return;
  }

  // =====================================================
  // CONVERT
  // =====================================================

  p = finger.image2Tz();

  if(p != FINGERPRINT_OK)
  {
    fingerStatus = "BAD IMAGE";

    lcdFast(
    "----------",
    "TRY AGAIN"
    );

    delay(500);

    lcdFast(
    "PLACE FINGER"
    );

    return;
  }

  // =====================================================
  // SEARCH
  // =====================================================

  p = finger.fingerFastSearch();

  // =====================================================
  // ACCESS OK
  // =====================================================

  if(p == FINGERPRINT_OK)
  {
    fingerStatus = "ACCESS OK";

    lcdFast(
    "ACCESS GRANTED",
    "ID:"+String(finger.fingerID)
    );

    beep(300);

    openCycle();

    fingerGateMode = false;

    fingerStatus = "READY";

    lcdFast("READY");
  }

  // =====================================================
  // ACCESS FAIL
  // =====================================================

  else
  {
    fingerStatus = "ACCESS DENIED";

    lcdFast(
    "ACCESS DENIED"
    );

    beep(500);

    delay(700);

    lcdFast(
    "PLACE FINGER"
    );
  }
}

// =====================================================

void apiEnroll()
{
  addCORS();

  if(server.hasArg("id"))
  {
    id =
    server.arg("id").toInt();

    server.send(
    200,
    "application/json",
    "{\"enroll\":\"started\"}"
    );

    delay(100);

    getFingerprintEnroll();
  }
}

// =====================================================

void apiDelete()
{
  addCORS();

  if(server.hasArg("id"))
  {
    id =
    server.arg("id").toInt();

    server.send(
    200,
    "application/json",
    "{\"delete\":\"started\"}"
    );

    delay(100);

    deleteFingerprint(id);
  }
}

// =====================================================

void keypadLoop()
{
  char key = keypad.getKey();

  if(!key)
  return;

  if(key == '*')
  {
    entered = "";

    lcdFast("PASSWORD CLEAR");

    delay(800);

    lcdFast("ENTER PASSWORD");

    return;
  }

  if(key >= '0' && key <= '9')
  {
    if(entered.length() < 6)
    {
      entered += key;

      lcd.setCursor(
      entered.length()-1,
      1
      );

      lcd.print("*");
    }
  }

  if(key == '#')
  {
    if(entered == password)
    {
      lcdFast("ACCESS GRANTED");

      beep(100);

      openCycle();
    }
    else
    {
      lcdFast("WRONG PASSWORD");

      beep(1000);

      delay(2000);
    }

    entered = "";

    lcdFast("ENTER PASSWORD");
  }
}

// =====================================================

void setup()
{
  Serial.begin(115200);

  pinMode(BUZZER, OUTPUT);

  pinMode(MOTOR1, OUTPUT);

  pinMode(MOTOR2, OUTPUT);
  digitalWrite(MOTOR1, LOW);

  digitalWrite(MOTOR2, LOW);

  pinMode(LIMIT_OPEN, INPUT_PULLUP);

  pinMode(LIMIT_CLOSE, INPUT_PULLUP);

  pinMode(ADMIN_BUTTON, INPUT_PULLUP);

  motorStop();

  lcd.init();

  lcd.backlight();

  lcdFast("SMART GATE");

  prefs.begin("lock", false);

  password =
  prefs.getString(
  "pass",
  "123456"
  );

  fingerSerial.begin(
  57600,
  SERIAL_8N1,
  16,
  17
  );

  finger.begin(57600);

  if(finger.verifyPassword())
  {
    lcdFast("FINGER READY");
  }
  else
  {
    lcdFast("FINGER ERROR");

    while(1);
  }

  delay(2000);

  wifiSetup();

  server.on("/api/status", handleStatus);

  server.on("/api/open", apiOpen);

  server.on("/api/close", apiClose);

  server.on("/api/password", apiPassword);

  server.on("/api/enroll", apiEnroll);

  server.on("/api/delete", apiDelete);

  server.begin();

  lcdFast("ENTER PASSWORD");
}

// =====================================================


void cloudHeartbeat()
{
  if(WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  String url = cloudHost + "/api/cloud/heartbeat/";
  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  String body = "{\"device_id\":\"" + cloudDeviceId + "\",\"token\":\"" + cloudToken + "\",\"gate_status\":\"" + gateStatus + "\",\"finger_status\":\"" + fingerStatus + "\",\"password_mask\":\"******\"}";
  http.POST(body);
  http.end();
}

void cloudPollCommand()
{
  if(WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  String url = cloudHost + "/api/cloud/next-command/?device_id=" + cloudDeviceId + "&token=" + cloudToken;
  http.begin(url);
  int code = http.GET();
  if(code != 200)
  {
    http.end();
    return;
  }

  String payload = http.getString();
  http.end();

  if(payload.indexOf("\"command\":\"OPEN\"") > -1) gateOpen();
  else if(payload.indexOf("\"command\":\"CLOSE\"") > -1) gateClose();
  else if(payload.indexOf("\"command\":\"PASSWORD\"") > -1)
  {
    int v = payload.indexOf("\"value\":\"");
    if(v > -1)
    {
      String p = payload.substring(v + 9);
      int e = p.indexOf("\"");
      if(e > -1) p = p.substring(0,e);
      if(p.length() == 6)
      {
        password = p;
        prefs.putString("pass", password);
      }
    }
  }
  else if(payload.indexOf("\"command\":\"ENROLL\"") > -1)
  {
    int v = payload.indexOf("\"value\":\"");
    if(v > -1)
    {
      String t = payload.substring(v + 9);
      int e = t.indexOf("\"");
      if(e > -1) t = t.substring(0,e);
      id = t.toInt();
      if(id > 0) getFingerprintEnroll();
    }
  }
  else if(payload.indexOf("\"command\":\"DELETE\"") > -1)
  {
    int v = payload.indexOf("\"value\":\"");
    if(v > -1)
    {
      String t = payload.substring(v + 9);
      int e = t.indexOf("\"");
      if(e > -1) t = t.substring(0,e);
      id = t.toInt();
      if(id > 0) deleteFingerprint(id);
    }
  }
}

void loop()
{
  server.handleClient();

  keypadLoop();

  // =====================================================
  // BUTTON ENABLES FINGER GATE MODE
  // =====================================================

  if(digitalRead(ADMIN_BUTTON)==LOW)
  {
    beep(200);

    fingerGateMode = true;

    fingerModeStart = millis();

    fingerStatus = "WAITING";

    lcdFast(
    "FINGER MODE",
    "PLACE FINGER"
    );

    delay(500);
  }

  // =====================================================

  checkFingerprintGate();

  if(millis() - lastCloudPoll > 2000)
  {
    lastCloudPoll = millis();
    cloudPollCommand();
  }

  if(millis() - lastHeartbeat > 5000)
  {
    lastHeartbeat = millis();
    cloudHeartbeat();
  }
}
