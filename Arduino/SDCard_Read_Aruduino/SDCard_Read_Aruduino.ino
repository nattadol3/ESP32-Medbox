#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <SD.h>
#include <SPI.h>
#include <RTClib.h>
#include <stdio.h>


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

// Arrays to keep track of notification time for each med
String notif1[4];
String notif2[4];
String notif3[4];

void setup() {
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

    // Close the file
    config.close();
    Serial.println("File closed.");

    // Output the content stored in the iniContent variable
    Serial.println("File contents:");
    Serial.println(iniContent);
  } else {
    Serial.println("Error opening config.in");
    return;
  }

  DateTime now = rtc.now();  // Get the current time
  String time = (twoDigits(now.hour()) + ":" + twoDigits(now.minute()) + ":" + twoDigits(now.second()));

 

  parseIni(iniContent);  // Save Config file contents into arrays and vars
  printMedications(); // Show medication info in terminal
   Med_log(FirstMedName, true, time);
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
  delay(1000);

  displayMedication(SecondMedName, SecondMedWhen, notif2);
  delay(1000);

  displayMedication(ThirdMedName, ThirdMedWhen, notif3);
  delay(1000);
}

void loop() {
  DateTime now = rtc.now();  // Get the current time
  String time = (twoDigits(now.hour()) + ":" + twoDigits(now.minute()) + ":" + twoDigits(now.second()));

  // Display time and system name on the OLED display
  displayReminder(time);
  displayCurrentTime(time);

  delay(1000);
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
void displayReminder(String time) {
  OLEDdisplay.clearDisplay();
  OLEDdisplay.setCursor(0, 0);
  OLEDdisplay.setTextSize(1.5);
  OLEDdisplay.setTextColor(SH110X_WHITE);
  OLEDdisplay.println("  Medicine reminder");
  OLEDdisplay.println("       system");
  OLEDdisplay.display();
  displayCurrentTime(time);
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

void Med_log(String med, bool taken, String time){
    DateTime now = rtc.now();
    String date = twoDigits(now.day()) + "/" + twoDigits(now.month()) + "/" + String(now.year());
    if (med == FirstMedName){
        writeFile("/log.txt", "Med taken: " + FirstMedName);
        if (taken == true){
            writeFile("/log.txt", "Taken at: " + date + " " + time);
        } else if (taken == false){
            writeFile("/log.txt", "Medication missed at " + date + " " + time);
        }
    }
    else if (med == SecondMedName){
        writeFile("/log.txt", "Med taken: " + SecondMedName);
        if (taken == true){
            writeFile("/log.txt", "Taken at: " + date + " " + time);
        } else if (taken == false){
            writeFile("/log.txt", "Medication missed at " + date + " " + time);
        }
    }
    else if (med == ThirdMedName){
        writeFile("/log.txt", "Med taken: " + ThirdMedName);
        if (taken == true){
            writeFile("/log.txt", "Taken at: " + date + " " + time);
        } else if (taken == false){
            writeFile("/log.txt", "Medication missed at " + date + " " + time);
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

String twoDigits(int num){
  if (num < 10){
    return "0" + String(num);
  }
  return String(num);
}