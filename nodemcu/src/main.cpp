#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <DHT.h>

// Wi‑Fi nastavenia
const char* ssid     = "TP-Link_185B";
const char* password = "dpAGmvS4";

// HTTP server
ESP8266WebServer server(80);

// DHT11 na D2
#define DHTPIN   D2
#define DHTTYPE  DHT11
DHT dht(DHTPIN, DHTTYPE);

// Stav merania
bool dhtReadingActive = false;
unsigned long lastDhtRead = 0;
const unsigned long DHT_READ_INTERVAL = 2000; // ms

// Prepis stavu WiFi na string
const char* wifiStatusToString(int status) {
  switch (status) {
    case WL_IDLE_STATUS:     return "IDLE";
    case WL_NO_SSID_AVAIL:   return "NO_SSID_AVAIL";
    case WL_SCAN_COMPLETED:  return "SCAN_COMPLETED";
    case WL_CONNECTED:       return "CONNECTED";
    case WL_CONNECT_FAILED:  return "CONNECT_FAILED";
    case WL_CONNECTION_LOST: return "CONNECTION_LOST";
    case WL_DISCONNECTED:    return "DISCONNECTED";
    default:                 return "UNKNOWN";
  }
}

// Health‑check endpoint
void handleRoot() {
  server.send(200, "text/plain", "ESP OK");
}

// Sensor endpoint: JSON {temperature,humidity}
void handleSensor() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  server.sendHeader("Access-Control-Allow-Origin", "*");
  if (isnan(h) || isnan(t)) {
    server.send(500, "application/json", "{\"error\":\"DHT read failed\"}");
  } else {
    char buf[64];
    snprintf(buf, sizeof(buf),
             "{\"temperature\":%.1f,\"humidity\":%.1f}", t, h);
    server.send(200, "application/json", buf);
  }
}

// Command endpoint
void handleCommand() {
  String body = server.arg("plain");
  StaticJsonDocument<128> doc;
  if (deserializeJson(doc, body)) {
    server.send(400, "application/json", "{\"status\":\"error\"}");
    return;
  }
  const char* cmdStr = doc["command"];
  char code = 'S';
  if      (!strcmp(cmdStr, "FORWARD"))       code = 'F';
  else if (!strcmp(cmdStr, "BACKWARD"))      code = 'B';
  else if (!strcmp(cmdStr, "LEFT"))          code = 'L';
  else if (!strcmp(cmdStr, "RIGHT"))         code = 'R';
  else if (!strcmp(cmdStr, "STOP"))          code = 'S';
  else if (!strcmp(cmdStr, "MEASURE_START")) code = 'm';
  else if (!strcmp(cmdStr, "MEASURE_STOP"))  code = 'x';

  // --------- TU je TX1 použité pre UNO ------------
  if (code != 'm' && code != 'x') {
    Serial1.write(code);  
    Serial.printf("[DBG] Sent motor code via TX1: %c\n", code);
  } else {
    Serial.printf("[DBG] Measure code: %c\n", code);
  }

  // spracovanie merania
  if (code == 'm') {
    dhtReadingActive = true;
    Serial.println("[DBG] DHT reading STARTED");
  }
  else if (code == 'x') {
    dhtReadingActive = false;
    Serial.println("[DBG] DHT reading STOPPED");
  }

  server.send(200, "application/json", "{\"status\":\"OK\"}");
}

void setup() {
  // debug cez USB
  Serial.begin(9600);
  // UART1 pre príkazy do UNO (TX1 na D4)
  Serial1.begin(9600);

  delay(100);
  dht.begin();

  // pripojenie Wi‑Fi
  Serial.printf("\nConnecting to Wi‑Fi \"%s\"\n", ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  unsigned long start=millis();
  while (WiFi.status()!=WL_CONNECTED && millis()-start<10000) {
    Serial.print(".");
    delay(500);
  }
  if (WiFi.status()==WL_CONNECTED) {
    Serial.println("\nWi‑Fi connected!");
    // TU vypíšeme IP
    IPAddress ip = WiFi.localIP();
    Serial.print("My IP address: ");
    Serial.println(ip);
  } else {
    Serial.println("\nWi‑Fi connection failed!");
  }

  // endpointy
  server.on("/",        HTTP_GET,  handleRoot);
  server.on("/sensor",   HTTP_GET,  handleSensor);
  server.on("/command",  HTTP_POST, handleCommand);
  server.begin();
  Serial.println("HTTP server running on port 80");
}

void loop() {
  server.handleClient();

  // manuálny debug cez USB (štart/stop merania)
  if (Serial.available()) {
    char c = Serial.read();
    if (c=='m') {
      dhtReadingActive=true;
      Serial.println("[DBG] USB: START meas");
    }
    else if (c=='x') {
      dhtReadingActive=false;
      Serial.println("[DBG] USB: STOP meas");
    }
  }

  // periodické čítanie DHT
  if (dhtReadingActive && millis()-lastDhtRead>=DHT_READ_INTERVAL) {
    lastDhtRead=millis();
    float h = dht.readHumidity(), t = dht.readTemperature();
    if (isnan(h)||isnan(t)) {
      Serial.println("[DBG] DHT read failed");
    } else {
      Serial.printf("Humidity: %.1f %%  Temp: %.1f *C\n", h, t);
    }
  }
}
