#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiClientSecure.h>
#include <TFT_eSPI.h>
#include "WebServer.h"
#include <Preferences.h>
#include <Arduino_JSON.h>
#include <HTTPClient.h>
#include <esp_wifi.h>
#include <string.h>

#include <FastLED.h>
#include "OneButton.h"
#include "pin_config.h"

#include "wifilogo.h"
#include "wifilogor.h"
#include "wifilogog.h"
#include "bitcoin.h"
#include "bitcoins.h"
#include "logo.h"
#include "btclogo.h"
#include "settings.h"

#define LGRAY    0xC618
#define GRAY     0x8410
#define DGRAY    0x7BEF
#define DGREEN   0x0606
#define DBLUE    0x1414
#define DRED     0xc0c0
#define DYELLOW  0xe6e6

#define TFT_GRAY 0xC618

#define BTN_PIN     0

#define LED_DI_PIN     40
#define LED_CI_PIN     39
#define TFT_LEDA_PIN   38


TFT_eSPI tft = TFT_eSPI();  // Invoke library, pins defined in User_Setup.h
CRGB leds;

//Bitcoin data via API from coingecko
const char* api1 = "https://api.coingecko.com/api/v3/coins/markets?vs_currency=USD&ids=bitcoin";  // Euro

const char* api2 = "https://api.coingecko.com/api/v3/coins/markets?vs_currency=EUR&ids=bitcoin";  // US Dollar

const char* api3 = "https://api.coingecko.com/api/v3/coins/markets?vs_currency=TWD&ids=bitcoin";  // Taiwan Doller
// the following variables are unsigned longs because the time, measuTFT_RED in
// milliseconds, will quickly become a bigger number than can be stoTFT_RED in an int.
unsigned long lastTime = 0;
// Timer set to 10 minutes (600000)
// unsigned long timerDelay = 600000;
// Set timer to 5 seconds (5000)
unsigned long timerDelay = 5000;

String getbitcoin;
float getDogeArr[3];

String bitcoin_coinname="";
double bitcoin_currentprice=0;

String btc_coinname="";
double btc_currentprice=0;
double btc_high_24h=0;
double btc_low_24h=0;
double btc_price_change_percentage_24h=0;

double btc_rank=0;

//get current TS as per IST
const char* ntpServer = "in.pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;

// CoinGecko limit is "no more than 10 per minute"
// Make sure to factor in if you are requesting more than one coin.
unsigned long api_mtbs = 60000; //mean time between api requests
unsigned long api_due_time = 0;

// Wifi and curency settings
const IPAddress apIP(192, 168, 4, 1);
const char* apSSID = "BTC-Ticker-SETUP";
boolean settingMode;
String ssidList;
String wifi_ssid;
String wifi_password;
String curren_cy;

// wifi and curency config store
Preferences preferences;

WiFiClientSecure client;

int count = 0;

int count2 = 0;

String dollar = "\xA3";

WebServer webServer(80);



void setup() {
  Serial.begin(115200);  /*Baud rate for serial communication*/
  Serial.println(F("Boot T-Dongle-S3"));

  preferences.begin("wifi-config");

  pinMode(TFT_LEDA_PIN, OUTPUT);
  tft.init();
  tft.setRotation(3);
  digitalWrite(TFT_LEDA_PIN, 0);
  FastLED.addLeds<APA102, LED_DI_PIN, LED_CI_PIN, BGR>(&leds, 1);
  tft.setSwapBytes(true);
  leds = CHSV(160, 255, 55);
  FastLED.show();  

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  //tft.pushImage(0, 0, logoWidth, logoHeight, logo);
  //tft.drawString("Startinng....",39, 33, 2);
  //delay(2000);

  tft.pushImage(0, 0, btclogoWidth, btclogoHeight, btclogo);
  tft.setTextColor(TFT_BLACK);
  tft.drawString("https://github.com/oxinon",1, 63, 2);
  tft.setTextColor(TFT_GRAY);
  delay(2000);


  if (restoreConfig()) {
    if (checkConnection()) {
      delay(2000);
      tft.fillScreen(TFT_BLACK); 
      settingMode = false;
      startWebServer();

  tft.fillScreen(TFT_BLACK);
  tft.pushImage(42, 30, bitcoinsWidth, bitcoinsHeight, bitcoins);
  tft.setTextColor(TFT_GRAY);
  tft.drawString("Loading data...", 36, 60, 2); 

      return;
    }
  }
  settingMode = true;
  setupMode();
}


