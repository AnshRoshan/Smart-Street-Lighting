#include "NewPing.h"

// Constants for the sensor
const byte TRIGGER_PIN = 12;
const byte ECHO_PIN = 11;
const byte DISPLAY_PIN = 13;

const int MAX_DISTANCE = 400;
const byte ITERATIONS = 5;
const byte TEMPERATURE = 30; // Temperature in Celsius
const byte HUMIDITY = 50;    // Humidity in Percentage
const float SPEED_OF_SOUND = (331.4 + (0.606 * TEMPERATURE) + (0.0124 * HUMIDITY)) / 10000;
const float DISTANCE_THRESHOLD = 5.0; // Threshold in cm to consider as the same object

// Create NewPing object
NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE);

// Variables for the sensor
float distance, duration;
float lastDistance = -1;
unsigned long objectStartTime = 0;
unsigned long currentTime;
unsigned long sameObjectDuration = 0;

void setup()
{
    Serial.begin(9600);
    pinMode(DISPLAY_PIN, OUTPUT);
}

void loop()
{
    // Sensor
    duration = sonar.ping_median(ITERATIONS);
    distance = (duration / 2) * SPEED_OF_SOUND;
    handleSensorData(distance, lastDistance, objectStartTime, sameObjectDuration, DISPLAY_PIN, "Sensor");

    // Since we are testing with one sensor, we do not calculate speed
}

void handleSensorData(float &distance, float &lastDistance, unsigned long &objectStartTime, unsigned long &sameObjectDuration, byte displayPin, const char *sensorName)
{
    if (distance >= MAX_DISTANCE || distance <= 25)
    {
        digitalWrite(displayPin, LOW);
        Serial.print(sensorName);
        Serial.println(": The object is out of range");
    }
    else
    {
        digitalWrite(displayPin, HIGH);
        Serial.print(sensorName);
        Serial.print(": Distance measured: ");
        Serial.println(distance);

        if (lastDistance != -1 && abs(distance - lastDistance) < DISTANCE_THRESHOLD)
        {
            currentTime = millis();
            sameObjectDuration = currentTime - objectStartTime;
            Serial.print(sensorName);
            Serial.print(": Same object detected for: ");
            Serial.print(sameObjectDuration / 1000.0);
            Serial.println(" seconds");
        }
        else
        {
            objectStartTime = millis();
            sameObjectDuration = 0;
        }
        lastDistance = distance;
    }
}