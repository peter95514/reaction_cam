#pragma once
#include <imgui.h>

namespace ui {

// 佔滿整個視窗、沒有標題列的容器
inline bool beginFullscreen(const char* id) {
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->WorkPos);
    ImGui::SetNextWindowSize(vp->WorkSize);
    return ImGui::Begin(id, nullptr,
                        ImGuiWindowFlags_NoDecoration |
                            ImGuiWindowFlags_NoMove |
                            ImGuiWindowFlags_NoSavedSettings |
                            ImGuiWindowFlags_NoBringToFrontOnFocus);
}

inline void centeredText(const char* s) {
    float w = ImGui::CalcTextSize(s).x;
    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - w) * 0.5f);
    ImGui::TextUnformatted(s);
}

inline bool centeredButton(const char* label, float w, float h) {
    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - w) * 0.5f);
    return ImGui::Button(label, {w, h});
}

}  // namespace ui
