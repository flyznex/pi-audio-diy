#include <element.h>
#include <ssd1306.h>

Icon::Icon(int16_t posX, int16_t posY, uint8_t *icon, bool isInverse)
    : posX(posX), posY(posY), icon(icon), iconH(iconHeight),
      iconW(iconWidth), isInverse(isInverse) {}

Icon::Icon(int16_t posX, int16_t posY, uint8_t *icon, int16_t w, int16_t h, bool isInverse)
    : posX(posX), posY(posY), icon(icon), iconH(w),
      iconW(h), isInverse(isInverse) {}

void Icon::setBgColor(bool isInverse)
{
    if (this->isInverse != isInverse)
    {
        this->isInverse = isInverse;
    }
}
void Icon::update()
{
    isUpdated = true;
}

void Icon::draw()
{
    ssd1036Display.drawBitmap(posX, posY, icon, iconW, iconH, isInverse ? INVERSE : WHITE);
    isInverse ? ssd1036Display.setTextColor(BLACK, WHITE)
              : ssd1036Display.setTextColor(WHITE);
    ssd1036Display.display();
}

void Icon::setIcon(uint8_t *icon)
{
    if (this->icon != icon)
    {
        this->icon = icon;
    }
}

void Icon::clear()
{
    ssd1036Display.clearDisplay();
}

// ================================
// TextElem implement
// ================================
TextElem::TextElem(int16_t posX, int16_t posY, bool isInverse)
    : posX(posX), posY(posY), isInverse(isInverse), algn(align_elm::AlignLeft)
{
}
TextElem::TextElem(int16_t posX, int16_t posY, align_elm algn, bool isInverse)
    : posX(posX), posY(posY), isInverse(isInverse), algn(algn)
{
}
TextElem::TextElem(int16_t posX, int16_t posY, const char *str, align_elm algn, bool isInverse)
    : posX(posX), posY(posY), isInverse(isInverse), algn(algn)
{
    setText(str);
}
void TextElem::setBgColor(bool isInverse)
{
}
void TextElem::update()
{
    isUpdated = true;
}
void TextElem::draw()
{
    if (strlen(str) == 0)
    {
        return;
    }
    ssd1036Display.setTextSize(1); // 6x8 pixel
    isInverse ? ssd1036Display.setTextColor(BLACK, WHITE)
              : ssd1036Display.setTextColor(WHITE);
    ssd1036Display.setCursor(posX, posY);
    ssd1036Display.println(str);
    ssd1036Display.display();
}
void TextElem::clear()
{
}
void TextElem::setText(const char *str)
{
    if (strncmp(this->str, str, charSize) == 0)
    {
        return;
    }
    memcpy(this->str, str, charSize);
    update();
}
void TextElem::setFormatText(const char *fmt, ...)
{
    char str_tmp[charSize];
    va_list va;
    va_start(va, fmt);
    vsprintf(str_tmp, fmt, va);
    va_end(va);
    setText(str_tmp);
}

// ===================================
// IconText class implement : TextElem
// ===================================
IconText::IconText(int16_t posX, int16_t posY, uint8_t *icon, bool isInverse)
    : TextElem(posX + 16, posY+4, isInverse), icon(posX, posY, icon, isInverse) {}
void IconText::setBgColor(bool isInverse) {}
void IconText::update()
{
    isUpdated = true;
    icon.update();
    TextElem::update();
}
void IconText::draw()
{
    if (strlen(str) == 0)
    {
        if (isUpdated)
        {
            icon.clear();
        }
    }
    else
    {
        icon.draw();
    }
    TextElem::draw();
}
void IconText::clear()
{
    icon.clear();
    TextElem::clear();
}
void IconText::setIcon(uint8_t *icon)
{
    this->icon.setIcon(icon);
}