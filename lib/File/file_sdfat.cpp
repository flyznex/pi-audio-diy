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
        if (file.isDir())
        {
            String fname = file.FileName();
            indexFiles(fname);
            rc = addPrefix(INDEX_ROW_PREFIX_DIR, fname);
        }
        else
        {
            String fname = file.fileName();
            rc = addPrefix(INDEX_ROW_PREFIX_FILE,fname));
        }
        addRecord(idxFile, rc);
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

static void readingIndex(const char *dirPath)
{
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
        char ch;
        tempFile.read(&ch, 1);
        if (ch == '\n')
        {
            fileCount++;
        }
    }
    tempFile.close();
}