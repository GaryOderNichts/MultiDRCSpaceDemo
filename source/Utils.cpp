#include "Utils.hpp"

#include <stdlib.h>
#include <coreinit/title.h>

#define HBL_TITLE_ID 0x0005000013374842
#define MII_MAKER_JPN_TITLE_ID 0x000500101004A000
#define MII_MAKER_USA_TITLE_ID 0x000500101004A100
#define MII_MAKER_EUR_TITLE_ID 0x000500101004A200

bool RunningFromHBL()
{
    uint64_t titleID = OSGetTitleID();

    return titleID == HBL_TITLE_ID ||
        titleID == MII_MAKER_JPN_TITLE_ID ||
        titleID == MII_MAKER_USA_TITLE_ID ||
        titleID == MII_MAKER_EUR_TITLE_ID;
}

float frand(float min, float max)
{
    return min + (float) rand() / ((float) RAND_MAX / (max - min));
}
