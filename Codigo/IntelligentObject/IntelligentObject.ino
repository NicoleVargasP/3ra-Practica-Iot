#include <WiFi.h>          // or ESP8266WiFi.h
#include <PubSubClient.h>
#include "SmartLED.h"
#include "LED.h"
#include "SonarSensor.h"

// ---------------- WIFI ----------------
const char* ssid = "POCO X3 NFC";
const char* password = "pikachu48";

// ---------------- MQTT ----------------
const char* mqtt_server = "broker.hivemq.com";

WiFiClient espClient;
PubSubClient client(espClient);

// ---------------- LEDs ----------------
SmartLED leds[] = {
  SmartLED(17, "IoT/ZubietaVargas/led/green/set", "IoT/ZubietaVargas/led/green/state"),
  SmartLED(18, "IoT/ZubietaVargas/led/yellow/set", "IoT/ZubietaVargas/led/yellow/state"),
  SmartLED(19, "IoT/ZubietaVargas/led/red/set", "IoT/ZubietaVargas/led/red/state"),
  SmartLED(16, "IoT/ZubietaVargas/led/blue/set", "IoT/ZubietaVargas/led/blue/state")
};

const int NUM_LEDS = 4;


// ---------------- SENSOR ----------------
SonarSensor sonar(26, 27);

// ---------------- TIMERS ----------------
unsigned long lastSensorPublish = 0;
const unsigned long BLINK_INTERVAL = 1000; // time in ms for a virtual blink(just for visual purposes in the mqtt panel app) meaning they arent synced with the physical leds
unsigned long lastBlinkTick = 0;

// ---------------- FUNCTION DECLARATIONS ----------------
void reconnect();
void callback(char* topic, byte* payload, unsigned int length);
void handleLEDCommand(LED &led, String message, const char* stateTopic);
void reconnectWiFi();
void handleVirtualBlink(LED &led, bool &state, const char* topic);

// ---------------- SETUP ----------------
void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected");

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

// ---------------- LOOP ----------------
void loop() {
  reconnectWiFi();

  if (!client.connected()) {
    reconnect();
  }

  client.loop();
//Updates every led in case it needs to blink
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i].update();
  }
//checks if 10 s passed in order to publish a distance once more
  if (millis() - lastSensorPublish > 10000) {
    lastSensorPublish = millis();
    float dist = sonar.getDistanceCm();
    client.publish("IoT/ZubietaVargas/sensor/distance", String(dist).c_str());
  }
//checks if the virtual blink interval has passed and makes them blink virtually(just for aesthetic purposes in the mqtt panel app)
  if (millis() - lastBlinkTick >= BLINK_INTERVAL) {
    lastBlinkTick = millis();

    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i].virtualBlink(client);
    }
  }
}

// ---------------- MQTT RECONNECT ----------------
void reconnect() {
  while (!client.connected()) {
    Serial.print("Connecting MQTT...");

    if (client.connect("ZubietaClient")) {
      Serial.println("connected");

      // Subscribe to LED topics
      client.subscribe("IoT/ZubietaVargas/led/green/set");
      client.subscribe("IoT/ZubietaVargas/led/yellow/set");
      client.subscribe("IoT/ZubietaVargas/led/red/set");
      client.subscribe("IoT/ZubietaVargas/led/blue/set");

      // Sensor request
      client.subscribe("IoT/ZubietaVargas/sensor/get");

    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      delay(2000);
    }
  }
}

// ---------------- CALLBACK ----------------
//Function called when the mqtt client receives a message in client.loop
void callback(char* topic, byte* payload, unsigned int length) {

  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  String topicStr = String(topic);

  for (int i = 0; i < NUM_LEDS; i++) {
    if (topicStr == leds[i].getTopicSet()) {
      leds[i].handleCommand(message, client);
      return;
    }
  }

  if (topicStr == "IoT/ZubietaVargas/sensor/get") {
    float dist = sonar.getDistanceCm();
    String msg = String(dist);
    client.publish("IoT/ZubietaVargas/sensor/distance", msg.c_str());
  }
}

//If the device has been disconnected from the internet this function tryes reconnecting it 
void reconnectWiFi() {

  if (WiFi.status() == WL_CONNECTED) return;

  Serial.println("WiFi lost. Reconnecting...");

  WiFi.disconnect();
  WiFi.begin(ssid, password);

  unsigned long startAttemptTime = millis();

  // Try for 10 seconds max
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi reconnected!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi reconnect failed.");
  }
}
