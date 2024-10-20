#include <Wire.h>
#include <RTC.h>
#include <MPU6050.h>


// Initialize Gyroscope
MPU6050 mpu;

// The following are various Constant Values that can be adjusted according to conditions

// varibles used for timer calculations (DO NOT CHANGE)
unsigned long previousMillis = 0; 
unsigned long exerciseStartTime = 0; 
unsigned long waitingStartTime = 0;

// time to wait before notifying them to exercise again (for mvp it will be 30 min)
const long interval = 10000;  

// once its time to exercise, this is how long of a window they have to do their exercise, it should be atleast 2 times as long as the duration they need to hold the tilt
const long exerciseWindow = 20000; 

// how long the rhonda needs to hold the angle for in order for it to be a succesful session
const unsigned long exerciseDuration = 5000; 

// minimum tilt angle to consider the exercise started (will need to be adjusted on the fly)
const unsigned long minTiltAngle = 15;

// intitialize various states and sub-states of the board to false for now
bool active = false; 
bool exerciseStarted = false;
bool waitingForExercise = false;

// Counts how many times they did their exercise throughout the day
int dailyExerciseCount = 0;

// Setup
void setup() {
  Serial.begin(9600);
  delay(1000);
  Wire.begin();
  RTC.begin();

  // Initialize the MPU6050 sensor
  mpu.initialize();    
  if (mpu.testConnection()) {
    Serial.println("MPU6050 connection successful");
  } else {
    Serial.println("MPU6050 connection failed");
  }


  // This section of the code automatically detects time of compilation 
  String testtime = __TIME__;   // holds the compile time in "HH:MM:SS"
  int testHour = testtime.substring(0, 2).toInt();   // Convert hour part to integer
  int testMinute = testtime.substring(3, 5).toInt(); // Convert minute part to integer
  int testSecond = testtime.substring(6, 8).toInt(); // Convert second part to integer

  // Initialize date and time for the RTC module
  RTCTime startTime(10, Month::OCTOBER, 2024, testHour, testMinute, testSecond, DayOfWeek::WEDNESDAY, SaveLight::SAVING_TIME_ACTIVE); 
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
    dailyExerciseCount = 0;
  }

  if (active) {
    // Step 3: 30-minute countdown when the board is active
    if (millis() - previousMillis >= interval) {
      Serial.println("It's time to do your exercise!!");
      // ADD CODE HERE FOR NOTIFYING USER IT IS TIME TO BEGIN THEIR EXERVCISE (FLASHING LIGHT)





      waitingForExercise = true;
      waitingStartTime = millis();
    }

    if (waitingForExercise) {
      if (millis() - waitingStartTime >= exerciseWindow) { //window to complete the exervise
        Serial.println("Failed to perform exercise, will notify next exercise time.");
        // ADD CODE HERE TO TURN OFF LIGHT THAT INDICATES ITS TIME TO EXERCISE (SEE PRINT STATEMENT ABOVE)



        waitingForExercise = false;
        previousMillis = millis();  // Reset 30-min countdown 
      } else {
        checkTilt();
        previousMillis = millis();  // Reset 30-min countdown 
      }
    }
  }

  // ADD CODE HERE TO PRINT OUT HOW MANY LEDS SHOULD BE ON BASED ON THE VALUE OF dailyExerciseCount


  
  delay(500);
}

void checkTilt() {
  int16_t ax, ay, az;
  mpu.getAcceleration(&ax, &ay, &az);
  float angle = atan2(ay, az) * 180 / PI;  // Calculate tilt angle

  if (abs(angle) > minTiltAngle) { // must tilt at minimum to the angle set to begin exercise
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
        dailyExerciseCount++;
        Serial.print("Good job! You did your exercise this time. Total Exercises Today: ");
        Serial.println(dailyExerciseCount);
        // ADD CODE HERE TO PLAY SUCESS MUSIC 



        
        // Step 6: Reset exercise timer for the next cycle
        exerciseStarted = false;
        waitingForExercise = false;
      }
    }
  } else {
    Serial.print("Tilt Angle: ");
    Serial.println(angle);  // Continuously print the angle even if it's less than 30 degrees
    if (exerciseStarted) {
      Serial.println("Went below angle threshhold.");
      exerciseStarted = false;  // Reset if tilt is less than 30 degrees
    }
  }}