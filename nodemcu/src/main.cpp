#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

//  Sem zadaj tvoje WiFi prihlasovacie údaje
const char* ssid = "TP-Link_185B";
const char* password = "dpAGmvS4";

const char* serverUrl = "http://192.168.1.100:5000/command"; // uprav podľa svojho servera

WiFiClient client;  // << dôležité

void sendCommandToServer(const String& cmd);

void setup() {
  Serial.begin(9600);
  Serial.println("NodeMCU pripravené");

  WiFi.begin(ssid, password);
  Serial.print("Pripájam sa na WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nPripojené k WiFi");
  Serial.print("IP adresa: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();

    if (command.length() > 0) {
      sendCommandToServer(command);
    }
  }
}

void sendCommandToServer(const String& cmd) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    // 👇 používa sa nový zápis: http.begin(client, url)
    http.begin(client, serverUrl);
    http.addHeader("Content-Type", "application/json");

    String jsonPayload = "{\"command\": \"" + cmd + "\"}";
    int httpResponseCode = http.POST(jsonPayload);

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("Odpoveď servera: " + response);
    } else {
      Serial.print("Chyba HTTP: ");
      Serial.println(httpResponseCode);
    }

    http.end();
  } else {
    Serial.println("Nie je pripojenie k WiFi");
  }
}
