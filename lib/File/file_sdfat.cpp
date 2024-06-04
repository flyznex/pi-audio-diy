#include<file_sdfat.h>
#include "SdFat.h"

static bool init(u_int8_t cs) {
    if (!sd.begin(SD_CONFIG)) return false;
    return root.open("/");
}
