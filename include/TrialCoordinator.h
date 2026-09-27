#pragma once

#include <optional>

#include "TrialController.h"
#include "TrialScreen.h"

class TrialCoordinator {
private:
    TrialController controller_;

    bool armed_ = false;
    bool latched_ = false;
    Side side_ = Side::Left;
    Clock::time_point onTime_;

public:
    struct ReactionResult {
        bool passed;
        Clock::duration latency;
    };

    void start();
    void stop();

    void notifyLightOn(Side side, Clock::time_point t);
    void notifyLightOff();

    std::optional<ReactionResult> poll();
};
