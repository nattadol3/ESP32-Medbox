#include "pitches.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <SD.h>
#include <SPI.h>
#include <stdio.h>
// time demo code -->
#include <RTClib.h>
#define I2C_SDA 21             //OLED and DS3231: SDA --> GPIO21
#define I2C_SCL 22             //OLED and DS3231: SCL --> GPIO22
#define OLED_I2C_ADDRESS 0x3c  //OLED I2C Address
#define SCREEN_WIDTH 128       // OLED display width, in pixels
#define SCREEN_HEIGHT 64       // OLED display height, in pixels
#define OLED_RESET -1          // QT-PY / XIAO
Adafruit_SH1106G OLEDdisplay = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
RTC_DS3231 rtc;

const int CS = 5;

// Config file content
File config;
String iniContent = "";

// Variable to keep track of med info
String FirstMedName;
String SecondMedName;
String ThirdMedName;
String FirstMedWhen;
String SecondMedWhen;
String ThirdMedWhen;

// Port for LEDs and Buttons.
const int slot1Pin = 33;
const int slot2Pin = 26;
const int slot3Pin = 14;
const int slot1PinM = 25;
const int slot2PinM = 27;
const int slot3PinM = 12;
const int butt1Pin = 4;
const int butt2Pin = 16;
const int butt3Pin = 2;
const int buzzerPin = 32;
const int reedPin = 15;

bool boxIsOpen1 = false;
bool boxIsOpen2 = false;
bool boxIsOpen3 = false;
bool taketime1 = false;
bool taketime2 = false;
bool taketime3 = false;
bool playsong = false;

// Arrays to keep track of notification time for each med
String notif1[4];
String notif2[4];
String notif3[4];

// Current time string
String currTime;

unsigned long lastTimeAlert1;
unsigned long lastTimeAlert2;
unsigned long lastTimeAlert3;
unsigned long currentTime;
int repeatTime1;
int repeatTime2;
int repeatTime3;

//notify time example code -->
int melody[] = {
  NOTE_C4, NOTE_C4, NOTE_G4, NOTE_G4,
  NOTE_A4, NOTE_A4, NOTE_G4, NOTE_F4,
  NOTE_F4, NOTE_E4, NOTE_E4, NOTE_D4,
  NOTE_D4, NOTE_C4, NOTE_G4, NOTE_G4,
  NOTE_F4, NOTE_F4, NOTE_E4, NOTE_E4,
  NOTE_D4, NOTE_G4, NOTE_G4, NOTE_F4,
  NOTE_F4, NOTE_E4, NOTE_E4, NOTE_D4,
};

int durations[] = {
  4, 4, 4, 4,
  4, 4, 8, 4,
  4, 4, 4, 4,
  4, 8, 4, 4,
  4, 4, 4, 4,
  8, 4, 4, 4,
  4, 4, 4, 8,
};
int misslody[] = {
  NOTE_C4, NOTE_C7, NOTE_C4, NOTE_C7
};
int murations[] = {
  8, 4, 8, 4
};

