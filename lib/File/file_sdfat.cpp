#include <file_sdfat.h>
#include "SdFat.h"

static bool initCard()
{
    if (!sd.begin(SD_CONFIG))
        return false;
    return root.open("/");
}

// indexing file in dir
static void indexFiles(const char *path)
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
        char fileName[FILE_NAME_DIM-2];
        if (file.isDir())
        {
            file.getName(fileName,sizeof(fileName));
            indexFiles(fileName);
            rc = addPrefix(INDEX_ROW_PREFIX_DIR, String(fileName));
        }
        else
        {
            file.getName(fileName,sizeof(fileName));
            rc = addPrefix(INDEX_ROW_PREFIX_FILE,String(fileName));
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

static long * loadIndexed(const char *dirPath)
{
    long * lineOffset;
    long cOffset = 0;
    String tempFilePath = String(dirPath) + "/" + INDEX_DB_FILE_NAME;
    file_t tempFile;
    if (!tempFile.open(tempFilePath.c_str(), O_RDONLY))
    {
        Serial.println("Failed to open index file");
        return;
    }
    fileCount = 0;
    //tempFile.fgets()
    while (tempFile.available())
    {
        cOffset++;
        char line[FILE_NAME_DIM];
        int n = tempFile.fgets(line,sizeof(line));
        if (line[n-1] != '\n' && n == (sizeof(line)-1)) {
            Serial.println("file name to long");
        } else {
            lineOffset[fileCount++] = cOffset;
        }
    }
    Serial.print("total file/folder indexing: ");
    Serial.println(fileCount);
    tempFile.close();
    return lineOffset;
}

static String readLine(const char * path, long offset) {
    file_t f;
    if (!f.open(path, O_RDONLY)) {
        Serial.println("can't open file");
        return "";
    }

    f.seekSet(offset);
    char line[FILE_NAME_DIM];
    while (f.available())
    {
        int n = f.fgets(line,sizeof(line));
        if (line[n-1] != '\n' && n == (sizeof(line)-1)) {
            Serial.println("file name to long");
        }
    }
    f.close();
    return String(line);
}
static String readLineByIndex(const char * path, long *offsets, int index) {
    return readLine(path,offsets[index]);
}

// check file by cut prefix;
static bool isFile(String line) {
    if (line.startsWith(INDEX_ROW_PREFIX_FILE)); return true;
    return false;
}
// get filename by cut prefix;
static String getFileName(String line) {
    return line.substring(2);
}