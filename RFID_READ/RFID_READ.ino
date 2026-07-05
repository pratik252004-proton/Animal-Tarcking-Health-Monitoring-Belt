#include <Wire.h>
#include <WiFi.h>
#include <Adafruit_PN532.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

/******** WIFI ********/
#define WIFI_SSID "Wifi SSID"
#define WIFI_PASS "Wifi Password"

/******** ADAFRUIT IO ********/
#define AIO_SERVER "io.adafruit.com"
#define AIO_SERVERPORT 1883
#define AIO_USERNAME "AdafruitIO Username"
#define AIO_KEY "AdafruitIO Key"

/******** PN532 ********/
#define SDA_PIN 8
#define SCL_PIN 9

Adafruit_PN532 nfc(SDA_PIN, SCL_PIN);

/******** MQTT ********/
WiFiClient client;
Adafruit_MQTT_Client mqtt(&client,
                          AIO_SERVER,
                          AIO_SERVERPORT,
                          AIO_USERNAME,
                          AIO_KEY);

/******** SINGLE FEED ********/
Adafruit_MQTT_Publish livestockFeed =
  Adafruit_MQTT_Publish(&mqtt,
  AIO_USERNAME "/feeds/rfid");

uint8_t keya[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
char buffer16[17];

/******** WIFI CONNECT ********/
void connectWiFi() {
  Serial.print("Connecting WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected");
}

/******** MQTT CONNECT ********/
void MQTT_connect() {
  if (mqtt.connected()) return;

  Serial.print("Connecting MQTT...");
  int8_t ret;

  while ((ret = mqtt.connect()) != 0) {
    Serial.println(mqtt.connectErrorString(ret));
    mqtt.disconnect();
    delay(3000);
  }
  Serial.println("Connected");
}

/******** READ NFC BLOCK ********/
String readBlock(uint8_t block,
                 uint8_t *uid,
                 uint8_t uidLength)
{
  uint8_t data[16];

  if (!nfc.mifareclassic_AuthenticateBlock(uid, uidLength, block, 0, keya))
    return "";

  if (!nfc.mifareclassic_ReadDataBlock(block, data))
    return "";

  memcpy(buffer16, data, 16);
  buffer16[16] = '\0';

  return String(buffer16);
}

void setup() {

  Serial.begin(115200);
  delay(1000);

  Wire.begin(SDA_PIN, SCL_PIN);

  Serial.println("Livestock NFC → Adafruit IO (STRING)");

  connectWiFi();

  nfc.begin();

  if (!nfc.getFirmwareVersion()) {
    Serial.println("PN532 not detected");
    while (1) delay(10);
  }

  nfc.SAMConfig();
  Serial.println("Tap NFC Card...");
}

void loop() {

  MQTT_connect();
  mqtt.processPackets(10);
  mqtt.ping();

  uint8_t uid[7];
  uint8_t uidLength;

  if (!nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A,
                               uid, &uidLength)) {
    delay(150);
    return;
  }

  Serial.println("Card detected");

  String id    = readBlock(4, uid, uidLength);
  String name  = readBlock(5, uid, uidLength);
  String breed = readBlock(6, uid, uidLength);
  String owner = readBlock(8, uid, uidLength);

  // remove labels stored in card
  id.replace("ID:", "");
  name.replace("Name:", "");
  breed.replace("Breed:", "");
  owner.replace("Owner:", "");

  id.trim();
  name.trim();
  breed.trim();
  owner.trim();

  // ---- CREATE HUMAN READABLE STRING ----
  String message =
    "ID: " + id +
    " | Name: " + name +
    " | Breed: " + breed +
    " | Owner: " + owner;

  Serial.println("Sending:");
  Serial.println(message);

  livestockFeed.publish(message.c_str());

  Serial.println("✅ Sent to Adafruit IO");

  delay(6000); // prevent repeated send
}