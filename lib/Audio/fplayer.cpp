#include "fplayer.h"

static AudioSourceSDFAT<sd_t, file_t> source("/", ".wav", SD_CS_PIN, 4 /* speedMHz */, SHARED_SPI);
static WAVDecoder decoder;
static I2SStream i2s;
AudioPlayer player(source, i2s, decoder);

void beginAudioPlayer()
{
    AudioLogger::instance().begin(Serial1, AudioLogger::Info);

    auto cfg = i2s.defaultConfig(TX_MODE);
    cfg.copyFrom(AUDIOINFO_DEFAULT);
    cfg.pin_bck = PIN_BCLK;
    cfg.pin_ws = PIN_WCLK;
    cfg.pin_data = PIN_DOUT;
    cfg.pin_mck = PIN_MCLK;
    cfg.mck_multiplier = MCLK_MULTI;
    if (!i2s.begin(cfg)) {
        Serial1.println("i2s.begin() failed");
    }

    player.setVolume(0.5f);
    if (!player.begin()) {
        Serial1.println("player.begin() failed");
    }
}
