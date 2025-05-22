// =========================
// ESP32 Weather Station Code (SoftAP Version)
// =========================

#include <WiFi.h>                      // Library for WiFi functions
#include <Wire.h>                      // Library for I2C communication
#include <Adafruit_GFX.h>             // Graphics library for OLED
#include <Adafruit_SH110X.h>          // OLED driver library
#include <DHT.h>                       // DHT sensor library
#include <Adafruit_BMP280.h>          // BMP280 sensor library
#include <math.h>                      // Math functions
#include <WebServer.h>                 // Web server for ESP32
#include <DNSServer.h>      
// === Access Point Credentials ===
const char* ap_ssid = "OLE Weather";       // SSID for ESP32 SoftAP
const char* ap_password = "olenepal123";   // Password for ESP32 SoftAP

// === Custom I2C Pins ===
#define SDA_PIN 16                         // SDA pin for I2C communication
#define SCL_PIN 17                         // SCL pin for I2C communication

// === OLED Display Setup ===
Adafruit_SH1106G display = Adafruit_SH1106G(128, 64, &Wire);  // OLED display object

// === DHT22 Sensor Setup ===
#define DHTPIN 15                          // GPIO pin connected to DHT22
#define DHTTYPE DHT22                      // Type of DHT sensor
DHT dht(DHTPIN, DHTTYPE);                 // DHT sensor object

// === MQ135 Gas Sensor Setup ===
#define MQ135_PIN 34                       // Analog pin connected to MQ135
float Ro = 10.0;                           // Initial assumed Ro value for MQ135 calibration

// === BMP280 Sensor Setup ===
Adafruit_BMP280 bmp;                      // BMP280 sensor object

// === Web Server Instance ===
WebServer server(80);                     // Create web server on port 80

const byte DNS_PORT = 53;                 // DNS port for captive portal
DNSServer dnsServer;                      // DNS server instance
IPAddress apIP(192, 168, 4, 1);           // IP for SoftAP

// === OLED Toggle Display Settings ===
unsigned long lastToggle = 0;             // Last time screen toggled
const unsigned long toggleInterval = 10000; // Interval for screen toggle (10 seconds)
int screenNumber = 0;                     // Current screen index (0 or 1)

// === Function Prototypes ===
float calibrateMQ135();                   // Function to calibrate MQ135
float getResistanceMQ135();              // Function to calculate Rs of MQ135
int calculateAQI(float co2_ppm);         // Function to compute AQI based on CO2
String generateHTML();                   // Function to generate webpage content

int getConnectedDeviceCount() {
  return WiFi.softAPgetStationNum();  // Returns the number of connected clients
}

void setup() {
  Serial.begin(115200);                  // Start serial communication
  delay(1000);                           // Delay for serial stabilization

  Wire.begin(SDA_PIN, SCL_PIN);          // Start I2C with custom SDA, SCL

  if (!display.begin(0x3C, true)) {      // Initialize OLED display
    Serial.println("OLED not found");
    while (1);                           // Halt if OLED not found
  }

  // === Welcome Screen on OLED ===
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(10, 20);
  display.println("Weather");
  display.setCursor(20, 45);
  display.println("Station");
  display.display();
  delay(2000);

  dht.begin();                           // Initialize DHT sensor

  if (!bmp.begin(0x76)) {                // Initialize BMP280 sensor
    Serial.println("BMP280 not found");
    while (1);                           // Halt if BMP not found
  }

  // === MQ135 Calibration Display ===
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 25);
  display.println("Calibrating MQ135...");
  display.display();
  delay(3000);
  Ro = calibrateMQ135();                 // Calibrate MQ135 sensor
  Serial.print("Calibrated Ro: ");
  Serial.println(Ro);

  // === Start Access Point ===
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0)); // Configure AP IP settings
  WiFi.softAP(ap_ssid);                                      // Start AP with SSID
  IPAddress myIP = WiFi.softAPIP();                          // Get IP address
  Serial.println("Access Point Started");
  Serial.print("IP address: ");
  Serial.println(myIP);
  dnsServer.start(DNS_PORT, "*", apIP);                      // Start DNS server to redirect all requests

  // === Setup Web Server Routes ===
  server.on("/", []() {
    server.send(200, "text/html", generateHTML());           // Serve HTML page on root request
  });

  server.onNotFound([]() {
    server.sendHeader("Location", String("http://") + WiFi.softAPIP().toString(), true);
    server.send(302, "text/plain", "");                      // Redirect unknown paths to root
  });

  server.begin();                                            // Start web server
}

