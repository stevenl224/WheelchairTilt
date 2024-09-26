#include <Wire.h>
#include <RTClib.h>
#include <MPU6050.h>

RTC_DS3231 rtc;
MPU6050 mpu;
unsigned long previousMillis = 0;
const long interval = 1800000;  // 30 minutes in milliseconds
bool active = false;
unsigned long exerciseStartTime = 0;
bool exerciseStarted = false;

void setup() {
  Serial.begin(9600);
  Wire.begin();
  rtc.begin();
  mpu.initialize();

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(9, OUTPUT);  // Speaker pin
}

void loop() {
  DateTime now = rtc.now();

  // Step 2: Check if current time is between 9 AM and 9 PM
  if (now.hour() >= 9 && now.hour() < 21) {
    active = true;
  } else {
    active = false;
  }

  if (active) {
    // Step 3: 30-minute countdown when the board is active
    if (millis() - previousMillis >= interval) {
      // Notify the user (LED + beep)
      digitalWrite(LED_BUILTIN, HIGH);  // LED on
      tone(9, 1000, 500);               // Speaker beep
      delay(500);
      digitalWrite(LED_BUILTIN, LOW);   // LED off

      // Step 4: Wait for tilt to exceed 30 degrees
      checkTilt();
      previousMillis = millis();  // Reset 30-min countdown after notification
    }
  }
}

void checkTilt() {
  int16_t ax, ay, az;
  mpu.getAcceleration(&ax, &ay, &az);
  float angle = atan2(ay, az) * 180 / PI;  // Calculate tilt angle

  if (abs(angle) > 30) {
    if (!exerciseStarted) {
      exerciseStartTime = millis();  // Start the 3-min timer
      exerciseStarted = true;
    } else if (millis() - exerciseStartTime >= 180000) {  // 3 minutes
      // Step 5: Completion notification
      tone(9, 1000, 500);  // Speaker beep
      digitalWrite(LED_BUILTIN, HIGH);  // LED on for completion
      delay(500);
      digitalWrite(LED_BUILTIN, LOW);
      
      // Step 6: Reset exercise timer for the next cycle
      exerciseStarted = false;
    }
  } else {
    exerciseStarted = false;  // Reset if tilt is less than 30 degrees
  }
}