void loop() {


  if (settingMode) {
  }
  webServer.handleClient();
  
  if (WiFi.status() == WL_CONNECTED) {
    btcticker();
  }
  
  count2++;
  
  delay(150);

}


boolean restoreConfig() {
  wifi_ssid = preferences.getString("WIFI_SSID");
  wifi_password = preferences.getString("WIFI_PASSWD");
  curren_cy = preferences.getString("CURRENCY");  


  // not the same, setup with SoftAP config
  if( wifi_ssid == "none" )  // New...setup wifi
  {
  startWebServer();// reboot with wifi configured
  }
 
 
  Serial.print("WIFI-SSID: ");
  //tft.drawString("WIFI-SSID: ", 0, 0, 2);
  Serial.println(wifi_ssid);
  //tft.drawString(wifi_ssid, 100, 0, 2);
  Serial.print("WIFI-PASSWD: ");
  //tft.drawString("WIFI-PASSWD: ", 0, 20, 2);
  Serial.println(wifi_password);
  //tft.drawString(wifi_password, 100, 20, 2);
  Serial.print("CURRENCY: ");
  //tft.drawString("CURRENCY: ", 0, 60, 2);
  Serial.println(curren_cy);
  //tft.drawString(curren_cy, 100, 60, 2);

  delay(500);
  
  WiFi.begin(wifi_ssid.c_str(), wifi_password.c_str());

  if(wifi_ssid.length() > 0) {
    return true;
  } 
    else {
    return false;
  }
}

boolean checkConnection() {

  int count = 0;

  while ( count < 30 ) {
    if (WiFi.status() == WL_CONNECTED) {
      tft.fillRect(0, 0, 160, 15, TFT_WHITE);
      tft.pushImage(5, 25, wifilogogWidth, wifilogogHeight, wifilogog);
      tft.fillRect(30, 54, 5, 2, TFT_WHITE);
      tft.fillRect(38, 30, 120, 20, TFT_WHITE);
      tft.drawString("Connected!",39, 33, 2);
      Serial.println("Connected!");
      return (true);
    }

    if ( count <= 0){
      tft.fillScreen(TFT_WHITE); 
      tft.pushImage(5, 25, wifilogoWidth, wifilogoHeight, wifilogo);
      tft.fillRect(30, 54, 5, 2, TFT_WHITE);
      tft.fillRect(38, 30, 120, 20, TFT_WHITE);
      tft.setTextColor(TFT_BLACK);
      Serial.print("Connecting to Wifi...");
      tft.drawString("Connecting to Wifi",39, 33, 2);
    }

    delay(500);
    Serial.print(".");
    tft.print(".");
    count++;
  }

  
  Serial.println("Timed out.");
  tft.fillScreen(TFT_WHITE);
    
  tft.pushImage(5, 25, wifilogorWidth, wifilogorHeight, wifilogor);
  tft.fillRect(30, 54, 5, 2, TFT_WHITE);
  tft.fillRect(38, 30, 120, 20, TFT_WHITE);
  tft.setTextColor(TFT_BLACK);
  tft.drawString("Timed out", 39, 33, 2);
  delay(2000);

  tft.fillRect(38, 30, 120, 20, TFT_WHITE);
  tft.setTextColor(TFT_BLACK);
  tft.drawString("Reset Wifi config", 39, 33, 2);
  delay(2000);

  tft.fillRect(38, 30, 120, 20, TFT_WHITE);
  tft.setTextColor(TFT_BLACK);
  tft.drawString("Restart", 39, 33, 2);
  delay(2000);

      // reset the wifi config
      preferences.remove("WIFI_SSID");
      preferences.remove("WIFI_PASSWD");
      preferences.remove("CMC_API_KEY");
      preferences.remove("CURRENCY");
      preferences.remove("THEME");
      delay(1000);
      ESP.restart();

  return false;
}

