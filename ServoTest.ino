#include <Servo.h>

const int SERVO_PIN = 11;
const int EXTEND = 1000;
const int RETRACT = 2000;

Servo testServo;

void setup() {
    Serial.begin(9600);
    testServo.attach(SERVO_PIN);
    Serial.println("Servo Test Started");
}

void loop() {
    Serial.println("Extending...");
    testServo.writeMicroseconds(EXTEND);
    delay(2000);
    
    Serial.println("Retracting...");
    testServo.writeMicroseconds(RETRACT);
    delay(2000);
}
