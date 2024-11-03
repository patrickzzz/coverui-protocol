#include <HardwareSerial.h>

#define MAX_MESSAGE_LENGTH 256
#define MAX_PRINTED_MESSAGES 100

struct PrintedMessage {
    char message[MAX_MESSAGE_LENGTH];
    int length;
    char sender;
    unsigned long lastPrintedTime;
};

void processIncomingSerialMessages(HardwareSerial &serial, char sender);
bool isCompleteMessage(const char* message, int length);
bool hasCorrectChecksum(const char* message, int length);
void printMessage(const char* message, int length, char sender);
bool isMessagePrinted(const char* message, int length, char sender);
void addPrintedMessage(const char* message, int length, char sender);

PrintedMessage printedMessages[MAX_PRINTED_MESSAGES];
int printedMessageCount = 0;

void setup() {
    Serial.begin(115200);
    Serial1.begin(115200, SERIAL_8N1, 16, 17); // Pins for UART1, RX=16, TX=17
    Serial2.begin(115200, SERIAL_8N1, 18, 19); // Pins for UART2, RX=18, TX=19

    Serial.println("Starting protocol listener..");
}

void loop() {
    processIncomingSerialMessages(Serial1, 'A');
    processIncomingSerialMessages(Serial2, 'B');
}

void processIncomingSerialMessages(HardwareSerial &serial, char sender) {
    static char message[MAX_MESSAGE_LENGTH];
    static int messageLength = 0;

    while (serial.available()) {
        int data = serial.read();

        if (messageLength == 0 && data != 0x55) {
            continue;
        }

        if (messageLength < MAX_MESSAGE_LENGTH) {
            message[messageLength++] = data;

            if (isCompleteMessage(message, messageLength)) {
                if (!hasCorrectChecksum(message, messageLength)) {
                    messageLength = 0;
                    memset(message, 0, MAX_MESSAGE_LENGTH);
                    continue;
                }

                // decide if duplicates shall be printed
                if (true || !isMessagePrinted(message, messageLength, sender)) {
                    printMessage(message, messageLength, sender);
                    addPrintedMessage(message, messageLength, sender);
                }

                messageLength = 0;
                memset(message, 0, MAX_MESSAGE_LENGTH);
            }
        } else {
            Serial.println("Message buffer overflow! Message was: ");
            for (int i = 0; i < messageLength; i++) {
                Serial.print(message[i], HEX);
                Serial.print(" ");
            }
            Serial.println();
            // Reset
            messageLength = 0;
            memset(message, 0, MAX_MESSAGE_LENGTH);
        }
    }
}

void printMessage(const char* message, int length, char sender) {
    Serial.print(sender);
    Serial.print(": ");
    for (int i = 0; i < length; i++) {
        Serial.print(message[i], HEX);
        Serial.print(" ");
    }
    Serial.println();
}

bool isCompleteMessage(const char* message, int length) {
    if (length < 4) return false;

    int expectedLength = message[2] + 4;
    return (length >= expectedLength);
}

bool hasCorrectChecksum(const char* message, int length) {
    if (length < 4) return false;

    int calculatedChecksum = 0;
    for (int i = 0; i < length - 1; ++i) {
        calculatedChecksum += static_cast<unsigned char>(message[i]);
    }

    calculatedChecksum = calculatedChecksum % 256;
    int messageChecksum = static_cast<unsigned char>(message[length - 1]);

    return (calculatedChecksum == messageChecksum);
}

bool isMessagePrinted(const char* message, int length, char sender) {
    for (int i = 0; i < printedMessageCount; i++) {
        if (printedMessages[i].sender == sender &&
            printedMessages[i].length == length &&
            memcmp(printedMessages[i].message, message, length) == 0) {

            unsigned long timeSinceLastPrint = millis() - printedMessages[i].lastPrintedTime;
            /*
            Serial.print("[DUPLICATE] Message was printed ");
            Serial.print(timeSinceLastPrint);
            Serial.print(" ms ago: ");

            // print message
            printMessage(message, length, sender);
            */

            printedMessages[i].lastPrintedTime = millis();
            return true;
        }
    }
    return false;
}

void addPrintedMessage(const char* message, int length, char sender) {
    if (printedMessageCount < MAX_PRINTED_MESSAGES) {
        memcpy(printedMessages[printedMessageCount].message, message, length);
        printedMessages[printedMessageCount].length = length;
        printedMessages[printedMessageCount].sender = sender;
        printedMessages[printedMessageCount].lastPrintedTime = millis();
        printedMessageCount++;
    } else {
        memcpy(printedMessages[0].message, message, length);
        printedMessages[0].length = length;
        printedMessages[0].sender = sender;
        printedMessages[0].lastPrintedTime = millis();

        for (int i = 1; i < MAX_PRINTED_MESSAGES; i++) {
            printedMessages[i - 1] = printedMessages[i];
        }
    }
}