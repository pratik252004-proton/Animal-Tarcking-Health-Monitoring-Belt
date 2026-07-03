#define DEBUG true

#include <WiFi.h>
#include <Wire.h>
#include <TinyGPS++.h>
#include "MAX30105.h"
#include "heartRate.h"
#include <Adafruit_SHT4x.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

//================ WIFI =================//
#define WIFI_SSID "Wifi SSID"
#define WIFI_PASS "Wifi Password"

//================ ADAFRUIT IO =================//
#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME    "AdafruitIO UserName"
#define AIO_KEY         "AdafruitIO Key"

//================ GPS UART =================//
#define GPS_RX 25
#define GPS_TX 33
HardwareSerial GPS_Serial(2);
TinyGPSPlus gps;

//================ BATTERY ADC =================//
#define BATTERY_PIN 34

//================ GEOFENCE =================//
#define GEOFENCE_LATITUDE  18.4575
#define GEOFENCE_LONGITUDE 73.8504
#define GEOFENCE_RADIUS    50

//================ SENSORS =================//
MAX30105 particleSensor;
Adafruit_SHT4x sht40;

//================ MQTT =================//
WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

Adafruit_MQTT_Publish gpsFeed(&mqtt, AIO_USERNAME "/feeds/gpsloc/csv");
Adafruit_MQTT_Publish geoFeed(&mqtt, AIO_USERNAME "/feeds/geofence");
Adafruit_MQTT_Publish batteryFeed(&mqtt, AIO_USERNAME "/feeds/battery");
Adafruit_MQTT_Publish pulseFeed(&mqtt, AIO_USERNAME "/feeds/bpm");
Adafruit_MQTT_Publish tempFeed(&mqtt, AIO_USERNAME "/feeds/temperature");
Adafruit_MQTT_Publish humidityFeed(&mqtt, AIO_USERNAME "/feeds/humidity");

//================ HEART RATE =================//
const int numSamples = 20;
int rates[numSamples] = {0};
int rateSpot = 0;
long lastBeat = 0;
const long noFingerThreshold = 5000;

//================ TIMING =================//
unsigned long lastPublish = 0;
const unsigned long publishInterval = 15000;  

//================ WIFI =================//
void connectWiFi()
{
  Serial.print("Connecting WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");
}

//================ MQTT =================//
void MQTT_connect()
{
  int8_t ret;
  if (mqtt.connected()) return;

  Serial.print("Connecting MQTT...");
  while ((ret = mqtt.connect()) != 0)
  {
    Serial.println(mqtt.connectErrorString(ret));
    mqtt.disconnect();
    delay(5000);
  }
  Serial.println("MQTT Connected!");
}

//================ DISTANCE =================//
float calculateDistance(float lat1,float lon1,float lat2,float lon2)
{
  float dlat = radians(lat2-lat1);
  float dlon = radians(lon2-lon1);

  float a = sin(dlat/2)*sin(dlat/2) +
            cos(radians(lat1))*cos(radians(lat2))*
            sin(dlon/2)*sin(dlon/2);

  return 6371000 * (2*atan2(sqrt(a),sqrt(1-a)));
}

//================ SETUP =================//
void setup()
{
  Serial.begin(115200);
  Wire.begin();

  analogReadResolution(12);
  analogSetPinAttenuation(BATTERY_PIN, ADC_11db);

  GPS_Serial.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);

  connectWiFi();
  MQTT_connect();

  particleSensor.begin(Wire);
  particleSensor.setup();
  particleSensor.setPulseAmplitudeRed(0x0A);
  particleSensor.setPulseAmplitudeIR(0x0A);

  sht40.begin();
}

//================ LOOP =================//
void loop()
{
  MQTT_connect();

  while (GPS_Serial.available())
    gps.encode(GPS_Serial.read());

  float lat = 0, lon = 0;
  String geoStatus = "No Fix";

  if (gps.location.isValid())
  {
    lat = gps.location.lat();
    lon = gps.location.lng();

    float distance = calculateDistance(lat,lon,
                                       GEOFENCE_LATITUDE,
                                       GEOFENCE_LONGITUDE);

    geoStatus = (distance <= GEOFENCE_RADIUS) ?
                "Inside Geofence" : "Outside Geofence";
  }

  long irValue = particleSensor.getIR();
  float avgBPM = 0;

  if(irValue > noFingerThreshold && checkForBeat(irValue))
  {
    long delta = millis() - lastBeat;
    lastBeat = millis();
    float bpm = 60/(delta/1000.0);

    if(bpm>20 && bpm<255)
    {
      rates[rateSpot++] = (int)bpm;
      rateSpot %= numSamples;
    }
  }

  int valid=0;
  for(int i=0;i<numSamples;i++)
    if(rates[i]>0){avgBPM+=rates[i]; valid++;}

  if(valid>0) avgBPM/=valid;

  sensors_event_t humidity, temp;
  sht40.getEvent(&humidity,&temp);

  float sum = 0;
  for(int i=0;i<20;i++){
    sum += analogRead(BATTERY_PIN);
    delay(2);
  }
  float raw = sum / 20.0;
  float voltage = (raw / 4095.0) * 3.3 * 11.0;

  if (millis() - lastPublish >= publishInterval)
  {
    Serial.println("\n========= DATA SENT =========");

    if (gps.location.isValid())
    {
      String coordinates = "0," + String(lat,6) + "," + String(lon,6) + ",0";
      gpsFeed.publish(coordinates.c_str());

      Serial.print("GPS Coordinates: ");
      Serial.println(coordinates);
    }
    else
    {
      Serial.println("GPS: No Fix");
    }

    geoFeed.publish(geoStatus.c_str());
    Serial.print("Geofence Status: ");
    Serial.println(geoStatus);

    pulseFeed.publish(avgBPM);
    Serial.print("Heart Rate (BPM): ");
    Serial.println(avgBPM);

    tempFeed.publish(temp.temperature);
    Serial.print("Temperature (°C): ");
    Serial.println(temp.temperature);

    humidityFeed.publish(humidity.relative_humidity);
    Serial.print("Humidity (%): ");
    Serial.println(humidity.relative_humidity);

    batteryFeed.publish(voltage);
    Serial.print("Battery Voltage (V): ");
    Serial.println(voltage);

    Serial.println("================================\n");

    lastPublish = millis();
  }
}