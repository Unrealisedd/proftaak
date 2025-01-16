#include <WiFiS3.h>

const char* ssid = "Netlab-OIL430";
const char* password = "DesignChallenge";

void setup() {
    Serial.begin(9600);
    Serial.println("WiFi Test Started");
    
    WiFi.begin(ssid, password);
    
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    
    Serial.println("\nConnected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
}

void loop() {
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("WiFi Connected - Signal Strength: " + String(WiFi.RSSI()) + " dBm");
    } else {
        Serial.println("WiFi Disconnected");
    }
    delay(2000);
}
