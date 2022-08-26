#pragma once

#define SCALE(x, oldMin, oldMax, targetMin, targetMax) \
    (((x - oldMin) / (oldMax - oldMin)) * (targetMax - targetMin) + targetMin)

#define COUNTOF(x) (sizeof(x) / sizeof(x[0]))

bool RunningFromHBL();

float frand(float min = 0.0f, float max = 1.0f);
