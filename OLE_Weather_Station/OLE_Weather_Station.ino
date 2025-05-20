#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <DHT.h>
#include <Adafruit_BMP280.h>
#include <math.h>
#include <WebServer.h>

// === Wi-Fi Credentials ===
const char* ssid = "oleb5";
const char* password = "0lesecond";

// === Custom I2C Pins ===
#define SDA_PIN 16
#define SCL_PIN 17

// === OLED Setup ===
Adafruit_SH1106G display = Adafruit_SH1106G(128, 64, &Wire);

// === DHT22 Setup ===
#define DHTPIN 15
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// === MQ135 Setup ===
#define MQ135_PIN 34
float Ro = 10.0;

// === BMP280 Setup ===
Adafruit_BMP280 bmp;

// === Web Server ===
WebServer server(80);

// === Toggle Screen Variables ===
unsigned long lastToggle = 0;
const unsigned long toggleInterval = 10000;
int screenNumber = 0;

// === Function Declarations ===
float calibrateMQ135();
float getResistanceMQ135();
int calculateAQI(float co2_ppm);
String generateHTML();

void setup() {
  Serial.begin(115200);
  delay(1000);

  // I2C Init
  Wire.begin(SDA_PIN, SCL_PIN);

  // OLED Init
  if (!display.begin(0x3C, true)) {
    Serial.println("OLED not found");
    while (1);
  }

  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(10, 20);
  display.println("Weather");
  display.setCursor(20, 45);
  display.println("Station");
  display.display();
  delay(2000);

  // DHT22 Init
  dht.begin();

  // BMP280 Init
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

  // Wi-Fi Init
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected");
  Serial.println(WiFi.localIP());

  // Web Server Routes
  server.on("/", []() {
    server.send(200, "text/html", generateHTML());
  });
  server.begin();
}

void loop() {
  server.handleClient();

  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  float rs = getResistanceMQ135();
  float ratio = rs / Ro;

  float co2_ppm = pow(10, (-0.42 * log10(ratio) + 1.92));
  float nh3_ppm = pow(10, (-1.5 * log10(ratio) + 2.3));
  float alcohol_ppm = pow(10, (-1.552 * log10(ratio) + 2.041));

  float pressure = bmp.readPressure() / 100.0F;
  float bmpTemp = bmp.readTemperature();
  int aqi = calculateAQI(co2_ppm);

  if (millis() - lastToggle > toggleInterval) {
    screenNumber = 1 - screenNumber;
    lastToggle = millis();
  }

  // OLED Display
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

    display.setCursor(0, 16);
    display.print("Temp(BMP): ");
    display.println(String(bmpTemp, 1) + "C");

    display.setCursor(0, 40);
    display.setTextSize(1.5);
    display.print("AQI: ");
    display.print(aqi);
  }

  display.display();
  delay(100);
}

// === MQ135 Calibration ===
float calibrateMQ135() {
  int val = 0;
  for (int i = 0; i < 100; i++) {
    val += analogRead(MQ135_PIN);
    delay(10);
  }
  val /= 100;
  float voltage = val * (3.3 / 4095.0);
  float Rs = ((3.3 - voltage) / voltage) * 10.0;
  float Ro = Rs / 3.6;
  return Ro;
}

// === MQ135 Rs Calculation ===
float getResistanceMQ135() {
  int val = analogRead(MQ135_PIN);
  float voltage = val * (3.3 / 4095.0);
  float Rs = ((3.3 - voltage) / voltage) * 10.0;
  return Rs;
}

// === AQI Estimate from CO2 ===
int calculateAQI(float co2) {
  if (co2 <= 350) return 0;
  else if (co2 <= 600) return 50;
  else if (co2 <= 1000) return 100;
  else if (co2 <= 2000) return 150;
  else if (co2 <= 5000) return 200;
  else return 300;
}

// === HTML Page for Web Server ===
String generateHTML() {
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  float rs = getResistanceMQ135();
  float ratio = rs / Ro;

  float co2_ppm = pow(10, (-0.42 * log10(ratio) + 1.92));
  float nh3_ppm = pow(10, (-1.5 * log10(ratio) + 2.3));
  float alcohol_ppm = pow(10, (-1.552 * log10(ratio) + 2.041));

  float pressure = bmp.readPressure() / 100.0F;
  float bmpTemp = bmp.readTemperature();
  int aqi = calculateAQI(co2_ppm);

  String aqiLabel = (aqi == 0) ? "Good" :
                    (aqi == 50) ? "Moderate" :
                    (aqi == 100) ? "Unhealthy (SG)" :
                    (aqi == 150) ? "Unhealthy" :
                    (aqi == 200) ? "Very Unhealthy" : "Hazardous";

  String html = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<meta http-equiv='refresh' content='5' />";
  html += "<style>body{font-family:Arial; text-align:center;} h2{color:#0A75AD;} .data{font-size:1.5em;}</style>";
  html += "<title>ESP32 Weather Station</title></head><body>";
  html += "<h2>ESP32 Weather Station</h2>";
  html += "<div class='data'>Temp (DHT): " + String(temperature, 1) + " °C<br>";
  html += "Humidity: " + String(humidity, 1) + " %<br>";
  html += "Pressure: " + String(pressure, 1) + " hPa<br>";
  html += "Temp (BMP): " + String(bmpTemp, 1) + " °C<br><br>";
  html += "CO2: " + String(co2_ppm, 0) + " ppm<br>";
  html += "NH3: " + String(nh3_ppm, 0) + " ppm<br>";
  html += "Alcohol: " + String(alcohol_ppm, 0) + " ppm<br><br>";
  html += "<strong>AQI: " + String(aqi) + " (" + aqiLabel + ")</strong></div>";
  html += "</body></html>";
  return html;
}