void startWebServer() {
  if (settingMode) {

  tft.fillRect(38, 30, 120, 20, TFT_WHITE);
  tft.setTextColor(TFT_BLACK);
  Serial.print("Starting Config mode");

  tft.fillScreen(TFT_WHITE); 
  tft.pushImage(5, 25, settingsWidth, settingsHeight, settings);
  tft.fillRect(30, 54, 5, 2, TFT_WHITE);

  tft.drawString("Setup mode !",40, 7, 2);
  tft.drawString("BTC-Ticker-SETUP",40, 33, 2);
  tft.drawString("IP: 192.168.4.1",40, 58, 2);

    tft.setTextColor(TFT_BLACK);
    Serial.print("Starting Web Server at ");
   
    Serial.println(WiFi.softAPIP());
    webServer.on("/settings", []() {
      String s = "<h1>BTC Ticker</h1><p><h2>Wi-Fi and Currency Setup</h2><p>Please select your WiFi SSID and enter the password.<br> Please enter your Currency USD or EUR</p>";
      s += "<form method=\"get\" action=\"setap\"><label>SSID: </label><select name=\"ssid\">";
      s += ssidList;
      s += "</select><br><br>Wifi Password: <input name=\"pass\" length=64 type=\"password\">";
      s += "<br><br>Currency: <input name=\"currency\" length=3 type=\"text\">";
      s += "<br><br><input type=\"submit\"></form>";      
      webServer.send(200, "text/html", makePage("Wi-Fi Settings", s));
    });
    webServer.on("/setap", []() {
      tft.fillScreen(TFT_WHITE);
      String ssid = urlDecode(webServer.arg("ssid"));
      Serial.print("SSID: ");
      tft.print("SSID: ");
      Serial.println(ssid);
      tft.println(ssid);
      String pass = urlDecode(webServer.arg("pass"));
      Serial.print("Password: ");
      tft.print("Password: ");
      Serial.println(pass);
      tft.println(pass);
      String currency = urlDecode(webServer.arg("currency"));
      Serial.print("Currency: ");
      tft.print("Currency: ");
      Serial.println(currency);
      tft.println(currency);
      Serial.println("Writing API to EEPROM...");
      tft.println("Writing API to EEPROM...");


      // Store wifi and currency config
      Serial.println("Writing Password to nvr...");
      tft.println("Writing Password to nvr...");
      preferences.putString("WIFI_SSID", ssid);
      preferences.putString("WIFI_PASSWD", pass);
      preferences.putString("CURRENCY", currency);


      Serial.println("Write nvr done!");
      tft.println("Write nvr done!");
      String s = "<h1>Setup complete.</h1><p>device will be connected to \"";
      s += ssid;
      s += "\" after the restart.";
      webServer.send(200, "text/html", makePage("Wi-Fi Settings", s));
      delay(3000);
      ESP.restart();
    });
    webServer.onNotFound([]() {
      String s = "<h1>AP mode</h1><p><a href=\"/settings\">Wi-Fi Settings</a></p>";
      webServer.send(200, "text/html", makePage("AP setup mode", s));
    });
  }
  else {
    
    Serial.print("Starting Web Server at ");
    Serial.println(WiFi.localIP());
        
    webServer.on("/", []() {
      String s = "<h1>STA mode</h1><p><a href=\"/reset\">Reset Wi-Fi Settings</a></p>";
      webServer.send(200, "text/html", makePage("STA mode", s));
    });

    webServer.on("/reset", []() {
      // reset the wifi config
      preferences.remove("WIFI_SSID");
      preferences.remove("WIFI_PASSWD");
      preferences.remove("CURRENCY");
      String s = "<h1>Wi-Fi settings was reset.</h1><p>Device will start automaticly.</p>";
      webServer.send(200, "text/html", makePage("Reset Wi-Fi Settings", s));
      delay(3000);
      ESP.restart();
    });
      
  }
 
  webServer.begin();
}

void setupMode() {
  WiFi.mode(WIFI_MODE_STA);
  WiFi.disconnect();
  delay(100);
  int n = WiFi.scanNetworks();
  delay(100);
  Serial.println(".");
  tft.println(".");
  for (int i = 0; i < n; ++i) {
    ssidList += "<option value=\"";
    ssidList += WiFi.SSID(i);
    ssidList += "\">";
    ssidList += WiFi.SSID(i);
    ssidList += "</option>";
  }
  delay(100);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(apSSID);
  WiFi.mode(WIFI_MODE_AP);
  startWebServer();
  Serial.print("Starting Access Point at \"");
  Serial.print(apSSID);
  Serial.println("\"");
}

