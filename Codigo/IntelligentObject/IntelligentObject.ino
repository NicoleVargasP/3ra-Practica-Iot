#include <WiFi.h>          // or ESP8266WiFi.h
#include <PubSubClient.h>

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
LED ledGreen(17);
LED ledYellow(18);
LED ledRed(19);
LED ledBlue(16);



bool greenVirtual = false;
bool yellowVirtual = false;
bool redVirtual = false;
bool blueVirtual = false;

// ---------------- SENSOR ----------------
SonarSensor sonar(26, 27);

// ---------------- TIMERS ----------------
unsigned long lastSensorPublish = 0;
const unsigned long BLINK_INTERVAL = 1500; // ms
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
  void reconnectWiFi();

  if (!client.connected()) {
    reconnect();
  }

  client.loop();

  // Update blinking LEDs
  ledGreen.update();
  ledYellow.update();
  ledRed.update();
  ledBlue.update();

  // Publish sensor every 10 seconds
  if (millis() - lastSensorPublish > 10000) {
    lastSensorPublish = millis();

    float dist = sonar.getDistanceCm();

    String msg = String(dist);
    client.publish("IoT/ZubietaVargas/sensor/distance", msg.c_str());
  }
  // Handle the virtual blinking for the LEDS
  if (millis() - lastBlinkTick >= BLINK_INTERVAL) {
  lastBlinkTick = millis();

  handleVirtualBlink(ledGreen, greenVirtual, "IoT/ZubietaVargas/led/green/state");
  handleVirtualBlink(ledYellow, yellowVirtual, "IoT/ZubietaVargas/led/yellow/state");
  handleVirtualBlink(ledRed, redVirtual, "IoT/ZubietaVargas/led/red/state");
  handleVirtualBlink(ledBlue, blueVirtual, "IoT/ZubietaVargas/led/blue/state");
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
void callback(char* topic, byte* payload, unsigned int length) {

  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  String topicStr = String(topic);

  // LED CONTROL
  if (topicStr == "IoT/ZubietaVargas/led/green/set") {
    handleLEDCommand(ledGreen, message, "IoT/ZubietaVargas/led/green/state");
  }

  else if (topicStr == "IoT/ZubietaVargas/led/yellow/set") {
    handleLEDCommand(ledYellow, message, "IoT/ZubietaVargas/led/yellow/state");
  }

  else if (topicStr == "IoT/ZubietaVargas/led/red/set") {
    handleLEDCommand(ledRed, message, "IoT/ZubietaVargas/led/red/state");
  }

  else if (topicStr == "IoT/ZubietaVargas/led/blue/set") {
    handleLEDCommand(ledBlue, message, "IoT/ZubietaVargas/led/blue/state");
  }

  // SENSOR ON DEMAND
  else if (topicStr == "IoT/ZubietaVargas/sensor/get") {
    float dist = sonar.getDistanceCm();
    String msg = String(dist);
    client.publish("IoT/ZubietaVargas/sensor/distance", msg.c_str());
  }
}

// ---------------- LED HANDLER ----------------
void handleLEDCommand(LED &led, String message, const char* stateTopic) {

  if (message == "ON") {
    led.setState(LED::ON);
    client.publish(stateTopic, "ON");
  }

  else if (message == "OFF") {
    led.setState(LED::OFF);
    client.publish(stateTopic, "OFF");
  }

  else if (message.startsWith("BLINK")) {

    int idx = message.indexOf(':');

    if (idx > 0) {
      int speed = message.substring(idx + 1).toInt();

      led.setBlinksPerSecond(speed);
      led.setState(LED::BLINK);

      client.publish(stateTopic, "BLINK");
    }
  }
}
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
void handleVirtualBlink(LED &led, bool &state, const char* topic) {

  // Solo si el LED está en modo BLINK
  if (led.getState() != LED::BLINK) return;

  state = !state;

  client.publish(topic, state ? "ON" : "OFF");
}