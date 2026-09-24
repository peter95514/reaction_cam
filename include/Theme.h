#pragma once
#include <imgui.h>

#include <filesystem>

namespace ui {

inline void setupTheme() {
    ImGuiIO& io = ImGui::GetIO();

    const char* fontPath = "fonts/NotoSansTC-Regular.ttf";
    ImFont* font = nullptr;
    if (std::filesystem::exists(fontPath))
        font = io.Fonts->AddFontFromFileTTF(
            fontPath, 22.0f, nullptr, io.Fonts->GetGlyphRangesChineseFull());
    if (!font) io.Fonts->AddFontDefault();  // 找不到字型時中文會顯示成 ?

    ImGui::StyleColorsDark();
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowRounding = 0;
    s.FrameRounding = 12;
    s.FramePadding = {14, 10};
    s.ItemSpacing = {14, 18};
    s.WindowPadding = {24, 24};
    s.FrameBorderSize = 0;

    ImVec4* c = s.Colors;
    const ImVec4 accent{0.29f, 0.56f, 1.00f, 1.00f};
    c[ImGuiCol_WindowBg] = {0.07f, 0.08f, 0.10f, 1.0f};
    c[ImGuiCol_Button] = {0.20f, 0.36f, 0.75f, 1.0f};
    c[ImGuiCol_ButtonHovered] = accent;
    c[ImGuiCol_ButtonActive] = {0.20f, 0.45f, 0.90f, 1.0f};
    c[ImGuiCol_Text] = {0.92f, 0.94f, 0.97f, 1.0f};
}

}  // namespace ui
