#include <Wire.h>
#include <RTC.h>
#include <MPU6050.h>
#include <Adafruit_TLC5947.h>
#include <DFRobotDFPlayerMini.h>
#include <SoftwareSerial.h>

/*
This Arduino code manages an exercise reminder and tracking system using tilt detection, LEDs, and sound. 
It tracks exercise completion based on tilt angle (detected via a gyroscope) and uses rotary encoders to adjust reminder intervals and exercise duration. 
The system provides LED feedback for progress and plays audio after each completed exercise, resetting daily.
*/


// --------------- Pin Setup  ---------------
// Represents number of LED boards used
#define NUM_TLC5947 1

// Defining pins for LEDs
#define data 4
#define clock 5
#define latch 6

// Encoder 1 Pins       all wires that arent below are going to ground for the encoder
const int clkPin = 9;  // brown wire
const int dtPin = 8;   // silver wire
const int swPin = 7;   // black wire

// Encoder 2 Pins       all wires that arent below are going to ground for the encoder
const int clkPin2 = 12;  // pink wire
const int dtPin2 = 11;   // blue wire
const int swPin2 = 10;   // black wire



// --------------- Configurable Variables  ---------------

// Defines active times for tilting
const static int STARTING_HOUR = 9;
const static int ENDING_HOUR = 21;

// Interval controls how often it notifes them, it has 3 levels which are toggled by the rotary encoder
// Define length of each interval based on this format to convert the desired time in minutes to ms
// level = (minutes) * MILLISECONDS
const int MILLISECONDS = 60 * 1000; 
long level1 = 30 * MILLISECONDS;
long level2 = 45 * MILLISECONDS;
long level3 = 60 * MILLISECONDS;
long interval = level1;  // initialize it to level 1

// How long they need to hold the tilt to get a succesful completion of exercise,  it also has 3 levels controlled by the other rotary encoder
int duration1 = 3 * MILLISECONDS;
int duration2 = 4 * MILLISECONDS;
int duration3 = 5 * MILLISECONDS;
int exerciseDuration = duration1;  // initialze it to level 1

long exerciseWindow = interval;       // The window of time they have to do the exercise
const unsigned long minTiltAngle = 15;  // Minimum angle they need to tilt to in order to begin exercise
const int dayOfMonth = 2;              // Configures day of the month for rtc (not super important but you should update when uploading to project)
int reminderDuration = 1 * MILLISECONDS;           // Once exercise begins it reminds them they should be tilting after this much of a delay

// --------------- Gyroscope SETUP ---------------

/* For the gyroscope the wiring is as follows:
VCC: 5V
GND: GND
SCL: A5
SDA: A4
*/

MPU6050 mpu;
float offset;                         // angle offset
const int calibrationButtonPin = 13;  // connect one side of the button to this pin and the other to GND
// Check for button press
static bool gyroButtonPressed = false;
static unsigned long gyroLastDebounceTime = 0;
const unsigned long gyroDebounceDelay = 50;  // 50ms debounce delay

// --------------- Timing Varibles SETUP ---------------

// Timer variables
unsigned long previousMillis = 0;
unsigned long exerciseStartTime = 0;
unsigned long waitingStartTime = 0;

// Status and tracking variables
bool active = false;
bool exerciseStarted = false;
bool waitingForExercise = false;
int dailyExerciseCount = 0;
int prevExerciseCount = -1;

// Purely for debugging purposes and result readability (youll notice all print statements print condtionally based on these variables)
unsigned long lastPrintTime = 0;
const unsigned long printInterval = 500;  // You can change this in order to increase or decrease the rate at which serial monitor prints out stuff to read


// --------------- Encoder SETUP ---------------

// Encoder Constants
int clkState, dtState;
int lastClkState;
int stepCounter = 0;
float currentAngle = 0.0;
const int stepsPerRevolution = 20;
float anglePerStep = 360.0 / stepsPerRevolution;

// Second encoder for exercise duration adjustment
int clkState2, dtState2;
int lastClkState2;
int stepCounter2 = 0;
float currentAngle2 = 0.0;

// Debounce timing variables
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 0.1;

// Button hold timing variables
unsigned long buttonPressStart1 = 0;
unsigned long buttonpressStart2 = 0;
const unsigned long holdTimeRequired = 3000;


