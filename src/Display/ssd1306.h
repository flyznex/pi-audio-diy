#ifndef SSD1306_H
#define SSD1306_H
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SPI.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Declaration for SSD1306 display connected using SPI
#define OLED_MOSI 11
#define OLED_CLK 10
#define OLED_DC 14
#define OLED_CS 13
#define OLED_RESET 12
static Adafruit_SSD1306 ssd1036Display(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI1, OLED_DC, OLED_RESET, OLED_CS);

bool initDisplay();
void drawStr(int posX, int posY, int size, String content, bool isInverse = false);

#endif