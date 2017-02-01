//
// ICOM CI-V to WIFI interface
// See www.d68.nl/civ or email to dkroeske@gmail.com
// Disclamer : 

//
// This software runs inside an ESP8266E12 creating an Icom CV-I to TCP brigde
//
// This software operates in 2 modes: as accesspoint when no other accesspoint is
// available or as normal station on a local WiFi network.
//
// Accesspoint mode. Use this mode when out of range of available WiFi accesspoint.
// This mode makes is poosible to communicate with the CI-V interface without any
// other TCP network (directly). In this mode the AP jumper must be set.
//
// Station mode. Use this mode when using e.g. at home where a WiFi network is available.
// In this mode it is possible to connect from anywhere in the network to a CI-V interface.
// In this mode the AP jumper must NOT be set. 
//
// In station mode, when no previous WiFi station is selected, this device creates an ad-hoc
// WiFi accesspoint where other wireless networks can be selected and credentials can be entered.
// After reboot this device connects to the previous selected wireless network. To factory reset
// this device the reset-to-factory button must be pressed for at least 1 seconds immediately after
// a powercycle.
//
// The LED reflects the mode the device is in:
// 
// LED fast blinking RED -> starting and connecting to previous selected wireless network
// LED fast blinking BLUE -> device requires WiFi credentials. Connect to the created ad-hoc 
// accesspoint (automaticly or via 192.168.10.4)
// LED continious BLUE -> device is in accesspoint mode for direct communication (10.10.0.1)
// LED continious GREEN ->  device is ready to accept connectings
// LED slowly breathing GREEN -> Someone connected to device and data is transfered. 
// 

#define UART_BAUD 9600
#define SERVER_PORT 10000

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <Ticker.h>
#include <SoftwareSerial.h>
#include <NeoPixelBus.h>
#include <ESP8266mDNS.h>

// Local variables
WiFiServer server(SERVER_PORT);
WiFiClient client;
Ticker ticker;
WiFiManager wifiManager;

// 
// Serial buffers
//
#define NETWORK_BUF_SIZE  256
uint8_t buf1[NETWORK_BUF_SIZE];
uint8_t i1=0;

#define SW_SERIAL_BUF_SIZE 256
uint8_t buf2[SW_SERIAL_BUF_SIZE];
uint8_t i2=0;

// Setup sofware serial
#define SOFTWARE_SERIAL_TX  4
#define SOFTWARE_SERIAL_RX  5
SoftwareSerial IcomCIV(SOFTWARE_SERIAL_TX, SOFTWARE_SERIAL_RX, false, SW_SERIAL_BUF_SIZE);

// Soft IP settings
IPAddress local_ip(192,168,10,1);
IPAddress gateway(192,168,10,0);
IPAddress subnet(255,255,255,0);

// Number of NEO pixels
#define PixelCount  1
#define PixelPort   2
// NEO Pixel
NeoPixelBus<NeoGrbFeature, NeoEsp8266Uart800KbpsMethod> neoPixel(PixelCount, PixelPort);
#define colorSaturation 64
RgbColor red(colorSaturation, 0, 0);
RgbColor green(0, colorSaturation, 0);
RgbColor blue(0, 0, colorSaturation);
RgbColor white(colorSaturation);
RgbColor black(0);

// Enum Switch modes. Switches are sampled at the ANA-pin 
typedef enum {
  SWITCH_FACTORY_RESET = 0,
  SWITCH_FORCE_AP,
  SWITCH_NOT_PRESSED 
} SWITCH_STATE;

//
// Ticker: boot callback's
//
void ticker_boot() {
  static int state = 0;
  state ^= 1;
  if(state) {
    neoPixel.SetPixelColor(0, red);
  } else {
    neoPixel.SetPixelColor(0, black);
  }
  neoPixel.Show();
}

//
// Ticker_boot connecting mode
//
void ticker_config() {
  static int state = 0;
  state ^= 1;
  if(state) {
    neoPixel.SetPixelColor(0, blue);
  } else {
    neoPixel.SetPixelColor(0, black);
  }
  neoPixel.Show();
}

//
// Ticker: factory_reset callback's
//
void ticker_factory_reset() {
  static int state = 0;
  state ^= 1;
  if(state) {
    neoPixel.SetPixelColor(0, blue);
  } else {
    neoPixel.SetPixelColor(0, black);
  }
  neoPixel.Show();
}

//
// Ticker: soft AP callback's
//
void ticker_softAP_mode() {
  static int state = 0;
  state ^= 1;
  if(state) {
    neoPixel.SetPixelColor(0, blue);
  } else {
    neoPixel.SetPixelColor(0, black);
  }
  neoPixel.Show();
}

//
// Ticker: Connected
//
void ticker_STA_mode() {
  static int state = 0;
  state ^= 1;
  if(state) {
    neoPixel.SetPixelColor(0, green);
  } else {
    neoPixel.SetPixelColor(0, black);
  }
  neoPixel.Show();
}

//
// Poll switches 
//
SWITCH_STATE pollSwitches()
{
  
  SWITCH_STATE retval = SWITCH_NOT_PRESSED;
  
  unsigned int val = (unsigned int) analogRead(A0);
  
  if( val <= 350 && val >= 250 ) {
    retval = SWITCH_FACTORY_RESET;
  }
  
  return retval;
}

