#include "NewPing.h"

// Constants for the three sensors
const byte LDRPIN = A0;				// Light sensor pin
const byte TRIGPINS[] = {13, 2};	// TRIG pins for ultrasonic sensors
const byte ECHOPINS[] = {12, 7};	// ECHO pins for ultrasonic sensors
const byte DISPLAYPINS[] = {11, 5}; // LED pins for distance indication

// Constants for distance measurement
const int MAX_DISTANCE = 200;  // Maximum distance to measure in cm
const int MIN_DISTANCE = 20;   // Minimum distance to consider an object
const byte MIN_BRIGHTNESS = 0; // Minimum brightness level
const byte MAX_BRIGHTNESS = 255;
const byte MIN_LDR = 50; // Maximum brightness level
const byte ITER = 10;
const unsigned long SENSOR_DELAY = 500; // Delay to avoid sensor overlap
const byte TEMPERATURE = 30;			// Temperature in Celsius
const byte HUMIDITY = 50;				// Humidity in Percentage
const float SPEED_OF_SOUND = (331.4 + (0.606 * TEMPERATURE) + (0.0124 * HUMIDITY)) / 10000;

// Function prototypes
float getDistance(NewPing &sonar);
void handleSensorData(float distance, byte displayPin, byte ldrValue);

// Create NewPing objects
NewPing sonar1(TRIGPINS[0], ECHOPINS[0], MAX_DISTANCE);
NewPing sonar2(TRIGPINS[1], ECHOPINS[1], MAX_DISTANCE);

void setup()
{
	Serial.begin(115200);
	for (byte i = 0; i < sizeof(DISPLAYPINS); i++)
	{
		pinMode(DISPLAYPINS[i], OUTPUT);
	}
}

void loop()
{
	// Read the ambient light level from the LDR sensor
	byte ldrValue = analogRead(LDRPIN);

	// Read distances from sensors
	float distance1 = getDistance(sonar1);
	delay(SENSOR_DELAY); // Small delay to avoid overwhelming the sensors
	float distance2 = getDistance(sonar2);
	delay(SENSOR_DELAY); // Small delay to avoid overwhelming the sensors

	// Handle sensor data and update brightness for each display
	handleSensorData(distance1, DISPLAYPINS[0], ldrValue);
	handleSensorData(distance2, DISPLAYPINS[1], ldrValue);
}

float getDistance(NewPing &sonar)
{
	unsigned long duration = sonar.ping_median(ITER);
	if (duration == 0)
	{
		Serial.println("Error: No ping received.");
		return -1; // Return an error value
	}
	return (duration / 2) * SPEED_OF_SOUND;
}

void handleSensorData(float distance, byte displayPin, byte ldrValue)
{
	if (distance < 0)
	{
		Serial.println("Error: Invalid distance.");
		return;
	}

	// Adjust brightness based on LDR value
	if (ldrValue > MIN_LDR)
	{
		// Turn off light
		analogWrite(displayPin, 0); // Set brightness to 0 (off)
	}
	else
	{
		// Check if the object is within range and update the light state
		if (distance >= MIN_DISTANCE && distance <= MAX_DISTANCE)
		{
			analogWrite(displayPin, MAX_BRIGHTNESS); // Set brightness
		}
		else
		{
			// Dim the light
			analogWrite(displayPin, MIN_BRIGHTNESS); // Set brightness to min(off)
		}
	}

	// Print the distance for debugging
	// which sensor is being used
	if (displayPin == DISPLAYPINS[0])
	{
		Serial.print("Sensor 1: ");
	}
	else if (displayPin == DISPLAYPINS[1])
	{
		Serial.print("Sensor 2: ");
	}

	// Print the distance
	Serial.print("Distance measured: ");
	Serial.println(distance);
}
