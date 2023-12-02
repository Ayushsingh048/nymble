#include <EEPROM.h>

const int BAUD_RATE = 2400;
const int BUFFER_SIZE = 256;

void setup() {
  Serial.begin(BAUD_RATE);
  while (!Serial) {
    ; // Wait for serial port to initialize
  }

  // Initialize EEPROM data index
  EEPROM.write(0, 0);
}

void loop() {
  static char receivedData[BUFFER_SIZE];
  static uint8_t dataIndex = 0;
  static bool transmissionComplete = false;

  // Check for incoming data
  while (Serial.available()) {
    char receivedChar = Serial.read();

    // Check for end of transmission
    if (receivedChar == '\n' && dataIndex > 0) {
      receivedData[dataIndex] = '\0';

      // Extract CRC from received data
      char *token = strtok(receivedData, ",");
      char *receivedText = strtok(NULL, ",");
      uint16_t crcFromPC = atoi(token);

      // Calculate CRC for the received text
      uint16_t calculatedCRC = calculateCRC(receivedText);

      // Check CRC
      if (crcFromPC == calculatedCRC) {
        // CRC is valid, store data in EEPROM
        storeInEEPROM(receivedText);
      }

      // Reset data index for the next transmission
      dataIndex = 0;

      // Check for end signal ("END")
      if (strcmp(receivedText, "END") == 0) {
        // Set flag to indicate end of transmission
        transmissionComplete = true;
      }
    } else {
      // Regular data, store in buffer
      receivedData[dataIndex] = receivedChar;
      dataIndex = (dataIndex + 1) % BUFFER_SIZE;
    }
  }

  // If transmission is complete, send stored data back to PC
  if (transmissionComplete) {
    sendDataBackToPC();
    // Reset flag for the next transmission
    transmissionComplete = false;
  }
}

uint16_t calculateCRC(const char *data) {
  const uint16_t polynomial = 0x8005;
  uint16_t crc = 0xFFFF; // Initial CRC value

  for (size_t i = 0; i < strlen(data); i++) {
    crc ^= (static_cast<uint16_t>(data[i]) << 8);

    for (int j = 0; j < 8; ++j) {
      if (crc & 0x8000) {
        crc = (crc << 1) ^ polynomial;
      } else {
        crc <<= 1;
      }
    }
  }

  return crc;
}

void storeInEEPROM(const char *data) {
  // Calculate length of data
  uint8_t len = strlen(data);

  // Get the current EEPROM data index
  uint8_t dataIndex = EEPROM.read(0);

  // Write data to EEPROM
  for (uint8_t i = 0; i < len; i++) {
    EEPROM.write(dataIndex, data[i]);
    dataIndex = (dataIndex + 1) % BUFFER_SIZE;
  }

  // Update EEPROM data index
  EEPROM.write(0, dataIndex);
}

void sendDataBackToPC() {
  // Read and send stored data back to PC
  for (uint8_t i = 0; i < BUFFER_SIZE; i++) {
    char dataByte = EEPROM.read(i);
    Serial.write(dataByte);
  }

  // Signal end of data
  Serial.write("END\n");
}
