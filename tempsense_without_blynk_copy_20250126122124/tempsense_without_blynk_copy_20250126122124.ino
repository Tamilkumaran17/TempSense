#include <DHT.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>

// ThingSpeak API key and server
String apiKey = "QAEI43AHI1L09US1";
const char* server = "api.thingspeak.com";

// WiFi credentials
// const char* ssid = "SECE_WIFI";
// const char* password = "Sece@123";

const char* ssid = "Tamizh";
const char* password = "99999999";

// Google Sheets script details
const char* host = "script.google.com";
const int httpsPort = 443;
WiFiClientSecure secureClient;
String GAS_ID = "AKfycbx3HYNmzxX-BKk7gnOb_1E8il5HN7PdHBEQ8rJRoMX1NUmnGXDIcmJSroDIp5a7PEfC4A";

// DHT sensor configuration
#define DHTPIN 2
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Pin assignments
#define ON_Board_LED 16
#define BUZZER_PIN 14

// Temperature thresholds
#define HIGH_TEMP_THRESHOLD 32.0
#define LOW_TEMP_THRESHOLD 25.0

WiFiClient client;
unsigned long previousMillis = 0;
const long interval = 2500;

void setup() {
  Serial.begin(115200);
  delay(1000);

  dht.begin();
  delay(500);

  pinMode(ON_Board_LED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(ON_Board_LED, HIGH);
  digitalWrite(BUZZER_PIN, LOW);

  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    digitalWrite(ON_Board_LED, LOW);
    delay(250);
    digitalWrite(ON_Board_LED, HIGH);
    delay(250);
  }
  digitalWrite(ON_Board_LED, HIGH);
  Serial.println("\nSuccessfully connected to:");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  secureClient.setInsecure();
}

void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    sendSensor();
  }
}

void sendSensor() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  Serial.print("Temperature : ");
  Serial.print(t);
  Serial.print("    Humidity : ");
  Serial.println(h);

  if (t >= HIGH_TEMP_THRESHOLD || t <= LOW_TEMP_THRESHOLD) {
    digitalWrite(BUZZER_PIN, HIGH);
  } else {
    digitalWrite(BUZZER_PIN, LOW);
  }

  sendDataToThingSpeak(t, h);
  delay(1000);
  sendDataToGoogleSheets(t, h);
}

void sendDataToThingSpeak(float t, float h) {
  if (client.connect(server, 80)) {
    String postStr = apiKey;
    postStr += "&field1=" + String(t);
    postStr += "&field2=" + String(h);
    postStr += "\r\n\r\n";

    client.print("POST /update HTTP/1.1\n");
    client.print("Host: api.thingspeak.com\n");
    client.print("Connection: close\n");
    client.print("X-THINGSPEAKAPIKEY: " + apiKey + "\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(postStr.length());
    client.print("\n\n");
    client.print(postStr);

    Serial.print("Temperature: ");
    Serial.print(t);
    Serial.print(" degrees Celsius, Humidity: ");
    Serial.print(h);
    Serial.println("%. Sent to ThingSpeak.");
  }
  client.stop();
  Serial.println("Waiting...");
}

void sendDataToGoogleSheets(float t, float h) {
  Serial.println("==========");
  Serial.print("connecting to ");
  Serial.println(host);

  if (!secureClient.connect(host, httpsPort)) {
    Serial.println("connection failed");
    return;
  }

  String string_temperature = String(t, 2);
  String string_humidity = String(h, DEC);
  String url = "/macros/s/" + GAS_ID + "/exec?temperature=" + string_temperature + "&humidity=" + string_humidity;
  Serial.print("requesting URL: ");
  Serial.println(url);

  secureClient.print(String("GET ") + url + " HTTP/1.1\r\n" + "Host: " + host + "\r\n" + "User-Agent: BuildFailureDetectorESP8266\r\n" + "Connection: close\r\n\r\n");

  Serial.println("request sent");

  while (secureClient.connected()) {
    String line = secureClient.readStringUntil('\n');
    if (line == "\r") {
      Serial.println("headers received");
      break;
    }
  }

  String line = secureClient.readStringUntil('\n');
  if (line.startsWith("{\"state\":\"success\"")) {
    Serial.println("Data logged to Google Sheets successfully!");
  } else {
    Serial.println("Data logging to Google Sheets failed.");
    Serial.print("Reply was: ");
    Serial.println(line);
  }

  Serial.println("closing connection");
  Serial.println("==========");
}
