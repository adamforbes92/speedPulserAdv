/*
SpeedPulser Pro - Forbes Automotive '25

A blend of the SpeedPulser and Can2Cluster.  Offering speed and RPM output to MK1 & MK2 Golf clusters.  Supports CAN (for RPM/Speed) and GPS (for speed).
Can capture hall sensor input for traditional speed input.  Can broadcast speed via. CAN or GPS if req.
*/

#include "speedPulserPro_defs.h"

// for motor
ESP32_FAST_PWM* motorPWM;                              // for PWM control.  ESP Boards need to be V2.0.17 - the latest version has known issues with LEDPWM(!)
RunningMedian samples = RunningMedian(averageFilter);  // for calculating median samples - there can be 'hickups' in the incoming signal, this helps remove them(!)

// for CAN
#include <ESP32_CAN.h>
ESP32_CAN<RX_SIZE_256, TX_SIZE_16> chassisCAN;

// for GPS
#include <TinyGPSPlus.h>
#include <SoftwareSerial.h>
SoftwareSerial ss(pinRX_GPS, pinTX_GPS);
TinyGPSPlus gps;

TickTwo tickEEP(writeEEP, eepRefresh);
TickTwo tickWiFi(disconnectWifi, wifiDisable);  // timer for disconnecting wifi after 30s if no connections - saves power
Preferences pref;

extern OneButton btnConfig(pinCal, false);

hw_timer_t* timer0 = NULL;
bool rpmTrigger = true;
long frequencyRPM = 20;  // 20 to 20000

void IRAM_ATTR onTimer0() {
  rpmTrigger = !rpmTrigger;
  digitalWrite(pinCoil, rpmTrigger);
}

// interrupt routine for the incoming pulse
void incomingHz() {                                               // Interrupt 0 service routine
  static unsigned long previousMicros = micros();                 // remember variable, initialize first time
  unsigned long presentMicros = micros();                         // read microseconds
  unsigned long revolutionTime = presentMicros - previousMicros;  // works fine with wrap-around of micros()
  if (revolutionTime < 1000UL) return;                            // avoid divide by 0, also debounce, speed can't be over 60,000 was 1000UL
  dutyCycleIncoming = (60000000UL / revolutionTime) / 60;         // calculate
  previousMicros = presentMicros;
  lastPulse = millis();

  ledCounter++;  // count LED counter - is used to flash onboard LED to show the presence of incoming pulses
}

void incomingMotorSpeed() {                                       // Interrupt 0 service routine
  static unsigned long previousMicros = micros();                 // remember variable, initialize first time
  unsigned long presentMicros = micros();                         // read microseconds
  unsigned long revolutionTime = presentMicros - previousMicros;  // works fine with wrap-around of micros()
  if (revolutionTime < 1000UL) return;                            // avoid divide by 0, also debounce, speed can't be over 60,000 was 1000UL
  dutyCycleMotor = (60000000UL / revolutionTime) / 60;            // calculate
  previousMicros = presentMicros;
}

void incomingCal() {
  if (testSpeedo == 0 || testSpeedo == 1) {
    testSpeedo = 2;
  }

  if (testSpeedo == 2) {
    testSpeedo = 0;
  }
}

// setup timers
void setupTimer() {
  timer0 = timerBegin(0, 40, true);  //div 80
  timerAttachInterrupt(timer0, &onTimer0, true);
}

// adjust output frequency
void setFrequencyRPM(long frequencyHz) {
  if (frequencyHz != 0) {
    timerAlarmDisable(timer0);
    timerAlarmWrite(timer0, 1000000l / frequencyHz, true);
    timerAlarmEnable(timer0);
  } else {
    timerAlarmDisable(timer0);
  }
}

void setup() {
#ifdef serialDebug
  Serial.begin(115200);
  DEBUG_PRINTLN("Initialising SpeedPulser Pro...");
#endif

  basicInit();   // init PWM, Serial, Pin IO etc.  Kept in '_io.ino' for cleanliness due to the number of Serial outputs
  setupTimer();  // setup the timers (with a base frequency)

  tickEEP.start();   // begin ticker for the EEPROM
  tickWiFi.start();  // begin ticker for the WiFi (to turn off after 60s)

  motorPWM->setPWM(pinMotorOutput, pwmFrequency, dutyCycle);  // set motor to off in first instance (100% duty)

  if (hasNeedleSweep) {
    needleSweep();  // enable needle sweep (in _io.ino)
  }

  connectWifi();         // enable / start WiFi
  WiFi.setSleep(false);  //For the ESP32: turn off sleeping to increase UI responsivness (at the cost of power use)
  setupUI();             // setup wifi user interface
}

