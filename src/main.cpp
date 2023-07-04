#include <Arduino.h>
#include <DHTesp.h>
#include <BH1750.h>
#include "devices.h"

DHTesp dht;
BH1750 lightMeter;

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
  Serial.println("System ready!");
}

void loop() {
  Serial.println("Hello World!");
  digitalWrite(LED_BUILTIN, HIGH);
  delay(1000);
  digitalWrite(LED_BUILTIN, LOW);
  delay(3000);

  float fHumidity = dht.getHumidity();
  float fTemperature = dht.getTemperature();
  float lux = lightMeter.readLightLevel();
  Serial.printf("Humidity: %.2f, Temperature: %.2f, Light: %.2f \n",
      fHumidity, fTemperature, lux);

  if (digitalRead(PUSH_BUTTON)==LOW)
  {
    Serial.println("Button pressed!");
    digitalWrite(LED_RED, HIGH);
  }
  else
  {
    digitalWrite(LED_RED, LOW);
  }
}

