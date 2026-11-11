#pragma once
#include <vector>

#include "ActionEvaluator.h"

enum class LightColor {Green, Red, Blue, Yellow};

struct LightSignal {
    int index = 0;// which light is lighting(idx from 0)
    LightColor color = LightColor::Green;
};

using Action = std::vector<AngleRule>;
using Cue = std::pair<LightSignal, Action>;
