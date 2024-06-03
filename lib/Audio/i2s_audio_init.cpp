#include "i2s_audio_init.h"

I2SAudio::I2SAudio(AudioInfo info = AUDIOINFO_DEFAULT, int mclk = PIN_MCLK,
                   int wclk = PIN_WCLK, int dout = PIN_DOUT, int bclk = PIN_BCLK, int mck_multi = PIN_MCLK_MULTI)
{
    auto cfg = i2s.defaultConfig(TX_MODE);
    cfg.copyFrom(info);
    cfg.pin_bck = bclk;
    cfg.pin_data = dout;
    cfg.pin_mck = mclk;
    cfg.pin_ws = wclk;
    cfg.mck_multiplier = mck_multi;
    i2s.begin(cfg);
}

I2SAudio::~I2SAudio() {
    i2s.end()
}

I2SAudio::I2SStream() I2SStream {
    return i2s;
}