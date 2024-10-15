#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

// Define WiFi credentials
const char* ssid = "Mahir";
const char* password = "Ahnaf2007";

// Define API Key and other constants
const char* API_KEY = "AIzaSyCaepy-tFgUFmhAYcL1oXEGoynCdjMxtSo";
const char* MAX_TOKENS = "100";

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
int cursorX = 0;  
int cursorY = 0;  
int previousX = 0;
int previousY = 0;

// Button pin and states
#define BUTTON_PIN 23
unsigned long buttonPressTime = 0;
bool buttonHeld = false;
bool textEntered = false; 

// Initialize WiFi
void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
}

// Send text to Gemini API and display response on OLED
void sendToGeminiAPI(String userInput) {
  HTTPClient https;
  
  // Construct API URL
  if (https.begin("https://generativelanguage.googleapis.com/v1beta/models/gemini-1.5-flash:generateContent?key=" + (String)API_KEY)) {
    https.addHeader("Content-Type", "application/json");
    
    // Prepare JSON payload
    String payload = "{\"contents\": [{\"parts\":[{\"text\":" + userInput + "}]}],\"generationConfig\": {\"maxOutputTokens\": " + (String)MAX_TOKENS + "}}";
    int httpCode = https.POST(payload);

    if (httpCode == HTTP_CODE_OK) {
      String response = https.getString();
      DynamicJsonDocument doc(2048);
      deserializeJson(doc, response);

      // Extract the response from the API
      String answer = doc["candidates"][0]["content"]["parts"][0]["text"];
      
      // Display the response on OLED
      display.fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SH110X_BLACK);  // Clear display
      display.setCursor(0, 0);
      display.print("AI: ");
      display.println(answer);
      display.display();
      
      Serial.println("Gemini AI Response: " + answer);
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

// Handle MPU6050 input to move the cursor
void handleMPUInput(sensors_event_t a) {
  const float threshold = 3.5;

  static unsigned long lastMoveTime = 0;
  const int moveDelay = 300;

  if (millis() - lastMoveTime > moveDelay) {
    if (a.acceleration.x > threshold) {
      cursorY++;
      if (cursorY >= 4) cursorY = 3;  // Prevent overflow
    } else if (a.acceleration.x < -threshold) {
      cursorY--;
      if (cursorY < 0) cursorY = 0;  // Prevent underflow
    }
    if (a.acceleration.y > threshold) {
      cursorX--;
      if (cursorX >= 10) cursorX = 9;  // Prevent overflow
    } else if (a.acceleration.y < -threshold) {
      cursorX++;
      if (cursorX < 0) cursorX = 0;  // Prevent underflow
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

  // Display the input text on OLED (or clear for AI response)
  display.fillRect(0, 0, SCREEN_WIDTH, 16, SH110X_BLACK);
  display.setCursor(0, 0);
  display.print("You: ");
  display.print(inputText);
  display.display();
}

// Handle button press to send text
void checkButton() {
  bool buttonState = digitalRead(BUTTON_PIN);

  if (buttonState == LOW) {  
    if (!buttonHeld) {
      buttonPressTime = millis();
      buttonHeld = true;
    }

    if (millis() - buttonPressTime > 5000 && !textEntered) {
      sendToGeminiAPI("\"" + inputText + "\"");  // Send input to API
      inputText = "";  // Clear input after sending
      textEntered = true;
    }
  } else {  
    if (buttonHeld && millis() - buttonPressTime < 5000) {
      selectKey();  
    }
    buttonHeld = false;  
    textEntered = false;
  }
}

void setup() {
  Serial.begin(9600);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

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

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);

  drawKeyboard();
  drawCursor(cursorX, cursorY);
}

void loop() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  handleMPUInput(a);
  checkButton();

  delay(100);  
}