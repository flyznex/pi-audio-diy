#include "file_sdfat.h"
#include "SdFat.h"
sd_t sd;
file_t root;
bool initCard()
{
    SPI.setCS(SD_CS_PIN);
    SPI.setTX(7);
    SPI.setRX(4);
    SPI.setSCK(6);
    SPI.begin();
    if (!sd.begin(SD_CONFIG))
        return false;
    return root.open("/");
}

// indexing file in dir
void indexFiles(const char *path)
{
    file_t dirFile;
    if (!dirFile.open(path, O_RDONLY))
    {
        // TODO: print to display
        Serial.println("Failed to open directory");
        return;
    }
    dirFile.rewind();
    // TODO: can check time via RTC, set TTL to refresh file.
    // or set function to force reindex.
    String idxFileName = String(path) + "/" + INDEX_DB_FILE_NAME;
    sd.remove(idxFileName.c_str()); // check
    if (!idxFile.open(idxFileName.c_str()), O_RDWR | O_CREAT)
    {
        Serial.println("Failed to create temp file");
        return;
    }
    // scan folder and insert value to index file.
    // format:
    // prefix|<child-directory-name>
    // prefix|<filename.ext>
    // prefix = 0 is directory
    // prefix = 1 is file
    while (dirFile.openNext(&file, O_RDONLY))
    {
        // check is directory
        String rc;
        // we need 2 char for prefix
        char fileName[FILE_NAME_DIM - 2];
        if (file.isDir())
        {
            file.getName(fileName, sizeof(fileName));
            indexFiles(fileName);
            rc = addPrefix(INDEX_ROW_PREFIX_DIR, String(fileName));
        }
        else
        {
            file.getName(fileName, sizeof(fileName));
            rc = addPrefix(INDEX_ROW_PREFIX_FILE, String(fileName));
        }
        addRecord(&idxFile, rc);
        file.close();
    }
    dirFile.close();
    idxFile.close();
}

String addPrefix(String prefix, String s)
{
    return prefix + s;
}

void addRecord(Print *pr, String rc)
{
    pr->write(rc.c_str(), rc.length());
    pr->write("\n");
}

long *loadIndexed(const char *dirPath)
{
    long *lineOffsets;
    long cOffset = 0;
    String tempFilePath = String(dirPath) + "/" + INDEX_DB_FILE_NAME;
    file_t tempFile;
    if (!tempFile.open(tempFilePath.c_str(), O_RDONLY))
    {
        Serial.println("Failed to open index file");
        return lineOffsets;
    }
    fileCount = 0;
    // tempFile.fgets()
    while (tempFile.available())
    {
        cOffset++;
        char line[FILE_NAME_DIM];
        int n = tempFile.fgets(line, sizeof(line));
        if (line[n - 1] != '\n' && n == (sizeof(line) - 1))
        {
            Serial.println("file name to long");
        }
        else
        {
            lineOffsets[fileCount++] = cOffset;
        }
    }
    Serial.print("total file/folder indexing: ");
    Serial.println(fileCount);
    tempFile.close();
    return lineOffsets;
}

String readLine(const char *path, long offset)
{
    file_t f;
    if (!f.open(path, O_RDONLY))
    {
        Serial.println("can't open file");
        return "";
    }

    f.seekSet(offset);
    char line[FILE_NAME_DIM];
    while (f.available())
    {
        int n = f.fgets(line, sizeof(line));
        if (line[n - 1] != '\n' && n == (sizeof(line) - 1))
        {
            Serial.println("file name to long");
        }
    }
    f.close();
    return String(line);
}
String readLineByIndex(const char *path, long *offsets, int index)
{
    return readLine(path, offsets[index]);
}

String *readPagingWithIndex(const char *path, long *offset, int index, int psize)
{
    String *page = new String[psize];
    int startIdx = (index / psize) * psize;
    int endIdx = min(startIdx + psize, fileCount);
    file_t f;
    if (!f.open(path, O_RDONLY))
    {
        Serial.println("can't open file");
        return page;
    }
    for (int i = startIdx; i <= endIdx; i++)
    {
        f.seek(startIdx);
        char line[FILE_NAME_DIM];
        while (f.available())
        {
            int n = f.fgets(line, sizeof(line));
            if (line[n - 1] != '\n' && n == (sizeof(line) - 1))
            {
                Serial.println("file name to long");
            }
            page[i] = String(line);
        }
    }
    f.close();
    return page;
}

// check file by cut prefix;
bool isFile(String line)
{
    if (line.startsWith(INDEX_ROW_PREFIX_FILE))
        ;
    return true;
    return false;
}
// get filename by cut prefix;
String getFileName(String line)
{
    return line.substring(2);
}