#include <ESP8266WiFi.h>          //ESP8266 Core WiFi Library (you most likely already have this in your sketch)

#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic

#define AP_Name "Kozakkenboys"
#define AP_Password "MAD2016TI"

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
  // put your main code here, to run repeatedly:

}
