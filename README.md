## Overview

This Arduino sketch implements a system that helps users perform regular exercise (tilting exercises) at set intervals throughout the day. The system:

- Uses a Real-Time Clock (RTC) to determine active hours during which reminders should be given.
- Monitors a gyroscope/accelerometer (MPU6050) to detect tilting of the device. A minimum tilt angle and a sustained duration are required to consider the exercise "complete."
- Allows the user to configure how frequently the exercise reminders occur (interval) and how long the user must hold the tilt (duration) through two rotary encoders.
- Provides audio feedback through a DFPlayer Mini MP3 module, playing reminder tones, encouragement messages, and success/failure notifications.
- Uses LEDs (driven by an Adafruit TLC5947 LED driver) to visually track the number of completed exercises throughout the day.
- Can be muted or unmuted with a dedicated mute button.
- Resets daily progress when inactive hours start (e.g., overnight).

## Hardware Components

1. **Arduino (e.g., Arduino Uno)**
2. **MPU6050 Gyroscope/Accelerometer**:
   - VCC -> 5V
   - GND -> GND
   - SCL -> A5
   - SDA -> A4
3. **RTC Module (e.g., DS3231 or DS1307)** via I2C:
   - SCL -> A5
   - SDA -> A4
4. **Adafruit TLC5947 24-Channel LED driver**:
   - Data (DIN) -> Pin 4
   - Clock (CLK) -> Pin 5
   - Latch (LAT) -> Pin 6
   - Additional channels connect to LEDs and power as per Adafruit documentation.
5. **DFPlayer Mini MP3 Module**:
   - TX from Arduino (Pin 2) -> RX of DFPlayer
   - RX from Arduino (Pin 3) -> TX of DFPlayer
   - VCC -> 5V
   - GND -> GND
6. **Rotary Encoders (2 encoders)**:
   - Encoder 1 for interval control:  
     - CLK -> Pin 9  
     - DT -> Pin 8  
     - SW (Button) -> Pin 7  
   - Encoder 2 for duration control:  
     - CLK -> Pin 12  
     - DT -> Pin 11  
     - SW (Button) -> Pin 10  
   - Common GND connections as required.
7. **Mute Button**:
   - Mute Button Pin -> Pin 13  
   - The other side of the button to GND.
   
Ensure all grounds are connected together.

## Configuration and Adjustable Parameters

- **Active Hours:** The system is active (reminders are given) between `STARTING_HOUR` and `ENDING_HOUR`.  
  Example: `STARTING_HOUR = 9; ENDING_HOUR = 21;` means active from 9:00 to 21:00.

- **Intervals (Reminder Frequency):** Controlled by the first rotary encoder. Three interval levels are predefined:
  - Level 1: 30 minutes
  - Level 2: 45 minutes
  - Level 3: 60 minutes
  The rotary encoder changes the interval by adjusting its angle. Each interval corresponds to a 120° segment of the encoder’s rotation (0°-120°, 120°-240°, 240°-360°).

- **Exercise Duration (Tilt Holding Time):** Controlled by the second rotary encoder. Three duration levels:
  - Duration 1: 3 minutes
  - Duration 2: 4 minutes
  - Duration 3: 5 minutes
  Like the interval, these durations also correspond to rotary encoder angle ranges.

- **Tilt Threshold:** The minimum tilt angle (`minTiltAngle`) and the duration the user must hold that tilt (`exerciseDuration`) determine a successful exercise.

- **Reminders:** If the user doesn’t start tilting within a given window, a reminder sound is played after 1 minute. If no exercise is performed within the entire interval, a "missed exercise" sound is played.

- **LED Feedback:** Up to 6 LEDs track the number of completed exercises each day. Completing each exercise lights an additional LED. At the end of the day, or when outside active hours, LEDs reset.

- **Audio Tracks:** The DFPlayer Mini expects specific files in its `mp3` folder on the SD card:
  - Track 1: Success Sound
  - Track 2: Exercise Missed
  - Track 3: Time to tilt (for duration1)
  - Track 4: Time to tilt (for duration2)
  - Track 5: Time to tilt (for duration3)
  - Track 6: Mid-interval reminder
  - Track 7: Reminders disabled
  - Track 8: Reminders enabled
  - Track 9: Recalibrating gyro
  - Track 10: Reset Duration Angle
  - Track 11: Reset Interval Angle
  - Track 12: Device Rebooting
  - Track 13: Tilt Threshold Passed, Timer Begins
  - Track 14: Reminder to tilt further

Adjust files as needed, ensuring the naming format `xxxx.mp3` (e.g., `0001.mp3`, `0002.mp3`, etc.).

## Special Button Functions

- **Mute Button (Pin 13):** Toggles sound on/off.
- **Rotary Encoders’ Buttons:**
  - Press both buttons simultaneously: Resets gyro calibration.
  - Hold the first encoder’s button for 3 seconds: Resets the interval angle reference to 0°.
  - Hold the second encoder’s button for 3 seconds: Resets the duration angle reference to 0°.

## Setup Steps

1. **Wiring:**  
   Connect all components as per the pin descriptions. Ensure I2C devices share the same SCL and SDA lines.
   
2. **SD Card for DFPlayer:**  
   Place the required `xxxx.mp3` files in a `mp3` folder on the SD card. Insert the card into the DFPlayer Mini.

3. **RTC Initialization:**
   The code sets the RTC time based on compile time. Adjust the `dayOfMonth` and other parameters in the code if needed.

4. **Calibration:**
   On system startup, the code calculates a gyro offset. Keep the device still and flat for about 5 seconds after powering up for accurate offset calibration.

5. **Testing:**
   - Rotate the first encoder to change the notification interval.
   - Rotate the second encoder to change the tilt duration.
   - Check that the MP3 module and LEDs function as expected.
   - Mute/unmute sounds with the mute button.
   - Simulate an exercise by tilting the device and holding for the set duration.

## How It Works

- **Normal Operation:**
  - After system initialization, the device enters a cycle:
    1. Waits for the current interval to pass.
    2. When interval elapses, plays an audio prompt and flashes LEDs as a reminder.
    3. The user has a certain "window" of time to start and complete the tilting exercise.
    4. If the user starts tilting above the threshold and holds it long enough, the exercise is marked as completed:
       - Increments daily count.
       - Plays success sound.
       - Lights up another LED on the display.
    5. If the user does not start tilting or complete the exercise within the window, a missed sound is played.
    6. Process repeats at the next interval.

- **Daily Reset:**
  Outside the configured active hours, the device resets the daily exercise count and clears all LEDs. This simulates a new "day" starting fresh.

## Troubleshooting

- **No Sound:**  
  Check DFPlayer connections, ensure files are correctly named and placed on SD card.
  
- **Incorrect Time:**  
  Ensure RTC is set correctly or adjust the code to set the time manually.
  
- **Unresponsive Encoders:**  
  Verify encoder wiring, ensure pull-up resistors are enabled, and try rotating slower.

- **Gyro Incorrect Angle:**  
  Press both encoder buttons simultaneously to recalibrate the gyro offset.

## License & Contribution

This code is provided as-is for educational and experimental purposes. Feel free to modify, adapt, and improve it for your projects. Contributions and suggestions are always welcome.

---

This README should help you understand the purpose, setup, and operation of the code.
