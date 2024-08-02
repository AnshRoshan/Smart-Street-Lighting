
# Project: - Smart street light


<details>

<summary>Arduino Code</summary> 

```cpp
#include "NewPing.h"

// Constants for the three sensors
const byte TRIGGER_PIN_1 = 13;
const byte ECHO_PIN_1 = 12;
const byte DISPLAY_PIN_1 = 11;

const byte TRIGGER_PIN_2 = 9;
const byte ECHO_PIN_2 = 8;
const byte DISPLAY_PIN_2 = 10;

const byte TRIGGER_PIN_3 = 7;
const byte ECHO_PIN_3 = 6;
const byte DISPLAY_PIN_3 = 5;

// Constants same for all sensors
// Distance in cm
const int MAX_DISTANCE = 200;
// Minimum distance in cm
const int MIN_DISTANCE = 10;
// Number of iterations for averaging
const byte ITERATIONS = 5;
// Temperature in Celsius
const float TEMPERATURE = 25.0;
// Humidity in %
const float HUMIDITY = 50.0;
// Speed of sound in m/s at 20 degrees Celsius
const float SOUND_SPEED = (331.4 + (0.606 * TEMPERATURE) + (0.0124 * HUMIDITY));
// Threshold in cm to consider as the same object
const byte DISTANCE_THRESHOLD = 10;
// Distance between the sensors in cm
const byte SENSOR_DISTANCE = 50;
// Height of the sensor from the ground in cm
const byte SENSOR_HEIGHT = 160;

const byte MIN_BRIGHTNESS = 100;                     // Minimum brightness level
const byte MAX_BRIGHTNESS = 255;                     // Maximum brightness level
const unsigned long BRIGHTNESS_UPDATE_INTERVAL = 50; // Interval to update brightness (in milliseconds)
unsigned long lastBrightnessUpdate = 0;              // Last update time for brightness
byte currentBrightness = MIN_BRIGHTNESS;             // Current brightness

// Create NewPing objects
NewPing sonar1(TRIGGER_PIN_1, ECHO_PIN_1, MAX_DISTANCE);
NewPing sonar2(TRIGGER_PIN_2, ECHO_PIN_2, MAX_DISTANCE);
NewPing sonar3(TRIGGER_PIN_3, ECHO_PIN_3, MAX_DISTANCE);

// Structure to store object data
struct Object
{
  int id;
  float lastDistance;
  unsigned long startTime[3];
  unsigned long endTime[3];
  bool detected[3];
  bool hasCrossedAllSensors;
  float speed;
  byte size;
  bool stateChanged;

  Object() : id(-1), lastDistance(-1), startTime{0, 0, 0}, endTime{0, 0, 0},
             detected{false, false, false}, hasCrossedAllSensors(false), speed(0.0), size(0), stateChanged(false) {}
};
// Array to store detected objects
Object objects[10];   // Increased size to accommodate more objects
int nextObjectId = 1; // Start with ID 1

unsigned long lastPrintTime = 0;

void setup()
{
  Serial.begin(9600);
  pinMode(DISPLAY_PIN_1, OUTPUT);
  pinMode(DISPLAY_PIN_2, OUTPUT);
  pinMode(DISPLAY_PIN_3, OUTPUT);
}

void loop()
{
  // Sensor 1
  float distance1 = getDistance(sonar1);
  handleSensorData(distance1, DISPLAY_PIN_1, 0);

  // Sensor 2
  float distance2 = getDistance(sonar2);
  handleSensorData(distance2, DISPLAY_PIN_2, 1);

  // Sensor 3
  float distance3 = getDistance(sonar3);
  handleSensorData(distance3, DISPLAY_PIN_3, 2);

  // Calculate speed if the object passes all three sensors
  for (int i = 0; i < 10; i++)
  {
    if (objects[i].id != -1 && objects[i].hasCrossedAllSensors)
    {
      calculateSpeed(objects[i]);
    }
  }

  // Remove objects that haven't been detected for a while
  removeInactiveObjects();

  // Print the list of objects that have not passed all sensors
  if (millis() - lastPrintTime >= 10000) // 10 seconds
  {
    printInactiveObjects();
    lastPrintTime = millis();
  }

  delay(100);
}

float getDistance(NewPing &sonar)
{
  unsigned long duration = sonar.ping_median(ITERATIONS);
  return (duration * SOUND_SPEED) / (2.0 * 10000.0); // Speed of sound divided by 2 (go and return)
}

void handleSensorData(float distance, byte displayPin, int sensorIndex)
{
  bool stateChanged = false;

  // Calculate target brightness based on distance
  byte targetBrightness = map(distance, MIN_DISTANCE, MAX_DISTANCE, MIN_BRIGHTNESS, MAX_BRIGHTNESS);
  targetBrightness = constrain(targetBrightness, MIN_BRIGHTNESS, MAX_BRIGHTNESS);

  // Check if it's time to update brightness
  if (millis() - lastBrightnessUpdate >= BRIGHTNESS_UPDATE_INTERVAL)
  {
    if (currentBrightness < targetBrightness)
      // Increase brightness
      currentBrightness++;
    else if (currentBrightness > targetBrightness)
      // Decrease brightness
      currentBrightness--;
    // Set the new brightness value
    analogWrite(displayPin, currentBrightness);
    // Update the last update time
    lastBrightnessUpdate = millis();
  }

  // Serial output for debugging
  Serial.print("Distance measured: ");
  Serial.println(distance);

  // Find if this object already exists
  int existingObjectIndex = -1;
  for (int i = 0; i < 10; i++)
  {
    if (objects[i].id != -1 && abs(distance - objects[i].lastDistance) < DISTANCE_THRESHOLD)
    {
      existingObjectIndex = i;
      break;
    }
  }

  // If object exists, update it
  if (existingObjectIndex != -1)
  {
    Object &obj = objects[existingObjectIndex];
    obj.size = SENSOR_HEIGHT - distance;
    if (!obj.detected[sensorIndex])
    {
      obj.startTime[sensorIndex] = millis();
      obj.detected[sensorIndex] = true;
      stateChanged = true;
    }
    obj.endTime[sensorIndex] = millis();
    obj.lastDistance = distance;

    if (sensorIndex == 2 && obj.detected[0] && obj.detected[1] && obj.detected[2])
    {
      obj.hasCrossedAllSensors = true;
      Serial.print("Object with ID ");
      Serial.print(obj.id);
      Serial.println(" has crossed all sensors.");
    }
  }
  else // Create a new object
  {
    for (int i = 0; i < 10; i++)
    {
      if (objects[i].id == -1)
      {
        Object &obj = objects[i];
        obj.id = nextObjectId++;
        obj.lastDistance = distance;
        obj.size = SENSOR_HEIGHT - distance;
        obj.detected[sensorIndex] = true;
        obj.startTime[sensorIndex] = millis();
        obj.stateChanged = true;
        Serial.print("New object detected with ID ");
        Serial.println(obj.id);
        break;
      }
    }
  }

  // Print data if the state has changed
  if (stateChanged)
  {
    Serial.print("Distance measured: ");
    Serial.println(distance);
  }
}

void calculateSpeed(Object &obj)
{
  unsigned long time1to2 = obj.endTime[1] - obj.startTime[0];
  unsigned long time2to3 = obj.endTime[2] - obj.startTime[1];

  if (time1to2 > 0 && time2to3 > 0)
  {
    float time1to2Seconds = time1to2 / 1000.0;
    float time2to3Seconds = time2to3 / 1000.0;

    float speed1to2 = SENSOR_DISTANCE / time1to2Seconds;
    float speed2to3 = SENSOR_DISTANCE / time2to3Seconds;

    obj.speed = (speed1to2 + speed2to3) / 2;

    Serial.print("Speed between sensor 1 and 2: ");
    Serial.print(speed1to2);
    Serial.println(" cm/s");

    Serial.print("Speed between sensor 2 and 3: ");
    Serial.print(speed2to3);
    Serial.println(" cm/s");

    Serial.print("Average speed: ");
    Serial.print(obj.speed);
    Serial.println(" cm/s");

    // Determine the object's state
    if (obj.speed < 1.0)
    {
      Serial.println("Object is stationary");
    }
    else if (obj.speed < 10.0)
    {
      Serial.println("Object is moving slowly");
    }
    else
    {
      Serial.println("Object is moving fast");
    }
  }
}

void removeInactiveObjects()
{
  for (int i = 0; i < 10; i++)
  {
    if (objects[i].id != -1 && (millis() - objects[i].startTime[0]) > 5000)
    {
      Serial.print("Object with ID ");
      Serial.print(objects[i].id);
      Serial.println(" removed.");
      objects[i].id = -1;
    }
  }
}

void printInactiveObjects()
{
  Serial.println("List of objects that have not passed all sensors:");
  for (int i = 0; i < 10; i++)
  {
    if (objects[i].id != -1 && !objects[i].hasCrossedAllSensors)
    {
      Serial.print("Object ID: ");
      Serial.print(objects[i].id);
      Serial.print(", Size: ");
      Serial.print(objects[i].size);
      Serial.println(" cm");
    }
  }
}

```cpp


