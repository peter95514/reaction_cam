#include "TrialScreen.h"

#include <imgui.h>

#include "Screen.h"
#include "Ui.h"

void TrialScreen::enter() {
    state_ = State::Idle;
    this->idleEnd_ = Clock::now() + toDur(cfg_.countdown);
}

void TrialScreen::exit() {}

Clock::duration TrialScreen::toDur(float sec) {
    return std::chrono::duration_cast<Clock::duration>(
        std::chrono::duration<float>(sec));
}

Clock::duration TrialScreen::randomGap() {
    std::uniform_real_distribution<float> d(cfg_.gapMin, cfg_.gapMax);
    return toDur(d(rng_));
}

void TrialScreen::beginRun(Clock::time_point now) {
    state_ = State::On;
    runEnd_ = now + toDur(cfg_.duration);
    nextChange_ = now + randomGap();
}

ScreenId TrialScreen::tick() {
    ScreenId next = ScreenId::Trial;
    const auto now = Clock::now();
    if (this->state_ == State::Idle && now >= idleEnd_) beginRun(now);

    if (this->state_ != State::Idle) {
        if (now >= this->runEnd_) {
            return ScreenId::Menu;
        }
    }
    return next;
}
