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
String phoneNumber = "+213664184457"; // Replace with your phone number

// SMS timing control
unsigned long lastSMSTime = 0;
const unsigned long smsInterval = 60000; // 1 minute in milliseconds

// Alert thresholds
const float TEMP_HIGH_THRESHOLD = 35.0;
const float TEMP_LOW_THRESHOLD = 10.0;
const float HUMIDITY_HIGH_THRESHOLD = 80.0;
const float HUMIDITY_LOW_THRESHOLD = 30.0;
const int SOIL_HIGH_THRESHOLD = 80;
const int SOIL_LOW_THRESHOLD = 30;

// Alert flags and timers
bool tempAlertSent = false;
bool humidityAlertSent = false;
bool soilAlertSent = false;
unsigned long lastAlertTime = 0;
const unsigned long ALERT_COOLDOWN = 300000; // 5 minutes cooldown between alerts

// Function prototypes
void handleRoot();
void handleData();
void handleSendSMS();
void handleUpdate();
String getTime();
void initGSM();
void sendStatusSMS();
void sendSMS(String number, String message);
void checkSensorAlerts();
void sendSensorAlert(String sensorName, float value, String unit, String condition);

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
  server.on("/data", handleData);
  server.on("/sendsms", handleSendSMS);
  server.on("/update", handleUpdate);
  
  // Start server
  server.begin();
  Serial.println("HTTP server started");
  
  // Initialize GSM module
  initGSM();
}

void loop() {
  server.handleClient();
  
  // Check if it's time to send SMS (every minute)
  unsigned long currentTime = millis();
  if (currentTime - lastSMSTime >= smsInterval) {
    sendStatusSMS();
    lastSMSTime = currentTime;
  }
  
  // Check for sensor alerts
  checkSensorAlerts();
}

void checkSensorAlerts() {
  unsigned long currentTime = millis();
  
  // Check if we're in cooldown period
  if (currentTime - lastAlertTime < ALERT_COOLDOWN) {
    return;
  }
  
  // Check temperature alerts
  if (temperature > TEMP_HIGH_THRESHOLD && !tempAlertSent) {
    sendSensorAlert("Temperature", temperature, "°C", "high");
    tempAlertSent = true;
    lastAlertTime = currentTime;
  } 
  else if (temperature < TEMP_LOW_THRESHOLD && !tempAlertSent) {
    sendSensorAlert("Temperature", temperature, "°C", "low");
    tempAlertSent = true;
    lastAlertTime = currentTime;
  }
  
  // Check humidity alerts
  if (humidity > HUMIDITY_HIGH_THRESHOLD && !humidityAlertSent) {
    sendSensorAlert("Humidity", humidity, "%", "high");
    humidityAlertSent = true;
    lastAlertTime = currentTime;
  } 
  else if (humidity < HUMIDITY_LOW_THRESHOLD && !humidityAlertSent) {
    sendSensorAlert("Humidity", humidity, "%", "low");
    humidityAlertSent = true;
    lastAlertTime = currentTime;
  }
  
  // Check soil moisture alerts
  if (soilMoisture > SOIL_HIGH_THRESHOLD && !soilAlertSent) {
    sendSensorAlert("Soil Moisture", soilMoisture, "%", "high");
    soilAlertSent = true;
    lastAlertTime = currentTime;
  } 
  else if (soilMoisture < SOIL_LOW_THRESHOLD && !soilAlertSent) {
    sendSensorAlert("Soil Moisture", soilMoisture, "%", "low");
    soilAlertSent = true;
    lastAlertTime = currentTime;
  }
  
  // Reset alert flags when values return to normal
  if (temperature > TEMP_LOW_THRESHOLD && temperature < TEMP_HIGH_THRESHOLD) {
    tempAlertSent = false;
  }
  if (humidity > HUMIDITY_LOW_THRESHOLD && humidity < HUMIDITY_HIGH_THRESHOLD) {
    humidityAlertSent = false;
  }
  if (soilMoisture > SOIL_LOW_THRESHOLD && soilMoisture < SOIL_HIGH_THRESHOLD) {
    soilAlertSent = false;
  }
}

