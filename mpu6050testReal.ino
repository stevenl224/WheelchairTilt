// #include <Wire.h>
// #include <MPU6050.h>

// MPU6050 mpu;

// void setup() {
//   Serial.begin(9600);  // Initialize serial communication
//   Wire.begin();        // Initialize I2C
//   mpu.initialize();    // Initialize the MPU6050 sensor

//   // Check if MPU6050 is connected
//   if (mpu.testConnection()) {
//     Serial.println("MPU6050 connection successful");
//   } else {
//     Serial.println("MPU6050 connection failed");
//   }
// }

// void loop() {
//   int16_t ax, ay, az;
  
//   // Get acceleration values
//   mpu.getAcceleration(&ax, &ay, &az);

//   // Calculate tilt angle in the X-Y plane using the Y and Z accelerometer values
//   float angle = atan2(ay, az) * 180 / PI;

//   // Print the angle to the Serial Monitor
//   Serial.print("Tilt Angle: ");
//   Serial.println(angle);

//   // Add a delay for readability in the Serial Monitor
//   delay(500);  // Adjust as needed for a smoother readout
// }
