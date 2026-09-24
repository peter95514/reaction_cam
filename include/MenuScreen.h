#pragma once
#include "Screen.h"
#include "Ui.h"

class MenuScreen : public Screen {
public:
    ScreenId tick() override;
    void enter() override;
    void exit() override;
    ~MenuScreen();
};
