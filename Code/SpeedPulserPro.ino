/*
SpeedPulser Pro - Forbes Automotive '25

A blend of the SpeedPulser and Can2Cluster.  Offering speed and RPM output to MK1 & MK2 Golf clusters.  Supports CAN (for RPM/Speed) and GPS (for speed).
Can capture hall sensor input for traditional speed input.  Can broadcast speed via. CAN or GPS if req.
Note that RPM output is high voltage and MAY cause interference between hall sensors.  Care should be taken to route the cables AWAY from each other.
*/

#include "speedPulserPro_defs.h"

// for motor PWM
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

TickTwo tickEEP(writeEEP, eepRefresh);          // refresh EEP
TickTwo tickWiFi(disconnectWifi, wifiDisable);  // timer for disconnecting wifi after (wifiDisable) if no connections - saves power
Preferences pref;                               // to record previously saved settings

extern OneButton btnConfig(pinCal, false);  // input for calibration button - currently unused

hw_timer_t* timer0 = NULL;
bool rpmTrigger = true;
long frequencyRPM = 20;  // 0 to 20000

void IRAM_ATTR onTimer0() {
  rpmTrigger = !rpmTrigger;           // flip/flop RPM trigger - will create a 50% duty at X freq.
  digitalWrite(pinCoil, rpmTrigger);  // drive coil transistor
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

void incomingCal() {  // ignored
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

  if (hasNeedleSweep) {
    needleSweep();  // enable needle sweep (in _io.ino).  Get this done immediately after setup so there isn't a visible 'lag'
  }

  tickEEP.start();   // begin ticker for the EEPROM
  tickWiFi.start();  // begin ticker for the WiFi (to turn off after 60s)

  motorPWM->setPWM(pinMotorOutput, pwmFrequency, dutyCycle);  // set motor to off in first instance (100% duty)

  connectWifi();                       // enable / start WiFi
  WiFi.setSleep(false);                // for the ESP32: turn off sleeping to increase UI responsivness (at the cost of power use)
  setupUI();                           // setup wifi user interface
  WiFi.setTxPower(WIFI_POWER_8_5dBm);  // set a lower power mode (some C3 aerials aren't great and leaving it high causes failures)
}

void loop() {
  btnConfig.tick();   // refresh the button ticker
  tickEEP.update();   // refresh the EEP ticker
  tickWiFi.update();  // refresh the WiFi ticker

  parseGPS();  // check for GPS updates - not an issue if not connected

  if (tempNeedleSweep) {  // only here if tested in WiFi
    needleSweep();
    tempNeedleSweep = false;  // reset the flag
  }

  if (ledCounter > averageFilter) {
    ledOnboard = !ledOnboard;                 // flip-flop the led trigger
    digitalWrite(pinOnboardLED, ledOnboard);  // flash the LED to show the presence of incoming pulses
    ledCounter = 0;                           // reset the counter
  }

  if (!testSpeedo) {
    if ((millis() - lastPulse) > durationReset) {  // it's been a while since the last hall input, so update to say 0 speed and reset vars
      ESPUI.updateLabel(label_speedHall, "Hall Speed: 0");
      dutyCycle = 0;
      dutyCycleIncoming = 0;
      vehicleSpeedHall = 0;
    }

    if ((dutyCycle != dutyCycleIncoming)) {                                                      // only update PWM IF speed has changed (can cause flicker otherwise)
      DEBUG_PRINTF("     DutyIncomingHall: %d", dutyCycleIncoming);                              // what is the incoming pulse count?
      dutyCycleIncoming = map(dutyCycleIncoming, minFreqHall, maxFreqHall, minSpeed, maxSpeed);  // map incoming range to this codes range.  Max Hz should match Max Speed - i.e., 200Hz = 200kmh, or 500Hz = 200kmh...
      DEBUG_PRINTF("     DutyPostProc1Hall: %d", dutyCycle);                                     // what is the new 'pulse count' - mapped to min/max hall and min/max speed

      if (rawCount < averageFilter) {
        samples.add(dutyCycleIncoming);  // add to a list to create an average
        rawCount++;
      }

      if (rawCount >= averageFilter) {
        vehicleSpeedHall = samples.getAverage(averageFilter / 2);  // get the average
        DEBUG_PRINTF("     getAverageHall: %d", dutyCycle);
        rawCount = 0;
        samples.clear();  // clear the list
      }

      dutyCycle = dutyCycleIncoming;  // re-introduce?  Could do some filter on big changes?  May skip over genuine changes though?
      DEBUG_PRINTLN("");
    }
  }

  // if testSpeedo, apply offests to to confirm working
  if (testSpeedo) {
    if (speedOffsetPositive) {
      dutyCycle = tempSpeed + speedOffset;
      DEBUG_PRINTLN("+");
      DEBUG_PRINTLN(dutyCycle);
    } else {
      if (tempSpeed - speedOffset > 0) {
        dutyCycle = tempSpeed - speedOffset;
        DEBUG_PRINTLN("-");
        DEBUG_PRINTLN(dutyCycle);
      } else {
        dutyCycle = 0;
      }
    }
  }

  //if NOT testSpeedo, apply the caught variables (Hall, GPS or CAN) and transfer across, applying any maps beforehand
  if (!testSpeedo) {
    vehicleSpeed = 0;
    if (vehicleSpeedHall > 0) {
      vehicleSpeed = vehicleSpeedHall;
    }
    if (vehicleSpeedCAN > 0) {
      vehicleSpeedCAN = map(vehicleSpeedCAN, minFreqCAN, maxFreqCAN, minSpeed, maxSpeed);  // map incoming range to this codes range.  Max Hz should match Max Speed - i.e., 200Hz = 200kmh, or 500Hz = 200kmh...
      vehicleSpeed = vehicleSpeedCAN;
    }
    if (vehicleSpeedGPS > 0) {
      vehicleSpeed = vehicleSpeedGPS;
    }

    // we now have a final speed - so apply any offsets
    if (speedOffsetPositive) {
      dutyCycle = vehicleSpeed + speedOffset;
      DEBUG_PRINTLN("+");
      DEBUG_PRINTLN(dutyCycle);
    } else {
      if (vehicleSpeed - speedOffset > 0) {
        dutyCycle = vehicleSpeed - speedOffset;
        DEBUG_PRINTLN("-");
        DEBUG_PRINTLN(dutyCycle);
      } else {
        dutyCycle = 0;
      }
    }
  }

  if (testRPM) {  // set vehicleRPM is testing or not
    vehicleRPM = tempRPM;
  } else {
    vehicleRPM = vehicleRPMCAN;
  }

  // change the frequency of both RPM & Speed as per receieved information
  if ((millis() - lastMillis2) > rpmPause) {  // check to see if x ms (rpmPause) has elapsed - slow down the frames!
    lastMillis2 = millis();

    updateLabels();  // update WiFi labels

    // set RPM frequency
    frequencyRPM = map(vehicleRPM, 0, clusterRPMLimit, 0, maxRPM);
    setFrequencyRPM(frequencyRPM);  // minimum speed may command 0 and setFreq. will cause crash, so +1 to error 'catch'

    // set motor duty cycle
    dutyCycle = findClosestMatch(dutyCycle);
    motorPWM->setPWM_manual(pinMotorOutput, dutyCycle);

    DEBUG_PRINTLN("vehicleSpeedHall: ");
    DEBUG_PRINTLN(vehicleSpeedHall);
    DEBUG_PRINTLN("vehicleSpeedCAN: ");
    DEBUG_PRINTLN(vehicleSpeedCAN);
    DEBUG_PRINTLN("vehicleSpeedGPS: ");
    DEBUG_PRINTLN(vehicleSpeedGPS);

    DEBUG_PRINTLN("vehicleSpeed: ");
    DEBUG_PRINTLN(vehicleSpeed);
    DEBUG_PRINTLN("vehicleRPM: ");
    DEBUG_PRINTLN(vehicleRPM);
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
  char bufSpeedHall[32];
  sprintf(bufSpeedHall, "Hall Speed: %d", vehicleSpeedHall);
  ESPUI.updateLabel(label_speedHall, String(bufSpeedHall));

  char bufSpeedGPS[32];
  sprintf(bufSpeedGPS, "GPS Speed: %d", vehicleSpeedGPS);
  ESPUI.updateLabel(label_speedGPS, String(bufSpeedGPS));

  char bufSpeedCAN[32];
  sprintf(bufSpeedCAN, "CAN Speed: %d", vehicleSpeedCAN);
  ESPUI.updateLabel(label_speedCAN, String(bufSpeedCAN));

  char bufRPM[32];
  sprintf(bufRPM, "CAN RPM: %d", vehicleRPMCAN);
  ESPUI.updateLabel(label_RPMCAN, String(bufRPM));
}