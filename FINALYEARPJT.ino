#define BLYNK_PRINT Serial
#define BLYNK_TEMPLATE_ID "TMPL3ntrUiNDX"
#define BLYNK_TEMPLATE_NAME "Electric Billing"
#define BLYNK_AUTH_TOKEN "81cG20HbB8ndMO68T2qOqo4JmnZbkQlT"
#include <WiFiClientSecure.h>
#include <ESP32_MailClient.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "DHT.h"
#include "EmonLib.h"
#include "ACS712.h"

#define DHTPIN 4
#define DHTTYPE DHT11

#define RELAY_PIN_1 13
#define RELAY_PIN_2 12
#define RELAY_PIN_3 14
#define RELAY_PIN_4 27

char ssid[] = "MyProject";
char pass[] = "12345678";

ACS712 ACS1(34, 3.3, 4095, 100);
ACS712 ACS2(35, 5.0, 4095, 100);
ACS712 ACS3(36, 3.3, 4095, 100);
ACS712 ACS4(39, 5.0, 4095, 100);

DHT dht(DHTPIN, DHTTYPE);
EnergyMonitor emon1;
EnergyMonitor emon2;
EnergyMonitor emon3;
EnergyMonitor emon4;

unsigned long lastMillis = 0;
unsigned long interval = 5000; // 5 seconds

double totalEnergy1 = 0.0;
double totalEnergy2 = 0.0;
double totalEnergy3 = 0.0;
double totalEnergy4 = 0.0;

const double chargingRate = 5.0; // Rs per kWh
// Data Logging Variables
#define STORAGE_SIZE 24   // 12 hours worth of 30-min logs
String dataLog[STORAGE_SIZE];
unsigned int logIndex = 0;
unsigned long lastLogMillis = 0;
unsigned long logInterval = 1800000;  // 30 minutes
unsigned long lastEmailMillis = 0;
unsigned long emailInterval = 43200000;  // 12 hours
// Email Function
void sendEmailWithDataLog() {
  SMTPData smtpData;
  smtpData.setLogin("smtp.gmail.com", 465, "sender@gmail.com", "12345678");  
  smtpData.setSender("Smart Billing System", "your_email@gmail.com");                   
  smtpData.setPriority("High");
  smtpData.setSubject("12-Hour Energy Report");
  smtpData.setMessage("Attached is the energy consumption report for the last 12 hours.", false);

  String csvContent = "Timestamp(min),Energy1(kWh),Energy2(kWh),Energy3(kWh),Energy4(kWh),Temp(C),Humidity(%)\n";
  for (int i = 0; i < STORAGE_SIZE; i++) {
    csvContent += dataLog[i] + "\n";
  }
  smtpData.addAttachData("EnergyReport.csv", csvContent.c_str(), csvContent.length(), "text/csv");
  smtpData.addRecipient("receiver@gmail.com");    

  if (!MailClient.sendMail(smtpData)) {
    Serial.println("Error sending Email: " + MailClient.smtpErrorReason());
  }
  smtpData.empty();
}

// LCD I2C configuration
LiquidCrystal_I2C lcd(0x27, 16, 2); // Change the address (0x27) if your LCD has a different address

void setup() {
  Serial.begin(115200);
  ACS1.zero();
  ACS2.zero();
  ACS3.zero();
  ACS4.zero();
  pinMode(RELAY_PIN_1, OUTPUT);
  pinMode(RELAY_PIN_2, OUTPUT);
  pinMode(RELAY_PIN_3, OUTPUT);
  pinMode(RELAY_PIN_4, OUTPUT);
  dht.begin();
  lcd.init(); // Initialize the LCD
  lcd.backlight(); // Turn on the backlight
  lcd.print("Smart Billing"); // Display project name on LCD
  delay(3000); // Wait for 2 seconds before starting Blynk
  lcd.clear();
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
}