String makePage(String title, String contents) {
  String s = "<!DOCTYPE html><html><head>";
  s += "<meta name=\"viewport\" content=\"width=device-width,user-scalable=0\">";
  s += "<title>";
  s += title;
  s += "</title></head><body>";
  s += contents;
  s += "</body></html>";
  return s;
}

String urlDecode(String input) {
  String s = input;
  s.replace("%20", " ");
  s.replace("+", " ");
  s.replace("%21", "!");
  s.replace("%22", "\"");
  s.replace("%23", "#");
  s.replace("%24", "$");
  s.replace("%25", "%");
  s.replace("%26", "&");
  s.replace("%27", "\'");
  s.replace("%28", "(");
  s.replace("%29", ")");
  s.replace("%30", "*");
  s.replace("%31", "+");
  s.replace("%2C", ",");
  s.replace("%2E", ".");
  s.replace("%2F", "/");
  s.replace("%2C", ",");
  s.replace("%3A", ":");
  s.replace("%3A", ";");
  s.replace("%3C", "<");
  s.replace("%3D", "=");
  s.replace("%3E", ">");
  s.replace("%3F", "?");
  s.replace("%40", "@");
  s.replace("%5B", "[");
  s.replace("%5C", "\\");
  s.replace("%5D", "]");
  s.replace("%5E", "^");
  s.replace("%5F", "-");
  s.replace("%60", "`");
  return s;
}


void wifistatus() 
{
  Serial.print("WiFi Signal strength: ");
  Serial.print(WiFi.RSSI());
              
  float RSSI = 0.0;
  int bars;
  RSSI = WiFi.RSSI();

  if (RSSI >= -55) {
       bars = 5;
       Serial.println(" 5 bars");
    } else if (RSSI < -55 & RSSI >= -65) {
        bars = 4;
    Serial.println(" 4 bars");
    }  else if (RSSI < -65 & RSSI >= -70) {
       bars = 3;
    Serial.println(" 3 bars");
    } else if (RSSI < -70 & RSSI >= -78) {
       bars = 2;
    Serial.println(" 2 bars");
    } else if (RSSI < -78 & RSSI >= -82) {
       bars = 1;
    Serial.println(" 1 bars");
    } else {
       bars = 0;
    Serial.println(" 0 bars");
  }

 tft.fillRect(130, 0, 30, 20, TFT_BLACK); //wifi RSSI

 for (int b = 0; b <= bars; b++) {
     tft.fillRect(127 + (b * 5), 20 - (b * 3), 4, b * 3, TFT_WHITE);   
  }

}


