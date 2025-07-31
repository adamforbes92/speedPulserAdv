#define _PWM_LOGLEVEL_ 0
#include "ESP32_FastPWM.h"
#include <RunningMedian.h>
#include <Arduino.h>
#include "TickTwo.h"      // for repeated tasks
#include <Preferences.h>  // for eeprom/remember settings
#include <ESPUI.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include "OneButton.h"  // for monitoring the stalk buttons - it's easier to use a lib. than parse each loop - and it counts hold presses

#define baudSerial 115200              // baud rate for serial feedback
#define serialDebug 1                  // for Serial feedback - disable on release(!) ** CAN CHANGE THIS **
#define serialDebugWifi 1              // for wifi feedback
#define serialDebugEEP 0
#define serialDebugGPS 0
#define ChassisCANDebug 0              // if 1, will print CAN 2 (Chassis) messages ** CAN CHANGE THIS **
#define eepRefresh 5000                // EEPROM Refresh in ms
#define wifiDisable 60000              // turn off WiFi in ms
#define wifiHostName "SpeedPulserPro"  // the WiFi name

extern bool testSpeedo = false;  // for testing only, vary final pwmFrequency for speed - disable on release(!) ** CAN CHANGE THIS **
extern bool testRPM = false;

extern bool hasNeedleSweep = false;  // for needle sweep ** CAN CHANGE THIS **
extern uint8_t sweepSpeed = 18;      // for needle sweep rate of change (in ms) ** CAN CHANGE THIS **
extern uint8_t speedType = 0;        // 0 = ECU, 1 = DSG, 2 = GPS, 3 = ABS

#define incomingType 0      // 0 = CAN; 1 = hall sensor  ** CAN CHANGE THIS **
#define averageFilter 6     // number of samples to take to average/remove erraticness from freq. changes.  Higher number, more samples ** CAN CHANGE THIS **
#define durationReset 1500  // duration of 'last sample' before reset speed back to zero

extern uint16_t minFreqHall = 0;    // min frequency for top speed using the 02J / 02M hall sensor  ** CAN CHANGE THIS **
extern uint16_t maxFreqHall = 200;  // max frequency for top speed using the 02J / 02M hall sensor ** CAN CHANGE THIS **
extern uint16_t minFreqCAN = 0;     // min frequency for top speed using the 02J / 02M hall sensor  ** CAN CHANGE THIS **
extern uint16_t maxFreqCAN = 200;   // max frequency for top speed using the 02J / 02M hall sensor ** CAN CHANGE THIS **

extern uint16_t minSpeed = 0;    // minimum cluster speed in kmh on the cluster ** CAN CHANGE THIS **
extern uint16_t maxSpeed = 200;  // minimum cluster speed in kmh on the cluster ** CAN CHANGE THIS **
extern uint16_t minRPM = 0;      // minimum cluster speed in kmh on the cluster ** CAN CHANGE THIS **
extern uint16_t maxRPM = 230;    // minimum cluster speed in kmh on the cluster ** CAN CHANGE THIS **

extern uint16_t clusterRPMLimit = 7000;  // rpm ** CAN CHANGE THIS **

// setup - step changes (for needle sweep)
extern float stepRPM = 1.2;
extern float stepSpeed = 1;
extern uint8_t speedOffset = 0;          // for adjusting a GLOBAL FIXED speed offset - so the entire range is offset by X value.  Might be easier to use this than the input max freq.
extern bool speedOffsetPositive = false;  // set to 1 for the above value to be ADDED, set to zero for the above value to be SUBTRACTED

#define pinMotorOutput 21  // pin for motor PWM output - needs stepped up to 5v for the motor (NPN transistor on the board).  Needs to support LED PWM(!)
#define pinMotorInput 18   // pin for motor speed input.  Assumed 5v, might be bad, should have checked(!)...
#define pinSpeedInput 26   // interrupt supporting pin for hall speed input
#define pinDirection 19    // motor direction pin (currently unused) but here for future revisions
#define pinOnboardLED 2    // for feedback for input checking / flash LED on input.  ESP32 C3 is Pin 8.  Devkit is 2
#define pinCal 36          // pin for calibration button