void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - lastMillis >= interval) {
    lastMillis = currentMillis;
    double i1 = ACS1.getCurrentAC();
    double i2 = ACS2.getCurrentAC();
    double i3 = ACS3.getCurrentAC();
    double i4 = ACS4.getCurrentAC();
    double voltage = 230.0;
    double energy1 = (i1 * voltage * interval) / (3600 * 1000); // Convert to kWh
    double energy2 = (i2 * voltage * interval) / (3600 * 1000);
    double energy3 = (i3 * voltage * interval) / (3600 * 1000);
    double energy4 = (i4 * voltage * interval) / (3600 * 1000);
    totalEnergy1 += energy1;
    totalEnergy2 += energy2;
    totalEnergy3 += energy3;
    totalEnergy4 += energy4;
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("B1: " + String(totalEnergy1, 2) + " kWh B2: " + String(totalEnergy2, 2) + " kWh");
    lcd.setCursor(0, 1);
    lcd.print("B3: " + String(totalEnergy3, 2) + " kWh B4: " + String(totalEnergy4, 2) + " kWh");
    Serial.println("Current 1: " + String(i1));
    Serial.println("Current 2: " + String(i2));
    Serial.println("Current 3: " + String(i3));
    Serial.println("Current 4: " + String(i4));
    Serial.println("Temperature: " + String(t));
    Serial.println("Total Energy 1: " + String(totalEnergy1) + " kWh");
    Serial.println("Total Energy 2: " + String(totalEnergy2) + " kWh");
    Serial.println("Total Energy 3: " + String(totalEnergy3) + " kWh");
    Serial.println("Total Energy 4: " + String(totalEnergy4) + " kWh");
    double totalPrice1 = totalEnergy1 * chargingRate;
    double totalPrice2 = totalEnergy2 * chargingRate;
    double totalPrice3 = totalEnergy3 * chargingRate;
    double totalPrice4 = totalEnergy4 * chargingRate;
    Serial.println("Total Price 1: Rs " + String(totalPrice1));
    Serial.println("Total Price 2: Rs " + String(totalPrice2));
    Serial.println("Total Price 3: Rs " + String(totalPrice3));
    Serial.println("Total Price 4: Rs " + String(totalPrice4));
    Blynk.virtualWrite(V0, h);
    Blynk.virtualWrite(V1, t);
    Blynk.virtualWrite(V6, totalEnergy1);
    Blynk.virtualWrite(V7, totalEnergy2);
    Blynk.virtualWrite(V8, totalEnergy3);
    Blynk.virtualWrite(V9, totalEnergy4);
    Blynk.virtualWrite(V2, totalPrice1);
    Blynk.virtualWrite(V3, totalPrice2);
    Blynk.virtualWrite(V4, totalPrice3);
    Blynk.virtualWrite(V5, totalPrice4);
    Blynk.run();
  }
  // Data Logging Every 30 Minutes 
  if (currentMillis - lastLogMillis >= logInterval) {
    lastLogMillis = currentMillis;

    String logEntry = String(millis() / 1000 / 60) + " min, "
                    + String(totalEnergy1, 2) + ", "
                    + String(totalEnergy2, 2) + ", "
                    + String(totalEnergy3, 2) + ", "
                    + String(totalEnergy4, 2) + ", "
                    + String(dht.readTemperature(), 1) + ", "
                    + String(dht.readHumidity(), 1);

    dataLog[logIndex] = logEntry;
    logIndex = (logIndex + 1) % STORAGE_SIZE;
  }

  // Emailing Dataset Every 12 Hours 
  if (currentMillis - lastEmailMillis >= emailInterval) {
    lastEmailMillis = currentMillis;
    sendEmailWithDataLog();
  }
}

}

BLYNK_WRITE(V10) {
  int relayState = param.asInt();
  digitalWrite(RELAY_PIN_1, !relayState);
}

BLYNK_WRITE(V11) {
  int relayState = param.asInt();
  digitalWrite(RELAY_PIN_2, !relayState);
}

BLYNK_WRITE(V12) {
  int relayState = param.asInt();
  digitalWrite(RELAY_PIN_3, !relayState);
}

BLYNK_WRITE(V13) {
  int relayState = param.asInt();
  digitalWrite(RELAY_PIN_4, !relayState);
}
