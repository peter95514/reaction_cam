#include <memory>

#include "GuiController.h"
#include "MenuScreen.h"
#include "TrialScreen.h"

int main() {
    GuiController gui("反應力測試", 1280, 720);

    gui.registerScreen(ScreenId::Menu, std::make_shared<MenuScreen>());
    gui.registerScreen(ScreenId::Trial, std::make_shared<TrialScreen>());

    gui.run(ScreenId::Menu);
    return 0;
}
