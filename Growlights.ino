#include <Wire.h>
#include <RTClib.h>

/* Create a WiFi access point and provide a web server on it. */
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>

#include <uri/UriBraces.h>

#include "program.h"

#ifndef APSSID
#define APSSID "Grow-Lights"
#define APPSK  "Grow1234!"
#endif

/* Set these to your desired credentials. */
const char *ssid = APSSID;
const char *password = APPSK;

ESP8266WebServer server(80);

void PrintLCDInfo();
void PrintInfoToSerial(DateTime timenow);
String getTimeString(DateTime timenow);

RTC_DS1307 rtc;

int announcePeriod = 10000;
unsigned long ticks_now = 0;

int startHour, startMinute, startSecond;
int endHour, endMinute, endSecond;

// char program[24000];

#define light D6
int lightOn = false;

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

/* Just a little test message.  Go to http://192.168.4.1 in a web browser
   connected to this access point to see it.
*/
void handleRoot() 
{
  String theTime = getTimeString(rtc.now());
  server.send(200, "text/html", "<h1>You are connected to Grow-Lights</h1><p> Time is: " + theTime + " </p>");
}

void handleTentPage() 
{
  digitalWrite(light, HIGH);
  // String theTime = getTimeString(rtc.now());
  server.send(200, "text/html", PAGE_TentPage );
}

void handleLightOn() 
{
  digitalWrite(light, HIGH);
  lightOn = true;
  String theTime = getTimeString(rtc.now());
  server.send(200, "text/html", "<h1>Light On</h1><p> Time is: " + theTime + " </p>");
}

void handleLightOff() 
{
  digitalWrite(light, LOW);
  lightOn = false;
  String theTime = getTimeString(rtc.now());
  server.send(200, "text/html", "<h1>Light Off</h1><p> Time is: " + theTime + " </p>");
}

void setup() 
{
  // put your setup code here, to run once:
  Serial.begin(57600);
  Serial.println("Welcome to Grow-Lights RTC");

  // Wire.begin(D2, D1);
  delay(1000);   

  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    // while (1);
  }

  if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running!");
    // following line sets the RTC to the date & time this sketch was compiled
    // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  } else {
    Serial.println("RTC is running!");
  }

  // This line sets the RTC with an explicit date & time, for example to set
  // January 21, 2014 at 3am you would call:
  // rtc.adjust(DateTime(2024, 1, 29, 12, 22, 0));
  
  pinMode(light, OUTPUT); 
  digitalWrite(light, LOW);
  lightOn= false;
  
  //lcd.begin();
  //lcd.backlight();  

  DateTime now = rtc.now();
    
  Serial.print("Configuring access point...");
  /* You can remove the password parameter if you want the AP to be open. */
  WiFi.softAP(ssid, password);

  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  server.on("/", handleRoot);
  server.on("/tentpage", handleTentPage);
  server.on("/lighton", handleLightOn);
  server.on("/lightoff", handleLightOff);

  // /settime/09:22:05T
  server.on(UriBraces("/settime/{}"), [](){
    String isotime = server.pathArg(0);
    char buf[30];
    isotime.toCharArray(buf, sizeof(buf));
    char *p = buf;
    char* datetime =  strtok(p,"T");
    char* timedata = strtok(0, "T");
    Serial.println("Time Parsing");
    Serial.println("Buf: "+ String(buf));
    // Serial.println("Time Data: " + String(timedata));

    char szTimeBuf[10];
    strcpy(szTimeBuf, buf);

    char* tmptr = strtok(szTimeBuf, ":");
    int thehour = atoi(tmptr);
    tmptr = strtok(NULL, ":");
    int theminute = atoi(tmptr);
    tmptr = strtok(NULL, ":");
    int thesec = atoi(tmptr);

    Serial.print("H: ");
    Serial.print(thehour);
    Serial.print(" M: ");
    Serial.print(theminute);
    Serial.print(" S: ");
    Serial.println(thesec);

    rtc.adjust(DateTime(2025, 3, 24, thehour, theminute, thesec));

    String theTime = getTimeString(rtc.now());
    server.send(200, "text", "OKAY");
  });

  server.begin();
  Serial.println("HTTP server started");

  startHour = 9;
  startMinute = 0;
  startSecond = 0;

  endHour = 21;
  endMinute = 30;
  endSecond = 0;
}

void loop() 
{
  DateTime now = rtc.now();
  // PrintLCDInfo();

  // Test for Turning ON lights
  if(!lightOn) {
    if (now.hour() == startHour && now.minute() == startMinute && now.second() >= startSecond && now.second() <= startSecond + 2)
    {
      lightOn = true;
      digitalWrite(light, HIGH);
    }
  }

  // Test for Turning OFF lights
  if(lightOn) {
    if (now.hour() == endHour && now.minute() == endMinute && now.second() >= endSecond && now.second() <= endSecond + 2)
    {
      lightOn = false;
      digitalWrite(light, LOW);
    }
  }
  

  if( millis() > ticks_now + announcePeriod )
  {
    ticks_now = millis();
    PrintInfoToSerial(now);
  }

  server.handleClient();
}

String getTimeString(DateTime timenow) 
{
  char timebuf[48];
  sprintf(timebuf,"%4d/%2d/%2d (%s) %2d:%2d:%2d", timenow.year(), timenow.month(), timenow.day(), daysOfTheWeek[timenow.dayOfTheWeek()], timenow.hour(), timenow.minute(), timenow.second());
  
  return timebuf;
}

void PrintInfoToSerial(DateTime timenow)
{
  Serial.print(timenow.year(), DEC);
  Serial.print('/');
  Serial.print(timenow.month(), DEC);
  Serial.print('/');
  Serial.print(timenow.day(), DEC);
  Serial.print(" (");
  Serial.print(daysOfTheWeek[timenow.dayOfTheWeek()]);
  Serial.print(") ");
  Serial.print(timenow.hour(), DEC);
  Serial.print(':');
  if(timenow.minute() <10)
    Serial.print('0');
  Serial.print(timenow.minute(), DEC);
  Serial.print(':');
  if(timenow.second() <10)
    Serial.print('0');
  Serial.print(timenow.second(), DEC);
  Serial.println();
}

void PrintLCDInfo() 
{
  /*
  lcd.print("DATE: ");
  lcd.print(now.year(), DEC);
  lcd.print('/');
  lcd.print(now.month(), DEC);
  lcd.print('/');
  lcd.print(now.day(), DEC);
  lcd.setCursor(0,1);
  lcd.print("TIME: ");
  lcd.print(now.hour(), DEC);
  lcd.print(':');
  if(now.minute() <10)
    lcd.print('0');
  lcd.print(now.minute(), DEC);
  lcd.print(':');
  if(now.second() <10)
    lcd.print('0');
  lcd.print( now.second(), DEC);
  lcd.setCursor(0,0);
  */
}
