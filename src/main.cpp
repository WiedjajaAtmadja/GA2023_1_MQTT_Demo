#include <Arduino.h>
#include <DHTesp.h>
#include <BH1750.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "devices.h"

#define WIFI_SSID "XXX" // ganti dengan SSID masing-masing
#define WIFI_PASSWORD "XXX" // ganti dengan password masing-masing

#define MQTT_BROKER  "broker.emqx.io"

#define MQTT_TOPIC_PUBLISH_DEVICE_STATUS  "Binus_Malang/214/data/deviceStatus"
#define MQTT_TOPIC_PUBLISH_TEMP           "Binus_Malang/214/data/Temperature"
#define MQTT_TOPIC_PUBLISH_HUM            "Binus_Malang/214/data/Humidity"
#define MQTT_TOPIC_PUBLISH_LIGHT          "Binus_Malang/214/data/Light"
#define MQTT_TOPIC_PUBLISH_HELP_BUTTON    "Binus_Malang/214/data/HelpButton"

#define MQTT_TOPIC_SUBSCRIBE              "Binus_Malang/214/cmd/#" 
#define MQTT_TOPIC_CMD_LED_RED            "Binus_Malang/214/cmd/LedRed" 
#define MQTT_TOPIC_CMD_LED_GREEN          "Binus_Malang/214/cmd/LedGreen" 

DHTesp dht;
BH1750 lightMeter;
WiFiClient espClient;
PubSubClient mqtt(espClient);

void WifiConnect()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }  
  Serial.print("System connected with IP address: ");
  Serial.println(WiFi.localIP());
  Serial.printf("RSSI: %d\n", WiFi.RSSI());
}

void mqttCommandRecv(char* topic, byte* payload, unsigned int len) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.write(payload, len);
  Serial.println();
  int nLed = 0;
  if (strcmp(topic, MQTT_TOPIC_CMD_LED_RED)==0)
    nLed = LED_RED;

  if (strcmp(topic, MQTT_TOPIC_CMD_LED_GREEN)==0)
    nLed = LED_GREEN;

  int nLedStatus = payload[0] - '0';
  if (nLed>0)
    digitalWrite(nLed, nLedStatus);  
}

void mqttPublish(const char* szTopic, const char* szMessage)
{
  Serial.printf("Publishing message to topic: %s -> %s\n", szTopic, szMessage);
  mqtt.publish(szTopic, szMessage);
}

boolean mqttConnect() {
  char g_szDeviceId[30];
  sprintf(g_szDeviceId, "esp32_%08X",(uint32_t)ESP.getEfuseMac());
  mqtt.setServer(MQTT_BROKER, 1883);
  mqtt.setCallback(mqttCommandRecv);
  Serial.printf("Connecting to %s clientId: %s\n", MQTT_BROKER, g_szDeviceId);

  boolean fMqttConnected = false;
  for (int i=0; i<3 && !fMqttConnected; i++) {
    Serial.print("Connecting to mqtt broker...");
    fMqttConnected = mqtt.connect(g_szDeviceId);
    if (fMqttConnected == false) {
      Serial.print(" fail, rc=");
      Serial.println(mqtt.state());
      delay(1000);
    }
  }

  if (fMqttConnected)
  {
    Serial.println(" success");
    mqtt.subscribe(MQTT_TOPIC_SUBSCRIBE);
    Serial.printf("Subcribe topic: %s\n", MQTT_TOPIC_SUBSCRIBE);
    mqttPublish(MQTT_TOPIC_PUBLISH_DEVICE_STATUS, "1");
  }
  return mqtt.connected();
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  dht.setup(DHT_PIN, DHTesp::DHT11);
  pinMode(PUSH_BUTTON, INPUT_PULLUP);
  Wire.begin();
  lightMeter.begin(); 
  WifiConnect();
  mqttConnect();
  Serial.println("System online");
}

unsigned long nTimerHelpButton = 0;
unsigned long nTimerSendData = 0;
void loop() {
  unsigned long nNow;

  mqtt.loop();
  nNow = millis();
  if ((nNow - nTimerHelpButton)>5000) // tiap 5 detik lakukan:
  {
    nTimerHelpButton = nNow;
    if (digitalRead(PUSH_BUTTON)==LOW)
    {
      Serial.println("Button pressed!");
      mqttPublish(MQTT_TOPIC_PUBLISH_HELP_BUTTON, "1");
    }
    else
    {
      mqttPublish(MQTT_TOPIC_PUBLISH_HELP_BUTTON, "0");
    }
  }

  nNow = millis();
  if (nNow - nTimerSendData>10000) // tiap 10 detik lakukan:
  {
    // digitalWrite(LED_BUILTIN, HIGH);
    nTimerSendData = nNow;
    Serial.println("Send Data to MQTT Broker");

    float fHumidity = dht.getHumidity();
    float fTemperature = dht.getTemperature();
    float lux = lightMeter.readLightLevel();
    Serial.printf("Humidity: %.2f, Temperature: %.2f, Light: %.2f \n",
        fHumidity, fTemperature, lux);
    mqttPublish(MQTT_TOPIC_PUBLISH_TEMP, String(fTemperature).c_str());
    mqttPublish(MQTT_TOPIC_PUBLISH_HUM, String(fHumidity).c_str());
    mqttPublish(MQTT_TOPIC_PUBLISH_LIGHT, String(lux).c_str());        
    // digitalWrite(LED_BUILTIN, LOW);
  }
}

