#include "MenuScreen.h"

#include "Ui.h"
#include "imgui.h"

ScreenId MenuScreen::tick() {
    ScreenId next = ScreenId::Menu;

    ui::beginFullscreen("##Menu");
    ImGui::Dummy({0, ui::px(100)});          // 修改
    ui::setFontScale(2.2f);                  // 修改
    ui::centeredText("反應力測試");
    ui::setFontScale(1.0f);                  // 修改
    ImGui::Dummy({0, ui::px(50)});

    if (ui::centeredButton("開始測試", 340, 68)) next = ScreenId::Trial;
    if (ui::centeredButton("設定", 340, 68)) next = ScreenId::Setting;
    if (ui::centeredButton("離開", 340, 68)) next = ScreenId::Exit;

    ImGui::End();

    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) next = ScreenId::Exit;
    return next;
}

MenuScreen::~MenuScreen() {}
void MenuScreen::enter() {}
void MenuScreen::exit() {}
