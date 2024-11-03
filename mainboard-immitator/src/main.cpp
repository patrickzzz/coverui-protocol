#include <HardwareSerial.h>
#include <vector>
#include <cstdint>

#define MAX_MESSAGE_LENGTH 256  // max length of message
#define TIMEOUT_DURATION 5000   // Timeout in ms

void tryHandshake();
void processHandshake();
std::vector<uint8_t> getHandshakeResponse(const char* message, int length);

void processCoverUIMessages();
void processMainboardMessages();

bool processIncomingSerialMessages(HardwareSerial &serial, char *messageBuffer, int &messageLength);
bool isCompleteMessage(const char* message, int length);
bool hasCorrectChecksum(const char* message, int length);

void addChecksumToMessage(std::vector<uint8_t>& message);
void sendMessage(const std::vector<uint8_t> &response);

struct HandshakeMessageResponsePair {
    std::vector<uint8_t> expectedMessage;
    std::vector<uint8_t> response;
    bool sent;
};

HandshakeMessageResponsePair handshakeMessageResponses[] = {
    {{0x55, 0xAA, 0x03, 0x40, 0x01, 0x00, 0x43}, {0x55, 0xAA, 0x02, 0xFF, 0xFF, 0xFF}, false},
    {{0x55, 0xAA, 0x02, 0xFF, 0xFF, 0xFF}, {0x55, 0xAA, 0x02, 0xFF, 0xFE, 0xFE}, false},
    {{0x55, 0xAA, 0x02, 0xFF, 0xFE, 0xFE}, {0x55, 0xAA, 0x05, 0xFF, 0xFD, 0x06, 0x50, 0x20, 0x76}, false},
    {{0x55, 0xAA, 0x03, 0xFF, 0xFD, 0x06, 0x04}, {0x55, 0xAA, 0x02, 0xFF, 0xFB, 0xFB}, false},
    {{0x55, 0xAA, 0x1B, 0xFF, 0xFB, 0x52, 0x4D, 0x20, 0x45, 0x43, 0x34, 0x5F, 0x56, 0x31, 0x2E, 0x30, 0x30, 0x5F, 0x32, 0x30, 0x32, 0x30, 0x28, 0x32, 0x30, 0x30, 0x39, 0x33, 0x30, 0x29, 0xA5}, {0x55, 0xAA, 0x02, 0xFF, 0xFB, 0xFB}, false},
    {{0x55, 0xAA, 0x1B, 0xFF, 0xFB, 0x52, 0x4D, 0x20, 0x45, 0x43, 0x34, 0x5F, 0x56, 0x31, 0x2E, 0x30, 0x30, 0x5F, 0x32, 0x30, 0x32, 0x30, 0x28, 0x32, 0x30, 0x30, 0x39, 0x33, 0x30, 0x29, 0xA5}, {0x55, 0xAA, 0x02, 0x00, 0x00, 0x01}, false},
    {{0x55, 0xAA, 0x03, 0x40, 0x01, 0x00, 0x43}, {0x0}, false},
};

bool handshakeSuccessful = false;
const int handshakeMessageResponsesCount = sizeof(handshakeMessageResponses) / sizeof(handshakeMessageResponses[0]);

