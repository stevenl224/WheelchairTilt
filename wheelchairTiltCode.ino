#include <Wire.h>
#include <RTC.h>
#include <MPU6050.h>

// Initialize Gyroscope
MPU6050 mpu;

// Various Constant Values
unsigned long previousMillis = 0;
const long interval = 10000;  // 10 seconds in milliseconds
bool active = false;
unsigned long exerciseStartTime = 0;
bool exerciseStarted = false;
bool waitingForExercise = false;
unsigned long waitingStartTime = 0;
const unsigned long exerciseDuration = 5000; // 5 seconds for the exercise

// Setup
void setup() {
  Serial.begin(9600);
  Wire.begin();
  RTC.begin();
  
  // Initialize the MPU6050 sensor
  mpu.initialize();    
  if (mpu.testConnection()) {
    Serial.println("MPU6050 connection successful");
  } else {
    Serial.println("MPU6050 connection failed");
  }

  // Initialize date and time for the RTC module
  RTCTime startTime(10, Month::OCTOBER, 2024, 8, 59, 40, DayOfWeek::WEDNESDAY, SaveLight::SAVING_TIME_ACTIVE); 
  RTC.setTime(startTime);
}

void loop() {

  // Get the current date and time from RTC
  RTCTime timeNow;
  RTC.getTime(timeNow);

  // Step 2: Check if current time is between 9 AM and 9 PM
  if (timeNow.getHour() >= 9 && timeNow.getHour() <= 21) {
    Serial.println("Board Active");
    active = true;
  } else {
    Serial.println("Board Inactive");
    active = false;
  }

  if (active) {
    // Step 3: 30-minute countdown when the board is active
    if (millis() - previousMillis >= interval) {
      // Notify the user (LED + beep)
      Serial.println("It's time to do your exercise!!");
      waitingForExercise = true;
      waitingStartTime = millis();
    }

    if (waitingForExercise) {
      if (millis() - waitingStartTime >= 10000) {
        Serial.println("Failed to perform exercise, will notify next exercise time.");
        waitingForExercise = false;
        previousMillis = millis();  // Reset 30-min countdown 
      } else {
        checkTilt();
        previousMillis = millis();  // Reset 30-min countdown 
      }
    }
  }
}

void checkTilt() {
  int16_t ax, ay, az;
  mpu.getAcceleration(&ax, &ay, &az);
  float angle = atan2(ay, az) * 180 / PI;  // Calculate tilt angle

  if (abs(angle) > 30) {
    if (!exerciseStarted) {
      exerciseStartTime = millis();  // Start the timer when tilt exceeds 30 degrees
      Serial.println("Exercise started!");
      exerciseStarted = true;
    } else {
      Serial.print("Tilt Angle: ");
      Serial.println(angle);  // Continuously print the angle during exercise

      // Calculate the remaining time
      unsigned long elapsedTime = millis() - exerciseStartTime;
      unsigned long remainingTime = exerciseDuration - elapsedTime;

      // Print the countdown timer
      Serial.print("Time remaining: ");
      Serial.print(remainingTime / 1000);  // Convert milliseconds to seconds
      Serial.println(" seconds");

      if (elapsedTime >= exerciseDuration) {  // Exercise completed after 5 seconds
        // Step 5: Completion notification
        Serial.println("Good job! You did your exercise this time.");
        
        // Step 6: Reset exercise timer for the next cycle
        exerciseStarted = false;
        waitingForExercise = false;
      }
    }
  } else {
    Serial.print("Tilt Angle: ");
    Serial.println(angle);  // Continuously print the angle even if it's less than 30 degrees
    if (exerciseStarted) {
      Serial.println("Exercise failed.");
      exerciseStarted = false;  // Reset if tilt is less than 30 degrees
    }
  }
}
