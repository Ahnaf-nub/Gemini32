#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

// WiFi credentials
const char* ssid = "Mahir";
const char* password = "Ahnaf2007";

// API Key and other constants
const char* API_KEY = "AIzaSyCaepy-tFgUFmhAYcL1oXEGoynCdjMxtSo";
const char* MAX_TOKENS = "100";

// OLED display dimensions
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire);

// MPU6050 object for accelerometer
Adafruit_MPU6050 mpu;

// Keyboard layout
char keyboard[4][10] = {
  {'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P'},
  {'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', '?'},
  {'Z', 'X', 'C', 'V', 'B', 'N', 'M', ' ', '.', '/'},
  {'1', '2', '3', '4', '5', '6', '7', '8', '9', '0'}
};

// Input text and cursor variables
String inputText = "";  
int cursorX = 0;  
int cursorY = 0;  
int previousX = 0;
int previousY = 0;

// Button pins and states
#define BUTTON_PIN_SELECT 23
#define BUTTON_PIN_SEND 12
bool buttonHeldSelect = false;
bool buttonHeldSend = false;

// AI response and scrolling variables
String aiResponse = "";
String responseLines[10];
int numResponseLines = 0;
int scrollOffset = 0;
bool displayingAnswer = false;  // New variable to track answer display

// WiFi setup with timeout
void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi...");
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000) {
    Serial.print('.');
    delay(1000);
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Failed to connect to WiFi.");
  }
}

// Send text to Gemini API and store response
void sendToGeminiAPI(String userInput) {
  HTTPClient https;
  if (https.begin("https://generativelanguage.googleapis.com/v1beta/models/gemini-1.5-flash:generateContent?key=" + (String)API_KEY)) {
    https.addHeader("Content-Type", "application/json");
    
    String payload = "{\"contents\": [{\"parts\":[{\"text\":\"" + userInput + "\"}]}],\"generationConfig\": {\"maxOutputTokens\": " + (String)MAX_TOKENS + "}}";
    int httpCode = https.POST(payload);

    if (httpCode == HTTP_CODE_OK) {
      String response = https.getString();
      DynamicJsonDocument doc(2048);  // Use DynamicJsonDocument instead of JsonDocument
      if (deserializeJson(doc, response)) {
        Serial.println("Failed to parse JSON");
        return;
      }

      aiResponse = doc["candidates"][0]["content"]["parts"][0]["text"].as<String>();
      numResponseLines = 0;
      for (int i = 0; i < aiResponse.length(); i += 20) {
        responseLines[numResponseLines++] = aiResponse.substring(i, i + 20);
        if (numResponseLines >= 10) break;
      }
      scrollOffset = 0;
      displayingAnswer = false;  // Do not display immediately
    } else {
      Serial.printf("[HTTPS] POST failed, error: %s\n", https.errorToString(httpCode).c_str());
    }
    https.end();
  } else {
    Serial.println("[HTTPS] Unable to connect");
  }
}

// Draw keyboard on the OLED
void drawKeyboard() {
  int xOffset = 0;
  int yOffset = 20;
  for (int row = 0; row < 4; row++) {
    for (int col = 0; col < 10; col++) {
      display.setCursor(xOffset + col * 12, yOffset + row * 12); 
      display.print(keyboard[row][col]);
    }
  }
}

// Draw and clear the cursor
void drawCursor(int x, int y) {
  int cursorXPos = x * 12;
  int cursorYPos = 20 + y * 12;
  display.drawRect(cursorXPos, cursorYPos, 12, 12, SH110X_WHITE);
}
void clearCursor(int x, int y) {
  int cursorXPos = x * 12;
  int cursorYPos = 20 + y * 12;
  display.fillRect(cursorXPos, cursorYPos, 12, 12, SH110X_BLACK);
  display.setCursor(cursorXPos, cursorYPos);
  display.print(keyboard[y][x]);
}
void updateCursorPosition(int x, int y) {
  clearCursor(previousX, previousY);
  drawCursor(x, y);
  display.display();
  previousX = x;
  previousY = y;
}

void selectKey() {
  char selectedKey = keyboard[cursorY][cursorX];
  inputText += selectedKey;
  display.clearDisplay();
  display.fillRect(0, 0, SCREEN_WIDTH, 16, SH110X_BLACK);
  display.setCursor(0, 0);
  display.print("You: ");
  display.print(inputText);
  drawKeyboard();
  drawCursor(cursorX, cursorY);
  display.display();
}