```

</details>


### Project 1: Simple Project

<details>
<summary>Arduino Code</summary> 


```cpp
#include "NewPing.h"

// Constants for the three sensors
const byte TRIGGER_PIN_1 = 13;
const byte ECHO_PIN_1 = 12;
const byte DISPLAY_PIN_1 = 11;

const byte TRIGGER_PIN_2 = 9;
const byte ECHO_PIN_2 = 8;
const byte DISPLAY_PIN_2 = 10;

const byte TRIGGER_PIN_3 = 7;
const byte ECHO_PIN_3 = 6;
const byte DISPLAY_PIN_3 = 5;

// Constants for distance measurement
const int MAX_DISTANCE = 200;         // Maximum distance to measure in cm
const int MIN_DISTANCE = 10;          // Minimum distance to consider an object
const byte MIN_BRIGHTNESS = 100;      // Minimum brightness level
const byte MAX_BRIGHTNESS = 255;      // Maximum brightness level
const unsigned long BRIGHTNESS_UPDATE_INTERVAL = 50; // Interval to update brightness in milliseconds

// Create NewPing objects
NewPing sonar1(TRIGGER_PIN_1, ECHO_PIN_1, MAX_DISTANCE);
NewPing sonar2(TRIGGER_PIN_2, ECHO_PIN_2, MAX_DISTANCE);
NewPing sonar3(TRIGGER_PIN_3, ECHO_PIN_3, MAX_DISTANCE);

