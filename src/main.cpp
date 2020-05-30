#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <ESP8266WebServer.h>
#include <FS.h>
#include <ArduinoJson.h>

ESP8266WebServer server(80);

void update_started(){
  Serial.println("CALLBACK:  HTTP update process started");
}

void update_finished(){
  Serial.println("CALLBACK:  HTTP update process finished");
}

void update_progress(int cur, int total){
  Serial.printf("CALLBACK:  HTTP update process at %d of %d bytes...\n", cur, total);
}

void update_error(int err){
  Serial.printf("CALLBACK:  HTTP update fatal error code %d\n", err);
}

void FIRMWARE_UPDATE(){
  Serial.println("UPDATE COMMAND RECIVED");
  server.send(200,"text/plain","Update Command Recieved");
  WiFiClient client;
  ESPhttpUpdate.onStart(update_started);
  ESPhttpUpdate.onEnd(update_finished);
  ESPhttpUpdate.onProgress(update_progress);
  ESPhttpUpdate.onError(update_error);
  t_httpUpdate_return ret = ESPhttpUpdate.update(client, "http://192.168.3.139:5000/update");
    
  switch (ret){
    case HTTP_UPDATE_FAILED:
      Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
      break;

    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("HTTP_UPDATE_NO_UPDATES");
      break;

    case HTTP_UPDATE_OK:
      Serial.println("HTTP_UPDATE_OK");
      break;
    }
}

void serveHome(){
  File file = SPIFFS.open("/Home.html", "r");
  server.streamFile(file, "text/html");
  file.close();
}

void serveConfig(){
  File file = SPIFFS.open("/Config.html", "r");
  server.streamFile(file, "text/html");
  file.close();
}

void WiFi_AP_MODE(){
  WiFi.softAPdisconnect(true);
  delay(1000);
  WiFi.softAP(F("TELEOPS CONFIG PORTAL")); // Uses default AP IP Address of 192.168.4.1
}

void WiFi_CONFIG(){
  String data = server.arg("plain");
  StaticJsonDocument<200> credentials;
  DeserializationError error = deserializeJson(credentials, data);

  if (error){
    Serial.println(error.c_str());
    return;
  }

  const String ssid = credentials["ssid"];
  const String password = credentials["pass"];
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED){
    delay(1000);
    if ((unsigned long)(millis() - startTime) >= 10000)break;
  }

  if (WiFi.status() == WL_CONNECTED){
    server.send(200, "application/json", "{\"status\" : \"ok\"}");
    delay(1000);
    Serial.println("WiFI Connected");
  }

  if (WiFi.status() != WL_CONNECTED){
    server.send(304, "application/json", "{\"MSG\" : \"INVALID CREDEDTIALS\"}");
    delay(1000);
    WiFi_AP_MODE();
  }
}

void setup(){
  Serial.begin(115200);
  SPIFFS.begin();
  WiFi.mode(WIFI_STA);
  WiFi.begin();

  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED){
    delay(500);
    if ((unsigned long)(millis() - startTime) >= 5000)break;
  }

  if (WiFi.status() != WL_CONNECTED){
    WiFi_AP_MODE();
  }
  server.on("/wificonfig", HTTP_POST, WiFi_CONFIG);
  server.on("/", serveHome);
  server.on("/wificonfig", serveConfig);
  server.on("/update", FIRMWARE_UPDATE);
  server.begin();
}

void loop(){
  server.handleClient();
}
