#include <WiFi.h>
#include <HTTPClient.h>
#include <ESP32Servo.h>

// ===================== USER CONFIG =====================
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASS = "YOUR_WIFI_PASSWORD";

String cloudHost = "https://truecollabs.pythonanywhere.com";
String cloudDeviceId = "esp32-servo-test-01";
String cloudToken = "CHANGE_ME_TOKEN";

// Preferred pin: D18 (GPIO18)
const int SERVO_PIN = 18;
const int SERVO_OPEN_ANGLE = 90;
const int SERVO_CLOSE_ANGLE = 0;
// =======================================================

Servo gateServo;
unsigned long lastPoll = 0;
unsigned long lastHeartbeat = 0;
String servoState = "CLOSED";

void connectWifi()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("WiFi connected. IP: ");
  Serial.println(WiFi.localIP());
}

void setServoOpen()
{
  gateServo.write(SERVO_OPEN_ANGLE);
  servoState = "OPEN";
  Serial.println("Servo moved to OPEN position");
}

void setServoClose()
{
  gateServo.write(SERVO_CLOSE_ANGLE);
  servoState = "CLOSED";
  Serial.println("Servo moved to CLOSED position");
}

void sendHeartbeat()
{
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  String url = cloudHost + "/api/cloud/heartbeat/";
  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  String body = "{\"device_id\":\"" + cloudDeviceId + "\","
                "\"token\":\"" + cloudToken + "\","
                "\"gate_status\":\"" + servoState + "\","
                "\"finger_status\":\"N/A\","
                "\"password_mask\":\"N/A\"}";

  int code = http.POST(body);
  Serial.print("Heartbeat code: ");
  Serial.println(code);
  http.end();
}

void pollCommand()
{
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  String url = cloudHost + "/api/cloud/next-command/?device_id=" + cloudDeviceId + "&token=" + cloudToken;
  http.begin(url);

  int code = http.GET();
  if (code != 200)
  {
    Serial.print("Poll failed code: ");
    Serial.println(code);
    http.end();
    return;
  }

  String payload = http.getString();
  http.end();

  Serial.print("Payload: ");
  Serial.println(payload);

  if (payload.indexOf("\"command\":\"OPEN\"") > -1)
  {
    setServoOpen();
  }
  else if (payload.indexOf("\"command\":\"CLOSE\"") > -1)
  {
    setServoClose();
  }
}

void setup()
{
  Serial.begin(115200);
  delay(1000);

  gateServo.setPeriodHertz(50);
  gateServo.attach(SERVO_PIN, 500, 2400);
  setServoClose();

  connectWifi();
  sendHeartbeat();
}

void loop()
{
  if (millis() - lastPoll > 2000)
  {
    lastPoll = millis();
    pollCommand();
  }

  if (millis() - lastHeartbeat > 5000)
  {
    lastHeartbeat = millis();
    sendHeartbeat();
  }
}