// --------------- LED SETUP ---------------
Adafruit_TLC5947 tlc = Adafruit_TLC5947(NUM_TLC5947, clock, data, latch);

// --------------- SPEAKER SETUP ---------------
// Define pins 8 and 9 to communicate with DFPlayer Mini
static const uint8_t PIN_MP3_TX = 2;  // Connects to the module's RX
static const uint8_t PIN_MP3_RX = 3;  // Connects to the module's TX
SoftwareSerial softwareSerial(PIN_MP3_RX, PIN_MP3_TX);

// Create the DF player object
DFRobotDFPlayerMini player;

bool reminderPlayed = false;

// 1 = Success Sound
// 2 = Exercise Missed
// 3 = Its time to tilt (Tier 1) -> 3 minutes
// 4 = Its time to tilt (Tier 2) -> 4 minutes
// 5 = Its time to tilt (tier 3) -> 5 minutes
// 6 = Reminder to tilt 1 min into the window time

// --------------- Begining of SETUP CODE ---------------
void setup() {
  Serial.begin(230400);
  softwareSerial.begin(9600);
  delay(1000);
  Wire.begin();
  RTC.begin();
  tlc.begin();

  // Encoder setup for exercise interval
  pinMode(clkPin, INPUT_PULLUP);
  pinMode(dtPin, INPUT_PULLUP);
  pinMode(swPin, INPUT_PULLUP);
  lastClkState = digitalRead(clkPin);

  // Encoder 2 setup for exercise duration
  pinMode(clkPin2, INPUT_PULLUP);
  pinMode(dtPin2, INPUT_PULLUP);
  pinMode(swPin2, INPUT_PULLUP);
  lastClkState2 = digitalRead(clkPin2);

  // Gryoscope related stuff
  pinMode(calibrationButtonPin, INPUT_PULLUP);
  mpu.initialize();
  if (mpu.testConnection()) {
    Serial.println("MPU6050 connection successful");
  } else {
    Serial.println("MPU6050 connection failed");
  }

  // SPeaker related stuff
  if (player.begin(softwareSerial)) {
    Serial.println("Serial connection with DFPlayer established.");
  } else {
    Serial.println("Connecting to DFPlayer Mini failed!");
  }

  String testtime = __TIME__;
  int testHour = testtime.substring(0, 2).toInt();
  int testMinute = testtime.substring(3, 5).toInt();
  int testSecond = testtime.substring(6, 8).toInt();
  RTCTime startTime(dayOfMonth, Month::NOVEMBER, 2024, testHour, testMinute, testSecond, DayOfWeek::WEDNESDAY, SaveLight::SAVING_TIME_ACTIVE);
  RTC.setTime(startTime);

  offset = calculateGyroOffset();
}


// Function to update interval based on angle
void updateIntervalFromAngle(float angle) {
  if (angle >= 0 && angle < 120) {
    interval = level1;
  } else if (angle >= 120 && angle < 240) {
    interval = level2;
  } else if (angle >= 240 && angle <= 360) {
    interval = level3;
  }
  exerciseWindow = interval;
}

// Function to update exercise duration based on angle from second encoder
void updateExerciseDurationFromAngle(float angle) {
  if (angle >= 0 && angle < 120) {
    exerciseDuration = duration1;
  } else if (angle >= 120 && angle < 240) {
    exerciseDuration = duration2;
  } else if (angle >= 240 && angle <= 360) {
    exerciseDuration = duration3;
  }
}

// Updated Encoder 1 angle function with improved reset handling
void updateEncoderAngle() {
  unsigned long currentTime = millis();
  clkState = digitalRead(clkPin);

  // Check if encoder state changed with debounce
  if ((clkState != lastClkState) && (currentTime - lastDebounceTime > debounceDelay)) {
    lastDebounceTime = currentTime;
    dtState = digitalRead(dtPin);
    if (clkState == HIGH && dtState == LOW) {
      stepCounter++;
    } else if (clkState == HIGH && dtState == HIGH) {
      stepCounter--;
    }

    currentAngle = stepCounter * anglePerStep;
    currentAngle = fmod(currentAngle, 360.0);
    if (currentAngle < 0.0) {
      currentAngle += 360.0;
    }

    updateIntervalFromAngle(currentAngle);

    Serial.print("Steps: ");
    Serial.print(stepCounter);
    Serial.print(" | Current Angle: ");
    Serial.print(currentAngle);
    Serial.println("°");
  }

  // Button press reset handling with dedicated timing
  if (digitalRead(swPin) == LOW) {
    if (buttonPressStart1 == 0) {
      buttonPressStart1 = currentTime;  // Record start time if button is pressed
    }
    if (currentTime - buttonPressStart1 >= holdTimeRequired) {
      stepCounter = 0;
      currentAngle = 0.0;
      Serial.println("Angle reset to 0° after holding button for 3 seconds");
      buttonPressStart1 = 0;  // Reset the press start time after reset action
    }
  } else {
    buttonPressStart1 = 0;  // Reset timing if button is not held
  }

  lastClkState = clkState;
}

