#pragma once
#include <map>
#include <memory>
#include <string>

#include "Screen.h"

struct GLFWwindow;

class GuiController {
private:
    std::map<ScreenId, std::shared_ptr<Screen>> screenid_;
    void switchTo(ScreenId next);

    GLFWwindow* window_;
    ScreenId current_ = ScreenId::Menu;

public:
    GuiController(const std::string name, int width, int height);
    ~GuiController();

    void registerScreen(ScreenId id, std::shared_ptr<Screen> screen);
    void run(ScreenId start);
};
