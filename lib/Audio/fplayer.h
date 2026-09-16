#pragma once

#include "AudioTools.h"
#include "AudioTools/AudioPlayer.h"
#include "file_sdfat.h"

#ifdef USE_SDFAT
#include "AudioLibs/AudioSourceSDFAT.h"
#endif

#define AUDIOINFO_DEFAULT AudioInfo(44100, 2, 16)
const int PIN_BCLK = 2;   // BCCLK bit-clock
const int PIN_WCLK = 3;   // LRCLK left-right clock
const int PIN_DOUT = 8;   // Data in/out (was GP4 — collided with SD_MISO_PIN)
const int PIN_MCLK = 9;   // Master-clock (was GP5 — collided with SD_CS_PIN)
const int MCLK_MULTI = 512;

extern AudioPlayer player;

// Configures I2S with this project's pins and starts playback from
// the SD card root. Call once from setup(), after initCard() succeeds.
void beginAudioPlayer();