byte currentBrightness1 = MIN_BRIGHTNESS; // Current brightness for display 1
byte currentBrightness2 = MIN_BRIGHTNESS; // Current brightness for display 2
byte currentBrightness3 = MIN_BRIGHTNESS; // Current brightness for display 3
unsigned long lastBrightnessUpdate1 = 0;  // Last update time for brightness of display 1
unsigned long lastBrightnessUpdate2 = 0;  // Last update time for brightness of display 2
unsigned long lastBrightnessUpdate3 = 0;  // Last update time for brightness of display 3

void setup()
{
  Serial.begin(9600);
  pinMode(DISPLAY_PIN_1, OUTPUT);
  pinMode(DISPLAY_PIN_2, OUTPUT);
  pinMode(DISPLAY_PIN_3, OUTPUT);
}

void loop()
{
  // Read distances from sensors
  float distance1 = getDistance(sonar1);
  float distance2 = getDistance(sonar2);
  float distance3 = getDistance(sonar3);

  // Handle sensor data and update brightness for each display
  handleSensorData(distance1, DISPLAY_PIN_1, currentBrightness1, lastBrightnessUpdate1);
  handleSensorData(distance2, DISPLAY_PIN_2, currentBrightness2, lastBrightnessUpdate2);
  handleSensorData(distance3, DISPLAY_PIN_3, currentBrightness3, lastBrightnessUpdate3);

  delay(100); // Small delay to avoid overwhelming the sensors
}

