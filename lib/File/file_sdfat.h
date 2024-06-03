#include <SPI.h>

#include "SdFat.h"

#define SD_FAT_TYPE 3

#define SPI_SPEED SD_SCK_MHZ(4)

#if SD_FAT_TYPE == 0
SdFat sd;
File file;
#elif SD_FAT_TYPE == 1
SdFat32 sd;
File32 file;
#elif SD_FAT_TYPE == 2
SdExFat sd;
ExFile file;
#elif SD_FAT_TYPE == 3
SdFs sd;
FsFile file;
#else  // SD_FAT_TYPE
#error Invalid SD_FAT_TYPE
#endif  // SD_FAT_TYPE

const uint8_t SD_CS_PIN = 10;
SdFat sd;

