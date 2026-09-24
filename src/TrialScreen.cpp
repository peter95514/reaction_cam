#include "TrialScreen.h"

#include <imgui.h>

#include "Screen.h"
#include "Ui.h"

void TrialScreen::enter() {}
void TrialScreen::exit() {}
ScreenId TrialScreen::tick() {
    ScreenId next = ScreenId::Trial;

    ui::beginFullscreen("##Trial");
    ImGui::Dummy({0, 100});
    ImGui::SetWindowFontScale(2.2f);
    ui::centeredText("測試");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::Dummy({0, 50});

    if (ui::centeredButton("開始測試", 340, 68)) next = ScreenId::Trial;
    if (ui::centeredButton("設定", 340, 68)) next = ScreenId::Setting;
    if (ui::centeredButton("離開", 340, 68)) next = ScreenId::Exit;

    ImGui::End();

    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) next = ScreenId::Exit;

    return next;
}