void setup() {
  // time demo code -->
  // Initailize RTC module
  disableCore0WDT();  // ปิด Watchdog Timer สำหรับ core0
  disableCore1WDT();  // ปิด Watchdog Timer สำหรับ core1
  
  // ตรงนี้เคย initialize RTC ถ้าไม่เวิร์คก็มาแก้ตรวงนี้

  // time demo code -->
  pinMode(butt1Pin, INPUT_PULLUP);
  pinMode(butt2Pin, INPUT_PULLUP);
  pinMode(butt3Pin, INPUT_PULLUP);
  pinMode(reedPin, INPUT);
  pinMode(slot1Pin, OUTPUT);
  pinMode(slot2Pin, OUTPUT);
  pinMode(slot3Pin, OUTPUT);
  pinMode(slot1PinM, OUTPUT);
  pinMode(slot2PinM, OUTPUT);
  pinMode(slot3PinM, OUTPUT);
  pinMode(buzzerPin, OUTPUT);

  Serial.begin(115200);

  // Initailize RTC module
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1)
      ;
  }

  if (rtc.lostPower()) {
    Serial.println("RTC lost power, setting the time!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  // SD card initialize
  if (!SD.begin(CS)) {
    Serial.println(("SD card Initialization falied"));
    return;
  }

  Serial.println("SD card Initialization done.");
  Serial.println("*********************************************");

  // Read config file
  config = SD.open("/config.ini", FILE_READ);
  if (config) {
    Serial.println("config.ini is opened");

    // Read all the content in the file and save it to a string
    while (config.available()) {
      char c = config.read();
      iniContent += c;  // Append each character to the iniContent variable
    }

    // Close file
    config.close();
    Serial.println("File closed.");

    // Output the content stored in the iniContent variable
    Serial.println("File contents:");
    Serial.println(iniContent);
  } else {
    Serial.println("Error opening config.ini");
    return;
  }

  parseIni(iniContent);  // Save Config file contents into arrays and vars
  printMedications();    // Show medication info in terminal

  // Initializing OLED
  Serial.println("Start OLED Display...");
  OLEDdisplay.begin(OLED_I2C_ADDRESS, true);
  OLEDdisplay.setContrast(0);  // dim display
  OLEDdisplay.display();
  Wire.begin(I2C_SDA, I2C_SCL);
  OLEDdisplay.clearDisplay();
  OLEDdisplay.setTextSize(1);
  OLEDdisplay.setTextColor(SH110X_WHITE);
  OLEDdisplay.setCursor(0, 0);

  // Display medication Info on the OLED display
  displayMedication(FirstMedName, FirstMedWhen, notif1);
  delay(5000);

  displayMedication(SecondMedName, SecondMedWhen, notif2);
  delay(5000);

  displayMedication(ThirdMedName, ThirdMedWhen, notif3);
  delay(5000);
}

void loop() {
  // Get current time from RTC module
  DateTime now = rtc.now();
  currTime = twoDigits(now.hour()) + ":" + twoDigits(now.minute()) + ":" + twoDigits(now.second());
  debug();
  Serial.println(currTime);

  // Display time and system name on the OLED display
  displayReminder();
  displayCurrentTime(currTime);
  drawAnalogClock(now);

  // Notify user and save to log
  notify();
  if (taketime1) {
    checkCountdown(&lastTimeAlert1, &repeatTime1, slot1Pin, slot1PinM, &taketime1, &boxIsOpen1, FirstMedName);
  }
  if (taketime2) {
    checkCountdown(&lastTimeAlert2, &repeatTime2, slot2Pin, slot2PinM, &taketime2, &boxIsOpen2, SecondMedName);
  }
  if (taketime3) {
    checkCountdown(&lastTimeAlert3, &repeatTime3, slot3Pin, slot3PinM, &taketime3, &boxIsOpen3, ThirdMedName);
  }
  if (playsong) {
    displayAlert();
    songplay();
    playsong = false;
  }
  taken();
  delay(1000);
}

// Buzzer sound setting
void songplay() {
  // Play the melody once
  int size = sizeof(durations) / sizeof(int);
  for (int note = 0; note < size; note++) {
    if (digitalRead(reedPin) == LOW) {
      noTone(buzzerPin);
      Serial.println("stop playing");
      break;
    }
    int duration = 1000 / durations[note];
    tone(buzzerPin, melody[note], duration);
    int pauseBetweenNotes = duration * 1.30;
    delay(pauseBetweenNotes);
    noTone(buzzerPin);
  }
}

void notify() {
  for (int i = 0; i < 4; i++) {
    if (currTime == notif1[i]) {
      Serial.println("alert");
      digitalWrite(slot1Pin, HIGH);
      repeatTime1 = 0;
      lastTimeAlert1 = millis();
      taketime1 = true;
      playsong = true;
    }
    if (currTime == notif2[i]) {
      Serial.println("alert");
      digitalWrite(slot2Pin, HIGH);
      repeatTime2 = 0;
      lastTimeAlert2 = millis();
      taketime2 = true;
      playsong = true;
      delay(10);
    }
    if (currTime == notif3[i]) {
      Serial.println("alert");
      digitalWrite(slot3Pin, HIGH);
      repeatTime3 = 0;
      lastTimeAlert3 = millis();
      taketime3 = true;
      playsong = true;
    }
  }
}

void checkCountdown(unsigned long *lastTimeAlert, int *repeatTime, int slotPin, int slotPinM, bool *taketime, bool *boxIsOpen, String MedName) {
  currentTime = millis();
  if (currentTime - *lastTimeAlert >= 30000) {  // 5 minutes delay
    *lastTimeAlert = millis();
    if (*repeatTime < 2) {
      *repeatTime += 1;
      playsong = true;

    } else {
      digitalWrite(slotPin, LOW);
      int size = sizeof(murations) / sizeof(int);
      for (int note = 0; note < size; note++) {
        int duration = 1000 / murations[note];
        tone(buzzerPin, misslody[note], duration);
        int pauseBetweenNotes = duration * 1.30;
        delay(pauseBetweenNotes);
        noTone(buzzerPin);
        if (note % 2 == 0) {
          digitalWrite(slotPinM, HIGH);
        } else {
          digitalWrite(slotPinM, LOW);
        }
      }
      *taketime = false;
      Serial.println("miss");
      Med_log(MedName, false, currTime);
      *boxIsOpen = false;
    }
  }
}

void taken() {
  if (digitalRead(reedPin) == LOW && taketime1 == true) {
    boxIsOpen1 = true;
    Serial.println("is in taketime1");
  }
  if (digitalRead(reedPin) == LOW && taketime2 == true) {
    boxIsOpen2 = true;
    Serial.println("is in taketime2");
  }
  if (digitalRead(reedPin) == LOW && taketime3 == true) {
    boxIsOpen3 = true;
    Serial.println("is in taketime3");
  }


  if (boxIsOpen1 == true && digitalRead(butt1Pin) == HIGH) {
    Serial.println("B1 is taken");
    digitalWrite(slot1Pin, LOW);
    boxIsOpen1 = false;
    taketime1 = false;
    delay(10);
    Med_log(FirstMedName, true, currTime);
  }
  if (boxIsOpen2 == true && digitalRead(butt2Pin) == HIGH) {
    Serial.println("B2 is taken");
    digitalWrite(slot2Pin, LOW);
    boxIsOpen2 = false;
    taketime2 = false;
    delay(10);
    Med_log(SecondMedName, true, currTime);
  }
  if (boxIsOpen3 == true && digitalRead(butt3Pin) == HIGH) {
    Serial.println("B3 is taken");
    digitalWrite(slot3Pin, LOW);
    boxIsOpen3 = false;
    taketime3 = false;
    delay(10);
    Med_log(ThirdMedName, true, currTime);
  }
}

// Function to save medication info into variables.
void parseIni(String ini) {
  String lines[12];  // Adjust size based on expected lines
  int lineCount = 0;

  // Split the string by newline characters
  int startIndex = 0;
  int endIndex = ini.indexOf('\n');
  while (endIndex != -1) {
    lines[lineCount++] = ini.substring(startIndex, endIndex);
    startIndex = endIndex + 1;
    endIndex = ini.indexOf('\n', startIndex);
  }
  lines[lineCount++] = ini.substring(startIndex);  // Last line

  // Process each section
  for (int i = 0; i < lineCount; i++) {
    if (lines[i].startsWith("[FirstMed]")) {
      FirstMedName = getValue(lines[++i]);  // Get name
      fillNotifArray(notif1, lines[++i]);   // Get notif_time
      FirstMedWhen = getValue(lines[++i]);  // Get when
    } else if (lines[i].startsWith("[SecondMed]")) {
      SecondMedName = getValue(lines[++i]);  // Get name
      fillNotifArray(notif2, lines[++i]);    // Get notif_time
      SecondMedWhen = getValue(lines[++i]);  // Get when
    } else if (lines[i].startsWith("[ThirdMed]")) {
      ThirdMedName = getValue(lines[++i]);  // Get name
      fillNotifArray(notif3, lines[++i]);   // Get notif_time
      ThirdMedWhen = getValue(lines[++i]);  // Get when
    }
  }
}

// Function to extract the value after '='
String getValue(String line) {
  return line.substring(line.indexOf('=') + 2, line.length());
}

// Function to fill notification time arrays
void fillNotifArray(String notif[], String line) {
  // Initialize the array with empty strings
  for (int i = 0; i < 4; i++) {
    notif[i] = "";
  }

  // Split the notif_time string by commas
  int startIndex = line.indexOf('=') + 2;  // Start after '='
  int endIndex = line.indexOf(',', startIndex);
  int index = 0;

  while (endIndex != -1 && index < 4) {
    notif[index++] = line.substring(startIndex, endIndex);  // Extract time
    startIndex = endIndex + 2;                              // Move past the comma and space
    endIndex = line.indexOf(',', startIndex);
  }

  // Add the last time (if exists)
  if (startIndex < line.length() && index < 4) {
    notif[index] = line.substring(startIndex);  // Add the last one
  }
}

// function to print medication info on terminal.
void printMedications() {
  // Print FirstMed
  Serial.println("First Medication:");
  Serial.print("Name: ");
  Serial.println(FirstMedName);
  Serial.print("When: ");
  Serial.println(FirstMedWhen);
  Serial.print("Notification Times: ");
  for (int i = 0; i < 4; i++) {
    Serial.print(notif1[i]);
    if (i < 3) Serial.print(", ");  // Print comma between times
  }
  Serial.println("\n");

  // Print SecondMed
  Serial.println("Second Medication:");
  Serial.print("Name: ");
  Serial.println(SecondMedName);
  Serial.print("When: ");
  Serial.println(SecondMedWhen);
  Serial.print("Notification Times: ");
  for (int i = 0; i < 4; i++) {
    Serial.print(notif2[i]);
    if (i < 3) Serial.print(", ");  // Print comma between times
  }
  Serial.println("\n");

  // Print ThirdMed
  Serial.println("Third Medication:");
  Serial.print("Name: ");
  Serial.println(ThirdMedName);
  Serial.print("When: ");
  Serial.println(ThirdMedWhen);
  Serial.print("Notification Times: ");
  for (int i = 0; i < 4; i++) {
    Serial.print(notif3[i]);
    if (i < 3) Serial.print(", ");  // Print comma between times
  }
  Serial.println("\n");
}

// Function to display medication info on OLED display.
void displayMedication(String medName, String medWhen, String notif[]) {
  OLEDdisplay.clearDisplay();
  OLEDdisplay.setCursor(0, 0);
  OLEDdisplay.setTextSize(1.5);
  OLEDdisplay.setTextColor(SH110X_WHITE);

  OLEDdisplay.print("Pill: ");
  OLEDdisplay.println(medName);
  OLEDdisplay.print("Time: ");
  for (int i = 0; i < 4; i++) {
    if (notif[i] != "") {           // Check if the current element is not empty
      OLEDdisplay.print(notif[i]);  // Print the current element

      // Check if the next element is not empty and within bounds
      if (i < 3 && notif[i + 1] != "") {
        OLEDdisplay.print("\n      ");  // Add new line with indentation if the next element is not empty
      }
    }
  }

  OLEDdisplay.print("\nWhen: ");
  OLEDdisplay.println(medWhen);

  OLEDdisplay.display();
}

// Display system name on OLED.
void displayReminder() {
  OLEDdisplay.clearDisplay();
  OLEDdisplay.setCursor(0, 0);
  OLEDdisplay.setTextSize(1.5);
  OLEDdisplay.setTextColor(SH110X_WHITE);
  OLEDdisplay.println("  Medicine reminder");
  OLEDdisplay.println("       system");
  OLEDdisplay.display();
  // displayCurrentTime(time);
}

// Display current time on OLED.
void displayCurrentTime(String time) {
  OLEDdisplay.setCursor(0, 50);
  OLEDdisplay.setTextSize(1.5);
  OLEDdisplay.setTextColor(SH110X_WHITE);
  OLEDdisplay.fillRect(0, 50, SCREEN_WIDTH, 14, SH110X_BLACK);  // Clear previous time
  OLEDdisplay.setCursor((SCREEN_WIDTH - 6 * 8) / 2, 50);        // Center time horizontally
  OLEDdisplay.println(time);
  OLEDdisplay.display();
}

void Med_log(String med, bool taken, String time) {
  DateTime now = rtc.now();
  String date = twoDigits(now.day()) + "/" + twoDigits(now.month()) + "/" + String(now.year());
  if (med == FirstMedName) {
    writeFile("/log.txt", "---------------------------------------------");
    writeFile("/log.txt", "Medicine: " + FirstMedName);
    if (taken == true) {
      writeFile("/log.txt", "Taken at: " + date + " " + time);
    } else if (taken == false) {
      writeFile("/log.txt", "Missed at " + date + " " + time);
    }
  } else if (med == SecondMedName) {
    writeFile("/log.txt", "---------------------------------------------");
    writeFile("/log.txt", "Medicine: " + SecondMedName);
    if (taken == true) {
      writeFile("/log.txt", "Taken at: " + date + " " + time);
    } else if (taken == false) {
      writeFile("/log.txt", "Missed at " + date + " " + time);
    }
  } else if (med == ThirdMedName) {
    writeFile("/log.txt", "---------------------------------------------");
    writeFile("/log.txt", "Medicine: " + ThirdMedName);
    if (taken == true) {
      writeFile("/log.txt", "Taken at: " + date + " " + time);
    } else if (taken == false) {
      writeFile("/log.txt", "Missed at " + date + " " + time);
    }
  }
}

void writeFile(String path, String message) {
  Serial.printf("Writing to file: %s\n", path.c_str());

  // Open the file in read mode first to search for empty lines
  File file = SD.open(path, FILE_READ);
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }

  // Create a temporary String to store the file content
  String fileContent = "";
  bool emptyLineFound = false;

  // Read the file line by line
  while (file.available()) {
    String line = file.readStringUntil('\n');

    // Check if the line is empty (i.e., no characters)
    if (line.length() == 0 && !emptyLineFound) {
      // If empty line is found, add the message there
      fileContent += message + "\n";
      emptyLineFound = true;
    } else {
      // If not empty, add the existing line back to the content
      fileContent += line + "\n";
    }
  }

  // If no empty line was found, append the message at the end
  if (!emptyLineFound) {
    fileContent += message + "\n";
  }

  // Close the file after reading
  file.close();

  // Now reopen the file for writing and overwrite it
  file = SD.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }

  // Write the updated content back to the file
  if (file.print(fileContent)) {
    Serial.println("File written successfully");
  } else {
    Serial.println("Write failed");
  }

  // Close the file after writing
  file.close();
}

