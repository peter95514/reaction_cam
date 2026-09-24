#pragma once

enum class ScreenId { Menu, Setting, Trial, Exit };

class Screen {
public:
    virtual ~Screen() = default;
    virtual void enter();
    virtual void exit();
    virtual ScreenId tick() = 0;
};
