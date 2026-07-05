#define DEBUG true

#include <WiFi.h>
#include <TinyGPS++.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

//**************** WIFI ****************//
#define WIFI_SSID "HACKED ONCE MORE"
#define WIFI_PASS "Skillup#"

//**************** ADAFRUIT IO ****************//
#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME    "AdafruitIO Username"
#define AIO_KEY         "AdafruitIO Key"

//**************** GPS UART ****************//
#define GPS_RX 25   // ESP32 RX  <- GPS TX
#define GPS_TX 33   // ESP32 TX  -> GPS RX

HardwareSerial GPS_Serial(2);
TinyGPSPlus gps;

//**************** GEOFENCE ****************//
#define GEOFENCE_LATITUDE  18.4640
#define GEOFENCE_LONGITUDE 73.8682
#define GEOFENCE_RADIUS    50

//**************** SEND INTERVAL ****************//
uint16_t Send_Data_After = 60;

//**************** VARIABLES (Same Style as A9G) ****************//
String location_data;
String lats;
String longi;
String Inside_Geofence = "Inside Geofence";
String Outside_Geofence = "Outside Geofence";

//**************** WIFI + MQTT ****************//
WiFiClient client;

Adafruit_MQTT_Client mqtt(&client,
                          AIO_SERVER,
                          AIO_SERVERPORT,
                          AIO_USERNAME,
                          AIO_KEY);

// Feeds
Adafruit_MQTT_Publish gpsFeed =
  Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/gpsloc/csv");

Adafruit_MQTT_Publish geoFeed =
  Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/geofence/csv");

Adafruit_MQTT_Publish batteryFeed =
  Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/battery");

// Task prototype
void MQTT_Task(void *pvParameters);

//--------------------------------------------------//
// WIFI CONNECT
//--------------------------------------------------//

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

//--------------------------------------------------//
// MQTT CONNECT
//--------------------------------------------------//

void MQTT_connect()
{
  int8_t ret;

  if (mqtt.connected())
    return;

  Serial.print("Connecting MQTT... ");

  while ((ret = mqtt.connect()) != 0)
  {
    Serial.println(mqtt.connectErrorString(ret));
    mqtt.disconnect();
    delay(5000);
  }

  Serial.println("MQTT Connected!");
}

//--------------------------------------------------//
// DISTANCE CALCULATION
//--------------------------------------------------//

float calculateDistance(float lat1, float lon1,
                        float lat2, float lon2)
{
  float latRad1 = radians(lat1);
  float lonRad1 = radians(lon1);
  float latRad2 = radians(lat2);
  float lonRad2 = radians(lon2);

  float dlon = lonRad2 - lonRad1;
  float dlat = latRad2 - latRad1;

  float a = pow(sin(dlat/2),2) +
            cos(latRad1)*cos(latRad2)*
            pow(sin(dlon/2),2);

  float c = 2 * atan2(sqrt(a), sqrt(1-a));

  return 6371000 * c;
}

//--------------------------------------------------//
// SETUP
//--------------------------------------------------//

void setup()
{
  Serial.begin(115200);

  GPS_Serial.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);

  connectWiFi();
  MQTT_connect();

  xTaskCreatePinnedToCore(
    MQTT_Task,
    "MQTT_Task",
    4096,
    NULL,
    1,
    NULL,
    1);
}

void loop()
{
  // FreeRTOS handles everything
}

//--------------------------------------------------//
// MQTT TASK
//--------------------------------------------------//

void MQTT_Task(void *pvParameters)
{
  for (;;)
  {
    MQTT_connect();

    // Continuous GPS parsing
    while (GPS_Serial.available())
    {
      gps.encode(GPS_Serial.read());
    }

    if (gps.location.isValid())
    {
      float lat = gps.location.lat();
      float lon = gps.location.lng();

      // ---------- SAME FORMAT AS A9G ----------
      location_data = String(lat,6) + "," + String(lon,6);

      int i = location_data.indexOf(',');

      lats  = location_data.substring(0, i);
      longi = location_data.substring(i + 1);

      Serial.print("Lat - "); Serial.print(lats); Serial.println("-");
      Serial.print("Longi - "); Serial.print(longi); Serial.println("-");

      int longi_length = longi.length();
      Serial.print("Longi Length - ");
      Serial.println(longi_length);

      String coordinates = "0," + lats + "," + longi + ",0";

      Serial.print("coordinates - ");
      Serial.println(coordinates);

      gpsFeed.publish(coordinates.c_str());
      Serial.println("Location DataSend");

      // ---------- GEOFENCE ----------
      float distance = calculateDistance(
        lat, lon,
        GEOFENCE_LATITUDE,
        GEOFENCE_LONGITUDE);

      if (distance <= GEOFENCE_RADIUS)
      {
        geoFeed.publish(Inside_Geofence.c_str());
        Serial.println("Inside Geofence");
      }
      else
      {
        geoFeed.publish(Outside_Geofence.c_str());
        Serial.println("Outside Geofence");
      }

      // ---------- BATTERY ----------
      int raw = analogRead(34);
      float voltage = (raw / 4095.0) * 3.3 * 2.0;

      String batt = String(voltage,2);
      batteryFeed.publish(batt.c_str());

      Serial.print("Battery Voltage: ");
      Serial.println(batt);
    }
    else
    {
      Serial.println("Waiting GPS Fix...");
    }

    vTaskDelay(Send_Data_After * 1000 / portTICK_PERIOD_MS);
  }
}