#include <SPI.h>

#include "SdFat.h"

#define SD_FAT_TYPE 3

#define SPI_CLOCK SD_SCK_MHZ(4)

#define INDEX_DB_FILE_NAME ".index"
#define INDEX_ROW_PREFIX_FILE "1|"
#define INDEX_ROW_PREFIX_DIR "0|"
const size_t LINE_DIM = 50;

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

sd_t sd;
file_t file;
file_t root;
file_t idxFile;
const uint8_t SD_CS_PIN = 10;
int fileCount = 0;


// Try to select the best SD card configuration.
#if HAS_SDIO_CLASS
#define SD_CONFIG SdioConfig(FIFO_SDIO)
#elif ENABLE_DEDICATED_SPI
#define SD_CONFIG SdSpiConfig(SD_CS_PIN, DEDICATED_SPI, SPI_CLOCK)
#else  // HAS_SDIO_CLASS
#define SD_CONFIG SdSpiConfig(SD_CS_PIN, SHARED_SPI, SPI_CLOCK)
#endif  // HAS_SDIO_CLASS

bool initCard();
void indexFiles(const char * path);
void readingIndex(const char * dirPath);
