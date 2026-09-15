#include "fplayer.h"

FPlayer::FPlayer() : AudioPlayer()
{
    setWorkingDir("/");
}

FPlayer::FPlayer(const uint8_t *path) : AudioPlayer()
{
    setWorkingDir(path);
}

bool FPlayer::begin(int index, bool isActive)
{
    // Track selection lands in Phase 2 — see
    // docs/superpowers/specs/2026-09-15-sd-audio-player-continuation-design.md §5.
    return false;
}
bool FPlayer::previous(int offset)
{
}
bool FPlayer::next(int offset)
{
}
bool FPlayer::setWorkingDir(const uint8_t *path)
{
    memcpy(this->path, path, strlen((const char *)path));
}