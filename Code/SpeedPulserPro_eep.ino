void readEEP() {
#if serialDebug
  DEBUG_PRINTLN("EEPROM initialising!");
#endif

  // use ESP32's 'Preferences' to remember settings.  Begin by opening the various types.  Use 'false' for read/write.  True just gives read access
  pref.begin("testSpeedo", false);
  pref.begin("testRPM", false);
  pref.begin("tempSpeed", false);
  pref.begin("hasNeedleSweep", false);
  pref.begin("sweepSpeed", false);
  pref.begin("minFreqHall", false);
  pref.begin("maxFreqHall", false);
  pref.begin("minFreqCAN", false);
  pref.begin("maxFreqCAN", false);
  pref.begin("minSpeed", false);
  pref.begin("maxSpeed", false);
  pref.begin("minRPM", false);
  pref.begin("maxRPM", false);
  pref.begin("clusterRPMLimit", false);
  pref.begin("speedOffset", false);
  pref.begin("speedOffsetPositive", false);

  // first run comes with EEP valve of 255, so write actual values.  If found/match SW version, read all the values
  if (pref.getUChar("testSpeedo") == 255) {
#if serialDebug
    DEBUG_PRINTLN("First run, set Bluetooth module, write Software Version etc");
    DEBUG_PRINTLN(pref.getUChar("testSpeedo"));
#endif
    pref.putBool("testSpeedo", testSpeedo);
    pref.putBool("testRPM", testRPM);
    pref.putUChar("tempSpeed", tempSpeed);
    pref.putBool("hasNeedleSweep", hasNeedleSweep);
    pref.putUChar("sweepSpeed", sweepSpeed);
    pref.putUChar("minFreqHall", minFreqHall);
    pref.putUShort("maxFreqHall", maxFreqHall);
    pref.putUChar("minFreqCAN", minFreqCAN);
    pref.putUShort("maxFreqCAN", maxFreqCAN);
    pref.putUChar("minSpeed", minSpeed);
    pref.putUShort("maxSpeed", maxSpeed);
    pref.putUChar("minRPM", minRPM);
    pref.putUShort("maxRPM", maxRPM);
    pref.putUShort("clusterRPMLimit", clusterRPMLimit);
    pref.putUChar("speedOffset", speedOffset);
    pref.putBool("speedOffsetPositive", speedOffsetPositive);
  } else {

    testSpeedo = pref.getBool("testSpeedo", false);
    tempRPM = pref.getBool("testRPM", false);
    tempSpeed = pref.getUChar("tempSpeed", 100);
    hasNeedleSweep = pref.getBool("hasNeedleSweep", false);
    sweepSpeed = pref.getUChar("sweepSpeed", 18);
    minFreqHall = pref.getUChar("minFreqHall", 0);
    maxFreqHall = pref.putUShort("maxFreqHall", 200);
    minFreqCAN = pref.getUChar("minFreqCAN", 0);
    maxFreqCAN = pref.putUShort("maxFreqCAN", 200);
    minSpeed = pref.getUChar("minSpeed", 0);
    maxSpeed = pref.putUShort("maxSpeed", 200);
    minRPM = pref.getUChar("minRPM", 0);
    maxRPM = pref.putUShort("maxRPM", 230);
    clusterRPMLimit = pref.putUShort("clusterRPMLimit", 7000);
    speedOffset = pref.getUChar("speedOffset", 0);
    speedOffsetPositive = pref.getBool("speedOffsetPositive", 0);
  }
#if serialDebug
  DEBUG_PRINTLN("EEPROM initialised with...");
#endif
}

void writeEEP() {
#if serialDebug
  DEBUG_PRINTLN("Writing EEPROM...");
#endif

  // update EEP only if changes have been made
  pref.putBool("testSpeedo", testSpeedo);
  pref.putBool("testRPM", testRPM);
  pref.putUChar("tempSpeed", tempSpeed);
  pref.putBool("hasNeedleSweep", hasNeedleSweep);
  pref.putUChar("sweepSpeed", sweepSpeed);
  pref.putUChar("minFreqHall", minFreqHall);
  pref.putUShort("maxFreqHall", maxFreqHall);
  pref.putUChar("minFreqCAN", minFreqCAN);
  pref.putUShort("maxFreqCAN", maxFreqCAN);
  pref.putUChar("minSpeed", minSpeed);
  pref.putUShort("maxSpeed", maxSpeed);
  pref.putUChar("minRPM", minRPM);
  pref.putUShort("maxRPM", maxRPM);
  pref.putUShort("clusterRPMLimit", clusterRPMLimit);
  pref.putUChar("speedOffset", speedOffset);
  pref.putBool("speedOffsetPositive", speedOffsetPositive);

#if serialDebugEEP
  DEBUG_PRINTLN("Written EEPROM with data:...");
  DEBUG_PRINTLN(testSpeedo);
  DEBUG_PRINTLN(tempSpeed);
  DEBUG_PRINTLN(hasNeedleSweep);
  DEBUG_PRINTLN(sweepSpeed);
  DEBUG_PRINTLN(minFreqHall);
  DEBUG_PRINTLN(maxFreqHall);
  DEBUG_PRINTLN(minFreqCAN);
  DEBUG_PRINTLN(maxFreqCAN);
  DEBUG_PRINTLN(minSpeed);
  DEBUG_PRINTLN(maxSpeed);
  DEBUG_PRINTLN(speedOffset);
  DEBUG_PRINTLN(speedOffsetPositive);
#endif
}
