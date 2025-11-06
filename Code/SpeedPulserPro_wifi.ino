void setupUI() {
  //Turn off verbose debugging
  ESPUI.setVerbosity(Verbosity::Quiet);
  ESPUI.sliderContinuous = true;

  // create basic tab
  auto tabBasic = ESPUI.addControl(Tab, "", "Basic");
  ESPUI.addControl(Separator, "Needle Sweep", "", Dark, tabBasic);
  bool_NeedleSweep = ESPUI.addControl(Switcher, "Needle Sweep", String(hasNeedleSweep), Dark, tabBasic, generalCallback);
  int16_sweepSpeed = ESPUI.addControl(Slider, "Rate of Change (ms)", String(sweepSpeed), Dark, tabBasic, generalCallback);
  ESPUI.addControl(Min, "", "0", Dark, int16_sweepSpeed);
  ESPUI.addControl(Max, "", "50", Dark, int16_sweepSpeed);
  int16_stepRPM = ESPUI.addControl(Slider, "Rate RPM", String(stepRPM), Dark, tabBasic, generalCallback);
  ESPUI.addControl(Min, "", "0", Dark, int16_stepRPM);
  ESPUI.addControl(Max, "", "10", Dark, int16_stepRPM);
  int16_stepSpeed = ESPUI.addControl(Slider, "Rate Speed", String(stepSpeed), Dark, tabBasic, generalCallback);
  ESPUI.addControl(Min, "", "0", Dark, int16_stepSpeed);
  ESPUI.addControl(Max, "", "10", Dark, int16_stepSpeed);
  ESPUI.addControl(Button, "Test Needle Sweep", "Test", Dark, tabBasic, extendedCallback, (void *)11);

  // create advanced speed tab
  auto tabAdvancedSpeed = ESPUI.addControl(Tab, "", "Speed");
  ESPUI.addControl(Separator, "Calibration:", "", Dark, tabAdvancedSpeed);
  int16_calNumber = ESPUI.addControl(Select, "Calibration", "", Dark, tabAdvancedSpeed, generalCallback);
  ESPUI.addControl(Option, "VW - 120mph; Martin Springell", "VW120Martin", Dark, int16_calNumber);
  ESPUI.addControl(Option, "VW - 120mph; Forbes-Automotive", "VW120Forbes", Dark, int16_calNumber);
  ESPUI.addControl(Option, "VW - 140mph; Forbes-Automotive", "VW140Forbes", Dark, int16_calNumber);
  ESPUI.addControl(Option, "VW - 160mph; Forbes-Automotive", "VW160Forbes", Dark, int16_calNumber);
  ESPUI.addControl(Option, "Ford - 120mph_1; Forbes-Automotive", "Ford120Forbes1", Dark, int16_calNumber);
  ESPUI.addControl(Option, "Ford - 120mph_2; Forbes-Automotive", "Ford120Forbes2", Dark, int16_calNumber);
  ESPUI.addControl(Option, "FIAT - 40-160mph; Forbes-Automotive", "FIAT160Forbes1", Dark, int16_calNumber);
  ESPUI.addControl(Option, "FIAT - 20-110mph; Forbes-Automotive", "FIAT160Forbes2", Dark, int16_calNumber);
  ESPUI.addControl(Option, "Merc - 120mph; Forbes-Automotive", "Merc120Forbes", Dark, int16_calNumber);

  ESPUI.addControl(Separator, "Testing", "", Dark, tabAdvancedSpeed);
  bool_testSpeedo = ESPUI.addControl(Switcher, "Test Speedo", "", Dark, tabAdvancedSpeed, generalCallback);
  int16_tempSpeed = ESPUI.addControl(Slider, "Go to Speed", String(tempSpeed), Dark, tabAdvancedSpeed, generalCallback);
  ESPUI.addControl(Min, "", "0", Dark, int16_tempSpeed);
  ESPUI.addControl(Max, "", "200", Dark, int16_tempSpeed);

  ESPUI.addControl(Separator, "Speed Offsets", "", Dark, tabAdvancedSpeed);
  bool_positiveOffset = ESPUI.addControl(Switcher, "Positive Offset", String(speedOffsetPositive), Dark, tabAdvancedSpeed, generalCallback);
  int16_speedOffset = ESPUI.addControl(Slider, "Speed Offset", String(speedOffset), Dark, tabAdvancedSpeed, generalCallback);
  ESPUI.addControl(Min, "", "0", Dark, int16_speedOffset);
  ESPUI.addControl(Max, "", "50", Dark, int16_speedOffset);

  ESPUI.addControl(Separator, "Speed Limits:", "", Dark, tabAdvancedSpeed);
  int16_minSpeed = ESPUI.addControl(Slider, "Minimum Speed", String(minSpeed), Dark, tabAdvancedSpeed, generalCallback);
  int16_maxSpeed = ESPUI.addControl(Slider, "Maximum Speed", String(maxSpeed), Dark, tabAdvancedSpeed, generalCallback);
  ESPUI.addControl(Min, "", "0", Dark, int16_minSpeed);
  ESPUI.addControl(Max, "", "400", Dark, int16_maxSpeed);
  ESPUI.addControl(Button, "Reset", "Reset", Dark, tabAdvancedSpeed, extendedCallback, (void *)12);

  ESPUI.addControl(Separator, "Hall Incoming Freq:", "", Dark, tabAdvancedSpeed);
  int16_minHall = ESPUI.addControl(Slider, "Minimum Hall", String(minFreqHall), Dark, tabAdvancedSpeed, generalCallback);
  int16_maxHall = ESPUI.addControl(Slider, "Maximum Hall", String(maxFreqHall), Dark, tabAdvancedSpeed, generalCallback);
  ESPUI.addControl(Min, "", "0", Dark, int16_minHall);
  ESPUI.addControl(Max, "", "400", Dark, int16_maxHall);
  ESPUI.addControl(Button, "Reset", "Reset", Dark, tabAdvancedSpeed, extendedCallback, (void *)13);

  ESPUI.addControl(Separator, "CAN Incoming Freq:", "", Dark, tabAdvancedSpeed);
  int16_minCAN = ESPUI.addControl(Slider, "Minimum CAN", String(minFreqCAN), Dark, tabAdvancedSpeed, generalCallback);
  int16_maxCAN = ESPUI.addControl(Slider, "Maximum CAN", String(maxFreqCAN), Dark, tabAdvancedSpeed, generalCallback);
  ESPUI.addControl(Min, "", "0", Dark, int16_minCAN);
  ESPUI.addControl(Max, "", "400", Dark, int16_maxCAN);
  ESPUI.addControl(Button, "Reset", "Reset", Dark, tabAdvancedSpeed, extendedCallback, (void *)14);

  // create advanced speed tab
  auto tabAdvancedRPM = ESPUI.addControl(Tab, "", "RPM");
  ESPUI.addControl(Separator, "Testing", "", Dark, tabAdvancedRPM);
  bool_testRPM = ESPUI.addControl(Switcher, "Test RPM", "", Dark, tabAdvancedRPM, generalCallback);
  int16_tempRPM = ESPUI.addControl(Slider, "Go to RPM", String(tempRPM), Dark, tabAdvancedRPM, generalCallback);
  ESPUI.addControl(Min, "", "0", Dark, int16_tempRPM);
  ESPUI.addControl(Max, "", "9000", Dark, int16_tempRPM);

  ESPUI.addControl(Separator, "Cluster Limits:", "", Dark, tabAdvancedRPM);
  int16_clusterRPM = ESPUI.addControl(Slider, "Maximum RPM", String(clusterRPMLimit), Dark, tabAdvancedRPM, generalCallback);
  ESPUI.addControl(Min, "", "0", Dark, int16_clusterRPM);
  ESPUI.addControl(Max, "", "9000", Dark, int16_clusterRPM);
  ESPUI.addControl(Button, "Reset", "Reset", Dark, tabAdvancedRPM, extendedCallback, (void *)15);

  ESPUI.addControl(Separator, "RPM Scaling:", "", Dark, tabAdvancedRPM);
  int16_RPMScaling = ESPUI.addControl(Slider, "RPM Scaling", String(maxRPM), Dark, tabAdvancedRPM, generalCallback);
  ESPUI.addControl(Min, "", "0", Dark, int16_RPMScaling);
  ESPUI.addControl(Max, "", "500", Dark, int16_RPMScaling);
  ESPUI.addControl(Button, "Reset", "Reset", Dark, tabAdvancedRPM, extendedCallback, (void *)16);

  // create advanced speed tab
  auto tabIO = ESPUI.addControl(Tab, "", "IO");
  ESPUI.addControl(Separator, "Incoming Data", "", Dark, tabIO);
  ESPUI.addControl(Separator, "Incoming Speed (Hall):", "", Dark, tabIO);
  label_speedHall = ESPUI.addControl(Label, "", "0", Dark, tabIO, generalCallback);

  ESPUI.addControl(Separator, "Incoming Speed (GPS):", "", Dark, tabIO);
  label_speedGPS = ESPUI.addControl(Label, "", "0", Dark, tabIO, generalCallback);
  label_hasGPS = ESPUI.addControl(Label, "", "0", Dark, tabIO, generalCallback);

  ESPUI.addControl(Separator, "Incoming RPM (Hall Type):", "", Dark, tabIO);
  label_RPMHall = ESPUI.addControl(Label, "", "0", Dark, tabIO, generalCallback);

  ESPUI.addControl(Separator, "CAN Available:", "", Dark, tabIO);
  label_hasCAN = ESPUI.addControl(Label, "", "0", Dark, tabIO, generalCallback);

  ESPUI.addControl(Separator, "Incoming Speed (CAN):", "", Dark, tabIO);
  label_speedCAN = ESPUI.addControl(Label, "", "0", Dark, tabIO, generalCallback);

  ESPUI.addControl(Separator, "Incoming RPM (CAN):", "", Dark, tabIO);
  label_RPMCAN = ESPUI.addControl(Label, "", "0", Dark, tabIO, generalCallback);

  //Finally, start up the UI.
  //This should only be called once we are connected to WiFi.
  ESPUI.begin(wifiHostName);
}