// Formatting function for time string to looki like this hh:mm:ss
String twoDigits(int num) {
  if (num < 10) {
    return "0" + String(num);
  }
  return String(num);
}

void debug() {
  if (digitalRead(butt3Pin) == HIGH) {
    Serial.println("butt3 put");
  }
  if (digitalRead(butt2Pin) == HIGH) {
    Serial.println("butt2 put");
  }
  if (digitalRead(butt1Pin) == HIGH) {
    Serial.println("butt1 put");
  }
  if (digitalRead(reedPin) == LOW) {
    Serial.println("open");
  }
}

void drawAnalogClock(DateTime now) {
  int clockCenterX = 62; // Center of the screen horizontally
  int clockCenterY = 33;  // Positioned lower to avoid overlapping with text

  // Clear the analog clock area
  OLEDdisplay.fillRect(clockCenterX - 12, clockCenterY - 12, 24, 24, SH110X_BLACK);

  // Draw clock face
  for (int i = 0; i < 2; i++) {
    OLEDdisplay.drawCircle(clockCenterX, clockCenterY, 11 - i, SH110X_WHITE);
  }

  int hour = now.hour();
  int minute = now.minute();
  int second = now.second();

  // Draw hour, minute, and second hands
  drawHand(clockCenterX, clockCenterY, hour * 30 + minute / 2, 5, SH110X_WHITE); // Hour hand
  drawHand(clockCenterX, clockCenterY, minute * 6, 10, SH110X_WHITE);            // Minute hand
  drawHand(clockCenterX, clockCenterY, second * 6, 11, SH110X_WHITE);            // Second hand

  OLEDdisplay.display();
}

