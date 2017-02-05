#include <ESP8266WiFi.h>          //ESP8266 Core WiFi Library (you most likely already have this in your sketch)
#include <ESP8266HTTPClient.h>

#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic

#define AP_Name "Kozakkenboys"
#define AP_Password "VierdeElftal"

#define payload_on "{\"on\": true}"
#define payload_off "{\"on\": false}"

#define bridgeUrl "http://192.168.1.179/api/wn3lnOTtZXlD0hueqBcddsqFsnVhSzvzvmKC0nk0/lights/8/state"
#define toggle_delay 3000

boolean on_off = true;

void setup() {
  Serial.begin(9600);
  WiFiManager wifiManager;
  //first parameter is name of access point, second is the password
  //wifiManager.autoConnect(AP_Name, AP_Password);
  //unsecured access point
  wifiManager.autoConnect(AP_Name);
  //if you want to use and auto generated name from 'ESP' and the esp's Chip ID use
  //wifiManager.autoConnect();
  
}

void loop() {
  HTTPClient http;

  Serial.print("[HTTP] begin... \n");
  http.begin(bridgeUrl);

  Serial.print("[HTTP] put... \n");
  int httpCode = -1;

  if (on_off) {
    httpCode = http.sendRequest("PUT", payload_on);     
  } else {
    httpCode = http.sendRequest("PUT", payload_off); 
  }
   
  if (httpCode > 0) {
     Serial.printf("[HTTP] PUT... code: %d\n", httpCode);
     if (httpCode == HTTP_CODE_OK) {
        on_off = !on_off;
        String response = http.getString();
        Serial.println(response);
     }
  } else {
    Serial.printf("[HTTP] SET... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();

  delay(toggle_delay);
}
