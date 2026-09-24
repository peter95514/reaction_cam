#pragma once
#include <chrono>
#include <functional>
#include <random>

#include "Screen.h"

enum class LightState { Idle, Waiting, Go };
using Clock = std::chrono::steady_clock;

class TrialScreen : public Screen {
private:
    void drawlight();

    LightState state_ = LightState::Idle;
    Clock::time_point lighting_time;
    std::mt19937 rng_{std::random_device{}()};

public:
    TrialScreen() = default;

    void enter() override;
    void exit() override;
    ScreenId tick() override;
};
