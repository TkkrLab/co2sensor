#include <SPI.h>
#include <Ethernet2.h>
#include <EthernetClient.h>
#include <EEPROM.h>
#include <SoftwareSerial.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

SoftwareSerial co2Serial(8, 9);
EthernetClient client;
Adafruit_MQTT_Client *mqtt;
Adafruit_MQTT_Publish *co2publish;
Adafruit_MQTT_Publish *co2statusPublish;
Adafruit_MQTT_Publish *co2minimumPublish;
Adafruit_MQTT_Publish *co2tempPublish;

//byte mac[] = {  0xDB, 0x33, 0x30, 0xC8, 0xDD, 0x65 };

byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEE, 0xEE, 0xEE
};

struct struct_settings {
  uint8_t magic;
  char name[32];
  char server[32];
  char username[32];
  char password[32];
  char topic[32];
  char topicStatus[32];
  char topicMinimum[32];
  char topicTemp[32];
} settings;

const int struct_settings_size = sizeof(struct_settings);

bool loadSettings() {
  EEPROM.get(0, settings);
  if (settings.magic != 0x42) return false;
  return true;
}

void saveSettings() {
  EEPROM.put(0, settings);
}

void serialInput(char* buffer, uint16_t len) {
  while(Serial.available()) Serial.read();
  for (uint16_t i = 0; i<len; i++) buffer[i] = 0;
  uint16_t pos = 0;
  while(pos<len) {
    while(Serial.available()) {
      buffer[pos] = Serial.read();
      if ((buffer[pos]=='\n') || (buffer[pos]=='\r')) {
        buffer[pos] = 0;
        Serial.println();
        while(Serial.available()) Serial.read();
        return;
      } else {
        Serial.print(buffer[pos]);
      }
      pos++;
    }
  }
}

void configure() {
  while(Serial.available()) Serial.read(); //Flush
  Serial.println(F("=== SETUP ==="));
  Serial.println(F("Name: "));
  serialInput(settings.name, 32);
  Serial.println(F("Server: "));
  serialInput(settings.server, 32);
  Serial.println(F("Username: "));
  serialInput(settings.username, 32);
  Serial.println(F("Password: "));
  serialInput(settings.password, 32);
  Serial.println(F("Topic: "));
  serialInput(settings.topic, 32);
  Serial.println(F("Topic for status: "));
  serialInput(settings.topicStatus, 32);
  Serial.println(F("Topic for minimum: "));
  serialInput(settings.topicMinimum, 32);
  Serial.println(F("Topic for temperature: "));
  serialInput(settings.topicTemp, 32);
  settings.magic = 0x42;
  saveSettings();
  Serial.println("=== Done! ===");
}

void setup() {
  Serial.begin(115200);
  co2Serial.begin(9600);
  if (!loadSettings()) {
    configure();
  }
  Serial.print("Press any key to enter setup...");
  while (Serial.available() > 0) Serial.read();
  for (uint8_t i = 0; i < 10; i++) {
    if (Serial.available() > 0) {
      while (Serial.available() > 0) Serial.read();
      Serial.println();
      configure();
      break;
    }
    Serial.print(".");
    delay(100);
  }
  Serial.println();

  Serial.println("Configuration");
  Serial.println("  Name              "+String(settings.name));
  Serial.println("  Server            "+String(settings.server));
  Serial.println("  Username          "+String(settings.username));
  Serial.println("  Password          "+String(settings.password));
  Serial.println("  Topic             "+String(settings.topic));
  Serial.println("  Topic status      "+String(settings.topicStatus));
  Serial.println("  Topic minimum     "+String(settings.topicMinimum));
  Serial.println("  Topic temperature "+String(settings.topicTemp));

  Serial.print("\n\nConnecting to ethernet... ");
  Ethernet.begin(mac);
  Serial.println("DONE");
  Serial.print("IP address: ");
  Serial.println(Ethernet.localIP());
  delay(1000);
  Serial.print("Starting MQTT... ");
  mqtt = new Adafruit_MQTT_Client(&client, settings.server, 1883, settings.username, settings.password);
  co2publish = new Adafruit_MQTT_Publish(mqtt, settings.topic);
  co2statusPublish = new Adafruit_MQTT_Publish(mqtt, settings.topicStatus);
  co2minimumPublish = new Adafruit_MQTT_Publish(mqtt, settings.topicMinimum);
  co2tempPublish = new Adafruit_MQTT_Publish(mqtt, settings.topicTemp);
  Serial.println("DONE");
}

int16_t lastTemp = -1;
int16_t lastMinimum = -1;
int16_t lastStatus = -1;

int16_t readCo2() {
  lastTemp = -1;
  lastMinimum = -1;
  while (co2Serial.available() > 0) co2Serial.read();
  uint8_t cmd[9] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
  co2Serial.write(cmd, 9);
  uint8_t timeout = 100;
  while (co2Serial.available() < 9) {
    timeout--;
    if (timeout < 1) return -1;
    delay(1);
  }
  uint8_t response[9];
  co2Serial.readBytes(response, 9);
  if (response[0] != 0xFF) return -2;
  if (response[1] != 0x86) return -3;
  int16_t co2_ppm = (256 * response[2]) + response[3];
  uint8_t temp = response[4] - 40;
  uint16_t minimum = (256 * response[6]) + response[7];
  uint8_t status = response[5];
  //Serial.println("CO2 "+String(co2_ppm)+", temp "+String(temp)+", minimum "+String(minimum)+", status "+String(status));
  lastTemp = temp;
  lastMinimum = minimum;
  lastStatus = status;
  return co2_ppm;
}

void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt->connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  while ((ret = mqtt->connect()) != 0) { // connect will return 0 for connected
       Serial.println(mqtt->connectErrorString(ret));
       Serial.println("Retrying MQTT connection in 5 seconds...");
       mqtt->disconnect();
       delay(5000);  // wait 5 seconds
  }
  Serial.println("MQTT Connected!");
}

void loop() {
  MQTT_connect();
  if(! mqtt->ping()) {
    Serial.println("MQTT disconnected");
    mqtt->disconnect();
  }
  
  int16_t co2 = readCo2();
  if (co2 >= 0) {
    Serial.println("CO2 measurement: "+String(co2)+" ppm (S:"+String(lastStatus)+", M:"+String(lastMinimum)+", T:"+String(lastTemp)+")");
    String data = String(co2);
    String status = String(lastStatus);
    String minimum = String(lastMinimum);
    String temp = String(lastTemp);
    if (co2publish->publish((uint8_t*) data.c_str(), data.length())) {
      if (co2statusPublish->publish((uint8_t*) status.c_str(), status.length())) {
        if (co2minimumPublish->publish((uint8_t*) minimum.c_str(), minimum.length())) {
          if (co2tempPublish->publish((uint8_t*) temp.c_str(), temp.length())) {
            //Serial.println("Published!");
          } else {
            Serial.println("Could not publish 4!");
          }
        } else {
          Serial.println("Could not publish 3!");
        }
      } else {
        Serial.println("Could not publish 2!");
      }
    } else {
      Serial.println("Could not publish 1!");
    }
  } else {
    Serial.println("CO2 sensor error!");
  }
  delay(5000);
}
