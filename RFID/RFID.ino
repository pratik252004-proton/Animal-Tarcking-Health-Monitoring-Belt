#include <Wire.h>
#include <Adafruit_PN532.h>

#define SDA_PIN 8
#define SCL_PIN 9

Adafruit_PN532 nfc(SDA_PIN, SCL_PIN);

bool written = false;

// default MIFARE key
uint8_t keya[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

void writeBlock(uint8_t block, uint8_t *data,
                uint8_t *uid, uint8_t uidLength)
{
  if (!nfc.mifareclassic_AuthenticateBlock(uid, uidLength, block, 0, keya)) {
    Serial.print("Auth failed block ");
    Serial.println(block);
    return;
  }

  if (nfc.mifareclassic_WriteDataBlock(block, data))
    Serial.printf("Block %d written OK\n", block);
  else
    Serial.printf("Write failed block %d\n", block);

  delay(50); // watchdog safe
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Wire.begin(SDA_PIN, SCL_PIN);

  Serial.println("\nLIVESTOCK NFC WRITE");

  nfc.begin();

  if (!nfc.getFirmwareVersion()) {
    Serial.println("PN532 not found");
    while (1) delay(10);
  }

  nfc.SAMConfig();
  Serial.println("Tap card...");
}

void loop() {

  if (written) return;

  uint8_t uid[7];
  uint8_t uidLength;

  if (!nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A,
                               uid, &uidLength)) {
    delay(100);
    return;
  }

  Serial.println("Card detected");

  // ---- 16 BYTE BLOCK DATA ----
  uint8_t block4[16] = "ID:Cow-004     ";
  uint8_t block5[16] = "Name:Gauri     ";
  uint8_t block6[16] = "Breed:Gir      ";
  uint8_t block8[16] = "Owner:Pratik   ";

  writeBlock(4, block4, uid, uidLength);
  writeBlock(5, block5, uid, uidLength);
  writeBlock(6, block6, uid, uidLength);
  writeBlock(8, block8, uid, uidLength);

  Serial.println("✅ Livestock data stored!");
  written = true;

  delay(2000);
}