void loop() {
  dnsServer.processNextRequest();       // Handle DNS requests
  server.handleClient();                // Handle web client requests

  // === Read Sensor Data ===
  float temperature = dht.readTemperature();  // Read temperature from DHT
  float humidity = dht.readHumidity();        // Read humidity from DHT
  float rs = getResistanceMQ135();            // Get resistance from MQ135
  float ratio = rs / Ro;                      // Compute Rs/Ro ratio

  // Convert ratio to gas concentrations using approximation formulas
  float co2_ppm = pow(10, (-0.42 * log10(ratio) + 1.92));
  float nh3_ppm = pow(10, (-1.5 * log10(ratio) + 2.3));
  float alcohol_ppm = pow(10, (-1.552 * log10(ratio) + 2.041));

  float pressure = bmp.readPressure() / 100.0F;      // Read pressure in hPa
  float altitude = 44330 * (1.0 - pow(pressure / 1013.25, 0.1903)); // Calculate altitude
  float bmpTemp = bmp.readTemperature();             // BMP280 temperature
  int aqi = calculateAQI(co2_ppm);                   // Calculate AQI

  // === OLED Toggle Display Logic ===
  if (millis() - lastToggle > toggleInterval) {
    screenNumber = 1 - screenNumber;                // Toggle screen
    lastToggle = millis();                          // Update last toggle time
  }

  // === Display Sensor Data on OLED ===
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  display.setTextSize(1);

  if (screenNumber == 0) {
    // Display DHT and MQ135 values
    display.setCursor(0, 0);
    display.print("Temp(DHT): ");
    isnan(temperature) ? display.println("Error") : display.println(String(temperature, 1) + "C");

    display.setCursor(0, 12);
    display.print("Humidity: ");
    isnan(humidity) ? display.println("Error") : display.println(String(humidity, 1) + "%");

    display.setCursor(0, 24);
    display.print("CO2: ");
    display.println(String(co2_ppm, 0) + " ppm");

    display.setCursor(0, 36);
    display.print("NH3: ");
    display.println(String(nh3_ppm, 0) + " ppm");

    display.setCursor(0, 48);
    display.print("Alcohol: ");
    display.println(String(alcohol_ppm, 0) + " ppm");
  } else {
    // Display BMP280 and AQI values
    display.setCursor(0, 0);
    display.print("Pressure: ");
    display.println(String(pressure, 1) + " KPa");

    display.setCursor(0, 12);
    display.print("Altitude: ");
    display.println(String(altitude, 1) + "m");

    display.setCursor(0, 24);
    display.print("Temp(BMP): ");
    display.println(String(bmpTemp, 1) + "C");

    display.setCursor(0, 36);
    display.print("AQI: ");
    display.print(aqi);

    int connectedDevices = getConnectedDeviceCount();

    display.setCursor(0, 48);
    display.print("Devices: ");
    display.println(connectedDevices);

  }

  display.display();                    // Show on OLED
  delay(100);                           // Short delay
}

// === Function to Calibrate MQ135 Sensor ===
float calibrateMQ135() {
  int val = 0;
  for (int i = 0; i < 100; i++) {
    val += analogRead(MQ135_PIN);      // Read analog value
    delay(10);
  }
  val /= 100;                           // Average value
  float voltage = val * (3.3 / 4095.0); // Convert ADC to voltage
  float Rs = ((3.3 - voltage) / voltage) * 10.0; // Compute Rs
  float Ro = Rs / 3.6;                  // Estimate Ro assuming clean air ratio ~3.6
  return Ro;
}

// === Function to Get Current Resistance of MQ135 ===
float getResistanceMQ135() {
  int val = analogRead(MQ135_PIN);     // Read analog value
  float voltage = val * (3.3 / 4095.0); // Convert to voltage
  float Rs = ((3.3 - voltage) / voltage) * 10.0; // Compute Rs
  return Rs;
}

// === Function to Compute Air Quality Index Based on CO2 ppm ===
int calculateAQI(float co2) {
  if (co2 <= 350) return 0;
  else if (co2 <= 600) return 50;
  else if (co2 <= 1000) return 100;
  else if (co2 <= 2000) return 150;
  else if (co2 <= 5000) return 200;
  else return 300;
}

// === Function to Generate HTML Page for Web Server ===
String generateHTML() {
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  float rs = getResistanceMQ135();
  float ratio = rs / Ro;

  float co2_ppm = pow(10, (-0.42 * log10(ratio) + 1.92));
  float nh3_ppm = pow(10, (-1.5 * log10(ratio) + 2.3));
  float alcohol_ppm = pow(10, (-1.552 * log10(ratio) + 2.041));

  float pressure = bmp.readPressure() / 100.0F;
  float altitude = 44330 * (1.0 - pow(pressure / 1013.25, 0.1903));
  float bmpTemp = bmp.readTemperature();
  int aqi = calculateAQI(co2_ppm);

  // Determine AQI label
  String aqiLabel = (aqi == 0) ? "Good" :
                    (aqi == 50) ? "Moderate" :
                    (aqi == 100) ? "Unhealthy (SG)" :
                    (aqi == 150) ? "Unhealthy" :
                    (aqi == 200) ? "Very Unhealthy" : "Hazardous";

  // === Create HTML Content ===
  String html = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<meta http-equiv='refresh' content='5' />";
  html += "<style>body{font-family:Arial; text-align:center;} h2{color:#0A75AD;} .data{font-size:1.5em;}</style>";
  html += "<title>ESP32 Weather Station</title></head><body>";
  html += "<h2>ESP32 Weather Station</h2>";
  html += "<div class='data'>Temp (DHT): " + String(temperature, 1) + " °C<br>";
  html += "Humidity: " + String(humidity, 1) + " %<br>";
  html += "Pressure: " + String(pressure, 1) + " hPa<br>";
  html += "Altitude: " + String(altitude, 1) + " m<br>";
  html += "Temp (BMP): " + String(bmpTemp, 1) + " °C<br><br>";
  html += "CO2: " + String(co2_ppm, 0) + " ppm<br>";
  html += "NH3: " + String(nh3_ppm, 0) + " ppm<br>";
  html += "Alcohol: " + String(alcohol_ppm, 0) + " ppm<br><br>";
  html += "<strong>AQI: " + String(aqi) + " (" + aqiLabel + ")</strong></div>";
  int connectedDevices = getConnectedDeviceCount();
html += "<p><b>Connected Devices:</b> " + String(connectedDevices) + "</p>";

  html += "</body></html>";

  return html;
}