void loop() {
  // check to see if in 'test mode' (testSpeedo = 1)
  btnConfig.tick();
  tickEEP.update();   // refresh the EEP ticker
  tickWiFi.update();  // refresh the WiFi ticker

  updateLabels();

  if (testSpeedo == 1) {
    DEBUG_PRINTLN("Test Speedo = 1");
    motorPWM->setPWM_manual(pinMotorOutput, 385);
  }

  if (tempNeedleSweep) {
    needleSweep();
    tempNeedleSweep = false;
  }

  if (ledCounter > averageFilter) {
    ledOnboard = !ledOnboard;                 // flip-flop the led trigger
    digitalWrite(pinOnboardLED, ledOnboard);  // flash the LED to show the presence of incoming pulses
    ledCounter = 0;                           // reset the counter
  }

  if (millis() - lastPulse > durationReset) {
    motorPWM->setPWM_manual(pinMotorOutput, 0);
    dutyCycle = 0;
    dutyCycleIncoming = 0;
  }

  if (testSpeedo == 0 || vehicleSpeed > 0) {
    if ((dutyCycle != dutyCycleIncoming)) {  // only update PWM IF speed has changed (can cause flicker otherwise)
      switch (incomingType) {
        case 0:  // can input
          DEBUG_PRINTF("     SpeedIncomingCAN: %d", vehicleSpeed);
          if (speedOffsetPositive) {
            dutyCycle = vehicleSpeed + speedOffset;
            dutyCycle = findClosestMatch(dutyCycle);
            motorPWM->setPWM_manual(pinMotorOutput, dutyCycle);
          } else {
            if (dutyCycle - speedOffset > 0) {
              dutyCycle = vehicleSpeed - speedOffset;
              dutyCycle = findClosestMatch(dutyCycle);
              motorPWM->setPWM_manual(pinMotorOutput, dutyCycle);
            } else {
              motorPWM->setPWM_manual(pinMotorOutput, 0);
            }
          }
          DEBUG_PRINTF("     FindClosetMatchC2C: %d", dutyCycle);
          break;

        case 1:  // hall sensor input
          DEBUG_PRINTF("     DutyIncomingHall: %d", dutyCycleIncoming);
          dutyCycleIncoming = map(dutyCycleIncoming, minFreqHall, maxFreqHall, minSpeed, maxSpeed);  // map incoming range to this codes range.  Max Hz should match Max Speed - i.e., 200Hz = 200kmh, or 500Hz = 200kmh...
          DEBUG_PRINTF("     DutyPostProc1Hall: %d", dutyCycle);

          if (rawCount < averageFilter) {
            samples.add(dutyCycleIncoming);
            rawCount++;
          }

          if (rawCount >= averageFilter) {
            dutyCycle = samples.getAverage(averageFilter / 2);
            DEBUG_PRINTF("     getAverageHall: %d", dutyCycle);

            if (speedOffsetPositive) {
              dutyCycle = dutyCycle + speedOffset;
              dutyCycle = findClosestMatch(dutyCycle);
              motorPWM->setPWM_manual(pinMotorOutput, dutyCycle);
            } else {
              if (dutyCycle - speedOffset > 0) {
                dutyCycle = dutyCycle - speedOffset;
                dutyCycle = findClosestMatch(dutyCycle);
                motorPWM->setPWM_manual(pinMotorOutput, dutyCycle);
              } else {
                motorPWM->setPWM_manual(pinMotorOutput, 0);
              }
            }

            DEBUG_PRINTF("     FindClosetMatchHall: %d", dutyCycle);
            rawCount = 0;
            samples.clear();
          }
          break;
      }

      dutyCycle = dutyCycleIncoming;  // re-introduce?  Could do some filter on big changes?  May skip over genuine changes though?
      DEBUG_PRINTLN("");
    }
  }

  frequencyRPM = map(vehicleRPM, 0, RPMLimit, 0, maxRPM);
  // change the frequency of both RPM & Speed as per CAN information
  if ((millis() - lastMillis2) > rpmPause) {  // check to see if x ms (linPause) has elapsed - slow down the frames!
    lastMillis2 = millis();
    DEBUG_PRINTLN(frequencyRPM);
    setFrequencyRPM(frequencyRPM);  // minimum speed may command 0 and setFreq. will cause crash, so +1 to error 'catch'
  }
}

uint16_t findClosestMatch(uint16_t val) {
  // for finding the nearest match of speed from the incoming duty.  There may be instances where the incoming speed does not equal a value in the array, so find the 'nearest' value
  // has to be 16_t due to 10 bit resolution
  // the 'find'/function returns array position, which is equal to the duty cycle
  uint16_t closest = 0;
  for (uint16_t i = 0; i < sizeof motorPerformance / sizeof motorPerformance[0]; i++) {
    if (abs(val - closest) >= abs(val - motorPerformance[i]))
      closest = motorPerformance[i];
  }
  for (uint16_t i = 0; i < (sizeof motorPerformance / sizeof motorPerformance[0]) - 1; i++) {
    if (motorPerformance[i] == closest) {
      return i;
    }
  }
  return 0;
}

void updateLabels() {
  char bufRPM[32];
  sprintf(bufRPM, "CAN RPM: %d", vehicleRPM);
  ESPUI.updateLabel(label_RPMCAN, String(bufRPM));

  char bufSpeedGPS[32];
  sprintf(bufSpeedGPS, "GPS Speed: %d", vehicleSpeedGPS);
  ESPUI.updateLabel(label_speedGPS, String(bufSpeedGPS));

  char bufSpeedCAN[32];
  sprintf(bufSpeedCAN, "GPS Speed: %d", vehicleSpeedCAN);
  ESPUI.updateLabel(label_speedCAN, String(bufSpeedCAN));
}