float getDistance(NewPing &sonar)
{
  unsigned long duration = sonar.ping_median(5); // Number of iterations for averaging
  return (duration * 0.0343) / 2.0; // Speed of sound in cm/us
}

void handleSensorData(float distance, byte displayPin, byte &currentBrightness, unsigned long &lastBrightnessUpdate)
{
  byte targetBrightness = map(distance, MIN_DISTANCE, MAX_DISTANCE, MIN_BRIGHTNESS, MAX_BRIGHTNESS);
  targetBrightness = constrain(targetBrightness, MIN_BRIGHTNESS, MAX_BRIGHTNESS);

  if (millis() - lastBrightnessUpdate >= BRIGHTNESS_UPDATE_INTERVAL)
  {
    if (currentBrightness < targetBrightness)
      currentBrightness++;
    else if (currentBrightness > targetBrightness)
      currentBrightness--;

    analogWrite(displayPin, currentBrightness);
    lastBrightnessUpdate = millis();
  }

  // Print the distance for debugging
  Serial.print("Distance measured: ");
  Serial.println(distance);

  // Check if the object is within range and update the light state
  if (distance >= MIN_DISTANCE && distance <= MAX_DISTANCE)
  {
    digitalWrite(displayPin, HIGH); // Turn on light
  }
  else
  {
    digitalWrite(displayPin, LOW); // Turn off light
  }
}


```
</details>

### Project 2: UltraSonic Sensor wthout Library

<details>
<summary>Arduino Code</summary> 


```cpp
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

// Variables for the sensor
float distance, duration;
float lastDistance = -1;
unsigned long objectStartTime = 0;
unsigned long currentTime;
unsigned long sameObjectDuration = 0;

