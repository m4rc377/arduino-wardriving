
#include <SPI.h>
#include <SD.h>
#include <TinyGPS++.h>
#include "ESP8266WiFi.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
 
LiquidCrystal_I2C lcd(0x27, 16, 2);
#define ARDUINO_USD_CS 15 // Pin D8
#define LOG_FILE_PREFIX "gpslog"
#define MAX_LOG_FILES 100
#define LOG_FILE_SUFFIX "csv" 
char logFileName[13];
#define LOG_COLUMN_COUNT 13
char * log_col_names[LOG_COLUMN_COUNT] = {
  "Latitude", "Longitude", "Altitude", "Speed", "Course", "Date", "Time (UTC)", "Satellites", "SSID", "Power", "Channel", "Encryption", "BSSID"
};

#define LOG_RATE 5000
unsigned long lastLog = 0;

TinyGPSPlus tinyGPS;
#define GPS_BAUD 9600 // GPS module's default baud rate

#include <SoftwareSerial.h>
#define ARDUINO_GPS_RX 2 // Pin D4
#define ARDUINO_GPS_TX 0 // Pin D3
SoftwareSerial ssGPS(ARDUINO_GPS_TX, ARDUINO_GPS_RX);
#define gpsPort ssGPS 

#define SerialMonitor Serial

void setup() {
  SerialMonitor.begin(115200);
  gpsPort.begin(GPS_BAUD);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Setting up SD card.");
  SerialMonitor.println("Setting up SD card.");
  delay(200);
  lcd.clear();
  if (!SD.begin(ARDUINO_USD_CS)) {
    lcd.setCursor(0, 1);
    lcd.print("Error initializing SD card.");
    SerialMonitor.println("Error initializing SD card.");
  }
  updateFileName();
  printHeader();
}

void loop() {
//  while (gpsPort.available() > 0)
//    tinyGPS.encode(gpsPort.read());
    
  if ((lastLog + LOG_RATE) <= millis()) {
    if (tinyGPS.location.isUpdated()) {
      if (logGPSData()) {
        SerialMonitor.println("GPS logged.");
        lcd.setCursor(0, 0);
        lcd.print("Lat: ");
        lcd.print(tinyGPS.location.lat(), 6);
        SerialMonitor.print(tinyGPS.location.lat(), 6);
        SerialMonitor.print(", ");
        SerialMonitor.println(tinyGPS.location.lng(), 6);
        lcd.setCursor(0, 1);
        lcd.print("Lon: ");
        lcd.print(tinyGPS.location.lng(), 6);
        lastLog = millis();
      } else {
        lcd.setCursor(0, 1);
        SerialMonitor.println("Failed to log new GPS data.");
      }
    } else {
      lcd.setCursor(0, 0);
      lcd.print("No GPS data.");
      lcd.setCursor(0, 1);
      lcd.print("Sats: ");
      lcd.print(tinyGPS.satellites.value());
      SerialMonitor.print("No GPS data. Sats: ");
      SerialMonitor.println(tinyGPS.satellites.value());
      delay(100);
    }
  }
  while (gpsPort.available())
    tinyGPS.encode(gpsPort.read());
}

byte logGPSData() {
  File logFile = SD.open(logFileName, FILE_WRITE);

  if (logFile) {
    int n = WiFi.scanNetworks(); 
    if (n == 0) {
      Serial.println("no networks found");
    } else {
      for (uint8_t i = 1; i <= n; ++i) {
        logFile.print(tinyGPS.location.lat(), 6);
        logFile.print(',');
        logFile.print(tinyGPS.location.lng(), 6);
        logFile.print(',');
        logFile.print(tinyGPS.altitude.meters(), 1);
        logFile.print(',');
        logFile.print(tinyGPS.speed.kmph(), 1);
        logFile.print(',');
        logFile.print(tinyGPS.course.deg(), 1);
        logFile.print(',');
        logFile.print(tinyGPS.date.month());
        logFile.print('/');
        logFile.print(tinyGPS.date.day());
        logFile.print('/');
        logFile.print(tinyGPS.date.year());
        logFile.print(',');
        logFile.print(tinyGPS.time.hour());
        logFile.print(':');
        logFile.print(tinyGPS.time.minute());
        logFile.print(':');
        logFile.print(tinyGPS.time.second());
        logFile.print(',');
        int sat = tinyGPS.satellites.value();
        logFile.print(sat);
        logFile.print(',');
        logFile.print(WiFi.SSID(i));
        logFile.print(',');
        logFile.print(WiFi.RSSI(i));
        logFile.print(',');
        logFile.print(WiFi.channel(i));
        logFile.print(',');
        logFile.print(getEncryption(i));
        logFile.print(',');
        logFile.print(WiFi.BSSIDstr(i));
        logFile.println();
      }
    }
    logFile.close();
    return 1;
  }
  return 0;
}

void printHeader() {
  File logFile = SD.open(logFileName, FILE_WRITE);
  if (logFile) {
    int i = 0;
    for (; i < LOG_COLUMN_COUNT; i++) {
      logFile.print(log_col_names[i]);
      if (i < LOG_COLUMN_COUNT - 1)
        logFile.print(',');
      else
        logFile.println();
    }
    logFile.close();
  }
}

void updateFileName() {
  int i = 0;
  for (; i < MAX_LOG_FILES; i++) {
    memset(logFileName, 0, strlen(logFileName));
    sprintf(logFileName, "%s%d.%s", LOG_FILE_PREFIX, i, LOG_FILE_SUFFIX);
    if (!SD.exists(logFileName)) {
      break;
    } else {
      SerialMonitor.print(logFileName);
      SerialMonitor.println(" exists");
    }
  }
  SerialMonitor.print("File name: ");
  SerialMonitor.println(logFileName);
}

String getEncryption(uint8_t network) {
  byte encryption = WiFi.encryptionType(network);
  switch (encryption) {
    case 2:
      return "WPA (TKIP)";
    case 5:
      return "WEP";
    case 4:
      return "WPA (CCMP)";
    case 7:
      return "NONE";
    case 8:
      return "AUTO";
  }
}