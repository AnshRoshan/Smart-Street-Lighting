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

    // Get the epoch time from the NTP client
    unsigned long epochTime = timeClient.getEpochTime();

    // Convert epoch time to time structure
    struct tm *ptm = localtime((time_t *)&epochTime);

    // Extract time components
    int hours = ptm->tm_hour;
    int day = ptm->tm_mday;
    int month = ptm->tm_mon;        // Months are 0-11
    int year = ptm->tm_year + 1900; // Years since 1900
    int minutes = ptm->tm_min;
    int seconds = ptm->tm_sec;
    int weekDay = ptm->tm_wday; // Day of the week (0 = Sunday, 6 = Saturday)

    // Format time components
    String yearStr = String(year);
    String monthStr = months[month]; // Month name
    String dayStr = (day < 10 ? "0" : "") + String(day);
    String hourStr = (hours < 10 ? "0" : "") + String(hours);
    String minStr = (minutes < 10 ? "0" : "") + String(minutes);
    String secStr = (seconds < 10 ? "0" : "") + String(seconds);
    String weekDayStr = weekDays[weekDay]; // Weekday name

    // Format as 'YYYY-MonthName-DDTHH:MM:SS-Weekday'
    String timeFormatted = yearStr + "-" + monthStr + "-" + dayStr + "T" + hourStr + ":" + minStr + ":" + secStr + "-" + weekDayStr;

    // Print the time to serial
    String timeam = hourStr + ":" + minStr;

    // Send current time to ATmega for synchronization
    Serial.println(timeam);

    // Check if there's any incoming data from ATmega
    if (Serial.available())
    {
        String ledLog = Serial.readStringUntil('\n');
        Serial.print("Received LED Log: ");
        Serial.println(ledLog);

        // Parse the log for LED and LDR values
        int led1Start = ledLog.indexOf("LED1:") + 5;
        int led1End = ledLog.indexOf(",LED2:");
        String led1Duration = ledLog.substring(led1Start, led1End);

        int led2Start = led1End + 6;
        int led2End = ledLog.indexOf(",LED3:");
        String led2Duration = ledLog.substring(led2Start, led2End);

        int led3Start = led2End + 6;
        int led3End = ledLog.indexOf(",LDR:");
        String led3Duration = ledLog.substring(led3Start, led3End);

        int ldrStart = led3End + 5;
        String ldrValue = ledLog.substring(ldrStart);

        // Create a FirebaseJson object for storing time, LED1Duration, LED2Duration, LED3Duration, and LDR value
        FirebaseJson content;
        content.set("fields/LED1/stringValue", led1Duration == "Not Working" ? "Sensor not working" : led1Duration);
        content.set("fields/LED2/stringValue", led2Duration == "Not Working" ? "Sensor not working" : led2Duration);
        content.set("fields/LED3/stringValue", led3Duration == "Not Working" ? "Sensor not working" : led3Duration);
        content.set("fields/LDR/stringValue", ldrValue);

        // Create document path for Firestore
        String documentPath = "LEDLogs/" + timeFormatted;

        // Update Firestore document with the new data
        if (Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.raw(), "LED1,LED2,LED3,LDR"))
        {
            Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
        }
        else
        {
            Serial.println(fbdo.errorReason());
        }
        delay(1000);
    }
}
