#include <Servo.h>
#include <SoftwareSerial.h>
#include <WiFiS3.h>
#include <ArduinoJson.h>
#include <SD.h>
#include <SPI.h>

namespace Config {
    // WiFi credentials
    const char* WIFI_SSID = "Netlab-OIL430";
    const char* WIFI_PASSWORD = "DesignChallenge";

    // Server details
    const char* API_IP = "192.168.122.29";
    const int API_PORT = 5000;
    const char* API_ENDPOINT = "/api/BottleReturn/process";

    // System parameters
    const int BAUD_RATE = 9600;
    const int CHECK_INTERVAL = 100;
    const int SERVO_DELAY = 5000;
    const float MIN_DISTANCE = 8.0;
    const float MAX_DISTANCE = 12.0;

    // Pin configurations
    namespace Pins {
        const int TRIG_PIN = 9;
        const int ECHO_PIN = 10;
        const int SERVO_PIN = 11;
        const int SCANNER_RX = 2;
        const int SCANNER_TX = 3;
    }

    // Servo positions
    namespace ServoPositions {
        const int EXTEND = 1000;
        const int RETRACT = 2000;
    }

    // SD Card settings
    namespace SDCard {
        const int CS_PIN = 4;  // Chip Select pin for SD card
        const String LOG_FILE = "system.log";
        const unsigned long WRITE_INTERVAL = 5000; // Write to SD every 5 seconds
    }
}

class Logger {
private:
    static String buffer;
    static unsigned long lastWrite;
    
    static void writeBufferToSD() {
        if (buffer.length() == 0) return;
        
        File logFile = SD.open(Config::SDCard::LOG_FILE, FILE_WRITE);
        if (logFile) {
            logFile.println(buffer);
            logFile.close();
            buffer = "";
        }
    }

public:
    static void init() {
        if (!SD.begin(Config::SDCard::CS_PIN)) {
            Serial.println("SD card initialization failed!");
            return;
        }
        Serial.println("SD card initialized successfully");
    }

    static void log(const String& message) {
        String timestamp = "[" + String(millis()) + "] ";
        String logMessage = timestamp + message;
        
        // Print to Serial
        Serial.println(logMessage);
        
        // Add to buffer
        buffer += logMessage + "\n";
        
        // Check if it's time to write to SD
        if (millis() - lastWrite >= Config::SDCard::WRITE_INTERVAL) {
            writeBufferToSD();
            lastWrite = millis();
        }
    }

    static void flush() {
        writeBufferToSD();
    }
};

// Initialize static members
String Logger::buffer = "";
unsigned long Logger::lastWrite = 0;

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
          minDistance(Config::MIN_DISTANCE),
          maxDistance(Config::MAX_DISTANCE) {}

    void init() {
        pinMode(trigPin, OUTPUT);
        pinMode(echoPin, INPUT);
        Logger::log("Distance sensor initialized");
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
        if (distance >= minDistance && distance <= maxDistance) {
            Logger::log("Object detected at distance: " + String(distance) + " cm");
            return true;
        }
        return false;
    }
};

class BarcodeScanner {
private:
    SoftwareSerial scanner;
    const int baudRate;

public:
    BarcodeScanner()
        : scanner(Config::Pins::SCANNER_RX, Config::Pins::SCANNER_TX),
          baudRate(Config::BAUD_RATE) {}

    void init() {
        scanner.begin(baudRate);
        Logger::log("Barcode scanner initialized");
    }

    String readBarcode() {
        String barcode = scanner.readStringUntil('\n');
        barcode.trim();
        if (barcode.length() > 0) {
            Logger::log("Barcode read: " + barcode);
        }
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
        Logger::log("Servo controller initialized");
    }

    void extend() {
        actuator.writeMicroseconds(Config::ServoPositions::EXTEND);
        isExtended = true;
        lastActivationTime = millis();
        Logger::log("Servo extended");
    }

    void retract() {
        actuator.writeMicroseconds(Config::ServoPositions::RETRACT);
        isExtended = false;
        Logger::log("Servo retracted");
    }

    void update() {
        if (isExtended && (millis() - lastActivationTime >= Config::SERVO_DELAY)) {
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
        Logger::log("Sending HTTP request...");
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
                    Logger::log("HTTP request successful");
                    return true;
                }
            }
            wifiClient.stop();
            Logger::log("HTTP request timeout");
        }
        Logger::log("HTTP request failed");
        return false;
    }

public:
    NetworkManager() : timeout(5000) {}

    bool connect() {
        Logger::log("Connecting to WiFi network: " + String(Config::WIFI_SSID));
        WiFi.begin(Config::WIFI_SSID, Config::WIFI_PASSWORD);
        unsigned long startTime = millis();
        
        while (WiFi.status() != WL_CONNECTED && millis() - startTime < timeout) {
            delay(500);
        }
        
        if (WiFi.status() == WL_CONNECTED) {
            Logger::log("WiFi connected. IP: " + WiFi.localIP().toString());
            return true;
        }
        Logger::log("WiFi connection failed");
        return false;
    }

    bool checkApiConnection() {
        Logger::log("Checking API connection...");
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
                    bool success = response.indexOf("Systeem werkt") != -1;
                    Logger::log(success ? "API connection successful" : "API connection failed");
                    return success;
                }
            }
            wifiClient.stop();
        }
        Logger::log("API connection failed");
        return false;
    }

    bool sendBottleData(const String& barcode, bool servoActivated) {
        Logger::log("Processing barcode: " + barcode);
        StaticJsonDocument<200> doc;
        doc["barcode"] = barcode;
        doc["servoactivated"] = servoActivated;
        doc["scanner_ID"] = "1";
        
        String jsonString;
        serializeJson(doc, jsonString);
        Logger::log("Sending data: " + jsonString);
        
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
    unsigned long lastStatusLog;
    const unsigned long STATUS_LOG_INTERVAL = 30000;

public:
    BottleReturnSystem() : lastDistanceCheck(0), lastStatusLog(0) {}

    void init() {
        Serial.begin(Config::BAUD_RATE);
        Logger::init();  // Initialize SD card logging
        Logger::log("Initializing Bottle Return System");
        
        distanceSensor.init();
        barcodeScanner.init();
        servoController.init();
        
        if (!networkManager.connect()) {
            Logger::log("System initialization failed at WiFi connection");
            return;
        }
        
        if (!networkManager.checkApiConnection()) {
            Logger::log("System initialization failed at API connection");
            return;
        }
        
        Logger::log("System initialization complete");
    }

    void update() {
        if (barcodeScanner.isDataAvailable()) {
            String barcode = barcodeScanner.readBarcode();
            if (barcode.length() > 0) {
                networkManager.sendBottleData(barcode, servoController.getIsExtended());
            }
        }
        
        if (millis() - lastDistanceCheck >= Config::CHECK_INTERVAL) {
            if (distanceSensor.isObjectInRange() && !servoController.getIsExtended()) {
                servoController.extend();
            }
            lastDistanceCheck = millis();
        }
        
        servoController.update();
        
        if (millis() - lastStatusLog >= STATUS_LOG_INTERVAL) {
            Logger::log("System status - Servo: " + 
                       String(servoController.getIsExtended() ? "Extended" : "Retracted"));
            lastStatusLog = millis();
        }
    }
};

BottleReturnSystem bottleSystem;

void setup() {
    bottleSystem.init();
}

void loop() {
    bottleSystem.update();
    Logger::flush();  // Ensure any remaining logs are written to SD
}