#define pinRX_CAN 16  // pin output for SN65HVD230 (CAN_RX)
#define pinTX_CAN 17  // pin output for SN65HVD230 (CAN_TX)
#define pinRX_GPS 14  // pin output for GPS NEO6M (GPS_RX)
#define pinTX_GPS 13  // pin output for GPS NEO6M (GPS_TX)
#define pinCoil 25    // pin output for RPM (MK2/High Output Coil Trigger)

#ifdef serialDebug
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#define DEBUG_PRINTF(x...) Serial.printf(x)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#define DEBUG_PRINTF(x...)
#endif

// Baud Rates
#define baudSerial 9600            // baud rate for debug
#define baudGPS 9600               // baud rate for the GPS device
extern uint16_t vehicleRPMCAN = 0;
extern uint16_t vehicleRPM = 0;    // current RPM.  If no CAN, this will catch dividing by zero by the map function

extern uint16_t calcSpeed = 0;     // temp var for calculating speed
extern uint16_t vehicleSpeed = 0;  // current Speed.  If no CAN, this will catch dividing by zero by the map function
extern uint16_t vehicleSpeedHall = 0;
extern uint16_t vehicleSpeedCAN = 0;  // current Speed.  If no CAN, this will catch dividing by zero by the map function
extern uint16_t vehicleSpeedGPS = 0;  // current Speed.  If no CAN, this will catch dividing by zero by the map function
extern bool tempNeedleSweep = false;

// DSG variables
#define PI 3.141592653589793
#define LEVER_P 0x8               // park position
#define LEVER_R 0x7               // reverse position
#define LEVER_N 0x6               // neutral position
#define LEVER_D 0x5               // drive position
#define LEVER_S 0xC               // spot position
#define LEVER_TIPTRONIC_ON 0xE    // tiptronic
#define LEVER_TIPTRONIC_UP 0xA    // tiptronic up
#define LEVER_TIPTRONIC_DOWN 0xB  // tiptronic down
#define gearPause 20              // Send packets every x ms ** CAN CHANGE THIS **
#define rpmPause 50

extern double ecuSpeed = 0;  // ECU speed (from analog speed sensor)
extern double dsgSpeed = 0;  // DSG speed (from RPM & Gear), ratios in '_dsg.ino'
extern double gpsSpeed = 0;  // GPS speed (from '_gps.ino')
extern double absSpeed = 0;  // ABS speed (from '_gps.ino')

// DSG variables
extern uint8_t gear = 0;   // current gear from DSG
extern uint8_t lever = 0;  // shifter position
extern uint8_t gear_raw = 0;
extern uint8_t lever_raw = 0;
uint32_t lastMillis = 0;  // Counter for sending frames x ms
uint32_t lastMillis2 = 0;

// ECU variables
extern bool vehicleEML = false;  // current EML light status
extern bool vehicleEPC = false;  // current EPC light status
extern bool vehicleReverse = false;
extern bool vehiclePark = false;

extern unsigned long dutyCycleIncoming = 0;  // Duty Cycle % coming in from Can2Cluster or Hall
extern unsigned long dutyCycleMotor = 0;     // Duty Cycle % coming in from Can2Cluster or Hall
extern long tempSpeed = 0;                   // for testing only, set fixed speed in kmh.  Can set to 0 to speed up / slow down on repeat with testSpeed enabled
extern long tempRPM = 0;                   // for testing only, set fixed speed in kmh.  Can set to 0 to speed up / slow down on repeat with testSpeed enabled
extern long pwmFrequency = 10000;            // PWM Hz (motor supplied is 10kHz)
extern long dutyCycle = 0;                   // starting / default Hz: 0% is motor 'off'
extern int pwmResolution = 10;               // number of bits for motor resolution.  Can use 8 or 10, although 8 makes it a bit 'jumpy'

extern int rawCount = 0;  // counter for pulses incoming
extern unsigned long lastPulse = 0;

// onboard LED for error
extern bool hasError = false;

// for 8 bit resolution
//uint8_t motorPerformance[] = { 0, 15, 20, 25, 32, 34, 39, 42, 46, 50, 53, 58, 62, 65, 70, 73, 78, 80, 83, 88, 90, 92, 96, 99, 102, 105, 108, 109, 111, 113, 115, 118, 120, 122, 123, 125, 130, 131, 135, 136, 138, 140, 141, 143, 144, 148, 150, 151, 152, 153, 154, 156, 157, 158, 159, 160, 162, 164, 166, 168, 169, 170, 171, 172, 175, 176, 178, 179, 179, 180, 181, 182, 183, 183, 184, 185, 185, 186, 186, 187, 187, 177, 189, 190, 190, 191, 191, 192, 192, 193, 193, 194, 195, 195, 196, 197, 198, 199, 200, 200 };

