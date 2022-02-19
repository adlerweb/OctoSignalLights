#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>

extern "C" {
#include "user_interface.h"
}

#define SSID "freifunk-myk.de"
#define PASS ""
#define OCTO "http://octoprint.meinnetz:5000"
#define TOKN "0123456789ABCDEF0123456789ABCDEF"

#define DEBUG   true

#define SLEEP delay(1000)
//#define SLEEP ESP.deepSleep(1000000)

#define CON_FREQ 5 //CON_FREQ * DEEP_SLEEP = Time between polls

#define red     D1
#define yellow  D2
#define green   D3
#define blue    D4
#define white   D5
#define beeper  D6

#define b_red     0b10000000
#define b_yellow  0b01000000
#define b_green   0b00100000
#define b_blue    0b00010000
#define b_white   0b00001000
#define b_beeper  0b00000100

#define API_PRT "/api/printer?exclude=sd"

ESP8266WiFiMulti WiFiMulti;
os_timer_t blinkTimer;

uint8_t LEDState = 0;
uint8_t LEDBlink = 0;
int8_t pollcnt = 0;

void clearLed(void) {
  if(DEBUG) Serial.println("[OO] Clear");
  digitalWrite(red, LOW);
  digitalWrite(yellow, LOW);
  digitalWrite(green, LOW);
  digitalWrite(blue, LOW);
  digitalWrite(white, LOW);
  digitalWrite(beeper, LOW);
}

/*void setLed(int LED, bool blink) {
  digitalWrite(LED, HIGH);
}*/

void setSingle(uint8_t mask, int LED) {
  if(!(LEDState & mask)) {
    digitalWrite(LED, LOW);
  }else if(!(LEDBlink & mask)) {
    digitalWrite(LED, HIGH);
  }
}

void setLeds(uint8_t state, uint8_t blink) {
  LEDState = state;
  LEDBlink = blink;

  setSingle(b_red, red);
  setSingle(b_yellow, yellow);
  setSingle(b_green, green);
  setSingle(b_blue, blue);
  setSingle(b_white, white);
  setSingle(b_beeper, beeper);
}

void blinkSingle(uint8_t mask, int LED) {
  if((LEDState & mask) && (LEDBlink & mask)) {
    digitalWrite(LED, !digitalRead(LED));
  }
}

void blinkLeds(void) {
  blinkSingle(b_red, red);
  blinkSingle(b_yellow, yellow);
  blinkSingle(b_green, green);
  blinkSingle(b_blue, blue);
  blinkSingle(b_white, white);
  blinkSingle(b_beeper, beeper);
}

void blinkTimerCallback(void *pArg) {
  blinkLeds();
} 

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println();

  Serial.println("[BB] Booting OctoLights 0.1");

  Serial.print("[CC] Connecting to WiFi ");
  Serial.println(SSID);

  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP(SSID, PASS);

  pinMode(red, OUTPUT);
  pinMode(yellow, OUTPUT);
  pinMode(green, OUTPUT);
  pinMode(blue, OUTPUT);
  pinMode(white, OUTPUT);
  pinMode(beeper, OUTPUT);

  clearLed();

  ArduinoOTA.begin();

  delay(100);

  os_timer_setfn(&blinkTimer, blinkTimerCallback, NULL);
  os_timer_arm(&blinkTimer, 1000, true);
}

void loop() {
  uint8_t ledStateNew = 0;
  uint8_t ledBlinkNew = 0;

  ArduinoOTA.handle();

  if(WiFiMulti.run() != WL_CONNECTED) {
    Serial.println("[!!] No WiFi");
    ledStateNew |= b_white;
  }else{
    if(pollcnt <= 0) {
      if(DEBUG) Serial.println("[DD] Pulling");
      WiFiClient client;
      HTTPClient http;
      String url_prt = String(OCTO)+String(API_PRT);
      const char *url_prt_c = url_prt.c_str();
      //if(DEBUG) Serial.print("[DD] URL: ");
      //if(DEBUG) Serial.println(url_prt_c);
      http.begin(client, url_prt_c);
      http.addHeader("X-Api-Key", TOKN);
      int http_status = http.GET();
      if(http_status == HTTP_CODE_OK) {
        if(DEBUG) Serial.println("[AA] HTTP 200 OK");
        DynamicJsonDocument json_prt(768);
        DeserializationError error = deserializeJson(json_prt, http.getStream());
        if(error) {
          Serial.print(F("deserializeJson() failed: "));
          Serial.println(error.f_str());
        }else{
          uint8_t bedTemp_a = (json_prt["temperature"]["bed"]["actual"]);
          uint8_t toolTemp_a = (json_prt["temperature"]["tool0"]["actual"]);
          uint8_t bedTemp_t = (json_prt["temperature"]["bed"]["target"]);
          uint8_t toolTemp_t = (json_prt["temperature"]["tool0"]["target"]);

          Serial.print("Temperatures - Bed:");
          Serial.print(bedTemp_a);
          Serial.print("/");
          Serial.print(bedTemp_t);
          Serial.print(" - Tool0:");
          Serial.print(toolTemp_a);
          Serial.print("/");
          Serial.println(toolTemp_t);

          if(json_prt["state"]["flags"]["operational"]) {
            Serial.println("[OO] Printer ready");
            ledStateNew |= b_green;
          /*}else if (json_prt["state"]["flags"]["ready"]) {
            Serial.println("[OO] Printer busy");
            ledStateNew |= b_green;
            ledBlinkNew |= b_green;*/
          }        
          if(json_prt["state"]["flags"]["printing"]) {
            Serial.println("[OO] Printer printing");
            ledStateNew |= b_blue;
          }
          if(json_prt["state"]["flags"]["paused"]) {
            Serial.println("[OO] Printer paused");
            ledStateNew |= b_blue;
            ledBlinkNew |= b_blue;
          }
          if(json_prt["state"]["flags"]["error"]) { 
            Serial.println("[OO] Printer error");
            ledStateNew |= b_red;
            ledBlinkNew |= b_red;
            ledStateNew |= b_beeper;
            ledBlinkNew |= b_beeper;
          }

          if((bedTemp_a >= 40 && bedTemp_a >= bedTemp_t-10 && bedTemp_t > 0) || (toolTemp_a >= 40 && toolTemp_a >= toolTemp_t-10 && toolTemp_t>0)) {
            ledStateNew |= b_yellow;
            Serial.println("[OO] Printer hot");
          }else if(bedTemp_a >= 40 || toolTemp_a >= 40) {
            ledStateNew |= b_yellow;
            ledBlinkNew |= b_yellow;
            Serial.println("[OO] Printer heating");
          }
        }
      }else if(http_status == HTTP_CODE_UNAUTHORIZED) {
        Serial.println("[EE] API Token rejected");
        ledStateNew |= b_white;
        ledBlinkNew |= b_white;
      }else if(http_status == HTTP_CODE_CONFLICT) {
        Serial.println("[II] Printer offline");
        ledStateNew = 0;
        //keep everything off
      }else{
        Serial.println("[EE] Unknown connection problem");
        ledStateNew |= b_white;
      }
      pollcnt = CON_FREQ;

      if(ledStateNew == 0) {
        clearLed();
      }else{
        setLeds(ledStateNew, ledBlinkNew);
      }
    }
  }

  SLEEP;
  pollcnt--;
}