#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <WiFiUdp.h>
#include <WakeOnLan.h>
#include <SPIFFS.h>
#include <FS.h>
#include <Wire.h>
#include "SSD1306Wire.h" 

#define FORMAT_SPIFFS_IF_FAILED true

WiFiUDP UDP;
WakeOnLan WOL(UDP);
WebServer server(80);
SSD1306Wire display(0x3c, SDA, SCL);

const char* ssid = "your_ssid";
const char* password = "your_pass";
IPAddress ip(192, 168, 1, 101);
IPAddress subnet(255, 255, 255, 0);
IPAddress gateway(192, 168, 1, 1);
IPAddress DNS(192, 168, 1, 1);

String readFile(const char * path){
  File file = SPIFFS.open(path);
  if(!file || file.isDirectory()){
    return "";
  }
  String buf = "";
  while(file.available()){
    char ch = file.read();
    buf += ch;
  }
  return buf;
}
void handleRoot() {
  Serial.println("root");
  server.send(200, "text/html", readFile("/index.html"));
}

void handleOn() {
  if (!server.hasArg("mac")) {
    Serial.println("error : bad mac address");
    server.send(400, "text/json", "{\"error\":\"bad mac address\"}");
    return;
  } 
  String mac = server.arg("mac");
  char mac_s[30];
  display.clear();
  display.drawString(0, 0, "Wake On Lan");
  String msg = "MAC : ";
  mac.toCharArray(mac_s, 20);
  msg += mac;
  display.drawString(0, 10, msg);
  display.display();
  WOL.sendMagicPacket(mac_s);
  String json = "{\"OK\":\"";
  json += mac;
  json += "\"}";
  Serial.print("ok : ");
  Serial.println(mac);
  server.send(200, "text/json", json);
}

void handleReady() {
  showIP();
  server.send(200, "text/json", "{\"ok\":1}");
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void showIP() {
  Serial.println("showIP");
  display.clear();
  display.drawString(0, 0, "Ready");
  String msg = "IP : ";
  msg += WiFi.localIP().toString();
  display.drawString(0, 10, msg);
  display.display();
}

void setup()
{
  Serial.begin(115200);

  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 0, "Initializing...");
  display.display();

  WOL.setRepeat(3, 100);

  if(!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)){
      Serial.println("SPIFFS Mount Failed");
      return;
  }
 
  WiFi.mode(WIFI_STA);
  WiFi.config(ip, gateway, subnet, DNS);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.print("Connect IP = ");
  Serial.println(WiFi.localIP());
  WOL.calculateBroadcastAddress(WiFi.localIP(), WiFi.subnetMask());
  server.on("/", handleRoot);
  server.on("/on", handleOn);
  server.on("/ready", handleReady);
  server.onNotFound(handleNotFound);
  server.begin();
  showIP();
}

void loop()
{
  server.handleClient();
  delay(2);
}