// for 10 bit resolution - VW MK1/MK2 - 120mph cluster
uint8_t motorPerformance[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 24, 26, 29, 30, 31, 32, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 43, 44, 45, 48, 50, 52, 52, 53, 54, 55, 56, 57, 57, 59, 59, 61, 61, 62, 62, 63, 64, 65, 67, 67, 69, 69, 70, 71, 72, 73, 74, 75, 75, 76, 76, 76, 77, 77, 77, 78, 78, 78, 78, 79, 79, 80, 81, 82, 83, 84, 85, 85, 85, 86, 87, 89, 90, 91, 91, 92, 92, 93, 95, 95, 96, 98, 99, 100, 101, 102, 102, 104, 105, 105, 107, 109, 110, 110, 111, 112, 112, 114, 114, 114, 115, 116, 116, 117, 119, 120, 121, 121, 122, 122, 122, 125, 125, 125, 126, 126, 127, 127, 128, 129, 130, 130, 130, 131, 131, 131, 131, 132, 132, 132, 132, 133, 134, 135, 135, 137, 137, 138, 138, 138, 139, 139, 140, 140, 140, 141, 141, 141, 142, 142, 143, 143, 144, 145, 145, 145, 146, 147, 148, 148, 148, 149, 149, 150, 150, 150, 151, 151, 151, 151, 151, 152, 153, 154, 155, 155, 155, 156, 156, 156, 156, 157, 157, 158, 159, 159, 159, 159, 159, 160, 160, 160, 160, 160, 162, 162, 162, 162, 162, 162, 163, 163, 164, 164, 165, 165, 166, 166, 166, 166, 167, 167, 167, 167, 168, 168, 168, 169, 170, 170, 171, 171, 171, 172, 172, 172, 172, 172, 173, 173, 174, 174, 175, 176, 176, 176, 176, 176, 176, 177, 177, 178, 178, 178, 179, 179, 179, 179, 179, 179, 179, 179, 180, 180, 180, 180, 180, 180, 181, 181, 181, 181, 182, 182, 182, 183, 183, 184, 184, 184, 184, 184, 184, 185, 185, 185, 185, 185, 185, 185, 185, 186, 186, 186, 186, 186, 186, 186, 186, 186, 186, 186, 186, 186, 187, 188, 188, 188, 188, 189, 189, 189, 190, 190, 191, 191, 191, 191, 191, 191, 192, 192, 192, 192, 192, 193, 193, 193, 193, 194, 194, 194, 194, 195, 195, 195, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 197, 197, 197, 197, 197, 197, 197, 197, 197, 197, 197, 197, 197, 197, 197, 197, 197, 198, 198, 198, 198, 199, 199, 199, 199, 199, 200, 200, 200, 200, 200 };

