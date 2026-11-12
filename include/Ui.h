#pragma once
#include <imgui.h>

#include <algorithm>

namespace ui {

// 設計基準解析度：所有寫死的尺寸都以 1280x720 為 1.0 倍
constexpr float kBaseW = 1280.f;
constexpr float kBaseH = 720.f;

// 依目前視窗大小算出縮放倍率，取較小邊避免內容超出畫面
inline float scale() {
    const ImVec2 s = ImGui::GetMainViewport()->WorkSize;
    return std::clamp(std::min(s.x / kBaseW, s.y / kBaseH), 0.5f, 4.0f);
}

// 把「基準尺寸」轉成目前實際像素
inline float px(float v) { return v * scale(); }

// 目前視窗的字型縮放 = mul * 視窗倍率 (mul 是相對於一般文字的倍數)
inline void setFontScale(float mul) {
    ImGui::SetWindowFontScale(mul * scale());
}

// 佔滿整個視窗、沒有標題列的容器
inline bool beginFullscreen(const char* id) {
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->WorkPos);
    ImGui::SetNextWindowSize(vp->WorkSize);
    const bool open = ImGui::Begin(
        id, nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoBringToFrontOnFocus);
    setFontScale(1.0f);  // 必須在 Begin 之後呼叫，所以放這裡統一處理
    return open;
}

inline void centeredText(const char* s) {
    float w = ImGui::CalcTextSize(s).x;
    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - w) * 0.5f);
    ImGui::TextUnformatted(s);
}

// w、h 傳入基準尺寸 (例如 340, 68)，函式內部自動縮放
inline bool centeredButton(const char* label, float w, float h) {
    w = px(w);
    h = px(h);
    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - w) * 0.5f);
    return ImGui::Button(label, {w, h});
}

}  // namespace ui