void drawHand(int cx, int cy, int angle, int length, uint16_t color) {
  float radian = (angle - 90) * 0.0174533;
  int x = cx + cos(radian) * length;
  int y = cy + sin(radian) * length;
  OLEDdisplay.drawLine(cx, cy, x, y, color);
}

void displayAlert(){
  OLEDdisplay.clearDisplay();
  OLEDdisplay.setCursor(0, 0);
  OLEDdisplay.setTextSize(1.5);
  OLEDdisplay.setTextColor(SH110X_WHITE);
  OLEDdisplay.println("     Medicine Time!");
  OLEDdisplay.println("  Please Take (^_^)");  
  if(taketime1 & taketime2 & taketime3){
    OLEDdisplay.println(FirstMedName);
    OLEDdisplay.println(SecondMedName);
    OLEDdisplay.println(ThirdMedName);
  }
   else if(taketime1 & taketime2){
    OLEDdisplay.println(FirstMedName);
    OLEDdisplay.println(SecondMedName);
  }
   else if(taketime1 & taketime3){
    OLEDdisplay.println(FirstMedName);
    OLEDdisplay.println(ThirdMedName);
  }
   else if(taketime2 & taketime3){
    OLEDdisplay.println(SecondMedName);
    OLEDdisplay.println(ThirdMedName);
  }
   else if(taketime1){
    OLEDdisplay.println(FirstMedName);
  }
   else if(taketime2){
    OLEDdisplay.println(SecondMedName);
  }
   else if(taketime3){
    OLEDdisplay.println(ThirdMedName);
  }
  OLEDdisplay.display();
}