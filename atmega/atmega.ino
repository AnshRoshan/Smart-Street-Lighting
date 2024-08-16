#include <NewPing.h>

// Constants
const byte ldrpin = A0;				   // Light sensor pin
const byte trigPins[] = {13, 2, 8};	   // TRIG pins for ultrasonic sensors
const byte echoPins[] = {12, 4, 7};	   // ECHO pins for ultrasonic sensors
const byte displayPins[] = {11, 3, 6}; // LED pins for distance indication
const unsigned int MAX_DISTANCE = 200; // Maximum distance to measure (in cm)

// Create NewPing objects for each sensor
NewPing sonar1(trigPins[0], echoPins[0], MAX_DISTANCE);
NewPing sonar2(trigPins[1], echoPins[1], MAX_DISTANCE);
NewPing sonar3(trigPins[2], echoPins[2], MAX_DISTANCE);

// Variables
int lightIntensity;
float distance_cm[3];
const unsigned long sendInterval = 5000; // Interval for sending sensor data
const int NIGHT_START = 18;				 // 6 PM
const int NIGHT_END = 6;				 // 6 AM

unsigned long lastSendTime = 0;			  // Last time data was sent
unsigned long lastLightCheck = 0;		  // Last time the light sensor was checked
unsigned long lightCheckInterval = 10000; // Check light sensor every 10 seconds

int currentHour = 0;
int currentMinute = 0;

void setup()
{
	Serial.begin(115200);
	for (int i = 0; i < 3; i++)
	{
		pinMode(displayPins[i], OUTPUT); // Set display pins as output for LEDs
	}
	pinMode(ldrpin, INPUT); // Set light sensor pin as input
}

void loop()
{
	readSerialTime();	// Check for serial data to update time
	readLightSensor();	// Read the light sensor value
	measureDistances(); // Measure distances from ultrasonic sensors
	controlLeds();		// Control LED brightness based on sensor readings
	sendSensorData();	// Send sensor data over serial

	delay(500); // Delay before the next loop iteration
}

// Function to read time from serial input
void readSerialTime()
{
	if (Serial.available() > 0)
	{
		String serialData = Serial.readStringUntil('\n');
		if (serialData.length() >= 5)
		{
			currentHour = serialData.substring(0, 2).toInt();
			currentMinute = serialData.substring(3, 5).toInt();
		}
	}
}

// Function to read light intensity from the LDR
void readLightSensor()
{
	unsigned long currentMillis = millis();
	if (currentMillis - lastLightCheck >= lightCheckInterval)
	{
		lastLightCheck = currentMillis;
		lightIntensity = analogRead(ldrpin);
		Serial.print("Light intensity: ");
		Serial.println(lightIntensity);
	}
}

// Function to measure distances using ultrasonic sensors
void measureDistances()
{
	distance_cm[0] = sonar1.ping_cm();
	distance_cm[1] = sonar2.ping_cm();
	distance_cm[2] = sonar3.ping_cm();
}

// Function to control LEDs based on sensor data and time
void controlLeds()
{
	bool isNight = (currentHour >= NIGHT_START || currentHour < NIGHT_END);
	bool ldrError = (lightIntensity < 150 && !isNight) || (lightIntensity > 600 && isNight);

	for (int i = 0; i < 3; i++)
	{
		if (ldrError)
		{
			analogWrite(displayPins[i], isNight ? 255 : 0); // Full brightness at night, off during day
		}
		else
		{
			if (isNight)
			{
				analogWrite(displayPins[i], lightIntensity < 100 ? 255 : 100); // Adjust based on intensity
			}
			else
			{
				analogWrite(displayPins[i], 0); // Off during the day
			}
		}
	}

	// Additional logic for distance-based LED brightness
	for (int i = 0; i < 3; i++)
	{
		if (distance_cm[i] > 10 && distance_cm[i] <= 25)
		{
			analogWrite(displayPins[i], 200); // Moderate brightness
		}
		else
		{
			analogWrite(displayPins[i], 20); // Low brightness
		}
	}
}

// Function to send sensor data over serial
void sendSensorData()
{
	unsigned long currentMillis = millis();
	if (currentMillis - lastSendTime >= sendInterval)
	{
		String distances = "Sensor Data - ";
		for (int i = 0; i < 3; i++)
		{
			distances += "LED" + String(i + 1) + ":";
			distances += (distance_cm[i] == 0 ? "Not Working" : String(distance_cm[i]) + " cm");
			if (i < 2)
			{
				distances += ",";
			}
		}
		distances += ",LDR:" + String(lightIntensity);
		Serial.println(distances);

		// Debug: Print the sent data to the console
		Serial.print("Sending data: ");
		Serial.println(distances);

		lastSendTime = currentMillis;
	}
}
