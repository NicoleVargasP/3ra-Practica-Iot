
#include <PubSubClient.h>
#include <Arduino.h>
#include "LED.h"

class SmartLED {
private:
  LED led;
  bool virtualState;
  String topicSet;
  String topicState;

public:
  SmartLED(int pin, String setT, String stateT)
    : led(pin), topicSet(setT), topicState(stateT), virtualState(false) {}

  void update() {
    led.update();
  }
  //the handler of the messages that have been sent to the LED and changes its state and configs
  void handleCommand(String message, PubSubClient &client) {

    if (message == "ON") {
      led.setState(LED::ON);
      client.publish(topicState.c_str(), "ON");
    }

    else if (message == "OFF") {
      led.setState(LED::OFF);
      client.publish(topicState.c_str(), "OFF");
    }

    else if (message.startsWith("BLINK")) {
      int idx = message.indexOf(':');

      if (idx > 0) {
        int speed = message.substring(idx + 1).toInt();
        led.setBlinksPerSecond(speed);
        led.setState(LED::BLINK);

        client.publish(topicState.c_str(), "BLINK");
      }
    }
  }
//virtual blink is made just for the visual and aesthetic aspect of the iot mqtt panel app to turn on and off leds when its set to blink
//these are virtual blinks that are not in sync with the real LEDS
//publishes into the topic of state led 
  void virtualBlink(PubSubClient &client) {
    if (led.getState() != LED::BLINK) return;

    virtualState = !virtualState;
    client.publish(topicState.c_str(), virtualState ? "ON" : "OFF");
  }

  String getTopicSet() {
    return topicSet;
  }
};