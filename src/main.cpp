#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <WiFiManager.h> 

// Define API Key and other constants
const char* API_KEY = "AIzaSyCaepy-tFgUFmhAYcL1oXEGoynCdjMxtSo";
const char* MAX_TOKENS = "120";

// Define OLED display dimensions
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Create SH1106 display object
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire);

// MPU6050 object for accelerometer
Adafruit_MPU6050 mpu;

// Keyboard layout
char keyboard[4][10] = {
  {'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P'},
  {'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', '?'},
  {'Z', 'X', 'C', 'V', 'B', 'N', 'M', ' ', '.', '/'}, // ' ' represents space key
  {'1', '2', '3', '4', '5', '6', '7', '8', '9', '0'}
};

// Variables to store input text
String inputText = "";  
String aiResponse = "";  // Store AI response for scrolling
int cursorX = 0;  
int cursorY = 0;  
int previousX = 0;
int previousY = 0;

// Scroll position
int scrollOffset = 0;

// Button pin and states
#define BUTTON_PIN_SELECT 23
#define BUTTON_PIN_SEND 12

bool buttonHeldSelect = false;
bool buttonHeldSend = false;
bool responseDisplayed = false;

// Initialize WiFi using WiFiManager
void initWiFi() {
  WiFiManager wifiManager;
  wifiManager.autoConnect("Gemini32");
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("To connect to WiFi Go to ");
  display.setCursor(0, 10);
  display.println(WiFi.localIP());
  display.display();
}

void displayAIResponse() {
  display.clearDisplay();
  
  int textHeight = ((aiResponse.length() / 21) + 1) * 8; // Estimate height based on length and line wrap
  int maxScrollOffset = max(0, textHeight - SCREEN_HEIGHT);

  // Constrain scrollOffset within limits
  scrollOffset = constrain(scrollOffset, 0, maxScrollOffset);

  // Set cursor for scrolling text
  display.setCursor(0, -scrollOffset);  // Adjust starting position

  display.setTextWrap(true);
  display.print("AI: ");
  display.println(aiResponse);

  display.display();
}

// Send text to Gemini API and display response on OLED
void sendToGeminiAPI(String userInput) {
  HTTPClient https;
  
  if (https.begin("https://generativelanguage.googleapis.com/v1beta/models/gemini-1.5-flash:generateContent?key=" + (String)API_KEY)) {
    https.addHeader("Content-Type", "application/json");
    
    String payload = "{\"contents\": [{\"parts\":[{\"text\":\"" + userInput + "\"}]}],\"generationConfig\": {\"maxOutputTokens\": " + (String)MAX_TOKENS + "}}";
    int httpCode = https.POST(payload);
    if (httpCode == HTTP_CODE_OK) {
      String response = https.getString();
      DynamicJsonDocument doc(2048);
      deserializeJson(doc, response);
    
      aiResponse = doc["candidates"][0]["content"]["parts"][0]["text"].as<String>();
      scrollOffset = 0;  // Reset scroll offset for new response
    
      displayAIResponse();
      responseDisplayed = true;
      Serial.println("Gemini AI Response: " + aiResponse);
    } else {
      Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
    }
    https.end();
  } else {
    Serial.printf("[HTTPS] Unable to connect\n");
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
  display.display(); // Ensure display refresh after drawing the keyboard
}

// Draw the cursor at the specified position
void drawCursor(int x, int y) {
  int cursorXPos = x * 12;
  int cursorYPos = 20 + y * 12;
  display.drawRect(cursorXPos, cursorYPos, 12, 12, SH110X_WHITE);
}

// Clear the cursor from its previous position
void clearCursor(int x, int y) {
  int cursorXPos = x * 12;
  int cursorYPos = 20 + y * 12;

  display.fillRect(cursorXPos, cursorYPos, 12, 12, SH110X_BLACK);
  display.setCursor(cursorXPos, cursorYPos);
  display.print(keyboard[y][x]);
}

// Update the cursor visually
void updateCursorPosition(int x, int y) {
  clearCursor(previousX, previousY);
  drawCursor(x, y);
  display.display();
  previousX = x;
  previousY = y;
}

// Handle MPU6050 input to move the cursor or scroll
void handleMPUInput(sensors_event_t a, sensors_event_t g) {
  const float threshold = 3.5;
  static unsigned long lastMoveTime = 0;
  const int moveDelay = 200;

  if (responseDisplayed) {
    // Track if the scrollOffset changes to trigger a display update
    int previousOffset = scrollOffset;

    // Calculate the angle of rotation around the Z-axis
    float angle = atan2(g.gyro.y, g.gyro.x) * 180 / PI;

    if (angle > 150) {
      scrollOffset += 10;
    } else if (angle < -150) {
      scrollOffset -= 10;
    }

    if (scrollOffset != previousOffset) {
      displayAIResponse();
      delay(5);
    }
    return;
  }

  // Handle cursor movement
  if (millis() - lastMoveTime > moveDelay) {
    if (a.acceleration.x > threshold) {
      cursorY++;
      if (cursorY >= 4) cursorY = 3;
    } else if (a.acceleration.x < -threshold) {
      cursorY--;
      if (cursorY < 0) cursorY = 0;
    }
    if (a.acceleration.y > threshold) {
      cursorX--;
      if (cursorX >= 10) cursorX = 9;
    } else if (a.acceleration.y < -threshold) {
      cursorX++;
      if (cursorX < 0) cursorX = 0;
    }

    if (cursorX != previousX || cursorY != previousY) {
      updateCursorPosition(cursorX, cursorY);
      lastMoveTime = millis();
    }
  }
}

// Select the key under the cursor
void selectKey() {
  char selectedKey = keyboard[cursorY][cursorX];
  inputText += selectedKey;

  display.fillRect(0, 0, SCREEN_WIDTH, 16, SH110X_BLACK);
  display.setCursor(0, 0);
  display.print("You: ");
  display.print(inputText);
  display.display();
}

// Handle button presses
void checkButtons() {
  bool selectButtonState = digitalRead(BUTTON_PIN_SELECT);
  bool sendButtonState = digitalRead(BUTTON_PIN_SEND);

  if (responseDisplayed && (selectButtonState == HIGH || sendButtonState == HIGH)) {
    // Clear response if a button is pressed after response is shown
    responseDisplayed = false;
    inputText = "";  
    display.clearDisplay();
    drawKeyboard();
    drawCursor(cursorX, cursorY); 
  }

  if (selectButtonState == HIGH && !buttonHeldSelect) {
    buttonHeldSelect = true;
    selectKey();  
  } else if (selectButtonState == LOW) {
    buttonHeldSelect = false;
  }

  if (sendButtonState == HIGH && !buttonHeldSend) {
    buttonHeldSend = true;
    
    if (inputText.length() > 0) {
      sendToGeminiAPI(inputText);
      inputText = "";  
    }
  } else if (sendButtonState == LOW) {
    buttonHeldSend = false;
  }
}

void setup() {
  Serial.begin(9600);

  pinMode(BUTTON_PIN_SELECT, INPUT_PULLUP);
  pinMode(BUTTON_PIN_SEND, INPUT_PULLUP);

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

  initWiFi();
  display.clearDisplay();
  drawKeyboard();
  drawCursor(cursorX, cursorY);
}

void loop() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  handleMPUInput(a, g);
  checkButtons();

  delay(100);  
}