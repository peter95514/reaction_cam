// motion_detector_profiler.cpp
//
// 用途：直接量測你現有 MotionDetector 的兩個 DNN 階段耗時：
//   1) get_rois()        -> yolov8n.onnx (人物偵測/ROI)
//   2) detect_skeleton()  -> yolov8n-pose.onnx (骨架推論，含內部 resize/blob/後處理)
// 另外也量測讀取影格、裁切 ROI 的耗時，方便判斷瓶頸在哪一段。
//
// 編譯 (依你環境調整 OpenCV include/lib 路徑)：
//   g++ -O2 -std=c++17 motion_detector_profiler.cpp MotionDetector.cpp \
//       `pkg-config --cflags --libs opencv4` -o profiler
//
// 執行：
//   ./profiler <video_path_or_camera_index>
//   例如：./profiler 0            (使用預設攝影機)
//   例如：./profiler test.mp4     (使用影片檔)

#include <MotionDetector.h>

#include <chrono>
#include <cstdio>
#include <numeric>
#include <opencv2/opencv.hpp>
#include <string>
#include <vector>

using Clock = std::chrono::steady_clock;

static double ms_between(const Clock::time_point &a, const Clock::time_point &b) {
    return std::chrono::duration<double, std::milli>(b - a).count();
}

struct StageStats {
    std::string name;
    std::vector<double> samples_ms;

    double total() const {
        return std::accumulate(samples_ms.begin(), samples_ms.end(), 0.0);
    }
    double average() const {
        return samples_ms.empty() ? 0.0 : total() / samples_ms.size();
    }
};

int main(int argc, char **argv) {
    const int NUM_FRAMES = 300;    // 想跑幾幀就改這裡
    const int WARMUP_FRAMES = 10;  // 前幾幀不計入統計 (避免模型初次載入/cache 影響)

    // ---------- 開啟影像來源 ----------
    cv::VideoCapture cap;
    if (argc > 1) {
        std::string src = argv[1];
        bool is_number = !src.empty() && std::all_of(src.begin(), src.end(), ::isdigit);
        if (is_number) {
            cap.open(std::stoi(src));
        } else {
            cap.open(src);
        }
    } else {
        cap.open(0);  // 預設用攝影機
    }

    if (!cap.isOpened()) {
        std::fprintf(stderr, "無法開啟影像來源，請確認路徑或攝影機編號\n");
        return 1;
    }

    // ---------- 初始化你的 MotionDetector ----------
    // 參數請依你實際使用的 history / varThreshold / detectShadows / minArea 調整
    MotionDetector detector(500, 16.0, true, 500.0);

    StageStats read_s{"read"};
    StageStats roi_s{"get_rois (yolov8n)"};
    StageStats crop_s{"crop ROI"};
    StageStats skeleton_s{"detect_skeleton (yolov8n-pose)"};

    std::vector<double> end_to_end_ms;
    end_to_end_ms.reserve(NUM_FRAMES);

    int frame_count = 0;
    cv::Mat frame;

    while (frame_count < NUM_FRAMES + WARMUP_FRAMES) {
        auto t_start = Clock::now();

        auto t0 = Clock::now();
        bool ok = cap.read(frame);
        auto t1 = Clock::now();
        if (!ok || frame.empty()) {
            std::fprintf(stderr, "讀不到影格，提前結束 (共讀了 %d 幀)\n", frame_count);
            break;
        }

        auto t_roi_start = Clock::now();
        std::vector<cv::Rect> rois = detector.get_rois(frame);
        auto t_roi_end = Clock::now();

        if (rois.empty()) {
            // 單人場景沒偵測到人時跳過這幀的骨架量測，但仍計入 read 時間
            frame_count++;
            continue;
        }

        auto t_crop_start = Clock::now();
        cv::Mat roi_frame = frame(rois[0]).clone();
        auto t_crop_end = Clock::now();

        auto t_skel_start = Clock::now();
        detector.detect_skeleton(roi_frame);
        auto t_skel_end = Clock::now();

        auto t_end = Clock::now();

        if (frame_count >= WARMUP_FRAMES) {
            read_s.samples_ms.push_back(ms_between(t0, t1));
            roi_s.samples_ms.push_back(ms_between(t_roi_start, t_roi_end));
            crop_s.samples_ms.push_back(ms_between(t_crop_start, t_crop_end));
            skeleton_s.samples_ms.push_back(ms_between(t_skel_start, t_skel_end));
            end_to_end_ms.push_back(ms_between(t_start, t_end));
        }

        frame_count++;
    }

    double total_all = read_s.total() + roi_s.total() + crop_s.total() + skeleton_s.total();

    auto print_stage = [&](const StageStats &s) {
        double pct = total_all > 0 ? (s.total() / total_all * 100.0) : 0.0;
        std::printf("%-32s avg %8.3f ms   佔比 %5.1f%%\n", s.name.c_str(), s.average(), pct);
    };

    double avg_e2e = end_to_end_ms.empty()
                         ? 0.0
                         : std::accumulate(end_to_end_ms.begin(), end_to_end_ms.end(), 0.0) / end_to_end_ms.size();

    std::printf("===== MotionDetector Profiling Result (有效統計 %zu 幀，warmup %d 幀已排除) =====\n",
                end_to_end_ms.size(), WARMUP_FRAMES);
    print_stage(read_s);
    print_stage(roi_s);
    print_stage(crop_s);
    print_stage(skeleton_s);
    std::printf("------------------------------------------------------------\n");
    std::printf("端到端平均延遲: %8.3f ms  (~%.1f FPS)\n", avg_e2e, avg_e2e > 0 ? 1000.0 / avg_e2e : 0.0);

    return 0;
}
