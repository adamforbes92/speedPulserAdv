void setupButton() {
  btnConfig.attachClick(singleClickUp);
  btnConfig.attachMultiClick(doubleClickUp);
  btnConfig.setPressMs(1000);  // that is the time when LongPressStart is called
}

void singleClickUp() {
  Serial.println("pressed");
  testSpeedo++;

  if (testSpeedo > 1) {
    testSpeedo = 0;
  }
}  // singleClickUp

void doubleClickUp() {
  Serial.println("double pressed");
}  // doubleClickUp