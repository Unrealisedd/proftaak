#include <Servo.h>
#include <SoftwareSerial.h>
#include <WiFiS3.h>
#include <ArduinoJson.h>

// Configuration
namespace Config {
    // WiFi credentials
    const char* wifinaam = "Netlab-OIL430";
    const char* wifipass = "DesignChallenge";

    // Server details
    const char* API_IP = "192.168.68.51";
    const int API_PORT = 5000;
    const char* API_ENDPOINT = "/api/BottleReturn/process";

    // System parameters
    const int baud = 9600;
    const int checkinterval = 100;
    const int servodelay = 5000;
    const float mindistance = 8.0;
    const float maxdistance = 12.0;

    // Pin configurations
    namespace Pins {
        const int TRIG_PIN = 9;
        const int ECHO_PIN = 10;
        const int SERVO_PIN = 11;
        const int RX = 2;
        const int TX = 3;
    }

    // Servo positions
    namespace ServoPositions {
        const int extend = 1000;
        const int retract = 2000;
    }
}

class DistanceSensor {
private:
    const int trigPin;
    const int echoPin;
    const float minDistance;
    const float maxDistance;

public:
    DistanceSensor() 
        : trigPin(Config::Pins::TRIG_PIN),
          echoPin(Config::Pins::ECHO_PIN),
          minDistance(Config::mindistance),
          maxDistance(Config::maxdistance) {}

    void init() {
        pinMode(trigPin, OUTPUT);
        pinMode(echoPin, INPUT);
    }

    float readDistance() {
        digitalWrite(trigPin, LOW);
        delayMicroseconds(2);
        digitalWrite(trigPin, HIGH);
        delayMicroseconds(10);
        digitalWrite(trigPin, LOW);
        
        long duration = pulseIn(echoPin, HIGH);
        return duration * 0.034 / 2;
    }

    bool isObjectInRange() {
        float distance = readDistance();
        return (distance >= minDistance && distance <= maxDistance);
    }
};

class BarcodeScanner {
private:
    SoftwareSerial scanner;
    const int baudRate;

public:
    BarcodeScanner()
        : scanner(Config::Pins::RX, Config::Pins::TX),
          baudRate(Config::baud) {}

    void init() {
        scanner.begin(baudRate);
    }

    String readBarcode() {
        String barcode = scanner.readStringUntil('\n');
        barcode.trim();
        return barcode;
    }

    bool isDataAvailable() {
        return scanner.available();
    }
};

class ServoController {
private:
    Servo actuator;
    bool isExtended;
    unsigned long lastActivationTime;

public:
    ServoController() : isExtended(false), lastActivationTime(0) {}

    void init() {
        actuator.attach(Config::Pins::SERVO_PIN);
        retract();
    }

    void extend() {
        actuator.writeMicroseconds(Config::ServoPositions::extend);
        isExtended = true;
        lastActivationTime = millis();
    }

    void retract() {
        actuator.writeMicroseconds(Config::ServoPositions::retract);
        isExtended = false;
    }

    void update() {
        if (isExtended && (millis() - lastActivationTime >= Config::servodelay)) {
            retract();
        }
    }

    bool getIsExtended() const {
        return isExtended;
    }
};

class NetworkManager {
private:
    WiFiClient wifiClient;
    const unsigned long timeout;

    bool sendHttpRequest(const String& payload) {
        if (wifiClient.connect(Config::API_IP, Config::API_PORT)) {
            String postRequest = "POST " + String(Config::API_ENDPOINT) + " HTTP/1.1\r\n" +
                               "Host: " + String(Config::API_IP) + "\r\n" +
                               "Content-Type: application/json\r\n" +
                               "Content-Length: " + String(payload.length()) + "\r\n" +
                               "Connection: close\r\n\r\n" +
                               payload;

            wifiClient.println(postRequest);
            
            unsigned long requestTime = millis();
            while (millis() - requestTime < timeout) {
                if (wifiClient.available()) {
                    wifiClient.readString();
                    wifiClient.stop();
                    return true;
                }
            }
            wifiClient.stop();
        }
        return false;
    }

public:
    NetworkManager() : timeout(5000) {}

    bool connect() {
        WiFi.begin(Config::wifinaam, Config::wifipass);
        unsigned long startTime = millis();
        
        while (WiFi.status() != WL_CONNECTED && millis() - startTime < timeout) {
            delay(500);
        }
        
        return WiFi.status() == WL_CONNECTED;
    }

    bool checkApiConnection() {
        if (wifiClient.connect(Config::API_IP, Config::API_PORT)) {
            String getRequest = String("GET /api/BottleReturn/status HTTP/1.1\r\n") +
                              "Host: " + String(Config::API_IP) + "\r\n" +
                              "Connection: close\r\n\r\n";
            
            wifiClient.println(getRequest);
            
            unsigned long requestTime = millis();
            while (millis() - requestTime < timeout) {
                if (wifiClient.available()) {
                    String response = wifiClient.readString();
                    wifiClient.stop();
                    return response.indexOf("Systeem werkt") != -1;
                }
            }
            wifiClient.stop();
        }
        return false;
    }

    bool sendBottleData(const String& barcode, bool servoActivated) {
        StaticJsonDocument<200> doc;
        doc["barcode"] = barcode;
        doc["servoactivated"] = servoActivated;
        doc["scanner_ID"] = "1";
        
        String jsonString;
        serializeJson(doc, jsonString);
        
        return sendHttpRequest(jsonString);
    }
};

class BottleReturnSystem {
private:
    DistanceSensor distanceSensor;
    BarcodeScanner barcodeScanner;
    ServoController servoController;
    NetworkManager networkManager;
    unsigned long lastDistanceCheck;

public:
    BottleReturnSystem() : lastDistanceCheck(0) {}

    void init() {
        Serial.begin(Config::baud);
        distanceSensor.init();
        barcodeScanner.init();
        servoController.init();
        
        Serial.println("Connecting to WiFi...");
        if (!networkManager.connect()) {
            Serial.println("WiFi connection failed!");
            return;
        }
        
        Serial.println("Checking API connection...");
        if (!networkManager.checkApiConnection()) {
            Serial.println("API connection failed!");
            return;
        }
        
        Serial.println("System initialized successfully!");
    }

    void update() {
        if (barcodeScanner.isDataAvailable()) {
            String barcode = barcodeScanner.readBarcode();
            if (barcode.length() > 0) {
                networkManager.sendBottleData(barcode, servoController.getIsExtended());
            }
        }
        
        if (millis() - lastDistanceCheck >= Config::checkinterval) {
            if (distanceSensor.isObjectInRange() && !servoController.getIsExtended()) {
                servoController.extend();
            }
            lastDistanceCheck = millis();
        }
        
        servoController.update();
    }
};

BottleReturnSystem bottleSystem;

void setup() {
    bottleSystem.init();
}

void loop() {
    bottleSystem.update();
}
