#ifndef PLAYER_H
#define PLAYER_H

#include "AudioTools/AudioPlayer.h"
#include "i2s_audio_init.h"
#include "file_sdfat.h"

#ifdef USE_SDFAT
#include "AudioLibs/AudioSourceSDFAT.h"
#endif

class FPlayer : public AudioPlayer {
public:
    FPlayer();
    FPlayer(const uint8_t *path);
    bool begin(int index = 0, bool isActive = true);
    bool previous(int offset = 1);
    bool next(int offset = 1);
    bool setWorkingDir(const uint8_t *path);
protected:
    I2SAudio i2sAudio;
    uint8_t *path;
    long *indexedFile;
};
#endif