// for 10 bit resolution - VW MK1/MK2 - 160mph cluster
//uint8_t motorPerformance[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 51, 52, 59, 62, 63, 65, 66, 69, 70, 72, 73, 75, 76, 78, 79, 80, 81, 82, 83, 84, 85, 86, 88, 90, 91, 93, 94, 95, 96, 97, 99, 100, 101, 102, 103, 103, 105, 106, 108, 109, 110, 111, 113, 114, 115, 115, 116, 117, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 134, 135, 136, 136, 137, 139, 140, 141, 142, 143, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 152, 153, 153, 154, 155, 155, 156, 157, 158, 160, 161, 162, 162, 162, 163, 163, 164, 164, 165, 165, 166, 167, 168, 169, 170, 171, 171, 172, 172, 173, 174, 174, 175, 175, 177, 178, 178, 179, 180, 181, 181, 182, 183, 183, 183, 183, 184, 184, 184, 185, 185, 186, 186, 187, 189, 190, 190, 191, 191, 192, 192, 193, 194, 194, 195, 195, 196, 197, 197, 198, 198, 199, 199, 200, 200, 201, 201, 202, 202, 202, 203, 203, 204, 204, 205, 205, 206, 206, 207, 207, 208, 209, 209, 210, 210, 211, 211, 212, 212, 212, 213, 213, 213, 213, 214, 214, 214, 214, 215, 215, 215, 216, 216, 217, 218, 218, 219, 219, 220, 220, 221, 222, 222, 223, 223, 224, 224, 224, 224, 225, 225, 225, 226, 226, 226, 226, 227, 227, 229, 229, 230, 230, 231, 232, 232, 233, 233, 233, 234, 234, 235, 235, 235, 235, 235, 235, 236, 236, 236, 236, 237, 237, 238, 238, 239, 239, 239, 239, 240, 240, 240, 240, 241, 241, 241, 241, 242, 242, 242, 242, 243, 243, 243, 243, 243, 243, 244, 244, 244, 244, 244, 244, 244, 245, 245, 245, 246, 246, 247, 248, 248, 249, 249, 249, 250, 250, 250, 250, 251, 251, 252, 252, 252, 252, 252, 253, 253, 253, 253, 253, 254, 254, 254, 254, 254, 254, 254, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 256, 256, 256, 256, 256, 256, 257, 257, 257, 257, 257, 257, 257, 258, 258, 258, 258, 258, 258, 258, 258, 258, 258, 258, 258, 258, 258, 258, 258, 258, 258, 258, 259, 259, 259, 259, 259, 259, 259, 259, 259, 260 };

// for 10 bit resolution - Ford Escort
//uint8_t motorPerformance[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 10, 12, 15, 18, 20, 21, 22, 23, 25, 26, 28, 29, 30, 31, 32, 33, 35, 36, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 51, 52, 54, 55, 57, 56, 58, 60, 61, 62, 63, 65, 66, 68, 69, 69, 70, 71, 72, 73, 74, 77, 78, 79, 80, 81, 82, 83, 84, 85, 85, 87, 88, 89, 90, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 104, 105, 105, 106, 106, 107, 108, 109, 110, 111, 112, 113, 113, 113, 113, 114, 114, 115, 115, 116, 117, 118, 119, 119, 119, 120, 121, 121, 122, 123, 123, 124, 125, 125, 126, 127, 128, 129, 130, 130, 131, 131, 132, 133, 133, 135, 135, 136, 138, 139, 139, 140, 140, 141, 142, 142, 143, 143, 143, 144, 144, 144, 145, 145, 146, 146, 147, 148, 149, 150, 150, 151, 151, 152, 152, 152, 153, 153, 154, 154, 155, 155, 156, 157, 158, 159, 159, 160, 160, 161, 161, 161, 162, 162, 163, 164, 164, 165, 165, 165, 165, 166, 166, 167, 167, 168, 168, 169, 169, 169, 170, 170, 170, 170, 170, 170, 171, 171, 171, 172, 172, 173, 173, 174, 174, 174, 174, 175, 175, 176, 177, 177, 178, 179, 179, 179, 179, 179, 179, 179, 180, 180, 180, 181, 181, 181, 181, 181, 182, 182, 182, 182, 182, 183, 183, 183, 183, 184, 184, 185, 185, 186, 186, 186, 187, 188, 189, 189, 189, 190, 190, 190, 190, 191, 191, 191, 191, 191, 191, 191, 191, 191, 192, 193, 193, 193, 193, 193, 194, 194, 195, 195, 195, 196, 196, 197, 197, 198, 199, 199, 199, 200, 200, 200, 200, 200, 200, 200, 201, 201, 201, 201, 201, 202, 202, 203, 203, 203, 204, 204, 205, 205, 206, 206, 206, 206, 207, 207, 207, 207, 208, 208, 208, 209, 209, 209, 209, 209, 210, 210, 210, 210, 210, 210, 210, 210, 210, 210, 210, 210, 211, 211, 211, 211, 211, 212, 212, 213, 213, 214, 214, 214, 214, 215, 215, 215, 215, 215, 215, 216, 216, 217, 218, 218, 219, 219, 219, 219, 219, 220, 220, 220, 220, 220, 221, 222 };