// Function to update angle and exercise duration based on second encoder
void updateEncoderAngle2() {
  unsigned long currentTime = millis();
  clkState2 = digitalRead(clkPin2);
  if ((clkState2 != lastClkState2) && (currentTime - lastDebounceTime > debounceDelay)) {
    lastDebounceTime = currentTime;
    dtState2 = digitalRead(dtPin2);
    if (clkState2 == HIGH && dtState2 == LOW) {
      stepCounter2++;
    } else if (clkState2 == HIGH && dtState2 == HIGH) {
      stepCounter2--;
    }

    currentAngle2 = stepCounter2 * anglePerStep;
    currentAngle2 = fmod(currentAngle2, 360.0);
    if (currentAngle2 < 0.0) {
      currentAngle2 += 360.0;
    }

    updateExerciseDurationFromAngle(currentAngle2);

    Serial.print("Encoder 2 - Current Angle: ");
    Serial.print(currentAngle2);
    Serial.println("°");
  }

  if (digitalRead(swPin2) == LOW) {
    if (buttonpressStart2 == 0) {
      buttonpressStart2 = currentTime;
    }
    if (currentTime - buttonpressStart2 >= holdTimeRequired) {
      stepCounter2 = 0;
      currentAngle2 = 0.0;
      Serial.println("Encoder 2 - Angle reset to 0° after holding button for 5 seconds");
      buttonpressStart2 = 0;
    }
  } else {
    buttonpressStart2 = 0;
  }

  lastClkState2 = clkState2;
}

void playTrack(int track, int volumeLevel) {
  player.volume(volumeLevel);   // controls volume level
  player.playMp3Folder(track);  // all we need to play a track
}

void informUser() {
  if (exerciseDuration == duration1) {
    Serial.println("User is tilting for duration1: ");
    Serial.print(duration1);
    playTrack(3, 15);
  } else if (exerciseDuration == duration2) {
    Serial.println("User is tilting for duration2: ");
    Serial.println(duration2);
    playTrack(4, 15);
  } else if (exerciseDuration == duration3) {
    Serial.println("User is tilting for duration3: ");
    Serial.println(duration3);
    playTrack(5, 15);
  }
}


void loop() {
  handleGyroRecalibration();  // checks if the calibration button has been pressed

  unsigned long currentTime = millis();

  updateEncoderAngle();
  updateEncoderAngle2();

  // Only print if the defined interval has passed
  if (currentTime - lastPrintTime >= printInterval) {
    updateEncoderAngle();
    Serial.print("Current Notification Duration: ");
    Serial.println(interval);

    RTCTime timeNow;
    RTC.getTime(timeNow);

    if (timeNow.getHour() >= STARTING_HOUR && timeNow.getHour() <= ENDING_HOUR) {
      Serial.println("Board Active");
      active = true;
    } else {
      Serial.println("Board Inactive");
      active = false;
      dailyExerciseCount = 0;
      resetLEDs();
    }

    if (active) {
      if (millis() - previousMillis >= interval) {
        Serial.println("It's time to do your exercise!!");
        informUser();
        LEDActivityReminder();
        waitingForExercise = true;
        waitingStartTime = millis();
      }

      if (waitingForExercise) {
        if (millis() - waitingStartTime >= reminderDuration) {
          if (!reminderPlayed) {
            playTrack(6, 15);  // Play reminder sound track
            reminderPlayed = true;
          }
        }
        
        if (millis() - waitingStartTime >= exerciseWindow) {
          if (exerciseStarted) {
            Serial.println("The user's tilting is still in progress");
            checkTilt();
            previousMillis = millis();
          } else {
            Serial.println("Failed to perform exercise, will notify next exercise time.");
            playTrack(2, 15);  // Play exercise missed sound track

            waitingForExercise = false;
            reminderPlayed = false;
            previousMillis = millis();
          }
        } else {
          checkTilt();
          previousMillis = millis();
        }
      }
    }

    if (dailyExerciseCount != prevExerciseCount) {
      checkCompletion();
      prevExerciseCount = dailyExerciseCount;
    }

    lastPrintTime = currentTime;  // Reset the timer for the next print cycle
  }
}
void checkCompletion() {
  int numLEDsToLight = min(dailyExerciseCount, 6);

  for (int j = 0; j < 6; j++) {
    tlc.setPWM(j, 0);
  }

  for (int i = 0; i < numLEDsToLight; i++) {
    tlc.setPWM(i, 1000);
  }

  tlc.write();
}

