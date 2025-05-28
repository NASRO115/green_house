#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>

// Define pins
#define DHT_PIN 33
#define SOIL_PIN 32
#define DHT_TYPE DHT11

// WiFi credentials for connecting to ESP8266 AP
const char* ssid = "ESP8266_AP";
const char* password = "password123";

// Server address (ESP8266 AP IP)
const char* serverAddress = "http://192.168.4.1/update";

// Initialize DHT sensor
DHT dht(DHT_PIN, DHT_TYPE);

// Variables to store sensor data
float temperature;
float humidity;
int soilMoisture;

void setup() {
  Serial.begin(115200);
  pinMode(SOIL_PIN, INPUT);
  
  // Initialize DHT sensor
  dht.begin();
  
  // Initialize WiFi
  WiFi.begin(ssid, password);
  
  Serial.println("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected!");
  Serial.print("Connected to: ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  // Read data from DHT11 sensor
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();
  
  // Read data from soil moisture sensor
  soilMoisture = analogRead(SOIL_PIN);
  // Convert to percentage (assuming 4095 is dry and 0 is wet for ESP32)
  soilMoisture = map(soilMoisture, 4095, 0, 0, 100);
  
  // Check if readings are valid
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
  } else {
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.print("°C, Humidity: ");
    Serial.print(humidity);
    Serial.print("%, Soil Moisture: ");
    Serial.print(soilMoisture);
    Serial.println("%");
    
    // Send data to ESP8266 server
    sendData();
  }
  
  // Wait before next reading
  delay(5000);
}

void sendData() {
  // Check WiFi connection status
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    
    // Create data string
    String httpRequestData = "temperature=" + String(temperature) + 
                            "&humidity=" + String(humidity) + 
                            "&soil=" + String(soilMoisture);
    
    // Send HTTP POST request
    http.begin(serverAddress);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    
    int httpResponseCode = http.POST(httpRequestData);
    
    if (httpResponseCode > 0) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
    } else {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }
    
    // Free resources
    http.end();
  } else {
    Serial.println("WiFi Disconnected, reconnecting...");
    WiFi.begin(ssid, password);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 10) {
      delay(500);
      Serial.print(".");
      attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("Reconnected to WiFi");
    }
  }
}