void updateCallback(Control *sender, int type) {
  updates = (sender->value.toInt() > 0);
}

void getTimeCallback(Control *sender, int type) {
  if (type == B_UP) {
    ESPUI.updateTime(mainTime);
  }
}

void graphAddCallback(Control *sender, int type) {
  if (type == B_UP) {
    ESPUI.addGraphPoint(graph, random(1, 50));
  }
}

void graphClearCallback(Control *sender, int type) {
  if (type == B_UP) {
    ESPUI.clearGraph(graph);
  }
}

void generalCallback(Control *sender, int type) {
#ifdef serialDebugWifi
  Serial.print("CB: id(");
  Serial.print(sender->id);
  Serial.print(") Type(");
  Serial.print(type);
  Serial.print(") '");
  Serial.print(sender->label);
  Serial.print("' = ");
  Serial.println(sender->value);
#endif

  uint8_t tempID = int(sender->id);
  switch (tempID) {
    case 3:
      hasNeedleSweep = sender->value.toInt();
      break;
    case 4:
      sweepSpeed = sender->value.toInt();
      break;
    case 7:
      stepRPM = sender->value.toInt();
      break;
    case 10:
      stepSpeed = sender->value.toInt();
      break;

    case 16:
      if (sender->value == "VW120Martin") motorPerformanceVal = 1;
      if (sender->value == "VW120Forbes") motorPerformanceVal = 2;
      if (sender->value == "VW140Forbes") motorPerformanceVal = 3;
      if (sender->value == "VW160Forbes") motorPerformanceVal = 4;
      if (sender->value == "Ford120Forbes1") motorPerformanceVal = 5;
      if (sender->value == "Ford120Forbes2") motorPerformanceVal = 6;
      if (sender->value == "FIAT160Forbes1") motorPerformanceVal = 7;
      if (sender->value == "FIAT160Forbes2") motorPerformanceVal = 8;
      if (sender->value == "Merc120Forbes") motorPerformanceVal = 9;
      updateMotorPerformance = true;
      break;

    case 27:
      testSpeedo = sender->value.toInt();
      break;
    case 28:
      tempSpeed = sender->value.toInt();
      break;
    case 32:
      speedOffsetPositive = sender->value.toInt();
      break;
    case 33:
      speedOffset = sender->value.toInt();
      break;
    case 37:
      minSpeed = sender->value.toInt();
      break;
    case 38:
      maxSpeed = sender->value.toInt();
      break;
    case 43:
      minFreqHall = sender->value.toInt();
      break;
    case 44:
      maxFreqHall = sender->value.toInt();
      break;

    case 49:
      minFreqCAN = sender->value.toInt();
      break;
    case 50:
      maxFreqCAN = sender->value.toInt();
      break;
    case 56:
      testRPM = sender->value.toInt();
      break;
    case 57:
      tempRPM = sender->value.toInt();
      break;
    case 61:
      clusterRPMLimit = sender->value.toInt();
      break;
    case 66:
      maxRPM = sender->value.toInt();
      break;
  }
}

