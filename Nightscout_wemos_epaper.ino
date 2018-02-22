
//load libraries - ESP8266 Wifi, SSL support, JSON parser, Time Libraries, and Adafruit Graphics library for epaper display
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <TimeLib.h>
#include <Time.h>
#include <GxEPD.h>

//define which epaper diaply we're using
#include <GxGDEH029A1/GxGDEH029A1.cpp>      // 2.9" b/w



// load fonts for epaper display
#include <Fonts/NotoSansUI9pt7b.h>
#include <Fonts/NotoSansUI12pt7b.h>
#include <Fonts/NotoSansUI18pt7b.h>
#include <Fonts/NotoSansUI24pt7b.h>
#include <Fonts/NotoSansUI36pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold24pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <GxIO/GxIO_SPI/GxIO_SPI.cpp>
#include <GxIO/GxIO.cpp>

//set epaper output pins. These should be connected as follows to the D1 Mini:
//  BUSY -> D2
//  RST -> D4
//  DC -> D3
//  CS -> D8
//  CLK -> D5
//  DIN -> D7
//  GND -> GND
//  3.3V -> 3.3V
//init setup for the epaper display
GxIO_Class io(SPI, SS, 0, 2); 
GxEPD_Class display(io); 



//wifi connection details
const char* ssid = "YOUR SSID HERE";
const char* password = "YOUR WIFI PASSWORD";
//seconds to sleep for
const int sleepTimeS = 60;
//host to connect to
const char* host = "NIGHSCOUT HOST HERE";
const int httpsPort = 443;
//setup JSON Buffer
const size_t bufferSize = 3*JSON_ARRAY_SIZE(1) + JSON_OBJECT_SIZE(1) + 2*JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(13) + 280;

// SHA1 fingerprint of the host's SSL certificate
const char* fingerprint = "CE:CE:49:37:75:F4:61:4D:5D:4A:D2:B3:E0:19:9B:FF:EF:E9:A0:76";

//read voltage on pin A0 to get battery voltage through resistor divider
int raw = analogRead(A0);
float volt=raw/1023.0;
float getVcc=volt*4.408;

void setup() {
  
  Serial.begin(115200);
  Serial.setTimeout(2000);
  Serial.println();
  display.init();
  //Serial.print("connecting to ");
  //Serial.println(ssid);
 
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);

  }
 //print Wifi address to Serial interface
  Serial.println("");
  Serial.print(WiFi.localIP());

  // Use WiFiClientSecure class to create TLS connection
  WiFiClientSecure client;
  if (!client.connect(host, httpsPort)) {
    Serial.println("connection failed");
    Serial.println("Sleeping...");
    //sleep if we failed, retry next time
    ESP.deepSleep(sleepTimeS * 1000000);
    return;
  }
//rudimentary check on Hash of SSL certificate - should match, but we carry on if the fingerprint changes
 if (client.verify(fingerprint, host)) {
   Serial.println("certificate matches");
 } else {
   Serial.println("certificate doesn't match");
 }
//get the data at "<host>/pebble"
  String url = "/pebble";
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "User-Agent: BuildFailureDetectorESP8266\r\n" +
               "Connection: close\r\n\r\n");

  //Serial.println("request sent");
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    //read until we get the headers, then continue
    if (line == "\r") {
      //Serial.println("headers received");
      break;
    }
  }
  //first line we don't need
  String line2 = client.readStringUntil('\n');
  Serial.println("==========");
  //read the JSON line into "line2"
  line2 = client.readStringUntil('\n');
        //parse the JSON into variables
        DynamicJsonBuffer jsonBuffer(bufferSize);
        JsonObject& root = jsonBuffer.parseObject(line2);
        String status0_now = root["status"][0]["now"]; // 1517649894544
        JsonObject& bgs0 = root["bgs"][0];
        const char* bgs0_sgv = bgs0["sgv"]; // "11.0"
        int bgs0_trend = bgs0["trend"]; // 4
        const char* bgs0_direction = bgs0["direction"]; // "Flat"
        String bgs0_datetime = bgs0["datetime"]; // 1517649566004
        const char* bgs0_battery = bgs0["battery"]; // "100"
        JsonObject& cals0 = root["cals"][0];
        float cals0_slope = cals0["slope"]; // 1047.705602117555
        float cals0_intercept = cals0["intercept"]; // 21408.668786983755
        int cals0_scale = cals0["scale"]; // 1

//process the time
            //strip milliseconds from the timestamp
            status0_now = status0_now.substring(0, status0_now.length() - 3);
            //make the time an integer instead of a string
            time_t status0_now1 = status0_now.toInt();
            //strip milliseconds from data timestamp
            bgs0_datetime = bgs0_datetime.substring(0, bgs0_datetime.length() - 3);
            //make the data timestamp an integer instead of a string
            time_t bgs0_datetime3 = bgs0_datetime.toInt();
            //generate timenow string "xx:xx" to include leading zeroes if needed
            String timeNow = String(hour(status0_now1)) + ":";
            if (minute(status0_now1) < 10){timeNow = timeNow + "0";} 
            timeNow = timeNow + String(minute(status0_now1));
            //generate dataAge in minutes 
            int dataAge = (status0_now1 - bgs0_datetime3) / 60;
        
Serial.print("Time Now: ");
Serial.println(timeNow);
Serial.print("As at: ");
Serial.print(hour(bgs0_datetime3));
Serial.print(":");
Serial.println(minute(bgs0_datetime3));
Serial.print("Data Age (minutes): ");
Serial.println(dataAge);
Serial.print("Blood Glucose is:  ");
Serial.print(String(bgs0_sgv) + " ");
Serial.print("Battery Voltage is:  ");
Serial.println(getVcc);

//call a function to display the data on our e-paper display
showdata(timeNow,bgs0_sgv,dataAge,getVcc);

Serial.println("Sleeping...");
//Go to sleep - Pin D0 must be connected to RST
ESP.deepSleep(sleepTimeS * 1000000);
  
}


void loop() {
//do nothing - everything is executed only once, in setup
}


void showdata(String timenow, String BG, int age, float volts)
{
  //display.fillScreen(GxEPD_WHITE);
  display.setTextColor(GxEPD_BLACK);

  display.setRotation(1);
  
  //Display BG Number
  display.setFont(&NotoSansUI_Regular12pt7b);
  display.setCursor(210, 34);
  display.print("BG:");
  display.setFont(&NotoSansUI_Regular36pt7b);
  display.setCursor(150,100);
  //display.println("22.4");
  display.println(BG);
  

  //display time
  display.setFont(&NotoSansUI_Regular24pt7b);
  display.setCursor(0, 36);
  display.println(timenow);

  //display data Age
  display.setFont(&FreeSansBold12pt7b);
  display.setCursor(0, 65);
  display.print("Age: ");
  display.print(String(age));
  display.println(" min");

 //display Voltage
  display.setFont(&NotoSansUI_Regular12pt7b);
  display.setCursor(0, 120);
  display.println(String(volts) + "V");
  
  //push all this to the display
  display.update();

}
