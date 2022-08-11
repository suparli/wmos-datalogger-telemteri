#include <Arduino.h>
#include <Wire.h>
#include <ErriezDS1307.h>
#include <SPI.h>
#include <SD.h>
#include <time.h>
#include "ADS1X15.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

const char *ssid = "Workshop MNI";
const char *password = "nokia3310";
const char *mqtt_server = "kendali-irigasi.com";
WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE (50)
char msg[MSG_BUFFER_SIZE];
int value = 0;

ADS1115 ADS(0x48);
float f = 0;

// Create RTC object
ErriezDS1307 rtc;

// Pin Out data SD Card
File myFile;
const int chipSelect = D8; // CS

// Variable
String humanTime, dataSave, nameFile, cm1;
int myTime, save, savingTime;
int sensor1, sensor2, sensor3, sensor4, sensorBit1, sensorBit2, sensorBit3, sensorBit4;
float sensorCm1, sensorCm2, sensorCm3, sensorCm4;
time_t epochTime;

void setup_wifi()
{

    delay(10);
    // We start by connecting to a WiFi network
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    randomSeed(micros());

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}

// void callback(char *topic, byte *payload, unsigned int length)
// {
//     Serial.print("Message arrived [");
//     Serial.print(topic);
//     Serial.print("] ");
//     for (int i = 0; i < length; i++)
//     {
//         Serial.print((char)payload[i]);
//     }
//     Serial.println();

//     // Switch on the LED if an 1 was received as first character
//     //   if ((char)payload[0] == '1') {
//     //     digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
//     //     // but actually the LED is on; this is because
//     //     // it is active low on the ESP-01)
//     //   } else {
//     //     digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
//     //   }
// }