void setup()
{
  Serial.begin(9600);
  pinMode(TRIGGER_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(DISPLAY_PIN, OUTPUT);
}

void loop()
{
  // Trigger the sensor
  digitalWrite(TRIGGER_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIGGER_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGGER_PIN, LOW);

  // Measure the duration of the echo pulse
  duration = pulseIn(ECHO_PIN, HIGH);

  // Calculate distance in cm
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

```
</details>

### project 3: Using 1 ultrasonic senor and NewPing.h library

<details>
<summary>Arduino Code</summary> 

```cpp
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

```
</details>



### Project 4: using three ultrasonic sensor

<details>
<summary>Arduino Code</summary> 

```cpp
#include "NewPing.h"

// Constants for the three sensors
const byte TRIGGER_PIN_1 = 12;
const byte ECHO_PIN_1 = 11;
const byte DISPLAY_PIN_1 = 13;

const byte TRIGGER_PIN_2 = 10;
const byte ECHO_PIN_2 = 9;
const byte DISPLAY_PIN_2 = 8;

const byte TRIGGER_PIN_3 = 7;
const byte ECHO_PIN_3 = 6;
const byte DISPLAY_PIN_3 = 5;


// Constants same for all.
const int MAX_DISTANCE = 400;
const byte ITERATIONS = 5;
const byte TEMPERATURE = 30; // Temperature in Celsius
const byte HUMIDITY = 50;    // Humidity in Percentage
const float SPEED_OF_SOUND = (331.4 + (0.606 * TEMPERATURE) + (0.0124 * HUMIDITY)) / 10000;
const float DISTANCE_THRESHOLD = 5.0; // Threshold in cm to consider as the same object
const float SENSOR_DISTANCE = 50.0;   // Distance between the sensors in cm

// Create NewPing objects
NewPing sonar1(TRIGGER_PIN_1, ECHO_PIN_1, MAX_DISTANCE);
NewPing sonar2(TRIGGER_PIN_2, ECHO_PIN_2, MAX_DISTANCE);
NewPing sonar3(TRIGGER_PIN_3, ECHO_PIN_3, MAX_DISTANCE);

// Variables for the three sensors
float distance1, duration1;
float distance2, duration2;
float distance3, duration3;

float lastDistance1 = -1;
float lastDistance2 = -1;
float lastDistance3 = -1;

unsigned long objectStartTime1 = 0;
unsigned long objectStartTime2 = 0;
unsigned long objectStartTime3 = 0;

unsigned long currentTime;
unsigned long sameObjectDuration1 = 0;
unsigned long sameObjectDuration2 = 0;
unsigned long sameObjectDuration3 = 0;

void setup()
{
  Serial.begin(9600);
  pinMode(DISPLAY_PIN_1, OUTPUT);
  pinMode(DISPLAY_PIN_2, OUTPUT);
  pinMode(DISPLAY_PIN_3, OUTPUT);
}

void loop()
{
  // Sensor 1
  duration1 = sonar1.ping_median(ITERATIONS);
  distance1 = (duration1 / 2) * SPEED_OF_SOUND;
  handleSensorData(distance1, lastDistance1, objectStartTime1, sameObjectDuration1, DISPLAY_PIN_1, "Sensor 1");

  // Sensor 2
  duration2 = sonar2.ping_median(ITERATIONS);
  distance2 = (duration2 / 2) * SPEED_OF_SOUND;
  handleSensorData(distance2, lastDistance2, objectStartTime2, sameObjectDuration2, DISPLAY_PIN_2, "Sensor 2");

  // Sensor 3
  duration3 = sonar3.ping_median(ITERATIONS);
  distance3 = (duration3 / 2) * SPEED_OF_SOUND;
  handleSensorData(distance3, lastDistance3, objectStartTime3, sameObjectDuration3, DISPLAY_PIN_3, "Sensor 3");

  // Calculate speed if the object passes all three sensors
  if (sameObjectDuration1 > 0 && sameObjectDuration2 > 0 && sameObjectDuration3 > 0)
  {
    calculateSpeed();
  }
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

void calculateSpeed()
{
  // Calculate the time differences
  unsigned long time1to2 = objectStartTime2 - objectStartTime1;
  unsigned long time2to3 = objectStartTime3 - objectStartTime2;

  // Convert time differences to seconds
  float time1to2Seconds = time1to2 / 1000.0;
  float time2to3Seconds = time2to3 / 1000.0;

  // Calculate speeds in cm/s
  float speed1to2 = SENSOR_DISTANCE / time1to2Seconds;
  float speed2to3 = SENSOR_DISTANCE / time2to3Seconds;

  // Average speed
  float averageSpeed = (speed1to2 + speed2to3) / 2;

  // Print speeds
  Serial.print("Speed between sensor 1 and 2: ");
  Serial.print(speed1to2);
  Serial.println(" cm/s");

  Serial.print("Speed between sensor 2 and 3: ");
  Serial.print(speed2to3);
  Serial.println(" cm/s");

  Serial.print("Average speed: ");
  Serial.print(averageSpeed);
  Serial.println(" cm/s");

  // Determine the object's state
  if (averageSpeed < 1.0)
  {
    Serial.println("Object is stationary");
  }
  else if (averageSpeed < 10.0)
  {
    Serial.println("Object is moving slowly");
  }
  else
  {
    Serial.println("Object is moving fast");
  }
}

```
</details>

### Project 5: my tinkercad old project

<details>
<summary>Arduino Code</summary> 

```cpp
// Define constants for LED, trigger, and echo pins
#define LED_1 11
#define LED_2 3
#define LED_3 9
#define LED_4 10
#define TRIG_1 8
#define ECHO_1 2
#define TRIG_2 5
#define ECHO_2 4
#define TRIG_3 7
#define ECHO_3 6

// Define constants for speed of sound (in cm/us), distance threshold (in cm), and analog read threshold
#define SPEED_OF_SOUND 0.034 
#define DISTANCE_THRESHOLD 300 
#define ANALOG_READ_THRESHOLD 800

// Function to measure distance using ultrasonic sensor and light up LEDs based on the measured distance
/**
 * Measures the distance using an ultrasonic sensor and controls LEDs based on the distance.
 * 
 * @param trigPin The pin connected to the trigger pin of the ultrasonic sensor.
 * @param echoPin The pin connected to the echo pin of the ultrasonic sensor.
 * @param ledPin1 The pin connected to the first LED.
 * @param ledPin2 The pin connected to the second LED.
 */
void ultra_sonic(int trigPin, int echoPin, int ledPin1, int ledPin2)
{
    // Declare variables for duration, time, and distance
    unsigned long duration;
    float distance, _time;

    // Send a short pulse on the trigger pin
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(5);
    digitalWrite(trigPin, LOW);

    // Measure the time it takes for the pulse to return to the echo pin
    duration = pulseIn(echoPin, HIGH);
    _time = duration / 2;  

    // Calculate the distance based on the measured time and the speed of sound
    distance = SPEED_OF_SOUND * _time;

    // If the distance is less than the threshold, turn on the LEDs
    if(distance < DISTANCE_THRESHOLD)
    {
        digitalWrite(ledPin1, HIGH);
        digitalWrite(ledPin2, HIGH);
    }
    // Otherwise, turn off the LEDs
    else
    {
        digitalWrite(ledPin1, LOW);
        // digitalWrite(ledPin2, LOW);
    }   
}

// Setup function runs once when the program starts
void setup()
{
    // Start serial communication at 9600 baud rate
    Serial.begin(9600);

    // Set the LED pins as output
    pinMode(LED_1, OUTPUT);
    pinMode(LED_2, OUTPUT);
    pinMode(LED_3, OUTPUT);
    pinMode(LED_4, OUTPUT);

    // Set the trigger pins as output and the echo pins as input
    pinMode(TRIG_1, OUTPUT);
    pinMode(ECHO_1, INPUT);
    pinMode(TRIG_2, OUTPUT);
    pinMode(ECHO_2, INPUT);
    pinMode(TRIG_3, OUTPUT);   
    pinMode(ECHO_3, INPUT); 
}

// Loop function runs continuously after the setup function finishes
void loop() 
{
    // Print the value read from the analog pin A0 to the serial monitor
    int ldrValue = analogRead(A0); // Read the value from the LDR
    Serial.println(ldrValue);

    // If the value read from A0 is greater than the threshold, trigger the ultrasonic sensors
    if(ldrValue > ANALOG_READ_THRESHOLD)
    {   
        // // LDR and Bulb
        // int bulbValue = map(ldrValue, 0, 1023, 0, 255); // Map the LDR value to a range suitable for the bulb
        // analogWrite(bulbPin, bulbValue); // Write the mapped value to the bulb
        ultra_sonic(TRIG_1, ECHO_1, LED_1, LED_2);
        ultra_sonic(TRIG_2, ECHO_2, LED_2, LED_3);
        ultra_sonic(TRIG_3, ECHO_3, LED_3, LED_4);
    }
    // Otherwise, turn off all the LEDs
    else
    {
        digitalWrite(LED_1, LOW);
        digitalWrite(LED_2, LOW);
        digitalWrite(LED_3, LOW);
        digitalWrite(LED_4, LOW);
    }
}
```
</details>
