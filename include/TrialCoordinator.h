#pragma once

#include <optional>

#include "TrialController.h"
#include "Cue.h"

class TrialCoordinator {
private:
    TrialController controller_;

    bool armed_ = false;
    bool latched_ = false;
    Clock::time_point onTime_;


public:
    struct ReactionResult {
        bool passed;
        Clock::duration latency;
    };
    bool debugFrame(cv::Mat& out) const { return controller_.debugFrame(out); }  // DEBUG

    void start();
    void stop();

    void notifyLightOn(const Cue& cue, Clock::time_point t);
    void notifyLightOff();

    std::optional<ReactionResult> poll();
};