void btcticker() 
{

    if(curren_cy == "EUR"){
      getbitcoin = httpGETRequest2(api2);
      }
    if(curren_cy == "eur"){
      getbitcoin = httpGETRequest2(api2);
      }

    if(curren_cy == "USD"){
      getbitcoin = httpGETRequest(api1);
      }
    if(curren_cy == "usd"){
      getbitcoin = httpGETRequest(api1);
      }

    if(curren_cy == "TWD"){
      getbitcoin = httpGETRequest3(api3);
      }
    if(curren_cy == "twd"){
      getbitcoin = httpGETRequest3(api3);
      }

      Serial.println(getbitcoin);
      JSONVar bitcoin_obj = JSON.parse(getbitcoin);  
    if (JSON.typeof(bitcoin_obj) == "undefined") {
        Serial.println("Parsing input failed!");
        return;
      }

      wifistatus();
    
      Serial.print("JSON object = ");
      Serial.println(bitcoin_obj);

      JSONVar bitcoin_keys = bitcoin_obj.keys();

      for (int i = 0; i < bitcoin_keys.length(); i++) {
        JSONVar bitcoin_value = bitcoin_obj[bitcoin_keys[i]];
        Serial.print(bitcoin_keys[i]);
        Serial.print(" = ");
        Serial.println(bitcoin_value);
        getDogeArr[i] = double(bitcoin_value);
      }

      Serial.print("Name = ");
      Serial.println(bitcoin_obj[0]["name"]);
      Serial.print("USD = ");
      Serial.println(bitcoin_obj[0]["current_price"]);
      bitcoin_currentprice=bitcoin_obj[0]["current_price"];
      //btc
      Serial.print("Name = ");
      Serial.println(bitcoin_obj[0]["name"]);

      Serial.print("Rank = ");
      Serial.println(bitcoin_obj[0]["market_cap_rank"]);
      btc_rank=bitcoin_obj[0]["market_cap_rank"];

      Serial.print("high_24h = ");
      Serial.println(bitcoin_obj[0]["high_24h"]);
      btc_high_24h=bitcoin_obj[0]["high_24h"];

      Serial.print("low_24h = ");
      Serial.println(bitcoin_obj[0]["low_24h"]);
      btc_low_24h=bitcoin_obj[0]["low_24h"]; 

      Serial.print("price_change_percentage_24h = ");
      Serial.println(bitcoin_obj[0]["price_change_percentage_24h"]);
      btc_price_change_percentage_24h=bitcoin_obj[0]["price_change_percentage_24h"];

      Serial.print("USD = ");
      Serial.println(bitcoin_obj[0]["current_price"]);
      btc_currentprice=bitcoin_obj[0]["current_price"];
    
  tft.pushImage(25, 2, bitcoinsWidth, bitcoinsHeight, bitcoins);
  tft.setTextColor(TFT_WHITE);
  

  if (btc_price_change_percentage_24h <= 0) {      
    tft.setTextColor(TFT_RED);
    leds = CHSV(0, 255, 55);
    FastLED.show();
   } 

  if (btc_price_change_percentage_24h >= 0) {
    tft.setTextColor(TFT_GREEN); 
    leds = CHSV(96, 255, 55);
    FastLED.show();     
  }   
    tft.fillRect(0, 30, 160, 25, TFT_BLACK);

  if (btc_currentprice >= 1000000 && btc_currentprice < 10000000) {
    tft.drawString(String(btc_currentprice , 0).c_str(), 16, 30, 4);

        if(curren_cy == "USD"){
          tft.drawString("USD", 119, 36, 2);
         }
        if(curren_cy == "usd"){
          tft.drawString("USD", 119, 36, 2);
         }

        if(curren_cy == "EUR"){
          tft.drawString("EUR", 119, 36, 2);
         }
        if(curren_cy == "eur"){
          tft.drawString("EUR", 119, 36, 2);
         }

        if(curren_cy == "TWD"){
          tft.drawString("TWD", 119, 36, 2);
         }
        if(curren_cy == "twd"){
          tft.drawString("TWD", 119, 36, 2);
         }
  } 

  if (btc_currentprice >= 100000 && btc_currentprice < 1000000) {
    tft.drawString(String(btc_currentprice , 0).c_str(), 23, 30, 4);

        if(curren_cy == "USD"){
          tft.drawString("USD", 112, 36, 2);
         }
        if(curren_cy == "usd"){
          tft.drawString("USD", 112, 36, 2);
         }

        if(curren_cy == "EUR"){
          tft.drawString("EUR", 112, 36, 2);
         }
        if(curren_cy == "eur"){
          tft.drawString("EUR", 112, 36, 2);
         }

        if(curren_cy == "TWD"){
          tft.drawString("TWD", 112, 36, 2);
         }
        if(curren_cy == "twd"){
          tft.drawString("TWD", 112, 36, 2);
         }
  } 

  if (btc_currentprice >= 10000 && btc_currentprice < 100000) {
    tft.drawString(String(btc_currentprice , 0).c_str(), 30, 30, 4);

        if(curren_cy == "USD"){
          tft.drawString("USD", 105, 36, 2);
         }
        if(curren_cy == "usd"){
          tft.drawString("USD", 105, 36, 2);
         }

        if(curren_cy == "EUR"){
          tft.drawString("EUR", 105, 36, 2);
         }
        if(curren_cy == "eur"){
          tft.drawString("EUR", 105, 36, 2);
         }

        if(curren_cy == "TWD"){
          tft.drawString("TWD", 105, 36, 2);
         }     
        if(curren_cy == "twd"){
          tft.drawString("TWD", 105, 36, 2);
         } 
  }
  
  // print 24h percentage change
  tft.fillRect(0, 60, 160, 20, TFT_BLACK);

  if (btc_price_change_percentage_24h <= 0) {
    tft.drawString(String(btc_price_change_percentage_24h , 2).c_str(), 110, 60, 2);
    tft.drawString("%", 148, 60, 2);      
  }  

  if (btc_price_change_percentage_24h <= -10) {
    tft.drawString(String(btc_price_change_percentage_24h , 2).c_str(), 103, 60, 2);
    tft.drawString("%", 148, 60, 2);      
  }  

  if (btc_price_change_percentage_24h >= 0) {
    tft.drawString(String(btc_price_change_percentage_24h , 2).c_str(), 116, 60, 2);
    tft.drawString("%", 148, 60, 2);      
  }  
  
  tft.setTextColor(TFT_GRAY);
  tft.drawString("Change in 24h", 4, 60, 2); 
  delay(20000);

  // print 24h high
  tft.fillRect(0, 60, 160, 20, TFT_BLACK);
  tft.setTextColor(TFT_GRAY);
  tft.drawString("24h high", 12, 60, 2); 
  tft.setTextColor(TFT_GREEN);

  if (btc_currentprice >= 1000000 && btc_currentprice < 100000000) {
    tft.drawString(String(btc_high_24h ,0).c_str(), 70, 60, 2); 
  } 
  if (btc_currentprice >= 100000 && btc_currentprice < 1000000) {
    tft.drawString(String(btc_high_24h ,0).c_str(), 78, 60, 2);  
  }
  if (btc_currentprice >= 10000 && btc_currentprice < 100000) {
    tft.drawString(String(btc_high_24h ,0).c_str(), 86, 60, 2);  
  }
  
  if(curren_cy == "USD"){
    tft.drawString("USD", 129, 66, 1);
  }
  if(curren_cy == "usd"){
    tft.drawString("USD", 129, 66, 1);
  }

  if(curren_cy == "EUR"){
    tft.drawString("EUR", 129, 66, 1);
  }
  if(curren_cy == "eur"){
    tft.drawString("EUR", 129, 66, 1);
  }

  if(curren_cy == "TWD"){
    tft.drawString("TWD", 129, 66, 1);
  } 
  if(curren_cy == "twd"){
    tft.drawString("TWD", 129, 66, 1);
  } 


  delay(20000);

  // print 24h low
  tft.fillRect(0, 60, 160, 20, TFT_BLACK);
  tft.setTextColor(TFT_GRAY);
  tft.drawString("24h low", 12, 60, 2); 
  tft.setTextColor(TFT_RED);

  if (btc_currentprice >= 1000000 && btc_currentprice < 100000000) {
    tft.drawString(String(btc_low_24h ,0).c_str(), 70, 60, 2); 
  } 
  if (btc_currentprice >= 100000 && btc_currentprice < 1000000) {
    tft.drawString(String(btc_low_24h ,0).c_str(), 78, 60, 2);  
  }
  if (btc_currentprice >= 10000 && btc_currentprice < 100000) {
    tft.drawString(String(btc_low_24h ,0).c_str(), 86, 60, 2);  
  }

  if(curren_cy == "USD"){
    tft.drawString("USD", 129, 66, 1);
  }
  if(curren_cy == "usd"){
    tft.drawString("USD", 129, 66, 1);
  }

  if(curren_cy == "EUR"){
    tft.drawString("EUR", 129, 66, 1);
  }
  if(curren_cy == "eur"){
    tft.drawString("EUR", 129, 66, 1);
  }

  if(curren_cy == "TWD"){
    tft.drawString("TWD", 129, 66, 1);
  } 
  if(curren_cy == "twd"){
    tft.drawString("TWD", 129, 66, 1);
  } 


  delay(20000);

delay( 1000 );

}

String httpGETRequest(const char* api1) {
  HTTPClient http;
    
  // Your IP address with path or Domain name with URL path 
  http.begin(api1);
  
  // Send HTTP POST request
  int httpResponseCode = http.GET();
  
  String payload = "{}"; 
  
  if (httpResponseCode>0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();

  return payload;
}


String httpGETRequest2(const char* api2) {
  HTTPClient http;
    
  // Your IP address with path or Domain name with URL path 
  http.begin(api2);
  
  // Send HTTP POST request
  int httpResponseCode = http.GET();
  
  String payload = "{}"; 
  
  if (httpResponseCode>0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();

  return payload;
}


String httpGETRequest3(const char* api3) {
  HTTPClient http;
    
  // Your IP address with path or Domain name with URL path 
  http.begin(api3);
  
  // Send HTTP POST request
  int httpResponseCode = http.GET();
  
  String payload = "{}"; 
  
  if (httpResponseCode>0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();

  return payload;
}