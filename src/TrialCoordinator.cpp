#include <TrialCoordinator.h>

void TrialCoordinator::start() {
    controller_.start();
}
void TrialCoordinator::stop() {
    controller_.stop();
}

void TrialCoordinator::notifyLightOn(const Cue &cue, Clock::time_point t) {
    controller_.setAction(cue.second);
    onTime_ = t;
    armed_ = true;
    latched_ = false;
}

void TrialCoordinator::notifyLightOff() {
    armed_ = false;
    controller_.setAction({});
}

std::optional<TrialCoordinator::ReactionResult> TrialCoordinator::poll() {
    if (!armed_ || latched_) return std::nullopt;

    for (const auto& r : controller_.snapshotRecords()) {
        if (r.t < onTime_ || !r.passed) continue;
        latched_ = true;
        return ReactionResult{true, r.t - onTime_};
    }
    return std::nullopt;
}
