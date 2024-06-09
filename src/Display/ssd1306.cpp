#include "ssd1306.h"

bool initDisplay()
{
    if (!ssd1036Display.begin(SSD1306_SWITCHCAPVCC, 0x3D))
    {
        Serial1.println(F("SSD1306 allocation failed"));
        return false;
    }
    ssd1036Display.display();
    return true;
}
void drawStr(int posX, int posY, int size, String content, bool isInverse) {
    Serial1.println(F("drawed"));
}