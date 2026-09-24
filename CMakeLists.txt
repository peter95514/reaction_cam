cmake_minimum_required(VERSION 3.14)
project(ReactionTest VERSION 1.0)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(OpenCV REQUIRED)
find_package(OpenGL REQUIRED)

include(FetchContent)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(glfw  GIT_REPOSITORY https://github.com/glfw/glfw.git  GIT_TAG 3.4)
FetchContent_Declare(imgui GIT_REPOSITORY https://github.com/ocornut/imgui.git GIT_TAG v1.91.9)
FetchContent_MakeAvailable(glfw imgui)

add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp)
target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
target_link_libraries(imgui PUBLIC glfw OpenGL::GL)

include_directories(include ${OpenCV_INCLUDE_DIRS})
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cpp" "include/*.h" "include/*.hpp")

add_executable(ReactionTest ${SOURCES})
target_link_libraries(ReactionTest imgui glfw OpenGL::GL ${OpenCV_LIBS})

# 模型與字型複製到執行檔旁邊 (目錄不存在就跳過，避免這一步 build 失敗)
foreach(dir model fonts)
    if(EXISTS ${CMAKE_SOURCE_DIR}/${dir})
        add_custom_command(TARGET ReactionTest POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_SOURCE_DIR}/${dir} $<TARGET_FILE_DIR:ReactionTest>/${dir})
    endif()
endforeach()