// for 10 bit resolution - Darren's Ford Escort
//uint8_t motorPerformance[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8, 10, 12, 14, 15, 16, 17, 18, 19, 19, 20, 21, 22, 22, 23, 25, 26, 26, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 36, 36, 37, 38, 39, 39, 40, 41, 42, 42, 44, 44, 45, 45, 46, 47, 48, 48, 49, 50, 50, 51, 51, 52, 53, 54, 54, 54, 55, 56, 57, 57, 58, 58, 59, 60, 60, 61, 61, 62, 63, 64, 64, 65, 65, 66, 67, 67, 68, 68, 69, 69, 70, 70, 71, 71, 71, 72, 73, 74, 74, 74, 75, 75, 76, 76, 77, 77, 78, 78, 79, 80, 80, 81, 81, 81, 82, 82, 83, 84, 84, 84, 85, 85, 86, 87, 87, 88, 88, 88, 89, 89, 89, 90, 90, 91, 91, 91, 92, 92, 93, 94, 94, 94, 94, 95, 95, 95, 96, 96, 97, 97, 97, 98, 98, 98, 99, 99, 99, 100, 100, 101, 101, 101, 101, 102, 102, 103, 103, 104, 104, 104, 104, 105, 105, 105, 106, 107, 107, 107, 107, 108, 108, 108, 108, 109, 109, 109, 110, 110, 111, 111, 112, 112, 112, 112, 113, 113, 113, 113, 114, 114, 114, 114, 114, 115, 115, 115, 116, 116, 116, 116, 117, 117, 117, 117, 118, 118, 118, 118, 119, 119, 119, 119, 120, 120, 120, 121, 121, 121, 122, 122, 122, 122, 123, 123, 123, 123, 124, 124, 124, 124, 124, 124, 125, 125, 125, 125, 125, 126, 126, 126, 126, 126, 126, 127, 127, 127, 127, 128, 128, 128, 128, 128, 129, 129, 129, 129, 129, 129, 129, 130, 130, 130, 130, 131, 131, 131, 131, 132, 132, 132, 132, 132, 133, 133, 133, 134, 134, 134, 134, 134, 135, 135, 135, 135, 135, 135, 135, 136, 136, 136, 136, 137, 137, 137, 137, 138, 138, 138, 138, 138, 139, 139, 139, 139, 139, 139, 139, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140 };

