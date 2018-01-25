#define DHTTYPE DHT11 
#define DHTPIN D3

#include <ESP8266WiFi.h>
#include "FS.h"
#include <WiFiClient.h>
#include <TimeLib.h>
#include <NtpClientLib.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESP8266mDNS.h>
#include <Ticker.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>
#include "FSWebServerLib.h"                       //async web server based on ESP8266WebServer 
#include <Hash.h>
//#include "OneWire.h"                              //DS18B20 temperature sensor
//#include "DHT.h"                                  //DHT11 temperatire & humidity sensor

//DHT     HT_sensor               = DHT();
//DHT       dht                       (DHTPIN, DHTTYPE);
//OneWire   ds                        (12);         // DS18B20 Data pin GPIO12 (D6)
String    strT                    = "n/a";        //string to json value "temp"
const int blueled                 = 2;            // GPIO2(pin D4) //internal led
const int led[3]                  = {16,5,4};     // GPIO16(pin D0)// GPIO5(pin D1)// GPIO4(pin D2)
int       current_leds_values[3]  = {0,0,0};// PWM values of leds inverted

void  callbackSET(AsyncWebServerRequest *request)
    {
    String param_name,param_value;
    String values = "url:" + request->url() + "\n";
    if (request->args() > 0)  //process args
        {
        for (uint8_t i = 0; i < request->args(); i++)
            {
            values += request->argName(i) + ":" + request->arg(i) + "\n";
            param_name=request->argName(i);
            param_value=request->arg(i);
            if (param_name=="led0") current_leds_values[0]=param_value.toInt();
            if (param_name=="led1") current_leds_values[1]=param_value.toInt();
            if (param_name=="led2") current_leds_values[2]=param_value.toInt();
            }
        }
    else values+="no args";

    AsyncWebServerResponse *response = request->beginResponse(200, "text/html", values);
    response->addHeader("Access-Control-Allow-Origin", "*");
    response->addHeader("Connection", "close");
    request->send(response);

    //request->send(200, "text/plain", values);
    values = "";
    }

void setup() {                                    // WiFi is started inside library
    Serial.begin(115200);
    Serial.println("\r\n\r\n-[INITIALIZING]-");
    Serial.println("ESP8266_AJAX_TEST v0.1a compiled " __DATE__ "," __TIME__);

    pinMode(blueled, OUTPUT);
    pinMode(led[0],  OUTPUT);
    pinMode(led[1],  OUTPUT);
    pinMode(led[2],  OUTPUT);

    analogWriteFreq(1000);                  //PWM 1kHz
    analogWriteRange(100);                  //PWM full range from 0 to 100

    analogWrite(led[0],current_leds_values[0]);//analogRead() DOESN'T WORK WITH PWM! storing values in current_leds_values[]
    analogWrite(led[1],current_leds_values[1]);
    analogWrite(led[2],current_leds_values[2]);

    SPIFFS.begin(); // Not really needed, checked inside library and started if needed
    ESPHTTPServer.begin(&SPIFFS);
    //ESPHTTPServer.setJSONCallback(callbackJSON);
    ESPHTTPServer.setSETCallback(callbackSET);
    //dht.begin();
    
    Serial.println("\r\n-[INIT DONE]-");
}

//------------------------------------------------------------------------------------

void loop() {
    static bool   temper_read       = true;
    static bool   temper_humid_read = true;
    unsigned long sec               = millis() / 1000;
    byte          cycle_sec         = sec % 60;               // every second

    //Serial.println(cycle_sec);
    /*if  (cycle_sec == 0 || cycle_sec == 30)                   // read DS18B20 every 30 seconds 
        {if (temper_read) Serial.println(temper());
        temper_read=false;}
    if  (cycle_sec == 1 || cycle_sec == 31) temper_read=true; // read DS18B20 only 1 time in second

    if  (cycle_sec == 58)                                     // read DHT11 every minute
        {if (temper_humid_read) Serial.println(temper_humid());
        temper_humid_read=false;}
    if  (cycle_sec == 59) temper_humid_read=true;             //read DHT11 only 1 time in minute
    */
    analogWrite(led[0],current_leds_values[0]);//analogRead() DOESN'T WORK WITH PWM! storing values in current_leds_values[]
    analogWrite(led[1],current_leds_values[1]);
    analogWrite(led[2],current_leds_values[2]);

    ESPHTTPServer.handle();                                   // DO NOT REMOVE. Attend OTA update from Arduino IDE
}

