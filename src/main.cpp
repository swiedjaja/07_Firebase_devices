/*
Google Firebase control and monitoring
* Control led: 
  - cmd/LedGreen
  - cmd/LedRed
  - cmd/LedYellow
* Receive sensor data
  - data/humidity
  - data/temperature
  - data/light
*/
#include <Arduino.h>
#include <Ticker.h>
#if defined(ESP32)  
  #include <WiFi.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
#endif
#include <Wire.h>
#include "BH1750.h"
#include "DHTesp.h" // Click here to get the library: http://librarymanager/All#DHTesp
#include "firebase.h"
#include "device.h"

const char* ssid = "Steff-IoT";
const char* password = "steffiot123";

Ticker timerPublish, ledOff;
DHTesp dht;
BH1750 lightMeter;

void WifiConnect();
void onSendSensor();

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
#if defined(ESP8266)  
  pinMode(LED_YELLOW, OUTPUT);
#endif
  pinMode(PIN_SW, INPUT_PULLUP);
  dht.setup(PIN_DHT, DHTesp::DHT11);
  Wire.begin(PIN_SDA, PIN_SCL);
  lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, 0x23, &Wire);

  WifiConnect();
  Firebase_Init("cmd");
  Serial.println("System ready.");
  digitalWrite(LED_BUILTIN, LED_BUILTIN_OFF);
}

void loop() {
  if (digitalRead(PIN_SW)==0)
  {
    onSendSensor();
  }
}

void onSendSensor()
{
  digitalWrite(LED_BUILTIN, LED_BUILTIN_ON);
  float humidity = dht.getHumidity();
  float temperature = dht.getTemperature();
  float lux = lightMeter.readLightLevel();
  if (dht.getStatus()==DHTesp::ERROR_NONE)
  {
    Serial.printf("Temperature: %.2f C, Humidity: %.2f %%, light: %.2f\n", 
      temperature, humidity, lux);
    Firebase.RTDB.setFloat(&fbdo, "/data/temperature", temperature);
    Firebase.RTDB.setFloat(&fbdo, "/data/humidity", humidity);
  }
  else
    Serial.printf("Light: %.2f lx\n", lux);

  Firebase.RTDB.setFloat(&fbdo, "/data/light", lux);

  ledOff.once_ms(100, [](){
      digitalWrite(LED_BUILTIN, LED_BUILTIN_OFF);
  });
}

void onFirebaseStream(FirebaseStream data)
{
  Serial.printf("onFirebaseStream: %s %s %s %s\n", data.streamPath().c_str(),
    data.dataPath().c_str(), data.dataType().c_str(), data.stringData().c_str());

  if (data.dataType()=="int")
  {
    String strDev = data.dataPath().substring(1);    
    byte nValue = data.stringData().charAt(0)-'0';
    Serial.printf("Device: %s -> %d\n", strDev.c_str(), nValue);

    if (nValue>=0 && nValue<=1)
    {
      if (strDev=="LedRed")
        digitalWrite(LED_RED, nValue);
      if (strDev=="LedGreen")
        digitalWrite(LED_GREEN, nValue);
    #ifdef ESP8266
      if (strDev=="LedYellow")
        digitalWrite(LED_YELLOW, nValue);
    #endif 
    }
  }
}

void WifiConnect()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }  
  Serial.print("System connected with IP address: ");
  Serial.println(WiFi.localIP());
  Serial.printf("RSSI: %d\n", WiFi.RSSI());
}
