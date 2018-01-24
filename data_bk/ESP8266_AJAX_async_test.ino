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
#include "OneWire.h"                              //DS18B20 temperature sensor
#include "DHT.h"                                  //DHT11 temperatire & humidity sensor

//DHT     HT_sensor               = DHT();
DHT       dht                       (DHTPIN, DHTTYPE);
OneWire   ds                        (12);         // DS18B20 Data pin GPIO12 (D6)
String    strT                    = "n/a";        //string to json value "temp"
const int blueled                 = 2;            // GPIO2(pin D4) //internal led
const int led[3]                  = {16,5,4};     // GPIO16(pin D0)// GPIO5(pin D1)// GPIO4(pin D2)
int       current_leds_values[3]  = {100,100,100};// PWM values of leds inverted

void  callbackJSON(AsyncWebServerRequest *request)
    {
    String values = "url:" + request->url() + "\n";
    if (request->args() > 0)  //process args
        {
        for (uint8_t i = 0; i < request->args(); i++)
            {
            values += request->argName(i) + ":" + request->arg(i) + "\n";
            }
        }

    request->send(200, "text/plain", values);
    values = "";
    }


void setup() {                                    // WiFi is started inside library
    Serial.begin(115200);
    Serial.println("\r\n\r\n--------------------------------INITIALIZING");
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
    ESPHTTPServer.setJSONCallback(callbackJSON);
    //HT_sensor.attach(0);// GPIO0, pin D3
    //HT_sensor.attach(D3);// GPIO0, pin D3
    dht.begin();
    
    Serial.println("\r\n--------------------------------INIT DONE");
}

String temper()
    {
    char    buf[30];
    byte    i;
    byte    present = 0;
    byte    data[12];
    byte    addr[8];
    float   celsius;
    if (!ds.search(addr))
        {
        ds.reset_search();
        delay(250);
        strT="DS18B20 not found";
        }
    else
        {
        OneWire::crc8(addr, 7);
        ds.reset();
        ds.select(addr);
        ds.write(0x44, 1); // start conversion, with parasite power on at the end
        delay(800); // maybe 750ms is enough, maybe not
        present = ds.reset();
        ds.select(addr);
        ds.write(0xBE); // Read Scratchpad
        for (i = 0; i < 9; i++)
            { // we need 9 bytes
            data[i] = ds.read();
            }
        // Convert the data to actual temperature
        // because the result is a 16 bit signed integer, it should
        // be stored to an "int16_t" type, which is always 16 bits
        // even when compiled on a 32 bit processor.
        int16_t raw = (data[1] << 8) | data[0];
        byte cfg = (data[4] & 0x60);
        // at lower res, the low bits are undefined, so let's zero them
        if (cfg == 0x00) raw = raw & ~7; // 9 bit resolution, 93.75 ms
        else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
        else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
                                              //// default is 12 bit resolution, 750 ms conversion time
        celsius = (float)raw / 16.0;
        dtostrf(celsius, 5, 2, buf);
        strT=buf;
        strT+="&deg;C";
        }
    return strT;
    }//temper

String temper_humid()
    {
    char    msg[128];
    String  strTH;

    // Reading temperature or humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
    float h = dht.readHumidity();
    // Read temperature as Celsius (the default)
    float t = dht.readTemperature();

    // Check if any reads failed and exit early (to try again).
    if (isnan(h) || isnan(t))
        {
        strTH="Failed to read from DHT sensor!";
        return strTH;
        }

    // Compute heat index in Celsius (isFahreheit = false)
    float hic = dht.computeHeatIndex(t, h, false);
    sprintf(msg, "Temperature = %dC, Humidity = %d%%, Heat index = %dC", t,h,hic);
    strTH=String(msg);
    return strTH;
    }//temper_humid
    
/*String temper_humid()
    {
    char    msg[128];
    String  strTH;

    HT_sensor.update();

    switch (HT_sensor.getLastError())
        {
        case DHT_ERROR_OK:
            // данные последнего измерения можно считать соответствующими
            // методами
            sprintf(msg, "Temperature = %dC, Humidity = %d%%", 
                HT_sensor.getTemperatureInt(), HT_sensor.getHumidityInt());
            break;
        case DHT_ERROR_START_FAILED_1:
            sprintf(msg,"Error: start failed (stage 1)");
            break;
        case DHT_ERROR_START_FAILED_2:
            sprintf(msg,"Error: start failed (stage 2)");
            break;
        case DHT_ERROR_READ_TIMEOUT:
            sprintf(msg,"Error: read timeout");
            break;
        case DHT_ERROR_CHECKSUM_FAILURE:
            sprintf(msg,"Error: checksum error");
            break;
        }
    strTH=String(msg);
    return strTH;
    }//temper_humid*/

void loop() {
    static bool   temper_read       = true;
    static bool   temper_humid_read = true;
    unsigned long sec               = millis() / 1000;
    byte          cycle_sec         = sec % 60;               // every second


    //Serial.println(cycle_sec);
    if  (cycle_sec == 0 || cycle_sec == 30)                   // read DS18B20 every 30 seconds 
        {if (temper_read) Serial.println(temper());
        temper_read=false;}
    if  (cycle_sec == 1 || cycle_sec == 31) temper_read=true; // read DS18B20 only 1 time in second

    if  (cycle_sec == 58)                                     // read DHT11 every minute
        {if (temper_humid_read) Serial.println(temper_humid());
        temper_humid_read=false;}
    if  (cycle_sec == 59) temper_humid_read=true;             //read DHT11 only 1 time in minute

    ESPHTTPServer.handle();                                   // DO NOT REMOVE. Attend OTA update from Arduino IDE
}

