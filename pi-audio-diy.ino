/*
   I2S stereo microphone (input) example
   Run using the Arduino Serial Plotter to see waveform.
   Released to the Public Domain by Earle F. Philhower, III

   For the Google AIY Voice Hat Microphone daughterboard, part
   of the Raspberry Pi AIY cardboard box, the I2S stereo pinout
   looking at the board top with the RPI logo on the left hand
   side:
            +--   ------------------------------------  --+
   left RPI | (1) GND (2) DIN (3) BCLK (4) LRCLK (5) 3.3V | AIY right
       logo +---------------------------------------------+ logo

   For an Adfruit I2S MEMS microphone (https://www.adafruit.com/product/3421),
   connect the pins as follows:

      DOUT -> GPIO0
      BCLK <- GPIO1
      LRCL <- GPIO2  # LRCLK = BCLK + 1
      GND <-> GND
      3V  <-> 3V3OUT

   The other idiosyncrasy of most modern MEMS microphones is that they
   require a minimum clock rate to wake up. For example, the SPH0645
   microphone needs at least 1MHz.
*/

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
