// Include required libraries
#if defined(ESP32)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif
#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

// Define Wi-Fi credentials
#define WIFI_SSID "AR"
#define WIFI_PASSWORD "orionpax"

// Define Firebase API Key, Project ID, and user credentials
#define API_KEY "AIzaSyBY-ACQerVAfWBSM3xdUgofaLQ_JowjcU4"
#define FIREBASE_PROJECT_ID "bce-ansh"
#define USER_EMAIL "ansh@email.com"
#define USER_PASSWORD "ansh@20329"

// Define Firebase Data object, Firebase authentication, and configuration
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 19800, 1000); // 19800 sec offset for IST (GMT+5:30), update every 1 second

// Arrays for converting numeric values to strings
String weekDays[7] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
String months[12] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};

void setup()
{
    // Initialize serial communication for debugging
    Serial.begin(115200);

    // Connect to Wi-Fi
    connectToWiFi();

    // Initialize the NTP client
    timeClient.begin();

    // Print Firebase client version
    Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

    // Assign the API key
    config.api_key = API_KEY;

    // Assign the user sign-in credentials
    auth.user.email = USER_EMAIL;
    auth.user.password = USER_PASSWORD;

    // Assign the callback function for the long-running token generation task
    config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h

    // Begin Firebase with configuration and authentication
    Firebase.begin(&config, &auth);

    // Reconnect to Wi-Fi if necessary
    Firebase.reconnectWiFi(true);
}

void loop()
{
    // Update the NTP client
    timeClient.update();

    // Get the formatted current time
    String timeFormatted = getFormattedTime();

    // Send the current time to ATmega for synchronization
    sendTimeToAtmega(timeFormatted);

    // Check and process incoming data from ATmega
    if (Serial.available())
    {
        String ledLog = Serial.readStringUntil('\n');
        Serial.print("Received LED Log: ");
        Serial.println(ledLog);

        // Parse and send data to Firebase
        if (parseAndSendData(ledLog, timeFormatted))
        {
            Serial.println("Data parsed and sent successfully.");
        }
        else
        {
            Serial.println("Failed to parse data.");
        }
    }

    delay(500);
}

void connectToWiFi()
{
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to Wi-Fi");
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(300);
    }
    Serial.println();
    Serial.print("Connected with IP: ");
    Serial.println(WiFi.localIP());
    Serial.println();
}

String getFormattedTime()
{
    // Get the epoch time from the NTP client
    unsigned long epochTime = timeClient.getEpochTime();

    // Extract the hours, minutes, and seconds from the epoch time
    int currentHour = timeClient.getHours();
    int currentMinute = timeClient.getMinutes();
    int currentSecond = timeClient.getSeconds();

    // Convert time components to strings with leading zeros
    String hourString = currentHour < 10 ? "0" + String(currentHour) : String(currentHour);
    String minuteString = currentMinute < 10 ? "0" + String(currentMinute) : String(currentMinute);
    String secondString = currentSecond < 10 ? "0" + String(currentSecond) : String(currentSecond);

    // Format the time string in HH:MM:SS format
    String formattedTime = hourString + ":" + minuteString + ":" + secondString;

    return formattedTime;
}

void sendTimeToAtmega(String timeFormatted)
{
    Serial.print(timeFormatted);
    Serial.print("\n");
}

// Function to parse and send data to Firebase
bool parseAndSendData(String ledLog, String timeFormatted)
{
    // Expected format: "Sensor Data - LED1:10 cm,LED2:15 cm,LED3:20 cm,LDR:500"
    int index1 = ledLog.indexOf("LED1:");
    int index2 = ledLog.indexOf(",LED2:");
    int index3 = ledLog.indexOf(",LED3:");
    int index4 = ledLog.indexOf(",LDR:");

    if (index1 == -1 || index2 == -1 || index3 == -1 || index4 == -1)
    {
        // Debug: Print error if any index is not found
        Serial.println("Error: Incorrect data format received.");
        return false;
    }

    String led1 = ledLog.substring(index1 + 5, index2);
    String led2 = ledLog.substring(index2 + 6, index3);
    String led3 = ledLog.substring(index3 + 6, index4);
    String ldr = ledLog.substring(index4 + 5);

    // Debug: Print parsed values for verification
    Serial.println("Parsed Values:");
    Serial.print("LED1: ");
    Serial.println(led1);
    Serial.print("LED2: ");
    Serial.println(led2);
    Serial.print("LED3: ");
    Serial.println(led3);
    Serial.print("LDR: ");
    Serial.println(ldr);

    // Prepare data to send to Firebase
    FirebaseJson json;
    json.set("fields/time/STRING_VALUE", timeFormatted);
    json.set("fields/LED1/STRING_VALUE", led1);
    json.set("fields/LED2/STRING_VALUE", led2);
    json.set("fields/LED3/STRING_VALUE", led3);
    json.set("fields/LDR/STRING_VALUE", ldr);

    // Construct the Firebase path using the current timestamp
    String documentPath = "ledLog/" + String(timeClient.getEpochTime());

    // Send data to Firebase
    if (Firebase.Firestore.createDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), json))
    {
        Serial.println("Data sent successfully to Firebase");
        return true;
    }
    else
    {
        Serial.print("Error sending data to Firebase: ");
        Serial.println(fbdo.errorReason());
        return false;
    }
}
