#include <ActionEvaluator.h>
#include <MotionDetector.h>

#include <cstdio>
#include <iostream>
#include <string>

// COCO 17 點骨架連線
static const int SKELETON_PAIRS[][2] = {
    {5, 6},  {5, 7},   {7, 9},   {6, 8},   {8, 10},  {5, 11},
    {6, 12}, {11, 12}, {11, 13}, {13, 15}, {12, 14}, {14, 16},
};
static const int NUM_PAIRS = sizeof(SKELETON_PAIRS) / sizeof(SKELETON_PAIRS[0]);

// ROI 座標 -> 完整畫面座標 (僅用於顯示)
std::vector<cv::Point> offsetKeypoints(const std::vector<cv::Point>& kpts,
                                       const cv::Rect& roi) {
    std::vector<cv::Point> result;
    result.reserve(kpts.size());
    for (const auto& p : kpts) {
        if (p.x < 0 || p.y < 0) {
            result.emplace_back(-1, -1);
            continue;
        }
        result.emplace_back(p.x + roi.x, p.y + roi.y);
    }
    return result;
}

void drawSkeleton(cv::Mat& frame, const std::vector<cv::Point>& kpts) {
    if (kpts.size() < 17) return;

    for (int i = 0; i < NUM_PAIRS; ++i) {
        const cv::Point& a = kpts[SKELETON_PAIRS[i][0]];
        const cv::Point& b = kpts[SKELETON_PAIRS[i][1]];
        if (a.x < 0 || a.y < 0 || b.x < 0 || b.y < 0) continue;
        cv::line(frame, a, b, cv::Scalar(255, 200, 0), 2);
    }
    for (const auto& p : kpts) {
        if (p.x < 0 || p.y < 0) continue;
        cv::circle(frame, p, 4, cv::Scalar(0, 255, 0), -1);
    }
}

int main() {
    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        std::cerr << "錯誤: 無法開啟攝影機！" << std::endl;
        return -1;
    }

    MotionDetector detector(500, 25.0, false, 1000.0);

    // ---------- 評估器：左手舉起 ----------
    ActionEvaluator evaluator(0.4f, 80.f, 0.5f);
    evaluator.changerule({
        {"L_shoulder", L_HIP, L_SHOULDER, L_ELBOW, 140.f, 180.f, 2.0f},
        {"L_elbow", L_SHOULDER, L_ELBOW, L_WRIST, 130.f, 180.f, 1.0f},
    });

    const int DETECT_INTERVAL = 5;  // 每 5 幀跑一次骨架偵測
    int frameIdx = 0;

    // 保留最近一次結果，沒做偵測的幀也持續顯示，避免閃爍
    bool hasPerson = false;
    cv::Rect lastRoi;
    std::vector<cv::Point> lastKpts;
    EvalResult lastResult;

    cv::Mat frame;
    while (true) {
        cap >> frame;
        if (frame.empty()) break;
        frameIdx++;

        // 單人場景：只取第一個 ROI
        std::vector<cv::Rect> rois = detector.get_rois(frame);
        cv::Rect safeRoi;
        bool roiOk = false;
        if (!rois.empty()) {
            safeRoi = rois[0] & cv::Rect(0, 0, frame.cols, frame.rows);
            roiOk = safeRoi.width > 0 && safeRoi.height > 0;
        }

        if (!roiOk) {
            hasPerson = false;
            lastKpts.clear();
        } else if (frameIdx % DETECT_INTERVAL == 0 || !hasPerson) {
            cv::Mat roiFrame = frame(safeRoi);
            detector.detect_skeleton(roiFrame);

            // 評估用 ROI 座標系即可 (角度不受平移影響)
            lastResult = evaluator.evaluate(detector.get_keypoints());

            // 顯示才需要加回 ROI 偏移量
            lastKpts = offsetKeypoints(detector.get_skeleton_point(), safeRoi);
            lastRoi = safeRoi;
            hasPerson = true;

            // 終端機印出實際角度，方便調 minDeg / maxDeg
            for (const auto& d : lastResult.details) {
                if (d.valid)
                    std::printf("%s: %.1f deg %s\n", d.name.c_str(), d.angle,
                                d.inRange ? "[OK]" : "");
                else
                    std::printf("%s: (not visible)\n", d.name.c_str());
            }
            std::printf("valid=%d score=%.0f passed=%d\n---\n",
                        lastResult.valid, lastResult.score, lastResult.passed);
        }

        // ---------- 繪製 ----------
        if (hasPerson) {
            cv::rectangle(frame, lastRoi, cv::Scalar(0, 255, 0), 2);
            drawSkeleton(frame, lastKpts);
        }

        std::string text;
        cv::Scalar color;
        if (!hasPerson) {
            text = "No person";
            color = cv::Scalar(200, 200, 200);
        } else if (!lastResult.valid) {
            text = "Cannot see arm clearly";
            color = cv::Scalar(0, 255, 255);  // 黃：無效幀
        } else if (lastResult.passed) {
            text = cv::format("LEFT HAND UP  (score %.0f)", lastResult.score);
            color = cv::Scalar(0, 255, 0);  // 綠：通過
        } else {
            text = cv::format("Not raised  (score %.0f)", lastResult.score);
            color = cv::Scalar(0, 0, 255);  // 紅：未通過
        }
        cv::putText(frame, text, {10, 30}, cv::FONT_HERSHEY_SIMPLEX, 0.8, color,
                    2);

        // 畫面上顯示各規則角度
        if (hasPerson) {
            int y = 60;
            for (const auto& d : lastResult.details) {
                std::string line = d.valid ? cv::format("%s: %.0f deg",
                                                        d.name.c_str(), d.angle)
                                           : d.name + ": n/a";
                cv::putText(frame, line, {10, y}, cv::FONT_HERSHEY_SIMPLEX, 0.6,
                            cv::Scalar(255, 255, 255), 2);
                y += 25;
            }
        }

        cv::imshow("Reaction Cam - Skeleton Detection", frame);
        if (cv::waitKey(1) == 27) break;  // ESC 離開
    }

    cap.release();
    cv::destroyAllWindows();
    return 0;
}