// for 10 bit resolution - Fiat Uno: 1st is full 160mph range, but starts at 40mph.  2nd is sensible range, starting at 20mph, but stops at 110mph.
//uint8_t motorPerformance[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 10, 12, 15, 18, 20, 21, 22, 23, 25, 26, 28, 29, 30, 31, 32, 33, 35, 36, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 51, 52, 54, 55, 57, 56, 58, 60, 61, 62, 63, 65, 66, 68, 69, 69, 70, 71, 72, 73, 74, 77, 78, 79, 80, 81, 82, 83, 84, 85, 85, 87, 88, 89, 90, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 104, 105, 105, 106, 106, 107, 108, 109, 110, 111, 112, 113, 113, 113, 113, 114, 114, 115, 115, 116, 117, 118, 119, 119, 119, 120, 121, 121, 122, 123, 123, 124, 125, 125, 126, 127, 128, 129, 130, 130, 131, 131, 132, 133, 133, 135, 135, 136, 138, 139, 139, 140, 140, 141, 142, 142, 143, 143, 143, 144, 144, 144, 145, 145, 146, 146, 147, 148, 149, 150, 150, 151, 151, 152, 152, 152, 153, 153, 154, 154, 155, 155, 156, 157, 158, 159, 159, 160, 160, 161, 161, 161, 162, 162, 163, 164, 164, 165, 165, 165, 165, 166, 166, 167, 167, 168, 168, 169, 169, 169, 170, 170, 170, 170, 170, 170, 171, 171, 171, 172, 172, 173, 173, 174, 174, 174, 174, 175, 175, 176, 177, 177, 178, 179, 179, 179, 179, 179, 179, 179, 180, 180, 180, 181, 181, 181, 181, 181, 182, 182, 182, 182, 182, 183, 183, 183, 183, 184, 184, 185, 185, 186, 186, 186, 187, 188, 189, 189, 189, 190, 190, 190, 190, 191, 191, 191, 191, 191, 191, 191, 191, 191, 192, 193, 193, 193, 193, 193, 194, 194, 195, 195, 195, 196, 196, 197, 197, 198, 199, 199, 199, 200, 200, 200, 200, 200, 200, 200, 201, 201, 201, 201, 201, 202, 202, 203, 203, 203, 204, 204, 205, 205, 206, 206, 206, 206, 207, 207, 207, 207, 208, 208, 208, 209, 209, 209, 209, 209, 210, 210, 210, 210, 210, 210, 210, 210, 210, 210, 210, 210, 211, 211, 211, 211, 211, 212, 212, 213, 213, 214, 214, 214, 214, 215, 215, 215, 215, 215, 215, 216, 216, 217, 218, 218, 219, 219, 219, 219, 219, 220, 220, 220, 220, 220, 221, 222 };
//uint8_t motorPerformance[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 28, 30, 32, 33, 36, 38, 39, 40, 41, 42, 42, 43, 43, 44, 45, 46, 47, 47, 48, 49, 50, 51, 52, 52, 53, 54, 55, 56, 57, 58, 59, 59, 60, 61, 62, 62, 63, 64, 65, 66, 67, 68, 68, 69, 70, 70, 71, 71, 72, 73, 73, 74, 74, 75, 76, 77, 78, 78, 79, 79, 80, 81, 81, 82, 83, 83, 83, 84, 85, 86, 87, 87, 88, 88, 89, 89, 90, 90, 91, 92, 93, 93, 94, 95, 96, 96, 97, 97, 98, 98, 98, 99, 99, 99, 100, 101, 102, 102, 102, 103, 103, 104, 104, 104, 105, 105, 105, 105, 106, 106, 107, 108, 109, 109, 110, 110, 111, 112, 112, 113, 113, 113, 113, 114, 114, 115, 115, 116, 116, 117, 118, 118, 119, 119, 119, 120, 120, 120, 120, 121, 122, 123, 123, 124, 125, 125, 125, 125, 126, 126, 127, 127, 128, 129, 129, 129, 130, 130, 130, 131, 131, 131, 132, 132, 132, 132, 133, 133, 133, 134, 134, 135, 135, 135, 136, 136, 136, 136, 137, 137, 137, 137, 138, 138, 139, 139, 139, 139, 140, 140, 140, 141, 141, 141, 142, 142, 142, 142, 143, 143, 143, 144, 144, 144, 145, 145, 145, 145, 145, 145, 146, 146, 147, 147, 147, 148, 148, 148, 148, 148, 148, 148, 149, 149, 150, 150, 150, 151, 151, 151, 152, 152, 152, 152, 152, 153, 153, 153, 153, 154, 154, 154, 154, 154, 155, 155, 155, 155, 155, 155, 155, 155, 156, 156, 157, 157, 157, 158, 158, 158, 158, 159, 159, 159, 159, 159, 159, 160, 160, 160, 160, 161, 161, 161, 162, 162, 163, 163, 163, 163, 163, 163, 163, 164, 164, 164, 164, 164, 165, 165, 165, 165, 165, 165, 165, 165, 165, 165, 165, 166, 167, 168, 169, 169, 169, 169, 169, 169, 169, 169, 169, 170, 170, 170, 170, 170, 170, 170, 171, 171, 171, 172, 172, 172, 172, 172, 172, 173, 173, 173, 173, 173, 173, 174, 174, 174, 174, 174, 175, 175, 175, 175, 175, 175, 175, 176, 176, 176, 177, 177, 177, 177, 177, 177, 177, 178, 178, 178, 179, 179, 179, 179, 179, 179, 180, 180, 180, 180, 180 };

