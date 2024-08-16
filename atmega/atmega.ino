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
const unsigned long sendInterval = 5000;
const int NIGHT_START = 18; // 6 PM
const int NIGHT_END = 6;	// 6 AM

// Last time data was sent
unsigned long lastSendTime = 0;
unsigned long lastLightCheck = 0;
unsigned long lightCheckInterval = 10000; // Check light sensor every 10 seconds

int currentHour = 0;
int currentMinute = 0;

void setup()
{
	Serial.begin(115200);

	for (int i = 0; i < 3; i++)
	{
		pinMode(displayPins[i], OUTPUT);
		// Set display pins as output for LEDs
	}

	pinMode(ldrpin, INPUT); // Set light sensor pin as input
}

void loop()
{
	// Check if there's any serial data available
	if (Serial.available() > 0)
	{
		// Read the incoming serial data
		String serialData = Serial.readStringUntil('\n');
		// Parse time format "HH:MM"
		if (serialData.length() >= 5)
		{
			currentHour = serialData.substring(0, 2).toInt();
			currentMinute = serialData.substring(3, 5).toInt();
			// Serial.print("Current Time: ");
			// Serial.print(currentHour);
			// Serial.print(":");
			// Serial.println(currentMinute);
		}
	}

	// Read light intensity
	lightIntensity = analogRead(ldrpin);

	// Measure distances with ultrasonic sensors
	distance_cm[0] = sonar1.ping_cm();
	distance_cm[1] = sonar2.ping_cm();
	distance_cm[2] = sonar3.ping_cm();

	// Prepare distance and light data strings
	String distances = "LED1:";
	for (int i = 0; i < 3; i++)
	{
		// Check if the sensor is not working
		if (distance_cm[i] == 0)
		{
			distances += "Not Working";
		}
		else
		{
			distances += String(distance_cm[i]) + " cm";
		}
		if (i < 2)
		{
			distances += ",";
		}
	}

	// Include LDR value in the data string
	distances += ",LDR:" + String(lightIntensity);

	// Send data every sendInterval
	unsigned long currentMillis = millis();
	if (currentMillis - lastSendTime >= sendInterval)
	{
		// Send structured data
		Serial.println(distances);

		lastSendTime = currentMillis;
	}

	// Debugging information for each sensor
	for (int i = 0; i < 3; i++)
	{
		// Serial.print("Sensor ");
		// Serial.print(i + 1);
		// Serial.print(" distance: ");
		// if (distance_cm[i] == 0)
		// {
		//     Serial.println("Not Working");
		// }
		// else
		// {
		//     Serial.print(distance_cm[i]);
		//     Serial.println(" cm");
		// }

		if (distance_cm[i] > 10 && distance_cm[i] <= 25)
		{
			analogWrite(displayPins[i], 200);
		}
		else
		{
			analogWrite(displayPins[i], 20);
		}
	}

	// Check light sensor every lightCheckInterval
	if (currentMillis - lastLightCheck >= lightCheckInterval)
	{
		lastLightCheck = currentMillis;

		// Print the light intensity
		Serial.print("Light intensity: ");
		Serial.println(lightIntensity);

		// Determine if it's day or night
		bool isNight = (currentHour >= NIGHT_START || currentHour < NIGHT_END);
		bool ldrError = (lightIntensity < 150 && !isNight) || (lightIntensity > 600 && isNight);

		if (ldrError)
		{
			Serial.println("LDR error: inconsistent readings");
			// Adjust LED brightness based on known time
			if (isNight)
			{
				for (int i = 0; i < 3; i++)
				{
					analogWrite(displayPins[i], 255); // Full brightness
				}
			}
			else
			{
				for (int i = 0; i < 3; i++)
				{
					analogWrite(displayPins[i], 0); // LEDs off
				}
			}
		}
		else
		{
			// Control LEDs based on light intensity
			if (isNight)
			{
				for (int i = 0; i < 3; i++)
				{
					if (lightIntensity < 100)
					{
						analogWrite(displayPins[i], 255); // Full brightness
					}
					else
					{
						analogWrite(displayPins[i], 100); // Dimmed
					}
				}
			}
			else
			{
				// Turn off LEDs during the day
				for (int i = 0; i < 3; i++)
				{
					analogWrite(displayPins[i], 0);
				}
			}
		}
	}

	delay(500); // Delay before the next loop iteration
}
