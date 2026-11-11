#pragma once
#include <chrono>

enum class ScreenId { Menu, Setting, Trial, Exit };
using Clock = std::chrono::steady_clock;

class Screen {
public:
    virtual ~Screen() = default;
    virtual void enter(){};
    virtual void exit(){};
    virtual ScreenId tick() = 0;
};
