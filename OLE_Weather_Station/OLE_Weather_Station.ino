// =========================
// ESP32 Weather Station Code
// =========================

#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <DHT.h>
#include <Adafruit_BMP280.h>
#include <math.h>
#include <WebServer.h>

// === Wi-Fi Credentials ===
const char* ssid = "realme";
const char* password = "12345678";

// === Custom I2C Pins ===
#define SDA_PIN 16
#define SCL_PIN 17

// === OLED Display Setup ===
Adafruit_SH1106G display = Adafruit_SH1106G(128, 64, &Wire);

// === DHT22 Sensor Setup ===
#define DHTPIN 15
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// === MQ135 Gas Sensor Setup ===
#define MQ135_PIN 34
float Ro = 10.0;  // Initial assumed Ro

// === BMP280 Sensor Setup ===
Adafruit_BMP280 bmp;

// === Web Server Instance ===
WebServer server(80);

// === OLED Toggle Display Settings ===
unsigned long lastToggle = 0;
const unsigned long toggleInterval = 10000; // ms
int screenNumber = 0;

// === Function Prototypes ===
float calibrateMQ135();
float getResistanceMQ135();
int calculateAQI(float co2_ppm);
String generateHTML();

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Initialize I2C with custom pins
  Wire.begin(SDA_PIN, SCL_PIN);

  // Initialize OLED
  if (!display.begin(0x3C, true)) {
    Serial.println("OLED not found");
    while (1);
  }

  // Startup display
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(10, 20);
  display.println("Weather");
  display.setCursor(20, 45);
  display.println("Station");
  display.display();
  delay(2000);

  // Initialize DHT sensor
  dht.begin();

  // Initialize BMP280 sensor
  if (!bmp.begin(0x76)) {
    Serial.println("BMP280 not found");
    while (1);
  }

  // MQ135 Calibration
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 25);
  display.println("Calibrating MQ135...");
  display.display();
  delay(3000);
  Ro = calibrateMQ135();
  Serial.print("Calibrated Ro: ");
  Serial.println(Ro);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.println(WiFi.localIP());

  // Setup HTTP route
  server.on("/", []() {
    server.send(200, "text/html", generateHTML());
  });

  server.begin();
}

void loop() {
  server.handleClient();

  // Sensor Readings
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

  // Toggle display screen
  if (millis() - lastToggle > toggleInterval) {
    screenNumber = 1 - screenNumber;
    lastToggle = millis();
  }

  // === OLED Display ===
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  display.setTextSize(1);

  if (screenNumber == 0) {
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
  }

  display.display();
  delay(100);
}

// === Calibrate MQ135 ===
float calibrateMQ135() {
  int val = 0;
  for (int i = 0; i < 100; i++) {
    val += analogRead(MQ135_PIN);
    delay(10);
  }
  val /= 100;
  float voltage = val * (3.3 / 4095.0);
  float Rs = ((3.3 - voltage) / voltage) * 10.0;
  float Ro = Rs / 3.6;  // Clean air ratio
  return Ro;
}

// === Get Sensor Resistance (Rs) for MQ135 ===
float getResistanceMQ135() {
  int val = analogRead(MQ135_PIN);
  float voltage = val * (3.3 / 4095.0);
  float Rs = ((3.3 - voltage) / voltage) * 10.0;
  return Rs;
}

// === Calculate AQI from CO2 concentration ===
int calculateAQI(float co2) {
  if (co2 <= 350) return 0;
  else if (co2 <= 600) return 50;
  else if (co2 <= 1000) return 100;
  else if (co2 <= 2000) return 150;
  else if (co2 <= 5000) return 200;
  else return 300;
}

// === Generate HTML for Web Interface ===
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

  String aqiLabel = (aqi == 0) ? "Good" :
                    (aqi == 50) ? "Moderate" :
                    (aqi == 100) ? "Unhealthy (SG)" :
                    (aqi == 150) ? "Unhealthy" :
                    (aqi == 200) ? "Very Unhealthy" : "Hazardous";

  // === HTML Content ===
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
  html += "</body></html>";

  return html;
}