void resetLEDs() {
  Serial.println("New day. All completed tilts are reset.");
  for (int i = 0; i < 6; i++) {
    tlc.setPWM(i, 0);
  }
  tlc.write();
}

void LEDActivityReminder() {
  unsigned long flickerDuration = 10000;
  unsigned long flickerInterval = 500;
  unsigned long startTime = millis();

  Serial.println("Reminder: It's time to tilt again!");

  while (millis() - startTime < flickerDuration) {
    tlc.setPWM(6, 1000);
    tlc.write();
    delay(flickerInterval);

    tlc.setPWM(6, 0);
    tlc.write();
    delay(flickerInterval);
  }
}

void checkTilt() {
  int16_t ax, ay, az;
  mpu.getAcceleration(&ax, &ay, &az);
  float angle = atan2(ay, az) * 180 / PI;

  angle = angle - offset;
  if (abs(angle) > minTiltAngle) {
    if (!exerciseStarted) {
      exerciseStartTime = millis();
      Serial.println("Exercise started!");
      exerciseStarted = true;
    } else {
      Serial.print("Tilt Angle: ");
      Serial.println(angle);

      unsigned long elapsedTime = millis() - exerciseStartTime;
      unsigned long remainingTime = exerciseDuration - elapsedTime;

      Serial.print("Time remaining: ");
      Serial.print(remainingTime / 1000);
      Serial.println(" seconds");

      if (elapsedTime >= exerciseDuration) {
        dailyExerciseCount++;
        Serial.print("Good job! You did your exercise this time. Total Exercises Today: ");
        Serial.println(dailyExerciseCount);
        playTrack(1, 15);  // Play exericse success sound track

        reminderPlayed = false;
        exerciseStarted = false;
        waitingForExercise = false;
        previousMillis = millis();
      }
    }
  } else {
    Serial.print("Tilt Angle: ");
    Serial.println(angle);
    if (exerciseStarted) {
      Serial.println("Went below angle threshold.");
      exerciseStarted = false;
    }
  }
}

float calculateGyroOffset() {
  int16_t ax, ay, az;
  float totalAngle = 0;
  const unsigned long offsetDuration = 5000;  // 5 seconds
  unsigned long startTime = millis();
  int sampleCount = 0;

  while (millis() - startTime < offsetDuration) {
    // Read accelerometer data
    mpu.getAcceleration(&ax, &ay, &az);

    // Calculate angle from accelerometer data
    float angle = atan2(ay, az) * 180 / PI;

    // Accumulate angle and count samples
    totalAngle += angle;
    sampleCount++;

    // Small delay for stability (adjust as needed for smoother readings)
    delay(10);
  }

  // Calculate the average angle
  float offset = totalAngle / sampleCount;
  Serial.print("Calculated Gyro Offset: ");
  Serial.println(offset);

  return offset;
}

void handleGyroRecalibration() {
  int gyroButtonState = digitalRead(calibrationButtonPin);

  if (gyroButtonState == LOW && !gyroButtonPressed) {
    // Button press detected
    unsigned long currentTime = millis();
    if (currentTime - gyroLastDebounceTime > gyroDebounceDelay) {
      gyroButtonPressed = true;
      gyroLastDebounceTime = currentTime;

      // Recalibrate gyroscope offset
      Serial.println("Recalibrating gyroscope...");
      offset = calculateGyroOffset();
      Serial.println("Recalibration complete.");
    }
  } else if (gyroButtonState == HIGH) {
    gyroButtonPressed = false;  // Reset button pressed state when button is released
  }
}
