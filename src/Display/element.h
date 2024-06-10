#ifndef ELEMENT_H
#define ELEMENT_H
class Element
{
public:
    typedef enum _align_elm
    {
        AlignLeft = 0,
        AlignRight,
        AlignCenter
    } align_elm;
    virtual void setBgColor(bool isInverse = false) = 0;
    virtual void update() = 0;
    virtual void draw() = 0;
    virtual void clear() = 0;
};

class Icon : public Element
{
public:
    Icon(int16_t posX, int16_t posY, uint8_t *icon, bool isInverse = false);
    void setBgColor(bool isInverse = false);
    void update();
    void draw();
    void clear();
    void setIcon(uint8_t *icon);
    static constexpr int iconWidth = 16;
    static constexpr int iconHeight = 16;

protected:
    bool isUpdated;
    int16_t posX, posY;
    bool isInverse;
    uint8_t *icon;
};

class TextElem : public Element
{
public:
    TextElem(int16_t posX, int16_t posY, bool isInverse = false);
    TextElem(int16_t posX, int16_t posY, align_elm algn, bool isInverse = false);
    TextElem(int16_t posX, int16_t posY, const char *str, align_elm algn, bool isInverse = false);
    void setBgColor(bool isInverse = false);
    void update();
    void draw();
    void clear();
    virtual void setText(const char *str);
    void setFormatText(const char *str, ...);
    static constexpr int charSize = 256;

protected:
    bool isUpdated;
    int16_t posX, posY;
    bool isInverse;
    align_elm algn;
    char str[charSize];
};

class IconText : public TextElem
{
public:
    IconText(int16_t posX, int16_t posY, uint8_t *icon, bool isInverse = false);
    void setBgColor(bool isInverse = false);
    void update();
    void draw();
    void clear();
    void setIcon(u_int8_t *icon);

protected:
    Icon icon;
};

#endif