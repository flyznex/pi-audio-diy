#include "ssd1306.h"

bool initDisplay()
{
#ifndef OLED_SPI
    Wire.setSCL(OLED_SCL);
    Wire.setSDA(OLED_SDA);
#endif
    if (!ssd1036Display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
    {
        Serial1.println(F("SSD1306 allocation failed"));
        return false;
    }
    ssd1036Display.display();
    delay(1000);
    return true;
}
void drawStr(int posX, int posY, int size, String content, bool isInverse)
{
    ssd1036Display.setTextSize(size);
    ssd1036Display.setTextColor(isInverse ? SSD1306_INVERSE : WHITE);
    ssd1036Display.setCursor(posX, posY);
    // Draw some text
    ssd1036Display.println(content);
    ssd1036Display.display();
}

void clearDisplay() {
     ssd1036Display.clearDisplay();
}