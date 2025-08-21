#include <HardwareSerial.h>
#include <SPI.h>
#include <SD.h>

// MQ-4 methane sensor pin
#define MQ4_PIN 34  

// SD card module pins (adjust CS pin as per your module)
#define SD_CS 5  

// Bluetooth Serial (UART1 on ESP32)
HardwareSerial BT(1); // RX=16, TX=17

File dataFile;

// Calibration constants for MQ-4 (example values – adjust after testing)
// Rs/R0 ratio vs. PPM curve from datasheet (log-log approximation)
float R0 = 9.83; // Sensor resistance in clean air (to be calibrated)

float readMQ4() {
  int adcValue = analogRead(MQ4_PIN);
  float voltage = (adcValue / 4095.0) * 3.3; // ESP32 ADC is 12-bit (0–4095)
  float Rs = (3.3 - voltage) / voltage;     // Load resistor = 1k assumed
  float ratio = Rs / R0;

  // Approximation: CH4 PPM = 1000 * pow(ratio, -1.5) [from datasheet curve]
  float ppm = 1000 * pow(ratio, -1.5);
  return ppm;
}

void setup() {
  Serial.begin(115200);
  BT.begin(9600, SERIAL_8N1, 16, 17); // RX=16, TX=17

  // Initialize SD card
  if (!SD.begin(SD_CS)) {
    Serial.println("SD card init failed!");
    return;
  }
  Serial.println("SD card initialized.");

  // Create/open file and add header if new
  dataFile = SD.open("/methane_log.csv", FILE_APPEND);
  if (dataFile) {
    if (dataFile.size() == 0) { // add header only once
      dataFile.println("Time(ms), Methane(ppm)");
    }
    dataFile.close();
  }

  Serial.println("System ready. Logging methane data...");
}

void loop() {
  // Read methane concentration
  float methanePPM = readMQ4();

  // Timestamp
  unsigned long timestamp = millis();

  // Save to SD card
  dataFile = SD.open("/methane_log.csv", FILE_APPEND);
  if (dataFile) {
    dataFile.print(timestamp);
    dataFile.print(",");
    dataFile.println(methanePPM, 2);
    dataFile.close();
  }

  // Send via Bluetooth
  BT.print("Time: ");
  BT.print(timestamp);
  BT.print(" ms, Methane: ");
  BT.print(methanePPM, 2);
  BT.println(" ppm");

  delay(2000); // log every 2 sec
}
