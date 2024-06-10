#include "src/Display/ssd1306.h"

void setup() {
  Serial1.begin(115200);
  Serial1.println("Hello Arduino\n");
  bool d = initDisplay();
  if (!d) {
    Serial1.println("can't start display module");
  }
  // NOTE: The following values are known to work with the Adafruit microphone:
  // i2s.setBitsPerSample(32);
  // i2s.setFrequency(16000);
  ssd1036Display.clearDisplay();
  ssd1036Display.setTextSize(1);
  ssd1036Display.setTextColor(SSD1306_WHITE);
  ssd1036Display.setCursor(0,0);
  // Draw some text
  ssd1036Display.println(F("str"));
  ssd1036Display.display();
}

void loop() {
  /* Nothing here */
  delay(1);
}
