# CogniLog 
> **Intelligent IoT Productivity Assistant powered by Edge AI**

![Status](https://img.shields.io/badge/Status-Active_Development-brightgreen)
![Platform](https://img.shields.io/badge/Platform-ESP32-blue)
![Framework](https://img.shields.io/badge/Framework-PlatformIO-orange)
![TinyML](https://img.shields.io/badge/AI-TinyML-red)

##  Why CogniLog?
Most productivity trackers rely on cloud analytics, raising privacy concerns and increasing latency.
CogniLog flips the model by performing **on-device inference**, proving that meaningful behavior analysis is possible even on constrained microcontrollers.


##  Overview
CogniLog is an embedded IoT system designed to track productivity and focus habits without relying on cloud dependency. Unlike traditional trackers, CogniLog processes data **on device** using TinyML, ensuring privacy and real time feedback.

**Core Goal:** Detect user presence and focus states using sensor fusion and lightweight neural networks running on the ESP32.

##  Key Features
* **Edge AI Processing:** Runs inference locally on ESP32 (No cloud latency).
* **Privacy First:** Data stays on the device; only metrics are logged.
* **Real time Analytics:** Tracks focus sessions and break intervals.
* **Energy Efficient:** Optimized for low power consumption using Deep Sleep modes.

##   Tech Stack
| Component | Technology |
|-----------|------------|
| **Hardware** | ESP32 (Xtensa LX6/LX7) |
| **Firmware** | C++ (ESP-IDF / Arduino Framework) |
| **IDE/Build** | VS Code + PlatformIO |
| **Edge AI** | TensorFlow Lite Micro / Edge Impulse |
| **Connectivity** | WiFi (MQTT), BLE |

##  Getting Started
1. **Clone the repo:**
   ```bash
   git clone [https://github.com/mohansudhandhiram-2k07/CogniLog.git](https://github.com/mohansudhandhiram 2k07/CogniLog.git)
