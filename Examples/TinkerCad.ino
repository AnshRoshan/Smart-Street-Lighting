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
    if (distance < DISTANCE_THRESHOLD)
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
    if (ldrValue > ANALOG_READ_THRESHOLD)
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