void extendedCallback(Control *sender, int type, void *param) {
#ifdef serialDebugWifi
  Serial.print("CB: id(");
  Serial.print(sender->id);
  Serial.print(") Type(");
  Serial.print(type);
  Serial.print(") '");
  Serial.print(sender->label);
  Serial.print("' = ");
  Serial.println(sender->value);
  Serial.print("param = ");
  Serial.println((long)param);
#endif

  uint8_t tempID = int(sender->id);
  switch (tempID) {
    case 13:
      if (type == B_UP) {
        tempNeedleSweep = true;
      }
      break;
    case 41:
      if (type == B_UP) {
        minSpeed = 0;
        maxSpeed = 200;
        ESPUI.updateSlider(int16_minSpeed, minSpeed);
        ESPUI.updateSlider(int16_maxSpeed, maxSpeed);
      }
      break;
    case 47:
      if (type == B_UP) {
        minFreqHall = 0;
        maxFreqHall = 200;
        ESPUI.updateSlider(int16_minHall, minFreqHall);
        ESPUI.updateSlider(int16_maxHall, maxFreqHall);
      }
      break;
    case 53:
      if (type == B_UP) {
        minFreqCAN = 0;
        maxFreqCAN = 200;
        ESPUI.updateSlider(int16_minCAN, minFreqCAN);
        ESPUI.updateSlider(int16_maxCAN, maxFreqCAN);
      }
      break;
    case 64:
      if (type == B_UP) {
        clusterRPMLimit = 7000;
        ESPUI.updateSlider(int16_clusterRPM, clusterRPMLimit);
      }
      break;
    case 69:
      if (type == B_UP) {
        maxRPM = 230;
        ESPUI.updateSlider(int16_RPMScaling, maxRPM);
      }
      break;
  }
}

void connectWifi() {
  int connect_timeout;

  WiFi.setHostname(wifiHostName);
  DEBUG_PRINTLN("Beginning WiFi...");
  DEBUG_PRINTLN("Creating Access Point...");
  WiFi.setTxPower(WIFI_POWER_8_5dBm);
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(IPAddress(192, 168, 1, 1), IPAddress(192, 168, 1, 1), IPAddress(255, 255, 255, 0));
  WiFi.softAP(wifiHostName);
}

void textCallback(Control *sender, int type) {
  //This callback is needed to handle the changed values, even though it doesn't do anything itself.
}

void randomString(char *buf, int len) {
  for (auto i = 0; i < len - 1; i++)
    buf[i] = random(0, 26) + 'A';
  buf[len - 1] = '\0';
}

void disconnectWifi() {
  DEBUG_PRINTF("Number of connections: ");
  DEBUG_PRINTLN(WiFi.softAPgetStationNum());

  if (WiFi.softAPgetStationNum() == 0) {
    DEBUG_PRINTLN("No connections, turning off");
    WiFi.disconnect(true, false);
    WiFi.mode(WIFI_OFF);
  }
}
