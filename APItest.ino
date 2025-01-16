#include <WiFiS3.h>
#include <ArduinoJson.h>

const char* ssid = "Netlab-OIL430";
const char* password = "DesignChallenge";
const char* serverIP = "192.168.68.51";
const int serverPort = 5000;

WiFiClient client;

void setup() {
    Serial.begin(9600);
    WiFi.begin(ssid, password);
    
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }
    
    Serial.println("API Test Started");
}

void loop() {
    StaticJsonDocument<200> doc;
    doc["barcode"] = "TEST123";
    doc["servoactivated"] = true;
    doc["scanner_ID"] = "1";
    
    String jsonString;
    serializeJson(doc, jsonString);
    
    if (client.connect(serverIP, serverPort)) {
        Serial.println("Sending test request...");
        
        client.println("POST /api/BottleReturn/process HTTP/1.1");
        client.println("Host: " + String(serverIP));
        client.println("Content-Type: application/json");
        client.println("Content-Length: " + String(jsonString.length()));
        client.println();
        client.println(jsonString);
        
        while (client.available()) {
            String response = client.readString();
            Serial.println("Response: " + response);
        }
        
        client.stop();
    }
    
    delay(5000);
}
