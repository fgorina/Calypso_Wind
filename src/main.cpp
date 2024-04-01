#include <Arduino.h>
#include <HardwareSerial.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <ArduinoWebsockets.h>
#include <Preferences.h>

#include <ArduinoJson.h>

#define DEBUG false
#define DEBUG_1 false

#define ONBOARD_LED 2
#define WIFI_LED 2
#define BLE_LED 2
#define FORCE_MDNS 26

#define RX 16
#define TX 17

HardwareSerial GPSSerial(2);

void sendData(uint8_t windSpeed, uint8_t windDirection);

// Calypso BLE Definitions

const char *windMeterName = "ULTRASONIC";
uint8_t sensors = 0;   // Activate compass/accelerometers (power comsumption increased )
uint8_t frequancy = 1; // 1, 4, 8 (Hz) Power consumption increased

// SignalK Definitions

const char *updateMessage =
    "{ \"context\": \"%s"
    "\", \"updates\": [ {  \"source\": {\"label\": \"%s\" }, \"values\": [ "
    "{ \"path\": \"environment.wind.angleApparent\",\"value\":%s },"
    "{ \"path\": \"environment.wind.speedApparent\",\"value\":%s },"
    "{ \"path\": \"electrical.batteries.99.name\",\"value\": \"%s\"}, "
    "{ \"path\": \"electrical.batteries.99.location\",\"value\": \"Mast\"},"
    "{ \"path\": \"electrical.batteries.99.capacity.stateOfCharge\",\"value\":%s "
    " }]}]}";

char ssid[20] = "Yamato";
char password[20] = "ailataN1991";
char device_name[20] = "wind_meter";
char skserver[20] = "";
int skport = 0; // It is 4 bytes
char skpath[100] = "/signalk/v1/stream?subscribe=none";

using namespace websockets;

bool mdnsDone = false; // Will be true when we have a server

bool wifi_connect = false;
int enabled = 0; // 0 Deshabilita les accions fins que s'ha rebut un command
WebsocketsClient client;
int socketState = -4; // -5 does not use WiFi, -4 -> Before connecting to WiFi, -3, -2.Connectingau

String me = "vessels.self";
char token[256] = "";
char bigBuffer[1024] = "";
char nmeaBuffer[1024] = "";
char nmeaBuffer1[1024] = "";

// Tasks

TaskHandle_t taskNetwork;
TaskHandle_t taskGPS;
TaskHandle_t taskNemea;
TaskHandle_t taskLed;

int ledState = 0;
int bleLedState = 0;
int ledOn = 0;
int ledOff = 100;

// LEDs

void clearLed()
{
  ledState = 1;
  digitalWrite(ONBOARD_LED, ledState);
  digitalWrite(WIFI_LED, ledState);
  if (!mdnsDone)
  {
    digitalWrite(BLE_LED, ledState);
  }
}

void setLed()
{
  ledState = 0;

  digitalWrite(ONBOARD_LED, ledState);
  digitalWrite(WIFI_LED, ledState);
  if (!mdnsDone)
  {
    digitalWrite(BLE_LED, ledState);
  }
}

void clearBLELed()
{
  bleLedState = 1;
  digitalWrite(BLE_LED, bleLedState);
}

void setBLELed()
{
  bleLedState = 0;
  digitalWrite(BLE_LED, bleLedState);
}

void toggleLed()
{
  if (ledState == 0)
  {
    ledState = 1;
  }
  else
  {
    ledState = 0;
  }

  digitalWrite(ONBOARD_LED, ledState);
  digitalWrite(WIFI_LED, ledState);
}

void sendData(uint8_t windSpeed, uint8_t windDirection, uint8_t battery)
{

  if (client.available())
  {

    char buff[10];
    char buff1[10];
    char buff2[10];

    char message[1024];
    double radians = double(windDirection) / 180.0 * PI;
    double speed = double(windSpeed) / 100.0;
    double level = double(battery) / 100.0;

    sprintf(message, updateMessage, me, windMeterName, dtostrf(radians, 6, 2, buff), dtostrf(speed, 6, 2, buff1), windMeterName, dtostrf(level, 6, 2, buff2));
    // String s = update1 + me + update2 + dtostrf(radians, 6, 2, buff) + update3 +  dtostrf(speed, 6, 2, buff1) + update4 + dtostrf(level, 6, 2, buff2) + update5;
    if (DEBUG)
    {
      Serial.print("Send: ");
      Serial.println(message);
    }
    clearBLELed();
    client.send(message);
    vTaskDelay(5);
    setBLELed();
  }
  else
  {
    if (DEBUG)
    {
      Serial.println("No connectat");
    }
  }
}