std::vector<uint8_t> defaultStatusMessage = {0x55, 0xAA, 0x15, 0x50, 0x8E, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00};
std::vector<uint8_t> onlyMonday = {0x55, 0xAA, 0x15, 0x50, 0x8E, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x10, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
std::vector<uint8_t> onlyTuesday = {0x55, 0xAA, 0x15, 0x50, 0x8E, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x10, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
std::vector<uint8_t> onlyClock = {0x55, 0xAA, 0x15, 0x50, 0x8E, 0x0, 0x0, 0x0, 0x0, 0x20, 0x20, 0x20, 0x20, 0x0, 0x0, 0x0, 0x0, 0x00, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
std::vector<uint8_t> currentStatusMessage = {};

void setup() {
    Serial.begin(115200);
    //Serial1.begin(115200, SERIAL_8N1, 18, 19); // Pins for UART1, RX=18, TX=19
    Serial1.begin(115200, SERIAL_8N1, 18, 16); // Pins for UART1, RX=18, TX=19

    Serial.println("Starting mainboard immitator..");
    Serial.println("Waiting for other side to start handshake..");

    // Wait for Serial1
    while (!Serial1.available()) {
        delay(100);
    }

    // Send start sequence
    Serial1.write(0x00);
    Serial1.write(0xFF);
    Serial.println("Start sequence sent: 0x00 0xFF");

    // set current status message to default
    addChecksumToMessage(defaultStatusMessage);
    addChecksumToMessage(onlyMonday);
    addChecksumToMessage(onlyTuesday);
    addChecksumToMessage(onlyClock);

    currentStatusMessage = defaultStatusMessage;
}

void loop() {
    if(!handshakeSuccessful) {
        // also check, if new handshake is needed (CoverUI will resend the handshake message)
        tryHandshake();
    }

/*
    // print current status message
    Serial.print("Current status message: ");
    for (int i = 0; i < currentStatusMessage.size(); i++) {
        Serial.print(currentStatusMessage[i], HEX);
        Serial.print(" ");
    }
    Serial.println();
*/

    // process messages from cover ui
    processCoverUIMessages();

    // process messages from mainboard
    processMainboardMessages();

    delay(25);
}

void processMainboardMessages() {
    char messageBuffer[MAX_MESSAGE_LENGTH];
    int messageLength = 0;

    sendMessage(currentStatusMessage);

    sendMessage({
        0x55, 0xAA, 0x02, 0x50, 0x62, 0xB3
    });

    sendMessage({
        0x55, 0xAA, 0x05, 0x50, 0x84, 0x00, 0x01, 0x01, 0xDA
    });
}

void processCoverUIMessages() {
    char messageBuffer[MAX_MESSAGE_LENGTH];
    int messageLength = 0;

    while (Serial1.available()) {
        if (processIncomingSerialMessages(Serial1, messageBuffer, messageLength)) {
          // skip handling for now and just ignore the message
          //handleCoverUIMessage(messageBuffer, messageLength);
          std::vector<uint8_t> mondayButtonMsg = {
              0x55, 0xAA, 0x0F, 0x50, 0x62, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x02, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0xC2
          };


          std::vector<uint8_t> tuesdayButtonMsg = {
              0x55, 0xAA, 0x0F, 0x50, 0x62, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x02,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0xC2
          };

          std::vector<uint8_t> okButtonMsg = {
               0x55, 0xAA, 0x0F, 0x50, 0x62, 0x00,
               0x00, 0x00, 0x00, 0x02, 0x00, 0x00,
               0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
               0xC2
           };

          std::vector<uint8_t> clockButtonMsg = {
               0x55, 0xAA, 0x0F, 0x50, 0x62, 0x02,
               0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
               0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
               0xC2
           };

          if(memcmp(messageBuffer, mondayButtonMsg.data(), mondayButtonMsg.size()) == 0) {
              currentStatusMessage = onlyMonday;
          }

          if(memcmp(messageBuffer, tuesdayButtonMsg.data(), tuesdayButtonMsg.size()) == 0) {
              currentStatusMessage = onlyTuesday;
          }

          if(memcmp(messageBuffer, okButtonMsg.data(), okButtonMsg.size()) == 0) {
              currentStatusMessage = defaultStatusMessage;
          }

          if(memcmp(messageBuffer, clockButtonMsg.data(), clockButtonMsg.size()) == 0) {
              currentStatusMessage = onlyClock;
          }

          // print out message
          Serial.print("Received message from CoverUI: ");

          bool isFirst = true;
          for (int i = 0; i < messageLength; i++) {
              if (isFirst) {
                  isFirst = false;
              } else if(messageBuffer[i] == 0x55 && i < messageLength - 1 && messageBuffer[i + 1] == 0xAA) {
                  Serial.println();
              }

              Serial.print(messageBuffer[i], HEX);
              Serial.print(" ");
          }
          Serial.println();
        }
    }
}

void tryHandshake() {
    while (!handshakeSuccessful) {
        Serial.println("Trying to handshake..");
        processHandshake();

        if(!handshakeSuccessful) {
            Serial.println("Handshake was not successfull! Retrying in 3 seconds..");
            delay(3000);
        }
    }
}

void processHandshake() {
    char messageBuffer[MAX_MESSAGE_LENGTH];
    int messageLength = 0;

     // set all response sent flags to false
    for (int i = 0; i < handshakeMessageResponsesCount; i++) {
        handshakeMessageResponses[i].sent = false;
    }

    // handshake is initialized and closed by CoverUI
    // so we react to responses of the CoverUI
    while (!handshakeSuccessful) {
        if (processIncomingSerialMessages(Serial1, messageBuffer, messageLength)) {
            std::vector<uint8_t> response = getHandshakeResponse(messageBuffer, messageLength);

            if(response.size() == 0) {
              Serial.print("Received handshake message was unknown: ");
              for (int i = 0; i < messageLength; i++) {
                  Serial.print(messageBuffer[i], HEX);
                  Serial.print(" ");
              }
              Serial.println();

              return;  // treat handshake as failed
            }else if(response.size() == 1 && response[0] == 0x0) {
              Serial.println("Handshake was successfull!");
              handshakeSuccessful = true;
              return;
            }

            sendMessage(response);
        }
    }

    // handshake was not successfull.. Normally, CoverUI resends the handshake messages..
    Serial.println("Handshake was not successfull! Retrying in 3 seconds..");
    delay(3000);
    return;
}

bool processIncomingSerialMessages(HardwareSerial &serial, char *messageBuffer, int &messageLength) {
    static char message[MAX_MESSAGE_LENGTH];
    static int currentLength = 0;
    static unsigned long lastReceiveTime = 0;

    while (serial.available()) {
        int data = serial.read();
        lastReceiveTime = millis();  // Update last receive time

        if (currentLength == 0 && data != 0x55) {
            continue;
        }

        if (currentLength < MAX_MESSAGE_LENGTH) {
            message[currentLength++] = data;

            if (isCompleteMessage(message, currentLength)) {
                if (!hasCorrectChecksum(message, currentLength)) {
                    currentLength = 0;
                    memset(message, 0, MAX_MESSAGE_LENGTH);
                    continue;
                }

                memcpy(messageBuffer, message, currentLength);
                messageLength = currentLength;

                currentLength = 0;
                memset(message, 0, MAX_MESSAGE_LENGTH);

                return true;
            }
        } else {
            Serial.println("Message buffer overflow! Message was: ");
            for (int i = 0; i < currentLength; i++) {
                Serial.print(message[i], HEX);
                Serial.print(" ");
            }
            Serial.println();
            currentLength = 0;
            memset(message, 0, MAX_MESSAGE_LENGTH);
        }
    }

    if (millis() - lastReceiveTime > TIMEOUT_DURATION && currentLength > 0) {
        Serial.print("Message complete due to timeout: ");
        for (int i = 0; i < currentLength; i++) {
            Serial.print(message[i], HEX);
            Serial.print(" ");
        }
        Serial.println();
        currentLength = 0;
    }

    return false;
}

bool isCompleteMessage(const char* message, int length) {
    if (length < 4) return false; // Minimum length is 4

    // check header
    int expectedLength = message[2] + 4; // message[2] = length of message, + 4 [55 AA Length {msg} Checksum]
    return (length >= expectedLength);

    return false;
}

/*
 * The checksum is calculated such that the sum of all bytes (including the checksum)
 * modulo 256 equals zero. This ensures the integrity of the transmitted message.
 */
bool hasCorrectChecksum(const char* message, int length) {
    if (length < 4) return false; // Minimum length for valid message is 4 bytes

    // Sum of all bytes except the last one (checksum)
    int calculatedChecksum = 0;
    for (int i = 0; i < length - 1; ++i) {
        calculatedChecksum += static_cast<unsigned char>(message[i]);
    }

    // Calculate checksum using modulo 256
    calculatedChecksum = calculatedChecksum % 256; // Instead of using & 0xFF, use modulo 256
    int messageChecksum = static_cast<unsigned char>(message[length - 1]);

    if(calculatedChecksum == messageChecksum) {
      return true;
    }else{
      return false;
    }
}

std::vector<uint8_t> getHandshakeResponse(const char* message, int length) {
    for (int i = 0; i < handshakeMessageResponsesCount; i++) {
        bool isMatch = memcmp(message, handshakeMessageResponses[i].expectedMessage.data(), handshakeMessageResponses[i].expectedMessage.size()) == 0;
        if (isMatch) {
          if(!handshakeMessageResponses[i].sent) {
              handshakeMessageResponses[i].sent = true;
          } else {
              continue;
          }

            return handshakeMessageResponses[i].response;
        }
    }

    return {};
}

void addChecksumToMessage(std::vector<uint8_t>& message) {
    int checksum = 0;

    for (size_t i = 0; i < message.size(); i++) {
        checksum += message[i];
    }

    checksum = checksum % 256;
    message.push_back(static_cast<uint8_t>(checksum));
}

// create send response that can take handshakeMessageResponses[i].response as parameter
void sendMessage(const std::vector<uint8_t> &response) {
    Serial.print("Sending message: ");

    bool isFirst = true;
    for (int i = 0; i < response.size(); i++) {
        if (isFirst) {
            isFirst = false;
        } else if(response[i] == 0x55 && i < response.size() - 1 && response[i + 1] == 0xAA) {
            Serial.println();
        }
        Serial1.write(response[i]);
        Serial.print(response[i], HEX);
        Serial.print(" ");
    }

    Serial.println();
}