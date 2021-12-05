#include "_config.h"
#include "calibration.h"

#include "SSD1306Wire.h" 
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Wire.h>


WiFiClient wifiClient;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;

int counter = 0;

String wifiMacString;
String wifiIPString;
String timeString = "00:00:00";

void handleRoot() {
  char page[800];
  int sec = millis() / 1000;
  int min = sec / 60;
  int hr = min / 60;
  snprintf(page, 800,
           "<html>\
              <head>\
                <meta http-equiv='refresh' content='5'/>\
                <title>Environment Hub</title>\
                <style>\
                  body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
                </style>\
              </head>\
              <body>\
                <p>Time: %s</p>\
                <p>Uptime: %02d:%02d:%02d</p>\
              </body>\
            </html>", timeString, hr, min % 60, sec % 60
          );
  httpServer.send(200, "text/html", page);
}


void setup()
{
    Serial.begin(115200);
    timeClient.begin();
    
    pinMode(LED_BUILTIN, OUTPUT);
    WiFi.mode(WIFI_AP_STA);
    WiFi.begin(ssid, password);

    Serial.println("Connecting");
    while (WiFi.status() != WL_CONNECTED)
    {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(500);
        digitalWrite(LED_BUILTIN, LOW);
        Serial.print(".");

    }
    Serial.println("");
    Serial.print("Connected to WiFi network with IP Address: ");
    Serial.println(WiFi.localIP());
    MDNS.begin(host);

  httpServer.on("/", handleRoot);

  httpUpdater.setup(&httpServer, update_path, update_username, update_password);
  httpServer.begin();

  MDNS.addService("http", "tcp", 80);
  Serial.printf("HTTPUpdateServer ready! Open http://%s.local%s in your browser and login with username '%s' and password '%s'\n", host, update_path, update_username, update_password);
    wifiMacString = WiFi.macAddress();
    wifiIPString = WiFi.localIP().toString();

    httpServer.on("/", handleRoot);
}

void loop()
{   
    httpServer.handleClient();
    MDNS.update();

    //Check WiFi connection status
    if (WiFi.status() == WL_CONNECTED)
    {
        if (millis() > (ntpInterval + ntpi)) {
          timeClient.update();
          timeString = timeClient.getFormattedTime();
          //Serial.println(timeClient.getFormattedTime());
          ntpi = millis();
        }
        
        HTTPClient http;

        // Your Domain name with URL path or IP address with path
        http.begin(wifiClient, serverName);

        // Specify content-type header
        http.addHeader("Content-Type", "application/x-www-form-urlencoded");

        String httpRequestData = "api_key=" + apiKeyValue;
        httpRequestData = httpRequestData + "&mac_address=" + String(wifiMacString) + "&ip_address=" + String(wifiIPString);

        if (millis() > (sendInterval + si)) {
          digitalWrite(LED_BUILTIN, LOW);
          // uncomment to see the post data in the monitor
          Serial.println(httpRequestData);
          
          // Send HTTP POST request
          int httpResponseCode = http.POST(httpRequestData);

          if (httpResponseCode > 0)
          {
              Serial.print("HTTP Response code: ");
              Serial.println(httpResponseCode);
          }
          else
          {
              Serial.print("Error code: ");
              Serial.println(httpResponseCode);
          }
          si = millis();
           digitalWrite(LED_BUILTIN, HIGH);
        }
        // Free resoclearurces
        http.end();
    }
    else
    {
        Serial.println("WiFi Disconnected");
    }
}