// Preferences

Preferences preferences;
void writePreferences();
void readPreferences();

#include "BLE_Client.h"
#include "signalk.h"

// Modiofie preferences so SSID/PASSWD are hardwired. If not must see a way to set them
void writePreferences()
{
  preferences.begin("windmeter", false);
  preferences.remove("SSID");
  preferences.remove("PASSWD");
  preferences.remove("PPHOST");
  preferences.remove("PPPORT");
  preferences.remove("TOKEN");

  // preferences.putString("SSID", ssid);
  // preferences.putString("PASSWD", password);
  preferences.putString("PPHOST", skserver);
  preferences.putInt("PPPORT", skport);
  preferences.putString("TOKEN", token);
  preferences.end();
}

void readPreferences(bool reset)
{

  preferences.begin("windmeter", true);
  // preferences.getString("SSID", ssid, 20);
  // preferences.getString("PASSWD", password, 20);
  preferences.getString("PPHOST", skserver, 20);
  skport = preferences.getInt("PPPORT", skport);
  preferences.getString("TOKEN", token, 200);
  preferences.end();
  if (reset)
  {
    skserver[0] = 0;
    skport = 0;
    token[0] = 0;
  }
  print_info();
}

void ledTask(void *parameter)
{

  for (;;)
  {
    if (ledOn > 0)
    {
      setLed();
      vTaskDelay(ledOn);
    }

    if (ledOff > 0)
    {
      clearLed();
      vTaskDelay(ledOff);
    }
  }
}

void gpsTask(void *parameter)
{
  for (;;)
  {
    int n = GPSSerial.readBytesUntil(10, nmeaBuffer, 250);
    nmeaBuffer[n] = 10;
    nmeaBuffer[n + 1] = 0;
    if (DEBUG)
    {
      Serial.println(nmeaBuffer);
    }

    if (clientNemea.connected())
    {
      clientNemea.print(nmeaBuffer);
    }
    vTaskDelay(3);
  }
}

void nmeaTask(void *parameter)
{
  for (;;)
  {

    if (clientNemea.connected())
    {
      if (clientNemea.available())
      {
        int n = clientNemea.readBytesUntil(10, nmeaBuffer1, 250);
        if (n > 0)
        {
          if (!((nmeaBuffer1[1] == 'G' && nmeaBuffer1[2] == 'P') || (nmeaBuffer1[1] == 'P' && nmeaBuffer1[2] == 'G')) || false)
          {
            nmeaBuffer1[n] = 10;
            nmeaBuffer1[n + 1] = 0;
            GPSSerial.write(nmeaBuffer1, n + 1);
            if (DEBUG || true)
            {
              Serial.println(nmeaBuffer1);
            }
          }
        }
        else
        {
          Serial.println("Timeout nmea sender");
        }
      }
    }
    vTaskDelay(5);
  }
}

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);
  GPSSerial.begin(38400, SERIAL_8N1, RX, TX);
  pinMode(ONBOARD_LED, OUTPUT);
  pinMode(WIFI_LED, OUTPUT);
  pinMode(BLE_LED, OUTPUT);
  pinMode(FORCE_MDNS, INPUT_PULLUP);

  bool reset = digitalRead(FORCE_MDNS) == 0;

  clearBLELed();
  clearLed();

  readPreferences(reset);
  if (strlen(skserver) != 0 && skport != 0)
  { // We already have the data, no need to do a mdns lookup
    mdnsDone = true;
  }

  xTaskCreatePinnedToCore(networkTask, "TaskNetwork", 4000, NULL, 1, &taskNetwork, 0);
  xTaskCreatePinnedToCore(ledTask, "TaskLed", 1000, NULL, 1, &taskLed, 0);
  xTaskCreatePinnedToCore(gpsTask, "TaskGps", 2000, NULL, 0, &taskGPS, 0);
  xTaskCreatePinnedToCore(nmeaTask, "TaskNemea", 2000, NULL, 0, &taskNemea, 0);
}

void loop()
{
  vTaskDelay(100);
}
