#include <TrialCoordinator.h>

void TrialCoordinator::start() {
    controller_.start();
}
void TrialCoordinator::stop() {
    controller_.stop();
}

void TrialCoordinator::notifyLightOn(Side side, Clock::time_point t) {
    controller_.resetRecords();
    side_ = side;
    onTime_ = t;
    armed_ = true;
    latched_ = false;
}

void TrialCoordinator::notifyLightOff() {
    armed_ = false;
    controller_.resetRecords();
}

std::optional<TrialCoordinator::ReactionResult> TrialCoordinator::poll() {
    if (!armed_ || latched_) return std::nullopt;

    for (const auto& r : controller_.snapshotRecords()) {
        bool pass = (side_ == Side::Left) ? r.lpass : r.rpass;
        if (pass) {
            latched_ = true;
            return ReactionResult{true, r.t - onTime_};
        }
    }
    return std::nullopt;
}