void displayAIResponse() {
  display.clearDisplay();
  display.fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SH110X_BLACK);
  for (int i = 0; i < 4; i++) {
    int lineIndex = scrollOffset + i;
    if (lineIndex < numResponseLines) {
      display.setCursor(0, i * 16);
      display.print(responseLines[lineIndex]);
    }
  }
  display.display();
}

// Add scrolling system with gyro sensor when displaying answer
void handleGyroScrolling(sensors_event_t g) {
  if (!displayingAnswer) return;

  const float gyroThreshold = 4.5;  // Adjust as needed
  static unsigned long lastGyroTime = 0;
  const int gyroDelay = 200;  // Delay between scrolls in milliseconds

  if (millis() - lastGyroTime > gyroDelay) {
    if (g.gyro.z > gyroThreshold) {
      scrollOffset++;
      if (scrollOffset > numResponseLines - 4) scrollOffset = numResponseLines - 4;
      displayAIResponse();
      lastGyroTime = millis();
    } else if (g.gyro.z < -gyroThreshold) {
      scrollOffset--;
      if (scrollOffset < 0) scrollOffset = 0;
      displayAIResponse();
      lastGyroTime = millis();
    }
  }
}

// Handle MPU6050 input for cursor movement
void handleMPUInput(sensors_event_t a) {
  if (displayingAnswer) return;

  const float thresholdx = 3.5;
  const float thresholdy = 4.5;
  static unsigned long lastMoveTime = 0;
  const int moveDelay = 200;

  if (millis() - lastMoveTime > moveDelay) {
    if (a.acceleration.x < thresholdx) {
      cursorY--;
      if (cursorY < 0) cursorY = 0;
    } else if (a.acceleration.x > -thresholdx) {
      cursorY++;
      if (cursorY >= 4) cursorY = 3;
    }
    
    if (a.acceleration.y > thresholdy) {
      cursorX--;
      if (cursorX < 0) cursorX = 0;
    } else if (a.acceleration.y < -thresholdy) {
      cursorX++;
      if (cursorX >= 10) cursorX = 9;
    }

    if (cursorX != previousX || cursorY != previousY) {
      updateCursorPosition(cursorX, cursorY);
      lastMoveTime = millis();
    }
  }
}

// Handle button inputs
void checkButtons() {
  bool selectButtonState = digitalRead(BUTTON_PIN_SELECT);
  bool sendButtonState = digitalRead(BUTTON_PIN_SEND);
  
  if (selectButtonState == HIGH && !buttonHeldSelect) {
    buttonHeldSelect = true;
    selectKey();
  } else if (selectButtonState == LOW) {
    buttonHeldSelect = false;
  }

  // Only send the prompt if BUTTON_PIN_SEND is released
  if (sendButtonState == HIGH && !buttonHeldSend) {
    buttonHeldSend = true;
    
    // Check if there's a response available to display or if inputText is ready to send
    if (aiResponse.length() > 0) {
      displayingAnswer = true;
      displayAIResponse();
    } else if (inputText.length() > 0 && digitalRead(BUTTON_PIN_SEND) == LOW) {  // Ensure button is not pressed
      sendToGeminiAPI(inputText);  // Send input to Gemini API only if button is released
      inputText = "";  
    }
  } else if (sendButtonState == LOW && buttonHeldSend) {
    buttonHeldSend = false;
    displayingAnswer = false;

    // Redraw keyboard interface when answer display ends
    display.clearDisplay();
    display.fillRect(0, 0, SCREEN_WIDTH, 16, SH110X_BLACK);
    display.setCursor(0, 0);
    display.print("You: ");
    display.print(inputText);
    drawKeyboard();
    drawCursor(cursorX, cursorY);
    display.display();
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(BUTTON_PIN_SELECT, INPUT_PULLUP);
  pinMode(BUTTON_PIN_SEND, INPUT_PULLUP);
  initWiFi();

  if (!display.begin(0x3C, true)) {
    Serial.println("Display initialization failed!");
    while (1);
  }

  if (!mpu.begin()) {
    Serial.println("MPU6050 initialization failed!");
    while (1);
  }

  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_5_HZ);

  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.clearDisplay();

  drawKeyboard();
  drawCursor(cursorX, cursorY);
}

void loop() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  if (displayingAnswer) {
    handleGyroScrolling(g);
  } else {
    handleMPUInput(a);
    checkButtons();
  }

  delay(100);  
}