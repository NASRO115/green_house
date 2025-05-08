#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <SoftwareSerial.h>

// Set AP credentials
const char* ssid = "ESP8266_AP";
const char* password = "password123";

// Web server on port 80
ESP8266WebServer server(80);

// GSM module setup
SoftwareSerial sim900(D1, D2); // RX, TX for ESP8266 (D1=GPIO5, D2=GPIO4)

// Variables to store received sensor data
float temperature = 0.0;
float humidity = 0.0;
int soilMoisture = 0;

// Target phone number for SMS alerts
String phoneNumber = "+213675743197"; // Replace with your phone number

// Last time SMS was sent (to avoid too many messages)
unsigned long lastSMSTime = 0;
const unsigned long smsInterval = 3600000; // 1 hour in milliseconds

void setup() {
  Serial.begin(115200);
  
  // Initialize GSM module
  sim900.begin(9600);
  delay(1000);
  Serial.println("GSM module initialized");
  
  // Initialize access point
  WiFi.softAP(ssid, password);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  
  // Define server routes
  server.on("/", handleRoot);
  server.on("/update", handleUpdate);
  
  // Start server
  server.begin();
  Serial.println("HTTP server started");
  
  // Initialize GSM module
  initGSM();
}

void loop() {
  server.handleClient();
}

void handleRoot() {
  String html = "<!DOCTYPE html>";
  html += "<html lang='en'>";
  html += "<head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>Sensor Data Dashboard</title>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; margin: 0; padding: 20px; background-color: #f2f2f2; }";
  html += ".container { max-width: 800px; margin: 0 auto; background-color: white; padding: 20px; border-radius: 10px; box-shadow: 0 0 10px rgba(0,0,0,0.1); }";
  html += "h1 { color: #333; text-align: center; }";
  html += ".data-container { display: flex; flex-wrap: wrap; justify-content: space-around; margin-top: 20px; }";
  html += ".data-card { width: 30%; background-color: #e6f7ff; border-radius: 10px; padding: 15px; margin-bottom: 15px; text-align: center; box-shadow: 0 0 5px rgba(0,0,0,0.1); }";
  html += ".data-value { font-size: 24px; font-weight: bold; margin: 10px 0; }";
  html += ".data-label { color: #666; }";
  html += "@media (max-width: 600px) { .data-card { width: 100%; } }";
  html += "</style>";
  html += "</head>";
  html += "<body>";
  html += "<div class='container'>";
  html += "<h1>Sensor Data Dashboard</h1>";
  html += "<div class='data-container'>";
  
  html += "<div class='data-card'>";
  html += "<div class='data-label'>Temperature</div>";
  html += "<div class='data-value'>" + String(temperature) + " °C</div>";
  html += "</div>";
  
  html += "<div class='data-card'>";
  html += "<div class='data-label'>Humidity</div>";
  html += "<div class='data-value'>" + String(humidity) + " %</div>";
  html += "</div>";
  
  html += "<div class='data-card'>";
  html += "<div class='data-label'>Soil Moisture</div>";
  html += "<div class='data-value'>" + String(soilMoisture) + " %</div>";
  html += "</div>";
  
  html += "</div>"; // end data-container
  html += "<p style='text-align: center; margin-top: 20px;'>Last updated: <span id='timestamp'>" + getTime() + "</span></p>";
  html += "<script>";
  html += "function refreshPage() { location.reload(); }";
  html += "setTimeout(refreshPage, 10000);"; // Refresh every 10 seconds
  html += "</script>";
  html += "</div>"; // end container
  html += "</body>";
  html += "</html>";
  
  server.send(200, "text/html", html);
}

String getTime() {
  // This is a placeholder - ESP8266 doesn't have real-time clock
  // You could add an RTC module or use NTP for accurate time
  static unsigned long startTime = millis();
  unsigned long currentTime = millis() - startTime;
  
  unsigned long seconds = currentTime / 1000;
  unsigned long minutes = seconds / 60;
  unsigned long hours = minutes / 60;
  
  String timeString = String(hours % 24) + ":" + 
                      (minutes % 60 < 10 ? "0" : "") + String(minutes % 60) + ":" + 
                      (seconds % 60 < 10 ? "0" : "") + String(seconds % 60);
  
  return timeString;
}

void handleUpdate() {
  if (server.hasArg("temperature") && server.hasArg("humidity") && server.hasArg("soil")) {
    // Update global variables with new sensor data
    temperature = server.arg("temperature").toFloat();
    humidity = server.arg("humidity").toFloat();
    soilMoisture = server.arg("soil").toInt();
    
    // Log received data
    Serial.println("Updated sensor data:");
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.print("°C, Humidity: ");
    Serial.print(humidity);
    Serial.print("%, Soil Moisture: ");
    Serial.print(soilMoisture);
    Serial.println("%");
    
    // Check if we should send an SMS alert
    checkSendSMSAlert();
    
    server.send(200, "text/plain", "Data updated");
  } else {
    server.send(400, "text/plain", "Missing parameters");
  }
}

void initGSM() {
  // Power up the module
  sim900.println("AT");
  delay(1000);
  
  // Set SMS mode to text
  sim900.println("AT+CMGF=1");
  delay(1000);
  
  Serial.println("GSM module ready");
}

void checkSendSMSAlert() {
  // Check if conditions warrant sending an SMS
  // For example, if temperature is too high or soil moisture is too low
  bool shouldSendAlert = false;
  String alertMessage = "";
  
  if (temperature > 30) {
    shouldSendAlert = true;
    alertMessage += "High temperature alert: " + String(temperature) + "C. ";
  }
  
  if (soilMoisture < 20) {
    shouldSendAlert = true;
    alertMessage += "Low soil moisture alert: " + String(soilMoisture) + "%. ";
  }
  
  // Only send SMS if an alert condition exists and enough time has passed since last SMS
  unsigned long currentTime = millis();
  if (shouldSendAlert && (currentTime - lastSMSTime > smsInterval)) {
    sendSMS(phoneNumber, alertMessage);
    lastSMSTime = currentTime;
  }
}

void sendSMS(String number, String message) {
  Serial.println("Sending SMS...");
  
  // Set SMS center
  sim900.println("AT+CSCA=\"+213675743197\""); // Replace with your SMS center number
  delay(1000);
  
  // Set recipient phone number
  sim900.print("AT+CMGS=\"");
  sim900.print(number);
  sim900.println("\"");
  delay(1000);
  
  // Set message content
  sim900.print(message);
  delay(100);
  
  // Send message
  sim900.write(26); // Ctrl+Z character to send the message
  delay(1000);
  
  Serial.println("SMS sent");
}