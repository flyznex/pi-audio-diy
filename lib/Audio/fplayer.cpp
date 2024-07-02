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
    String fname = getFileName(readLineByIndex((const char *)path, indexedFile, index));
    setPath(fname.c_str());
    //set source
    
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