/*
  Titre      : Utilisation du RPC avec LED
  Auteur     : Yvan Tankeu
  Date       : 27/10/2022
  Description: Stock les données provenant des differents capteurs dans la carte SD du MEM
  envoie les donnees sur TB, faire du RPC, Eteindre le wifi
  Version    : 0.0.1
*/

#include <Arduino.h>
#include <SPI.h>

bool LedStatus;

// Include pour la carte SD
#include <SD.h>

// Include pour le capteur AHTX0
#include <Adafruit_AHTX0.h>

// 03 Includes pour faire fonctionner le rtc
#include "RTClib.h"

// Librairie de branchement
#include "WIFIConnector_MKR1000.h"
#include "MQTTConnector.h"

const int CHIPSELECT = 4;
const int LED_BLEUE = 3;
const int LED_ROUGE = 14;

RTC_DS3231 rtc;
Adafruit_AHTX0 aht;

String buffer;
String valHumidite, valTemperature, unixTime = "";

int espace1, espace2 = 0;

unsigned int const fileWriteTime = 4000;  // 4 secondes
unsigned int const timeToSend = 10000;    // 10 secondes
unsigned int const timeWifiClosed = 5000; // 5 secondes

#include <Arduino.h>

boolean runEveryShort(unsigned long interval)

{
  static unsigned long previousMillis = 0;

  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval)
  {
    previousMillis = currentMillis;

    return true;
  }

  return false;
}

boolean runEveryLong(unsigned long interval)

{
  static unsigned long previousMillis = 0;

  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval)
  {
    previousMillis = currentMillis;

    return true;
  }

  return false;
}

void dataToSend(String chaine)
{
  // Retrouver la position des 2 espaces vide dans une chaine de caracteres
  for (int i = 0; i < chaine.length(); i++)
  {
    if (chaine.charAt(i) == ' ')
    {
      if (espace1 == 0)
      {
        espace1 = i;
      }
      else
      {
        espace2 = i;
      }
    }
  }

  // Obtenir le 1er char dans un string
  char firstCharOfString = chaine.charAt(0);

  // Obtenier l'index  du 1er char dans un string
  int firstCharIndexOfString = chaine.indexOf(firstCharOfString);

  // Extraction de  l'unixTime
  unixTime = chaine.substring(firstCharIndexOfString, espace1);

  // Extraction de  la temperature
  valTemperature = chaine.substring(espace1 + 1, espace2);

  // Extraction de  l'hunidite
  valHumidite = chaine.substring(espace2, chaine.length());
}

void setup()
{
  // Branchement au reseau
  wifiConnect(); // Branchement au réseau WIFI
  MQTTConnect(); // Branchement au broker MQTT à Thingsboard*/

  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  /*while (!Serial)
  {
    ; // wait for serial port to connect. Needed for native USB port only
  }*/
  pinMode(LED_BLEUE, OUTPUT);
  pinMode(LED_ROUGE, OUTPUT);

  // digitalWrite(LED_PIN, LOW);

  // see if the card is present and can be initialized:
  if (!SD.begin(CHIPSELECT))
  {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    while (1)
      ;
  }

  // Init de AHT20
  if (!aht.begin())
  {
    Serial.println("Impossible de trouver l'AHT ? Vérifiez le câblage");
    while (1)
      delay(10);
  }
  Serial.println(" AHT20 trouve");

  // Init du RTC
  if (!rtc.begin())
  {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    while (1)
      delay(10);
  }

  if (rtc.lostPower())
  {
    Serial.println("RTC lost power, let's set the time!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  File dataFile = SD.open("datalog.txt", FILE_WRITE | O_TRUNC);
  // dataFile.println("");
  dataFile.close();
}

void loop()
{
  ClientMQTT.loop();

  DateTime now = rtc.now();

  sensors_event_t humidity, temp;
  aht.getEvent(&humidity, &temp);

  String stringToWrite = String(now.unixtime() + 10800 ) + "000" + " " + temp.temperature + " " + humidity.relative_humidity + "\n";
  delay(1000);

  if (runEveryShort(fileWriteTime))
  {

    File dataFile = SD.open("datalog.txt", FILE_WRITE);
    // if the file is available, write to it:
    if (dataFile)
    {

      dataFile.print(stringToWrite);
      Serial.print(stringToWrite);

      dataFile.close();
    }
    else
    {
      Serial.println("ouverture d'erreur datalog.txt");
    }
  }

  if (runEveryLong(timeToSend))
  {
    wifiConnect(); // Branchement au réseau WIFI
    MQTTConnect();

    ClientMQTT.loop();

    File dataFile = SD.open("datalog.txt", FILE_READ);
    if (dataFile)
    {
      // read from the file until there's nothing else in it:
      while (dataFile.available())
      {
        buffer = dataFile.readStringUntil('\n');
        Serial.println(buffer);
        dataToSend(buffer); // Extraction des donnees de la chaine

        appendTimestamps(unixTime.toFloat());
        appendPayload("Temperature", valTemperature.toFloat());
        appendPayload("Humidite", valHumidite.toFloat());
        sendPayload();
      }

      if (runEveryShort(timeWifiClosed))
      {
        status = WL_IDLE_STATUS;
        WiFi.disconnect();
        WiFi.end();
        digitalWrite(LED_BUILTIN, LOW);
      }


      // close the file:
      dataFile.close();
      SD.remove("datalog.txt");
    }
    else
    {
      // if the file didn't open, print an error:
      Serial.println("ouverture d'erreur datalog.txt");
    }
  }

  if (LedStatus == true)
  {
    digitalWrite(LED_BLEUE, HIGH);
    digitalWrite(LED_ROUGE, LOW);
  }
  else if (LedStatus == false)
  {
    digitalWrite(LED_BLEUE, LOW);
    digitalWrite(LED_ROUGE, HIGH);
  }
  else
  {
    digitalWrite(LED_BLEUE, LOW);
    digitalWrite(LED_ROUGE, LOW);
  }
}