void sendSensorAlert(String sensorName, float value, String unit, String condition) {
  String message = "ALERT: " + sensorName + " is too " + condition + "!\n";
  message += "Current value: " + String(value) + unit + "\n";
  message += "Threshold: ";
  
  if (condition == "high") {
    if (sensorName == "Temperature") message += String(TEMP_HIGH_THRESHOLD) + unit;
    else if (sensorName == "Humidity") message += String(HUMIDITY_HIGH_THRESHOLD) + unit;
    else message += String(SOIL_HIGH_THRESHOLD) + unit;
  } else {
    if (sensorName == "Temperature") message += String(TEMP_LOW_THRESHOLD) + unit;
    else if (sensorName == "Humidity") message += String(HUMIDITY_LOW_THRESHOLD) + unit;
    else message += String(SOIL_LOW_THRESHOLD) + unit;
  }
  
  message += "\nCheck your farm immediately!";
  
  sendSMS(phoneNumber, message);
  Serial.println("Sent alert: " + message);
}

void handleRoot() {
  String html = R"rawliteral(
  <!DOCTYPE html>
  <html lang="en">
  <head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Smart Farm Dashboard</title>
    <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.4.0/css/all.min.css">
    <style>
      * {
        box-sizing: border-box;
        margin: 0;
        padding: 0;
        font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
      }
      
      body {
        background: linear-gradient(135deg, #1a2a6c, #2d939b);
        color: #fff;
        min-height: 100vh;
        padding: 20px;
      }
      
      .container {
        max-width: 1200px;
        margin: 0 auto;
      }
      
      header {
        text-align: center;
        padding: 20px 0;
        margin-bottom: 30px;
      }
      
      h1 {
        font-size: 2.8rem;
        margin-bottom: 10px;
        text-shadow: 0 2px 4px rgba(0,0,0,0.3);
      }
      
      .subtitle {
        font-size: 1.2rem;
        opacity: 0.9;
      }
      
      .dashboard {
        display: grid;
        grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
        gap: 25px;
        margin-bottom: 40px;
      }
      
      .card {
        background: rgba(255, 255, 255, 0.15);
        backdrop-filter: blur(10px);
        border-radius: 20px;
        padding: 25px;
        box-shadow: 0 8px 32px rgba(0, 0, 0, 0.2);
        border: 1px solid rgba(255, 255, 255, 0.18);
        transition: transform 0.3s ease;
      }
      
      .card:hover {
        transform: translateY(-10px);
      }
      
      .card-header {
        display: flex;
        align-items: center;
        margin-bottom: 20px;
      }
      
      .card-icon {
        width: 60px;
        height: 60px;
        border-radius: 50%;
        background: rgba(255, 255, 255, 0.2);
        display: flex;
        align-items: center;
        justify-content: center;
        margin-right: 15px;
        font-size: 28px;
      }
      
      .card-title {
        font-size: 1.4rem;
        font-weight: 600;
      }
      
      .gauge-container {
        position: relative;
        width: 100%;
        height: 180px;
        margin: 0 auto;
      }
      
      .gauge {
        width: 100%;
        height: 100%;
      }
      
      .value {
        position: absolute;
        top: 50%;
        left: 50%;
        transform: translate(-50%, -50%);
        font-size: 2.5rem;
        font-weight: bold;
      }
      
      .unit {
        font-size: 1.2rem;
        margin-left: 4px;
      }
      
      .status-container {
        display: flex;
        justify-content: space-between;
        margin-top: 15px;
      }
      
      .status-item {
        text-align: center;
        padding: 10px;
        border-radius: 10px;
        background: rgba(255, 255, 255, 0.1);
        flex: 1;
        margin: 0 5px;
      }
      
      .alert-status {
        color: #ff5555;
        font-weight: bold;
      }
      
      .normal-status {
        color: #55ff55;
      }
      
      .controls {
        display: flex;
        justify-content: center;
        gap: 20px;
        margin-top: 30px;
      }
      
      .btn {
        padding: 15px 30px;
        border: none;
        border-radius: 50px;
        background: linear-gradient(to right, #00b09b, #96c93d);
        color: white;
        font-size: 1.1rem;
        font-weight: 600;
        cursor: pointer;
        box-shadow: 0 4px 15px rgba(0, 0, 0, 0.2);
        transition: all 0.3s ease;
        display: flex;
        align-items: center;
        gap: 10px;
      }
      
      .btn:hover {
        transform: scale(1.05);
        box-shadow: 0 6px 20px rgba(0, 0, 0, 0.3);
      }
      
      .btn-sms {
        background: linear-gradient(to right, #3494E6, #EC6EAD);
      }
      
      .btn-alert {
        background: linear-gradient(to right, #ff5555, #ff0000);
      }
      
      .timestamp {
        text-align: center;
        margin-top: 30px;
        font-size: 1.1rem;
        opacity: 0.8;
      }
      
      .alert-notification {
        position: fixed;
        top: 20px;
        left: 50%;
        transform: translateX(-50%);
        padding: 15px 30px;
        background: #e74c3c;
        color: white;
        border-radius: 50px;
        box-shadow: 0 5px 15px rgba(0, 0, 0, 0.3);
        z-index: 100;
        opacity: 0;
        transition: opacity 0.3s ease;
      }
      
      .alert-notification.show {
        opacity: 1;
      }
      
      @media (max-width: 768px) {
        .dashboard {
          grid-template-columns: 1fr;
        }
        
        .controls {
          flex-direction: column;
          align-items: center;
        }
      }
    </style>
  </head>
  <body>
    <div class="container">
      <header>
        <h1><i class="fas fa-seedling"></i> Smart Farm Dashboard</h1>
        <div class="subtitle">Monitoring System</div>
      </header>
      
      <div class="dashboard">
        <div class="card">
          <div class="card-header">
            <div class="card-icon">
              <i class="fas fa-thermometer-half"></i>
            </div>
            <h2 class="card-title">Temperature</h2>
          </div>
          <div class="gauge-container">
            <div id="temp-gauge" class="gauge"></div>
            <div class="value"><span id="temp-value">0.0</span><span class="unit">°C</span></div>
          </div>
          <div class="status-container">
            <div class="status-item">Low: 10°C</div>
            <div class="status-item">Optimal: 25°C</div>
            <div class="status-item">High: 35°C</div>
          </div>
          <div id="temp-alert" class="alert-status" style="display: none;">ALERT: Temperature out of range!</div>
        </div>
        
        <div class="card">
          <div class="card-header">
            <div class="card-icon">
              <i class="fas fa-tint"></i>
            </div>
            <h2 class="card-title">Humidity</h2>
          </div>
          <div class="gauge-container">
            <div id="humidity-gauge" class="gauge"></div>
            <div class="value"><span id="humidity-value">0.0</span><span class="unit">%</span></div>
          </div>
          <div class="status-container">
            <div class="status-item">Dry: 30%</div>
            <div class="status-item">Optimal: 60%</div>
            <div class="status-item">Wet: 80%</div>
          </div>
          <div id="humidity-alert" class="alert-status" style="display: none;">ALERT: Humidity out of range!</div>
        </div>
        
        <div class="card">
          <div class="card-header">
            <div class="card-icon">
              <i class="fas fa-leaf"></i>
            </div>
            <h2 class="card-title">Soil Moisture</h2>
          </div>
          <div class="gauge-container">
            <div id="soil-gauge" class="gauge"></div>
            <div class="value"><span id="soil-value">0</span><span class="unit">%</span></div>
          </div>
          <div class="status-container">
            <div class="status-item">Dry: 30%</div>
            <div class="status-item">Optimal: 50%</div>
            <div class="status-item">Wet: 80%</div>
          </div>
          <div id="soil-alert" class="alert-status" style="display: none;">ALERT: Soil moisture out of range!</div>
        </div>
      </div>
      
      <div class="controls">
        <button class="btn" onclick="refreshData()">
          <i class="fas fa-sync-alt"></i> Refresh Data
        </button>
        <button class="btn btn-sms" onclick="sendSMS()">
          <i class="fas fa-mobile-alt"></i> Send Status SMS
        </button>
        <button class="btn btn-alert" onclick="testAlert()">
          <i class="fas fa-bell"></i> Test Alert
        </button>
      </div>
      
      <div class="timestamp">
        Last updated: <span id="update-time">Loading...</span>
      </div>
    </div>
    
    <div id="alert" class="alert-notification">SMS Sent Successfully!</div>
    
    <script>
      // Initialize gauges
      function createGauge(elementId, value, max, colors) {
        const container = document.getElementById(elementId);
        container.innerHTML = '';
        
        const width = container.clientWidth;
        const height = container.clientHeight;
        const radius = Math.min(width, height) * 0.4;
        const cx = width / 2;
        const cy = height / 2;
        
        const svg = document.createElementNS('http://www.w3.org/2000/svg', 'svg');
        svg.setAttribute('width', width);
        svg.setAttribute('height', height);
        
        // Background circle
        const bgCircle = document.createElementNS('http://www.w3.org/2000/svg', 'circle');
        bgCircle.setAttribute('cx', cx);
        bgCircle.setAttribute('cy', cy);
        bgCircle.setAttribute('r', radius);
        bgCircle.setAttribute('fill', 'none');
        bgCircle.setAttribute('stroke', 'rgba(255, 255, 255, 0.1)');
        bgCircle.setAttribute('stroke-width', '12');
        svg.appendChild(bgCircle);
        
        // Value arc
        const valueArc = document.createElementNS('http://www.w3.org/2000/svg', 'circle');
        valueArc.setAttribute('cx', cx);
        valueArc.setAttribute('cy', cy);
        valueArc.setAttribute('r', radius);
        valueArc.setAttribute('fill', 'none');
        valueArc.setAttribute('stroke', getGradientColor(value/max, colors));
        valueArc.setAttribute('stroke-width', '12');
        valueArc.setAttribute('stroke-linecap', 'round');
        valueArc.setAttribute('stroke-dasharray', 2 * Math.PI * radius);
        valueArc.setAttribute('stroke-dashoffset', 2 * Math.PI * radius * (1 - value/max));
        valueArc.setAttribute('transform', `rotate(-90, ${cx}, ${cy})`);
        svg.appendChild(valueArc);
        
        container.appendChild(svg);
      }
      
      function getGradientColor(ratio, colors) {
        const colorStops = colors.length - 1;
        const stopIndex = Math.min(Math.floor(ratio * colorStops), colorStops - 1);
        const stopRatio = (ratio - (stopIndex / colorStops)) * colorStops;
        
        const r = Math.round(
          colors[stopIndex].r + (colors[stopIndex+1].r - colors[stopIndex].r) * stopRatio
        );
        const g = Math.round(
          colors[stopIndex].g + (colors[stopIndex+1].g - colors[stopIndex].g) * stopRatio
        );
        const b = Math.round(
          colors[stopIndex].b + (colors[stopIndex+1].b - colors[stopIndex].b) * stopRatio
        );
        
        return `rgb(${r}, ${g}, ${b})`;
      }
      
      // Update all gauges
      function updateGauges(temp, hum, soil) {
        // Temperature gauge (blue to red)
        createGauge('temp-gauge', temp, 50, [
          {r: 65, g: 105, b: 225},   // Royal Blue
          {r: 46, g: 204, b: 113},   // Emerald Green
          {r: 241, g: 196, b: 15},   // Sunflower
          {r: 231, g: 76, b: 60}     // Alizarin
        ]);
        
        // Humidity gauge (blue gradient)
        createGauge('humidity-gauge', hum, 100, [
          {r: 236, g: 240, b: 241},  // Clouds
          {r: 52, g: 152, b: 219},   // Peter River
          {r: 41, g: 128, b: 185}    // Belize Hole
        ]);
        
        // Soil moisture gauge (brown gradient)
        createGauge('soil-gauge', soil, 100, [
          {r: 210, g: 180, b: 140},  // Tan
          {r: 139, g: 69, b: 19},    // Saddle Brown
          {r: 85, g: 107, b: 47}     // Dark Olive Green
        ]);
        
        // Update values
        document.getElementById('temp-value').textContent = temp.toFixed(1);
        document.getElementById('humidity-value').textContent = hum.toFixed(1);
        document.getElementById('soil-value').textContent = soil;
        
        // Update alert indicators
        updateAlertIndicator('temp-alert', temp < 10 || temp > 35);
        updateAlertIndicator('humidity-alert', hum < 30 || hum > 80);
        updateAlertIndicator('soil-alert', soil < 30 || soil > 80);
      }
      
      function updateAlertIndicator(elementId, isAlert) {
        const element = document.getElementById(elementId);
        if (isAlert) {
          element.style.display = 'block';
        } else {
          element.style.display = 'none';
        }
      }
      
      // Fetch data from server
      function fetchData() {
        fetch('/data')
          .then(response => response.json())
          .then(data => {
            updateGauges(data.temperature, data.humidity, data.soil);
            document.getElementById('update-time').textContent = new Date().toLocaleTimeString();
          })
          .catch(error => console.error('Error fetching data:', error));
      }
      
      // Send SMS command
      function sendSMS() {
        fetch('/sendsms')
          .then(response => response.text())
          .then(result => {
            showAlert('SMS Sent Successfully!');
          })
          .catch(error => {
            showAlert('Error sending SMS');
            console.error('SMS Error:', error);
          });
      }
      
      // Test alert function
      function testAlert() {
        fetch('/sendsms', {
          method: 'POST',
          headers: {
            'Content-Type': 'application/json',
          },
          body: JSON.stringify({
            testAlert: true
          })
        })
        .then(response => response.text())
        .then(result => {
          showAlert('Test Alert Sent!');
        })
        .catch(error => {
          showAlert('Error sending test alert');
          console.error('Test Alert Error:', error);
        });
      }
      
      // Show alert notification
      function showAlert(message) {
        const alert = document.getElementById('alert');
        alert.textContent = message;
        alert.classList.add('show');
        
        setTimeout(() => {
          alert.classList.remove('show');
        }, 3000);
      }
      
      // Refresh data manually
      function refreshData() {
        fetchData();
        showAlert('Data Refreshed');
      }
      
      // Initialize on load
      window.onload = function() {
        // Initial data fetch
        fetchData();
        
        // Update data every 5 seconds
        setInterval(fetchData, 5000);
        
        // Initialize with sample data
        updateGauges(0, 0, 0);
      };
    </script>
  </body>
  </html>
  )rawliteral";
  
  server.send(200, "text/html", html);
}

void handleData() {
  String json = "{";
  json += "\"temperature\":" + String(temperature) + ",";
  json += "\"humidity\":" + String(humidity) + ",";
  json += "\"soil\":" + String(soilMoisture);
  json += "}";
  
  server.send(200, "application/json", json);
}

void handleSendSMS() {
  if (server.hasArg("plain")) {
    String body = server.arg("plain");
    if (body.indexOf("testAlert") != -1) {
      sendSMS(phoneNumber, "TEST ALERT: This is a test alert from your Smart Farm Monitoring System.");
      server.send(200, "text/plain", "Test alert sent");
      return;
    }
  }
  sendStatusSMS();
  server.send(200, "text/plain", "SMS triggered");
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
    
    server.send(200, "text/plain", "Data updated");
  } else {
    server.send(400, "text/plain", "Missing parameters");
  }
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

void initGSM() {
  // Power up the module
  sim900.println("AT");
  delay(1000);
  
  // Set SMS mode to text
  sim900.println("AT+CMGF=1");
  delay(1000);
  
  Serial.println("GSM module ready");
}

void sendStatusSMS() {
  String message = "Current Sensor Readings:\n";
  message += "Temperature: " + String(temperature) + "°C\n";
  message += "Humidity: " + String(humidity) + "%\n";
  message += "Soil Moisture: " + String(soilMoisture) + "%";
  
  // Add alert status
  message += "\n\nAlert Status:";
  if (temperature > TEMP_HIGH_THRESHOLD) message += "\n- Temperature HIGH!";
  else if (temperature < TEMP_LOW_THRESHOLD) message += "\n- Temperature LOW!";
  
  if (humidity > HUMIDITY_HIGH_THRESHOLD) message += "\n- Humidity HIGH!";
  else if (humidity < HUMIDITY_LOW_THRESHOLD) message += "\n- Humidity LOW!";
  
  if (soilMoisture > SOIL_HIGH_THRESHOLD) message += "\n- Soil Moisture HIGH!";
  else if (soilMoisture < SOIL_LOW_THRESHOLD) message += "\n- Soil Moisture LOW!";
  
  if (message.indexOf("HIGH!") == -1 && message.indexOf("LOW!") == -1) {
    message += "\n- All values normal";
  }
  
  message += "\nSITE: " + String("http://") + WiFi.softAPIP().toString() + "/";
  
  sendSMS(phoneNumber, message);
}

void sendSMS(String number, String message) {
  Serial.println("Sending SMS...");
  
  // Set SMS mode to text (redundant but ensures state)
  sim900.println("AT+CMGF=1");
  delay(500);
  
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