# Gemini32 - ESP32 based AI Handheld Keyboard with OLED Display and MPU6050 based Scrolling System.

This project implements a virtual AI-powered keyboard on an ESP32 with an SH1106 OLED display and an MPU6050 accelerometer for navigation. It connects to the internet via WiFi, allowing users to input text and receive responses from Google's Gemini API. With WiFiManager, users can easily configure WiFi settings without modifying the code, making it simple to set up the device anywhere with WiFi access.

## Features
- **AI Interaction**: Send typed text to the Gemini AI model and receive responses in real-time.
- **Gesture-Based Control**: Navigate the virtual keyboard and scroll through AI responses using MPU6050 accelerometer-based controls.
- **OLED Display Interface**: Displays text input, AI responses, and a virtual keyboard on a 128x64 SH1106 OLED screen.
- **WiFiManager Integration**: Easily set up WiFi credentials through a web portal without hardcoding SSID and password.
- **Button Integration**: Use dedicated buttons to select characters, send messages, and reset AI responses.

## Hardware Requirements
- **ESP32**: Microcontroller for WiFi connectivity and processing.
- **SH1106 OLED Display (128x64)**: For displaying the virtual keyboard, typed text, and AI responses.
- **MPU6050 Accelerometer/Gyroscope**: Detects device tilt to control cursor movements.
- **Buttons**: 
  - **Select Button**: Selects characters on the virtual keyboard.
  - **Send Button**: Sends text input to the AI and displays the response.

## Software Libraries
- [WiFiManager](https://github.com/tzapu/WiFiManager): Simplifies WiFi configuration through a captive portal.
- [WiFi](https://github.com/espressif/arduino-esp32): ESP32 WiFi library for network connection.
- [HTTPClient](https://www.arduino.cc/en/Reference/HTTPClient): For HTTP communication with the Gemini AI API.
- [ArduinoJson](https://arduinojson.org/): Handles JSON parsing of the AI response.
- [Adafruit_GFX](https://github.com/adafruit/Adafruit-GFX-Library): Basic graphics library for drawing on the OLED screen.
- [Adafruit_SH110X](https://github.com/adafruit/Adafruit_SH110X): For interfacing with SH1106 OLED displays.
- [Adafruit_MPU6050](https://github.com/adafruit/Adafruit_MPU6050): For accelerometer and gyroscope readings.

## Installation and Setup
1. **Connect Hardware**: 
   - Set up the SH1106 OLED and MPU6050 according to their respective pin configurations on the ESP32.
   - Connect the select and send pushbuttons to GPIO pins `23` and `12`, respectively.
2. **Install Libraries**:
   - Install the libraries listed above using the Arduino Library Manager or by cloning them into the Arduino libraries folder.
3. **Upload Code**:
   - Compile and upload the code to the ESP32 via the Arduino IDE.

### WiFi Configuration with WiFiManager
1. **Initial Setup**: On first boot, WiFiManager will start a configuration portal.
2. **Connect to Portal**: Use any WiFi-enabled device to connect to the WiFi network named `AutoConnectAP`.
3. **Enter WiFi Credentials**: Once connected, the configuration portal will appear. Select your WiFi network and enter the password.
4. **Auto-Reconnect**: After configuration, the ESP32 will automatically reconnect to the configured WiFi network in future sessions.

## Usage
1. **Power On**: Power the ESP32. WiFiManager will initiate the WiFi configuration portal if no credentials are saved.
2. **Navigate Keyboard**: Use tilt gestures to move the cursor across the keyboard. The `Select` button inputs the chosen character into the text field.
3. **Send Text to AI**: Press the `Send` button to send the typed text to the Gemini AI. The response appears on the OLED display.
4. **Scroll AI Response**: Tilt the device to scroll through the AI's response if it exceeds one screen.
