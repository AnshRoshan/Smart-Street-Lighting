
WEPAPIKEY=AIzaSyBY-ACQerVAfWBSM3xdUgofaLQ_JowjcU4

<details>
<summary>Code</summary >

```cpp
#include <ESP8266WiFi.h>

const char* ssid = "AR";
const char* password = "omegalock";

// Define Firebase API Key, Project ID, and user credentials
#define API_KEY "AIzaSyBI8gDd6XTD2r6D_RuzoFwaY8RmPrgUMBA"
#define FIREBASE_PROJECT_ID "espfirestore-7b1b9"
#define USER_EMAIL "aryantfk5@gmail.com"
#define USER_PASSWORD "zxcvbnm123"

// Define Firebase Data object, Firebase authentication, and configuration
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Initialize the DHT sensor
DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(115200);
  delay(100);
  connectToWiFi();
  pinMode(D3, OUTPUT);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected, reconnecting...");
    connectToWiFi();
  }
  delay(1000);  // Check WiFi connection every second
}

void connectToWiFi() {
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 10) {
    delay(1000);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("");
    digitalWrite(14, HIGH);
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } 
  else {
    Serial.println("");
    Serial.println("Failed to connect to WiFi");
     digitalWrite(14, LOW);
  }
}

```
</details>


```cpp


#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>

char auth[] = BLYNK_AUTH_TOKEN;

char ssid[] = "SSID";
char pass[] = "Password";

// This function is called every time the Virtual Pin 0 state changes
BLYNK_WRITE(V0)
{
  int value = param.asInt();
  value ? digitalWrite(D1, HIGH) : digitalWrite(D1, LOW);
}

void setup()
{
  pinMode(D1, OUTPUT);
  pinMode(D4, OUTPUT);
  pinMode(D8, OUTPUT);
  Serial.begin(115200);
  Blynk.begin(auth, ssid, pass);
}

void loop()
{
  Blynk.run();
}

```

details