// for 10 bit resolution, with smoothing - could be more inaccurate.  Kept for reference.
//uint8_t motorPerformance[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6, 11, 16, 19, 22, 25, 27, 29, 31, 32, 34, 35, 36, 37, 39, 40, 41, 41, 42, 44, 45, 47, 48, 49, 51, 52, 53, 54, 55, 56, 57, 58, 58, 59, 60, 61, 62, 62, 64, 64, 66, 66, 67, 68, 69, 70, 71, 72, 73, 74, 74, 75, 75, 76, 76, 77, 77, 77, 77, 78, 78, 79, 79, 80, 81, 81, 82, 83, 84, 84, 85, 86, 87, 88, 89, 90, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 112, 113, 114, 114, 115, 116, 117, 118, 119, 120, 120, 121, 122, 123, 123, 124, 124, 125, 126, 126, 127, 128, 128, 129, 129, 130, 130, 130, 131, 131, 131, 131, 132, 132, 133, 134, 134, 135, 136, 136, 137, 137, 138, 138, 139, 139, 140, 140, 140, 141, 141, 141, 142, 142, 143, 144, 144, 144, 145, 146, 146, 147, 147, 148, 148, 149, 149, 150, 150, 150, 150, 151, 151, 151, 152, 153, 153, 154, 154, 155, 155, 155, 156, 156, 157, 157, 158, 158, 158, 158, 159, 159, 159, 159, 160, 160, 161, 161, 161, 161, 162, 162, 162, 163, 163, 163, 164, 164, 165, 165, 165, 166, 166, 166, 166, 167, 167, 167, 168, 168, 169, 169, 170, 170, 171, 171, 171, 171, 172, 172, 172, 173, 173, 173, 174, 175, 175, 175, 175, 176, 176, 176, 177, 177, 177, 178, 178, 178, 178, 179, 179, 179, 179, 179, 179, 180, 180, 180, 180, 180, 180, 180, 181, 181, 181, 181, 182, 182, 183, 183, 183, 183, 184, 184, 184, 184, 184, 185, 185, 185, 185, 185, 185, 185, 186, 186, 186, 186, 186, 186, 186, 186, 186, 186, 186, 186, 187, 187, 187, 187, 188, 188, 188, 189, 189, 190, 190, 190, 190, 191, 191, 191, 191, 191, 192, 192, 192, 192, 192, 193, 193, 193, 193, 194, 194, 194, 194, 195, 195, 195, 195, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 197, 197, 197, 197, 197, 197, 197, 197, 197, 197, 197, 197, 197, 197, 197, 197, 197, 198, 198, 198, 198, 198, 199, 199, 199, 199, 199, 200 };

extern uint8_t incomingFreqMedian[] = { 0, 0, 0, 0, 0 };

extern bool ledOnboard = false;
extern int ledCounter = 0;

// define CAN Addresses.  All not req. but here for keepsakes
#define MOTOR1_ID 0x280
#define MOTOR2_ID 0x288
#define MOTOR3_ID 0x380
#define MOTOR5_ID 0x480
#define MOTOR6_ID 0x488
#define MOTOR7_ID 0x588

#define MOTOR_FLEX_ID 0x580
#define GRA_ID 0x38A   // byte 1 & 3 are shaft speeds?
#define gear_ID 0x440  // lower 4 bits of byte 2 are gear?

#define BRAKES1_ID 0x1A0
#define BRAKES2_ID 0x2A0
#define BRAKES3_ID 0x4A0
#define BRAKES5_ID 0x5A0

#define gearLever_ID 0x448
#define mWaehlhebel_1_ID 0x540  // DQ250 DSG ID

#define HALDEX_ID 0x2C0

#define emeraldECU1_ID 0x1000
#define emeraldECU2_ID 0x1001

extern void canInit(void);
extern void onBodyRX(void);

//Function Prototypes
extern void connectWifi();
extern void disconnectWifi();
extern void setupUI();
extern void textCallback(Control *sender, int type);
extern void generalCallback(Control *sender, int type);
extern void updateCallback(Control *sender, int type);
extern void getTimeCallback(Control *sender, int type);
extern void graphAddCallback(Control *sender, int type);
extern void graphClearCallback(Control *sender, int type);
extern void randomString(char *buf, int len);
extern void extendedCallback(Control *sender, int type, void *param);

extern void readEEP();
extern void writeEEP();

//UI handles
uint16_t bool_NeedleSweep, int16_sweepSpeed, int16_stepSpeed, int16_stepRPM;
uint16_t bool_testSpeedo, int16_tempSpeed, bool_testRPM;

uint16_t bool_positiveOffset, int16_speedOffset;
uint16_t int16_minSpeed, int16_maxSpeed, int16_minHall, int16_maxHall, int16_minCAN, int16_maxCAN;
uint16_t int16_minRPM, int16_maxRPM, int16_tempRPM, int16_clusterRPM, int16_RPMScaling;

int label_speedHall, label_speedGPS, label_speedCAN, label_RPMCAN;

uint16_t graph;
uint16_t mainTime;
volatile bool updates = false;
