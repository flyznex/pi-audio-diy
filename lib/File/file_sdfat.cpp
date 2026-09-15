#include "file_sdfat.h"
#include "SdFat.h"

sd_t sd;
file_t root;

bool initCard()
{
    SPI.setCS(SD_CS_PIN);
    SPI.setTX(SD_MOSI_PIN);
    SPI.setRX(SD_MISO_PIN);
    SPI.setSCK(SD_SCK_PIN);
    SPI.begin();
    if (!sd.begin(SD_CONFIG))
        return false;
    return root.open("/");
}

int listDir(const char *path, DirEntry *out, int maxEntries, int startIndex)
{
    file_t dir;
    if (!dir.open(path, O_RDONLY))
    {
        Serial1.println("Failed to open directory");
        return 0;
    }
    dir.rewind();

    file_t entry;
    int skipped = 0;
    int filled = 0;
    while (filled < maxEntries && entry.openNext(&dir, O_RDONLY))
    {
        if (skipped < startIndex)
        {
            skipped++;
            entry.close();
            continue;
        }
        entry.getName(out[filled].name, FILE_NAME_DIM);
        out[filled].isDir = entry.isDir();
        entry.close();
        filled++;
    }
    dir.close();
    return filled;
}
