# 設定最低支援的 CMake 版本 (3.12 以上支援自動偵測新檔案)
cmake_minimum_required(VERSION 3.12)

project(ReactionTest VERSION 1.0)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 尋找 OpenCV
find_package(OpenCV REQUIRED)

# ==========================================
# 1. 設定標頭檔 (include) 搜尋路徑
# ==========================================
# 讓 src 裡面的程式碼可以直接寫 #include "CameraManager.hpp"
# 而不需要寫成 #include "../include/CameraManager.hpp"
include_directories(include ${OpenCV_INCLUDE_DIRS})

# ==========================================
# 2. 自動收集原始碼與標頭檔
# ==========================================
# 掃描 src 底下所有 .cpp 與 include 底下所有 .hpp
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    "src/*.cpp"
    "include/*.hpp"
    "include/*.h"
)

# ==========================================
# 3. 建立執行檔與連結
# ==========================================
# 將剛才掃描到的所有檔案打包編譯
add_executable(ReactionTest ${SOURCES})

# 連結 OpenCV 動態函式庫
target_link_libraries(ReactionTest ${OpenCV_LIBS})

# ==========================================
# 4. 建置完成後,自動複製模型檔到執行檔目錄
# ==========================================
add_custom_command(
    TARGET ReactionTest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/model/yolov8n-pose.onnx"
        "${CMAKE_SOURCE_DIR}/model/yolov8n.onnx"
        "$<TARGET_FILE_DIR:ReactionTest>/yolov8n-pose.onnx"
        "$<TARGET_FILE_DIR:ReactionTest>/yolov8n.onnx"
    COMMENT "複製 YOLO pose 模型到執行目錄"
)
