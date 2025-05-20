
# 🌤️ ESP32 Weather & Air Quality Monitoring Station

![Platform](https://img.shields.io/badge/platform-ESP32-blue)
![Sensors](https://img.shields.io/badge/sensors-DHT22%2C%20BMP280%2C%20MQ135-orange)
![License](https://img.shields.io/badge/license-MIT-green)
![Status](https://img.shields.io/badge/status-Active-brightgreen)

An advanced weather and air quality monitoring system built using the ESP32 microcontroller, featuring real-time data from DHT22, BMP280, and MQ135 sensors. The project displays live environmental readings on an OLED screen and a web interface via Wi-Fi.

---

## 📸 Demo

<p align="center">
  <img src="https://github.com/Prabesh-Shrestha/OLE-Weather-Station/blob/master/assets/demo.gif" width="500" alt="demo"/>
</p>

---
      
## 🔧 Features

- 🌡️ **Temperature & Humidity** (DHT22)
- 🌬️ **Pressure & Altitude** (BMP280)
- 🏭 **Air Quality Monitoring** (CO2, NH3, Alcohol via MQ135)
- 📊 **Real-time AQI Calculation**
- 📟 **OLED Display** (Auto-switching screens)
- 🌐 **Live Web Dashboard** (Local network)
- 📡 **Wi-Fi Connected ESP32**

---

## 🛠️ Hardware Used

| Component     | Description                         |
|--------------|-------------------------------------|
| ESP32        | Microcontroller with Wi-Fi support  |
| DHT22        | Temperature & Humidity Sensor       |
| BMP280       | Pressure & Temperature Sensor       |
| MQ135        | Air Quality Gas Sensor              |
| SH1106 OLED  | 128x64 I2C OLED Display             |
| Resistors    | For sensor interfacing              |
| Jumper Wires | Connections                         |
| Breadboard   | Prototyping                         |

---

## 📦 Libraries Required

Install these libraries from the Arduino Library Manager:

- `Adafruit SH110X`
- `Adafruit GFX`
- `Adafruit BMP280`
- `Adafruit Unified Sensor`
- `DHT sensor library`

---

## 🚀 Getting Started

### 1. Clone this Repo

```bash
git clone https://github.com/Prabesh-Shrestha/OLE-Weather-Station.git 
cd OLE-Weather-Station
````

### 2. Configure Wi-Fi

Edit the following lines in the code:

```cpp
const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";
```

### 3. Upload to ESP32

* Open the `.ino` file in Arduino IDE.
* Select your board: **ESP32 Dev Module**.
* Connect your ESP32 via USB.
* Upload the code.

### 4. Open Serial Monitor

Once connected to Wi-Fi, you'll see the local IP address of the ESP32. Use it to access the live web dashboard.

---

## 🌐 Web Dashboard Preview

Access your dashboard via: `http://<your-esp32-ip>`

![Web Dashboard](https://github.com/Prabesh-Shrestha/OLE-Weather-Station/blob/master/assets/web_preview.png)

---

## 📋 Sensor Calibrations

* **MQ135** sensor is calibrated on startup.
* **Ro** value is calculated based on ambient air quality.
* The code uses logarithmic regression to estimate gas concentrations.

---

## 🧠 AQI Calculation

AQI is estimated based on CO₂ ppm levels using EPA thresholds:

| CO₂ (ppm) | AQI Level            |
| --------- | -------------------- |
| < 350     | Good (0)             |
| 350–600   | Moderate (50)        |
| 600–1000  | Unhealthy (100)      |
| 1000–2000 | Very Unhealthy (150) |
| 2000–5000 | Hazardous (200+)     |

---

## 📄 License

This project is licensed under the **MIT License**.
Feel free to use, modify, and share it.

---

## 💡 Author

Made with ❤️ by [Prabesh Shrestha](https://prabesh-shrestha.com.np)

* GitHub: [@prabesh-shrestha](https://github.com/prabesh-shrestha)
* Email: [prabesh.sh3.14@gmail.com](mailto:prabesh.sh3.14@gmail.com)

---

## 🌟 Star This Repo

If you found this project useful or interesting, please consider giving it a ⭐ — it helps others find it!


