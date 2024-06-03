#pragma once

#include <AudioTools.h>

#define AUDIOINFO_DEFAULT AudioInfo(44100, 2, 16)
const int PIN_MCLK = 5; // Master-clock
const int PIN_WCLK = 3; // LRCLK left-right clock
const int PIN_DOUT = 4; // Data in/out
const int PIN_BCLK = 2; // BCCLK bit-clock
const int PIN_MCLK_MULTI = 512;
class I2SAudio
{
public:
    I2SAudio(AudioInfo info = AUDIOINFO_DEFAULT, int mclk = PIN_MCLK,
             int wclk = PIN_WCLK, int dout = PIN_DOUT, int bclk = PIN_BCLK,
             int mck_multi = PIN_MCLK_MULTI);
    ~I2SAudio();
    I2SStream() I2SStream;
private:
    I2SStream i2s;
}