void reconnect()
{
    // Loop until we're reconnected
    while (!client.connected())
    {
        Serial.print("Attempting MQTT connection...");
        // Create a random client ID
        String clientId = "ESP8266Client-";
        clientId += String(random(0xffff), HEX);
        // Attempt to connect
        if (client.connect(clientId.c_str()))
        {
            Serial.println("connected");
            // Once connected, publish an announcement...
            // client.publish("outTopic", "hello world");
            // // ... and resubscribe
            // client.subscribe("inTopic");
        }
        else
        {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }
}

void writeSD()
{

    //===================Simpan data ke sd Card
    myFile = SD.open(nameFile, FILE_WRITE);
    Serial.println(myFile);
    if (myFile)
    {
        Serial.println("Saving data please wait");
        myFile.println(humanTime + "," + sensorCm1 + "," + sensorCm2 + "," + sensorCm3 + "," + sensorCm4);
        myFile.close();
        delay(2000);
        Serial.println("Save data success...  ");
    }
    else
    {
        Serial.println("error open file");
    }
    delay(1000);
}

String parse(String data, char separator, int index)
{
    int found = 0;
    int strIndex[] = {0, -1};
    int maxIndex = data.length() - 1;

    for (int i = 0; i <= maxIndex && found <= index; i++)
    {
        if (data.charAt(i) == separator || i == maxIndex)
        {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i + 1 : i;
        }
    }

    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void setup()
{
    // Initialize serial port
    delay(500);
    Serial.begin(9600);

    setup_wifi();
    client.setServer(mqtt_server, 1883);
    // client.setCallback(callback);

    while (!Serial)
    {
        ;
    }
    Serial.println(F("\nErriez DS1307 set get time example"));

    // Initialize I2C
    Wire.begin();
    Wire.setClock(100000);

    // Initialize RTC
    while (!rtc.begin())
    {
        Serial.println(F("RTC not found"));
        delay(3000);
    }

    // Set date/time: 12:34:56 31 December 2020 Sunday
    // if (!rtc.setDateTime(10, 10, 00, 10, 8, 2022, 0))
    // {
    //     Serial.println(F("Set date/time failed"));
    // }

    // Set square wave out pin
    // SquareWaveDisable, SquareWave1Hz, SquareWave4096Hz, SquareWave8192Hz, SquareWave32768Hz
    rtc.setSquareWave(SquareWaveDisable);

    // Set save data in milliseconds
    savingTime = 600;
    // sensor1 = A0;
    // sensor2 = A1;
    // sensor3 = A2;
    // sensor4 = A3;
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);

    // see if the card is present and can be initialized:
    Serial.print("Initializing SD card...");
    if (!SD.begin(chipSelect))
    {
        Serial.println("Card failed, or not present");
        // don't do anything more:
        while (1)
            ;
    }
    Serial.println("card initialized.");

    if (!ADS.begin())
    {
        Serial.println("Failed to initialize ADS.");
        while (1)
            ;
    }
}

void loop()
{
    if (!client.connected())
    {
        reconnect();
    }
    client.loop();

    uint8_t hour;
    uint8_t min;
    uint8_t sec;
    uint8_t mday;
    uint8_t mon;
    uint16_t year;
    uint8_t wday;

    ADS.setGain(1);

    int16_t a0 = ADS.readADC(0);
    // int16_t a1 = ADS.readADC(1);
    // int16_t a2 = ADS.readADC(2);
    // int16_t a3 = ADS.readADC(3);
    
    //15bit
    sensorCm1 = 0.0042*float(a0);    
    // sensorCm2 = 0.1629*float(a1);
    // sensorCm3 = 0.1629*float(a2);
    // sensorCm4 = 0.1629*float(a3);

    // Read date/time
    if (!rtc.getDateTime(&hour, &min, &sec, &mday, &mon, &year, &wday))
    {
        Serial.println(F("Read date/time failed"));
        return;
    }

    // Read epochtime
    epochTime = rtc.getEpoch();

    humanTime = String(mday) + "/" + String(mon) + "/" + String(year) + " " + String(hour) + ":" + String(min);
    save = epochTime % savingTime;

    if (!SD.begin(chipSelect))
    {
        Serial.println("Card failed, or not present");
        digitalWrite(LED_BUILTIN, LOW);
    }

    if (save == 0)
    {
        nameFile = String(year) + String(mon) + String(mday) + ".txt";
        writeSD();
        cm1 = "{\"id\":311,\"data\":"+String(sensorCm1)+"}";
        char cm1Buff[100];
        cm1.toCharArray(cm1Buff, 100);
        client.publish("dataonline/311", cm1Buff);
    }

    // Serial.println("Sensor 1: " + String(a0));
    Serial.println("Water Level : " + String(sensorCm1) + " cm");
    // Serial.println("Sensor 2: " + String(sensorBit2));
    // Serial.println("Sensor Cm 2 : " + String(sensorCm2));
    //  Serial.println("Sensor 3: " + String(sensorBit3));
    // Serial.println("Sensor Cm 3 : " + String(sensorCm3));
    //  Serial.println("Sensor 4: " + String(sensorBit4));
    // Serial.println("Sensor Cm 4 : " + String(sensorCm4));
    Serial.println("Waktu Save : " + String(save));
    Serial.println("Waktu : " + String(humanTime));

    if (Serial.available() > 0)
    {
        // read the incoming byte:
        String incomingByte = Serial.readString();

        // say what you got:
        Serial.print("I received: ");
        Serial.println(incomingByte);

        if (parse(incomingByte, ',', 0) == "setwaktu")
        {
            String tanggal = parse(incomingByte, ',', 1);
            String bulan = parse(incomingByte, ',', 2);
            String tahun = parse(incomingByte, ',', 3);
            String jam = parse(incomingByte, ',', 4);
            String menit = parse(incomingByte, ',', 5);
            if (!rtc.setDateTime(jam.toInt(), menit.toInt(), 0, tanggal.toInt(), bulan.toInt(), tahun.toInt(), 0))
            {
                Serial.println(F("Set date/time failed"));
            }
            if (!rtc.getDateTime(&hour, &min, &sec, &mday, &mon, &year, &wday))
            {
                Serial.println(F("Read date/time failed"));
                return;
            }

            Serial.print("Waktu berhasil di setting");
        }
    }

    digitalWrite(LED_BUILTIN, LOW);
    delay(500);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
}