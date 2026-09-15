#pragma once

#include <SPI.h>

#include "SdFat.h"

#define SPI_CLOCK SD_SCK_MHZ(4)

const size_t FILE_NAME_DIM = 50;
const uint8_t SD_CS_PIN = 5;
const uint8_t SD_MOSI_PIN = 7;
const uint8_t SD_MISO_PIN = 4;
const uint8_t SD_SCK_PIN = 6;

#if SD_FAT_TYPE == 0
typedef SdFat sd_t;
typedef File file_t;
#elif SD_FAT_TYPE == 1
typedef SdFat32 sd_t;
typedef File32 file_t;
#elif SD_FAT_TYPE == 2
typedef SdExFat sd_t;
typedef ExFile file_t;
#elif SD_FAT_TYPE == 3
typedef SdFs sd_t;
typedef FsFile file_t;
#else  // SD_FAT_TYPE
#error Invalid SD_FAT_TYPE
#endif  // SD_FAT_TYPE

extern sd_t sd;
extern file_t root;

// Try to select the best SD card configuration.
#if HAS_SDIO_CLASS
#define SD_CONFIG SdioConfig(FIFO_SDIO)
#elif ENABLE_DEDICATED_SPI
#define SD_CONFIG SdSpiConfig(SD_CS_PIN, DEDICATED_SPI, SPI_CLOCK)
#else  // HAS_SDIO_CLASS
#define SD_CONFIG SdSpiConfig(SD_CS_PIN, SHARED_SPI, SPI_CLOCK)
#endif  // HAS_SDIO_CLASS

bool initCard();

struct DirEntry {
    char name[FILE_NAME_DIM];
    bool isDir;
};

/// @brief List up to maxEntries children of `path`, skipping the first startIndex.
/// @param path directory to list
/// @param out caller-allocated array of at least maxEntries DirEntry
/// @param maxEntries capacity of `out`
/// @param startIndex how many entries to skip before filling `out`
/// @return number of entries written into `out`
int listDir(const char *path, DirEntry *out, int maxEntries, int startIndex = 0);
