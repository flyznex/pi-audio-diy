#include <SPI.h>

#include "SdFat.h"

#define SPI_CLOCK SD_SCK_MHZ(4)

#define INDEX_DB_FILE_NAME ".index"
#define INDEX_ROW_PREFIX_FILE "1|"
#define INDEX_ROW_PREFIX_DIR "0|"
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
extern file_t file;
extern file_t root;
extern file_t idxFile;
extern int fileCount;
extern long currentIndex;

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
// return array contains list offset of file;
long * loadIndexed(const char * dirPath);
/// @brief read a line of file at offset
/// @param path file
/// @param offset offset want to get of file
/// @return line typed string
String readLine(const char * path, long offset);
/// @brief read a line of file at index (line number)
/// @param path file path
/// @param offsets array offsets of file, result of loadIndexed
/// @param index index (line number)
/// @return line typed string 
String readLineByIndex(const char * path, long *offsets, int index);

String * readPagingWithIndex(const char *path, long *offset, int index, int psize = 4);
// check file by cut prefix;
bool isFile(String line);
// get filename by cut prefix;
String getFileName(String line);

void addRecord(Print *pr, String rc);
String addPrefix(String prefix, String s);