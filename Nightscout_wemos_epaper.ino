//load libraries - ESP8266 Wifi, SSL support, JSON parser, Time Libraries, and Adafruit Graphics library for epaper display
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <TimeLib.h>
#include <Time.h>
#include <LiquidCrystal_I2C.h>
#include <GxEPD.h>

//define which epaper diaply we're using
#include <GxGDEH029A1/GxGDEH029A1.cpp>      // 2.9" b/w e-paper screen



// FreeFonts from Adafruit_GFX
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





const char* ssid = "SSID"; //enter your SSID
const char* password = "password"; //enter your wifi password
//seconds to sleep for
const int sleepTimeS = 60;

// this section gets the current voltage (approximately) on pin A0 if it is connected in a resistor voltage divider like this: VCC -> 100K -> A0 -> 330K -> GND
int raw = analogRead(A0);
float volt=raw/1023.0;
float getVcc=raw / 194.2;//*4.408;


const char* host = "NIGHTSCOUT HOST"; //enter your nightscout host here - no "https://" needed
const int httpsPort = 443;
  const size_t bufferSize = 3*JSON_ARRAY_SIZE(1) + JSON_OBJECT_SIZE(1) + 2*JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(13) + 280;
// SHA1 fingerprint of the host's SSL certificate
const char* fingerprint = "CE:CE:49:37:75:F4:61:4D:5D:4A:D2:B3:E0:19:9B:FF:EF:E9:A0:76"; //enter the https fingerprint if you want this verification


void setup() {
  
  Serial.begin(115200);
  Serial.setTimeout(2000);
  Serial.println();
  Serial.println(raw);
  Serial.println(getVcc);
  display.init();
 
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);

  }
  Serial.println("");
 
  //lcd.print(WiFi.localIP());

  // Use WiFiClientSecure class to create TLS connection
  WiFiClientSecure client;

  if (!client.connect(host, httpsPort)) {
    Serial.println("connection failed");
    Serial.println("Sleeping...");
    ESP.deepSleep(sleepTimeS * 1000000);
    return;
  }

 if (client.verify(fingerprint, host)) {
   Serial.println("certificate matches");
 
 } else {
   Serial.println("certificate doesn't match");
 
 }

  String url = "/pebble";
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "User-Agent: BuildFailureDetectorESP8266\r\n" +
               "Connection: close\r\n\r\n");

  //Serial.println("request sent");
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      //Serial.println("headers received");
      break;
    }
  }
  String line2 = client.readStringUntil('\n');
  Serial.println("==========");
  //read the JSON line into "line2"
  line2 = client.readStringUntil('\n');
        //parse the JSON into variables
        DynamicJsonBuffer jsonBuffer(bufferSize);
        JsonObject& root = jsonBuffer.parseObject(line2);
        
        String status0_now = root["status"][0]["now"]; 
            status0_now = status0_now.substring(0, status0_now.length() - 3);
            time_t status0_now1 = status0_now.toInt();
        JsonObject& bgs0 = root["bgs"][0];
        const char* bgs0_sgv = bgs0["sgv"]; 
        int bgs0_trend = bgs0["trend"]; 
        const char* bgs0_direction = bgs0["direction"]; 
            time_t bgs0_datetime = bgs0["datetime"];
        String bgs0_datetime2 = bgs0["datetime"]; 
            bgs0_datetime2 = bgs0_datetime2.substring(0, bgs0_datetime2.length() - 3);
            time_t bgs0_datetime3 = bgs0_datetime2.toInt();
//check for British summer time and adjust display time accordingly
            if (isBST(year(status0_now1), month(status0_now1), day(status0_now1), hour(status0_now1))){
              status0_now1 = status0_now1 +3600;
              }
            if (isBST(year(bgs0_datetime3), month(bgs0_datetime3), day(bgs0_datetime3), hour(bgs0_datetime3))){
              bgs0_datetime3 = bgs0_datetime3 +3600;
              }
            

        const char* bgs0_battery = bgs0["battery"]; 
        JsonObject& cals0 = root["cals"][0];
        float cals0_slope = cals0["slope"]; 
        float cals0_intercept = cals0["intercept"]; 
        int cals0_scale = cals0["scale"]; 

        
Serial.print("Time Now: ");
Serial.print(hour(status0_now1));
Serial.print(":");
Serial.println(minute(status0_now1));
String timeNow = String(hour(status0_now1)) + ":";
if (minute(status0_now1) < 10){timeNow = timeNow + "0";} 
timeNow = timeNow + String(minute(status0_now1));
//print current time
Serial.print("As at: ");
Serial.print(hour(bgs0_datetime3));
Serial.print(":");
Serial.println(minute(bgs0_datetime3));
int dataAge = (status0_now1 - bgs0_datetime3) / 60;
Serial.print("Data Age (minutes): ");
Serial.println(dataAge);
Serial.print("Blood Glucose is:  ");
Serial.print(String(bgs0_sgv) + " ");
char* directarr = "";
if (String(bgs0_direction) == "Flat") {
    directarr = "→︎";
}  else if (String(bgs0_direction) == "FortyFiveUp") {
    directarr = "↗︎";
}  else if (String(bgs0_direction) == "FortyFiveDown") {
    directarr = "↘";
} else if (String(bgs0_direction) == "DoubleUp") {
    directarr = "↑↑";
} else if (String(bgs0_direction) == "DoubleDown") {
    directarr = "↓↓";
 }
Serial.println(directarr);

Serial.print("Battery Voltage is:  ");
Serial.println(getVcc);

//call the "showdata" function to show the data on the LCD screen
showdata(timeNow,bgs0_sgv,dataAge,getVcc);


/sleep for a minute
  Serial.println("Sleeping...");
  ESP.deepSleep(sleepTimeS * 1000000);

}
void loop() {
}


void showdata(String timenow, String BG, int age, float volts)
{

  display.setTextColor(GxEPD_BLACK);

  display.setRotation(1);
  
  //Display BG Number
  display.setFont(&NotoSansUI_Regular12pt7b);
  display.setCursor(210, 34);
  display.print("BG:");
  display.setFont(&NotoSansUI_Regular36pt7b);
  display.setCursor(150,100);
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
  
  display.update();

}
bool isBST(int year, int month, int day, int hour)
{
    // bst begins at 01:00 gmt on the last sunday of march
    // and ends at 01:00 gmt (02:00 bst) on the last sunday of october
    // january, february, and november are out
    if (month < 3 || month > 10) { return false; }
    // april to september are in
    if (month > 3 && month < 10) { return true; }
    // last sunday of march
    int lastMarSunday =  (31 - (5* year /4 + 4) % 7);
    // last sunday of october
    int lastOctSunday = (31 - (5 * year /4 + 1) % 7);
    // in march we are bst if its past 1am gmt on the last sunday in the month
    if (month == 3)
    {
        if (day > lastMarSunday)
        {
            return true;
        }
        if (day < lastMarSunday)
        {
            return false;
        }
        if (hour < 1)
        {
            return false;
        }
        return true;
    }
    // in october we must be before 1am gmt (2am bst) on the last sunday to be bst
    if (month == 10)
    {
        if (day < lastOctSunday)
        {
            return true;
        }
        if (day > lastOctSunday)
        {
            return false;
        }
        if (hour >= 1)
        {
            return false;
        }
        return true;
    }
}
