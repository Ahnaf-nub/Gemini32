#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

// Define display dimensions
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Create an SH1106 display object
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire);

// MPU6050 object
Adafruit_MPU6050 mpu;

// Keyboard layout
char keyboard[4][10] = {
  {'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P'},
  {'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', '?'},
  {'Z', 'X', 'C', 'V', 'B', 'N', 'M', ',', '.', '/'},
  {'1', '2', '3', '4', '5', '6', '7', '8', '9', '0'}
};

String inputText = "";  // Stores the input text
int cursorX = 0;  // Cursor position (column)
int cursorY = 0;  // Cursor position (row)
int previousX = 0;  // To track previous position and avoid redundant redraw
int previousY = 0;

// Draw the keyboard layout on the OLED
void drawKeyboard() {
  int xOffset = 0;
  int yOffset = 20;  // Offset to start drawing keyboard below the input text area

  // Loop through the keyboard array and draw keys
  for (int row = 0; row < 4; row++) {
    for (int col = 0; col < 10; col++) {
      display.setCursor(xOffset + col * 12, yOffset + row * 12);  // Adjust key spacing
      display.print(keyboard[row][col]);
    }
  }
}

// Draw the cursor at the specified position
void drawCursor(int x, int y) {
  int cursorXPos = x * 12;  // Horizontal cursor position
  int cursorYPos = 20 + y * 12;  // Vertical cursor position (offset by the input text space)

  // Draw a small rectangle around the current key to represent the cursor
  display.drawRect(cursorXPos, cursorYPos, 12, 12, SH110X_WHITE);
}

// Clear the cursor from its previous position
void clearCursor(int x, int y) {
  int cursorXPos = x * 12;  // Horizontal cursor position
  int cursorYPos = 20 + y * 12;  // Vertical cursor position (offset by the input text space)

  // Clear the rectangle where the cursor was by redrawing the key
  display.fillRect(cursorXPos, cursorYPos, 12, 12, SH110X_BLACK);  // Clear the box
  display.setCursor(cursorXPos, cursorYPos);
  display.print(keyboard[y][x]);  // Redraw the key itself
}

// Update only the input text when it changes
void displayInputText() {
  display.fillRect(0, 0, SCREEN_WIDTH, 16, SH110X_BLACK);  // Clear only the input text area
  display.setCursor(0, 0);  // Start at the top
  display.print("Input: ");
  display.print(inputText);  // Print current input text
  display.display();  // Render the updated input text
}

// Add a function for selecting text by pressing a button or gesture
void selectKey() {
  char selectedKey = keyboard[cursorY][cursorX];  // Get the current key under the cursor
  inputText += selectedKey;  // Append the selected key to the input text
  displayInputText();  // Update the displayed input text
}

// Update cursor position visually
void updateCursorPosition(int x, int y) {
  clearCursor(previousX, previousY);  // Clear old cursor position
  drawCursor(x, y);  // Draw new cursor
  display.display();  // Refresh display
  previousX = x;  // Update previous position
  previousY = y;
}

// Function to handle MPU6050 tilt input for cursor navigation
void handleMPUInput(sensors_event_t a) {
  const float threshold = 3.5;  // Set the tilt sensitivity threshold

  static unsigned long lastMoveTime = 0;
  const int moveDelay = 200;  // Delay in milliseconds to slow down the cursor movement

  // Only move if enough time has passed since the last move
  if (millis() - lastMoveTime > moveDelay) {
    if (a.acceleration.x > threshold) {
      cursorY++;
      if (cursorY >= 4) cursorY = 3;  // Prevent overflow
    } else if (a.acceleration.x < -threshold) {
      cursorY--;
      if (cursorY < 0) cursorY = 0;  // Prevent underflow
    }
    if (a.acceleration.y < threshold) {
      cursorX++;
      if (cursorX >= 10) cursorX = 9;  // Prevent overflow
    } else if (a.acceleration.y > -threshold) {
      cursorX--;
      if (cursorX < 0) cursorX = 0;  // Prevent underflow
    }

    // If the cursor moved, update its position
    if (cursorX != previousX || cursorY != previousY) {
      updateCursorPosition(cursorX, cursorY);
      lastMoveTime = millis();  // Update the last move time
    }
  }
}

void setup() {
  Serial.begin(9600);
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
  
  drawKeyboard();  // Draw the initial keyboard layout
  drawCursor(cursorX, cursorY);  // Draw the cursor at the starting position
  displayInputText();  // Draw the input text area
}

void loop() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  // Process accelerometer data for cursor movement
  handleMPUInput(a);

  // Add more input-handling logic here (e.g., button press to select keys)
  delay(100);  // Small delay for smoother display
}
