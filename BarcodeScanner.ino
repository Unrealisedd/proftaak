#include <SoftwareSerial.h>

const int SCANNER_RX = 2;
const int SCANNER_TX = 3;

SoftwareSerial scanner(SCANNER_RX, SCANNER_TX);

void setup() {
    Serial.begin(9600);
    scanner.begin(9600);
    Serial.println("Barcode Scanner Test Started");
}

void loop() {
    if (scanner.available()) {
        String barcode = scanner.readStringUntil('\n');
        Serial.print("Scanned Barcode: ");
        Serial.println(barcode);
    }
}
