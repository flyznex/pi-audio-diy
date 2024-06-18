#ifndef SSD1306_H
#define SSD1306_H
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SPI.h>
#include <Wire.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
typedef Adafruit_SSD1306 ssd1306;
// Declaration for SSD1306 display connected using SPI
#ifdef OLED_SPI
#define OLED_MOSI 11
#define OLED_CLK 10
#define OLED_DC 14
#define OLED_CS 13
#define OLED_RESET 12
#define SCREEN_ADDRESS 0x3D
extern ssd1306 ssd1036Display;
#else
#define OLED_RESET -1
#define OLED_SDA 12
#define OLED_SCL 13
#define SCREEN_ADDRESS 0x3C
extern ssd1306 ssd1036Display;
#endif

bool initDisplay();
void clearDisplay();
void drawStr(int posX, int posY, int size, String content, bool isInverse = false);

#endif