#include "i2s_audio_init.h"

I2SAudio::I2SAudio(AudioInfo info, int mclk,
                   int wclk, int dout, int bclk, int mck_multi)
{
    auto cfg = i2s.defaultConfig(TX_MODE);
    cfg.copyFrom(info);
    cfg.pin_bck = bclk;
    cfg.pin_data = dout;
    cfg.pin_mck = mclk;
    cfg.pin_ws = wclk;
    cfg.mck_multiplier = mck_multi;
    this -> cfg = cfg;
    
}

I2SAudio::~I2SAudio()
{
    i2s.end();
}
bool I2SAudio::begin() {
    return i2s.begin(cfg);
}

bool I2SAudio::setAudioInfo(AudioInfo inf) {
    cfg.copyFrom(inf);
    return true;
}

I2SStream& I2SAudio::getStream() {
    return i2s;
}