/**************************************************************************/
/*! 

This example is for communicating with the PN532 chip using I2C. Wiring 
should be as follows:
  PN532 SDA -> EFM32 D21 Pin (with 4.7K resistor already present)
  PN532 SCL -> EFM32 D22 Pin(with 4.7K resistor already present)
  PN532 IRQ -> EFM32 D4 Pin
  PN32 RST0 -> EFM32 D5 Pin
  PN532 3.3v -> 3.3v
  PN532 GND -> GND

Based on readMifareClassicIrq.pde by Adafruit
*/
/**************************************************************************/
#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_PN532.h>
#include <bits/stdc++.h>
#include <vector>
#include <algorithm>

static void startListeningToNFC();
static String handleCardDetected();

// Pins used for I2C IRQ
#define PN532_IRQ 4
#define PN532_RESET 5

const int DELAY_BETWEEN_CARDS = 500;
long timeLastCardRead = 0;
boolean readerDisabled = false;
int irqCurr;
int irqPrev;

const int aantalGebruikers = 4;
int gebruikersTeller = 0;
bool registeringCards = true;
//uint32_t idGebruikers[aantalGebruikers];
std::vector<String> idGebruikers;
void registerCard(String id);
String hexToString(uint8_t *cardid);

// This example uses the IRQ line, which is available when in I2C mode.
Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET);

void setup(void)
{
  Serial.begin(115200); //Adapt the platformio.ini with correct monitor_speed

  ///nfc setup
  Serial.println("Begin NFC532 Scanning Software.");
  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata)
  {
    Serial.print("Didn't find PN532 board");
    while (1)
      ; // halt
  }
  // Got ok data, print it out!
  Serial.print("Found chip PN5");
  Serial.println((versiondata >> 24) & 0xFF, HEX);
  Serial.print("Firmware ver. ");
  Serial.print((versiondata >> 16) & 0xFF, DEC);
  Serial.print('.');
  Serial.println((versiondata >> 8) & 0xFF, DEC);

  // configure board to read RFID tags
  nfc.SAMConfig();

  startListeningToNFC();
}

void loop(void)
{
  if (readerDisabled)
  {
    if (millis() - timeLastCardRead > DELAY_BETWEEN_CARDS)
    {
      readerDisabled = false;
      startListeningToNFC();
    }
  }
  else
  {
    irqCurr = digitalRead(PN532_IRQ);

    // When the IRQ is pulled low - the reader has got something for us.
    if (irqCurr == LOW && irqPrev == HIGH)
    {
      //Serial.println("Got NFC IRQ");
      String id = handleCardDetected();
      if (registeringCards)
      {
        registerCard(id);
      }
    }
    irqPrev = irqCurr;
  }
}

void startListeningToNFC()
{
  // Reset our IRQ indicators
  irqPrev = irqCurr = HIGH;

  Serial.println("Present an ISO14443A Card ...");
  nfc.startPassiveTargetIDDetection(PN532_MIFARE_ISO14443A);
}

boolean validate(uint8_t *cardid, uint8_t length)
{
  std::stringstream ss;
  for (int i = 0; i < length - 1; ++i)
  {
    ss << "0x";
    if ((int)cardid[i] <= 15)
    {
      ss << "0";
    }
    ss << std::hex << (int)cardid[i];
    ss << " ";
  }
  ss << "0x";
  if ((int)cardid[length - 1] <= 15)
  {
    ss << "0";
  }
  ss << std::hex << (int)cardid[length - 1];
  std::string mystrC = ss.str();
  String mystr = mystrC.c_str();
  mystr.toUpperCase();
  mystrC = mystr.c_str();
  Serial.print("Onze hex: ");
  Serial.println(mystr.c_str());

  std::string waarde[] = {"0X02 0X82 0X00 0X08 0X7B 0X2B 0XC3",
                          "0X04 0X9C 0X49 0X6A 0X99 0X5B 0X80",
                          "0X69 0X42 0XA2 0XB8",
                          "0XB3 0XF7 0XC6 0X02",
                          "0X04 0X6B 0X0F 0XE2 0X50 0X5A 0X80",
                          "0XA9 0XAF 0XAE 0XC2",
                          "0X04 0X07 0XCC 0X52 0XA8 0X58 0X81"};
  for (int i = 0; i < 7; i++)
  {
    Serial.println(waarde[i].c_str());
    Serial.println(mystrC.c_str());
    if (waarde[i].compare(mystrC) == 0)
    {
      return true;
    }
  }
  return false;
}

String hexToString(uint8_t *cardid)
{
  int length = 7;
  std::stringstream ss;
  for (int i = 0; i < length - 1; ++i)
  {
    ss << "0x";
    if ((int)cardid[i] <= 15)
    {
      ss << "0";
    }
    ss << std::hex << (int)cardid[i];
    ss << " ";
  }
  ss << "0x";
  if ((int)cardid[length - 1] <= 15)
  {
    ss << "0";
  }
  ss << std::hex << (int)cardid[length - 1];
  std::string mystrC = ss.str();
  String mystr = mystrC.c_str();
  mystr.toUpperCase();
  return mystr;
}

String handleCardDetected()
{
  uint8_t success = false;
  uint8_t uid[] = {0, 0, 0, 0, 0, 0, 0}; // Buffer to store the returned UID
  uint8_t uidLength;                     // Length of the UID (4 or 7 bytes depending on ISO14443A card type)

  // read the NFC tag's info
  success = nfc.readDetectedPassiveTargetID(uid, &uidLength);
  Serial.println(success ? "Read successful" : "Read failed (not a card?)");

  if (success)
  {
    // Display some basic information about the card
    //Serial.println("Found an ISO14443A card");
    //Serial.print("  UID Length: ");Serial.print(uidLength, DEC);Serial.println(" bytes");
    //Serial.print("  UID Value: ");
    //Serial.print("Card ID HEX Value: ");
    //nfc.PrintHex(uid, uidLength);

    /*if (uidLength == 4)
    {
      // We probably have a Mifare Classic card ...
      uint32_t cardid = uid[0];
      cardid <<= 8;
      cardid |= uid[1];
      cardid <<= 8;
      cardid |= uid[2];
      cardid <<= 8;
      cardid |= uid[3];
      //Serial.print("Seems to be a Mifare Classic card #");
      Serial.print("Card ID NUMERIC Value: ");
      Serial.println(cardid);
      for (int i = 0; i < uidLength; i++)
      {
        Serial.print(uid[i]);
      }
    }*/
    timeLastCardRead = millis();
  }

  // The reader will be enabled again after DELAY_BETWEEN_CARDS ms will pass.
  readerDisabled = true;
  return hexToString(uid);
}

void registerCard(String id)
{
  if (gebruikersTeller < aantalGebruikers)
  {
    if (std::count(idGebruikers.begin(), idGebruikers.end(), id))
    {
      Serial.println("Tag reeds geregistreerd");
    }
    else
    {
      idGebruikers.push_back(id);
      gebruikersTeller++;
    }
  }
  else
  {
    Serial.println("Maximaal aantal gebruiker bereikt: ");
    for (const String i : idGebruikers)
    {
      Serial.println(i);
    }
  }
}