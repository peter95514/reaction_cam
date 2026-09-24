#include <GLFW/glfw3.h>
#include <GuiController.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <stdexcept>

#include "Theme.h"
GuiController::GuiController(const std::string name, int width, int height) {
    if (!glfwInit()) throw std::runtime_error("glfwInit failed");

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    window_ = glfwCreateWindow(width, height, name.c_str(), nullptr, nullptr);
    if (!window_) {
        glfwTerminate();
        throw std::runtime_error("create window failed");
    }
    glfwMakeContextCurrent(window_);
    glfwSwapInterval(1);  // vsync

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().IniFilename = nullptr;  // 不產生 imgui.ini
    ui::setupTheme();

    ImGui_ImplGlfw_InitForOpenGL(window_, true);
    ImGui_ImplOpenGL3_Init("#version 330");
}

void GuiController::registerScreen(ScreenId Id,
                                   std::shared_ptr<Screen> screen) {
    screenid_[Id] = std::move(screen);
}

GuiController::~GuiController() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window_);
    glfwTerminate();
}

void GuiController::switchTo(ScreenId next) {
    screenid_.at(current_)->exit();
    current_ = next;
    if (current_ != ScreenId::Exit) screenid_.at(current_)->enter();
}

void GuiController::run(ScreenId start) {
    current_ = start;
    screenid_.at(current_)->enter();

    while (current_ != ScreenId::Exit && !glfwWindowShouldClose(window_)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ScreenId next = screenid_.at(current_)->tick();

        ImGui::Render();
        int w, h;
        glfwGetFramebufferSize(window_, &w, &h);
        glViewport(0, 0, w, h);
        glClearColor(0.07f, 0.08f, 0.10f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window_);

        if (next != current_) switchTo(next);
    }

    if (current_ != ScreenId::Exit) screenid_.at(current_)->exit();
}