//
// Setup soft AP
//
void setupSoftAP() {

  WiFi.mode(WIFI_AP);
  
  Serial.print("Setting soft-AP configuration ... ");
  Serial.println(WiFi.softAPConfig(local_ip, gateway, subnet) ? "Ready" : "Failed!");

  Serial.print("Setting soft-AP ... ");
  Serial.println(WiFi.softAP("ICOM_CIV_192.168.10.1") ? "Ready" : "Failed!");

  Serial.print("Soft-AP IP address = ");
  Serial.println(WiFi.softAPIP());  
}


//flag for saving data
bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

//
// msg simulator
//
uint8_t debugMsg[] = { 0xFE, 0xFE, 0x00, 0x86, 0x00, 0x00, 0x00, 0x00, 0x34, 0x01, 0xFD };

void ticker_SimCIVMsg() {
  static uint8_t index = 0;
  client.write( (debugMsg+index), 1 );
  index++;
  index%=11;
  if( index == 0 ) {
    debugMsg[8]+=10;
  }
}

//
// Setup: 
//
void setup() {

  // Init NeoPixel
  neoPixel.Begin();
  neoPixel.SetPixelColor(0, red);
  neoPixel.Show();


  // IO
  pinMode(A0, INPUT);
  
  // Startup delay
  delay(4000);

  // Setup serial ports
  Serial.begin(UART_BAUD);
  IcomCIV.begin(UART_BAUD);

  neoPixel.SetPixelColor(0, blue);
  neoPixel.Show();

  // Handle boot config. This depends on RESET or FORCE_AP mode
  SWITCH_STATE sw = pollSwitches();
  
  if( sw == SWITCH_FACTORY_RESET ) {
    Serial.println("factory reset");
    wifiManager.resetSettings();
    delay(4000);
    ESP.reset();
  } else {
    if( sw == SWITCH_FORCE_AP ) {
      setupSoftAP();
    } else {
      //set minimu quality of signal so it ignores AP's under that quality
      //defaults to 8%
      wifiManager.setMinimumSignalQuality(20);
  
      //sets timeout until configuration portal gets turned off
      //useful to make it all retry or go to sleep
      //in seconds
      wifiManager.setTimeout(300);

      //set config save notify callback
      wifiManager.setSaveConfigCallback(saveConfigCallback);
      
      if (!wifiManager.autoConnect("ICOM_CI-V")) {
        Serial.println("failed to connect and hit timeout");
        delay(3000);
        //reset and try again, or maybe put it to deep sleep
        ESP.reset();
        delay(5000);
      }      
    }
  }

  //
  // save the custom parameters to FS
  //
  if (shouldSaveConfig) {
  }

  if( !MDNS.begin("ICOM_DI51_MONITOR") ) {
    Serial.print("Error setting up mDNS");
  } else {
    MDNS.addService("civ2tcp", "tcp", 10000);
  }
  

  // Start server
  server.begin();
  server.setNoDelay(true);

  
  Serial.print("local ip: ");
  Serial.println(WiFi.localIP());

  neoPixel.SetPixelColor(0, green);
  neoPixel.Show();

  //ticker.detach();
  //ticker.attach(0.05, ticker_SimCIVMsg);
}

//
// Main loop
//
void loop() {
  
  //
  if( server.hasClient() ) {
    if( !client || !client.connected() ) {
      if( client) {
        client.stop();
      }
      client = server.available();
      Serial.println("New client connected");
    }
  }

  // Network to Serial
  if( client && client.connected() ) {
    if( client.available() ) {
      while( client.available() ) {
        neoPixel.SetPixelColor(0, black); neoPixel.Show();    
        char b = client.read();
        Serial.print( b, HEX);
        IcomCIV.write( b );
        neoPixel.SetPixelColor(0, green); neoPixel.Show();    
      }
    } 
  }

  // Serial to network
  if( IcomCIV.available() ) {
    size_t len = IcomCIV.available();
    IcomCIV.readBytes(buf2, len);
    if( client && client.connected() ) {
      neoPixel.SetPixelColor(0, black); neoPixel.Show();    
      client.write( (uint8_t *)buf2, len);
      neoPixel.SetPixelColor(0, green); neoPixel.Show();    
    }
  }
}


////
//// Main loop
////
//void loop() {
//  
////    if( pollSwitches() == SWITCH_FACTORY_RESET ) { 
////      Serial.println("SWITCH_FACTORY_RESET" );
////    } else {
////      Serial.println("SWITCH_NOT_PRESSED" );
////    }
//
//  //
//  if( !client.connected() ) {
//    client = server.available();
//    return;
//  }
//
//  // Network to Serial
//  if( client.available() ) {
//    while( client.available()) {
//      buf1[i1] = (uint8_t) client.read();
//      if(i1<64) i1++;
//    } 
//     
//    neoPixel.SetPixelColor(0, black); neoPixel.Show();    
//    IcomCIV.write(buf1,i1);
//    neoPixel.SetPixelColor(0, green); neoPixel.Show();
//
//    i1 = 0;
//  }
//
//  // Serial to network
//  if(IcomCIV.available()) {
//    while(IcomCIV.available()) {
//      buf2[i2] = (uint8_t)IcomCIV.read();
//      if(i2<64) i2++;
//    }
//    
//    neoPixel.SetPixelColor(0, black); neoPixel.Show();    
//    client.write( (uint8_t*)buf2, i2);
//    neoPixel.SetPixelColor(0, green); neoPixel.Show();
//    
//    i2 = 0;
//  }
//}
