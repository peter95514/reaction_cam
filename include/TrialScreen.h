#pragma once
#include <chrono>
#include <functional>
#include <random>
#include <vector>

#include "Cue.h"
#include "Screen.h"
#include "TrialCoordinator.h"

enum class State { Idle, Waiting, On };

class TrialScreen : public Screen {
public:
    struct Config {
        float countdown = 1.0f;
        float duration = 30.0f;
        float onSec = 1.0f;
        float gapMax = 1.0f;
        float gapMin = 0.4f;
        int lightCount = 2;
        std::vector<Cue> cues;
    };

private:
    State state_ = State::Idle;
    Clock::time_point lighting_time;
    std::mt19937 rng_{std::random_device{}()};

    Clock::time_point runEnd_;
    Clock::time_point nextChange_;
    Clock::time_point idleEnd_;

    Config cfg_;  // config
    bool islig_;  // is the light on?
    Cue cue_;

    Clock::duration toDur(float sec);
    Clock::duration randomGap();

    void beginRun(Clock::time_point now);
    void updateLight(Clock::time_point now);
    void drawLight();

    TrialCoordinator coordinator_;

public:
    TrialScreen();

    void enter() override;
    void exit() override;
    ScreenId tick() override;
};
