#include <element.h>
#include <ssd1306.h>

Icon::Icon(int16_t posX, int16_t posY, uint8_t *icon, bool isInverse) 
    : posX(posX),posY(posY),icon(icon),iconH(iconHeight),
    iconW(iconWidth),isInverse(isInverse) {}

Icon::Icon(int16_t posX, int16_t posY, uint8_t *icon,int16_t w,int16_t h, bool isInverse)
    : posX(posX),posY(posY),icon(icon),iconH(w),
    iconW(h),isInverse(isInverse) {}

void Icon::setBgColor(bool isInverse) {
    if (this -> isInverse != isInverse) {
        this->isInverse = isInverse;
    }
}
void Icon::update() {
    isUpdated = true;
}

void Icon::draw() {
    Serial1.println(F("drawing..."));
    ssd1036Display.clearDisplay();
    ssd1036Display.drawBitmap(posX,posY,icon,iconW,iconH,isInverse? SSD1306_INVERSE : WHITE);
    ssd1036Display.display();
}

void Icon::setIcon(uint8_t *icon) {
    if(this->icon != icon) {
        this->icon = icon;
    }
}

void Icon::clear() {

}
