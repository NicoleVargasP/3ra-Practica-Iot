#include <Arduino.h>
class SonarSensor
{
private:
  byte trigPin;// what number of pin in the board the trigger pin is connected to
  byte echoPin;// what number of pin in the board the echo pin is connected to

public:

//constructor
  SonarSensor(byte trigPin, byte echoPin)
  {
    this->trigPin=trigPin;
    this->echoPin=echoPin;
    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);
    digitalWrite(trigPin, LOW);
  }
  //sends an pulse and times the response time and converts it into centimeters
  float getDistanceCm()
  {

    //sends the trigger pulse
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);

    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    // measures time it takes to return a response
    unsigned long duration = pulseIn(echoPin, HIGH, 30000);// timesout at 30 milliseconds

    if (duration == 0)
      return -1; // didnt detect an object
    return (duration * 0.0343) / 2.0;// converts time into cms
  }
};