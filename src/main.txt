#include <MotionDetector.h>

#include <iostream>

// demo_skeleton_webcam.cpp
//
// 即時攝影機骨架偵測 demo:
//   1. 用 MotionDetector::get_rois() 抓出動態區域
//   2. 對每個 ROI 裁切後丟進 detect_skeleton()
//   3. 把回傳的關鍵點座標加回 ROI 偏移量,畫在完整畫面上
//
// 編譯範例:
//   g++ demo_skeleton_webcam.cpp MotionDetector.cpp -o demo_skeleton_webcam \
//       `pkg-config --cflags --libs opencv4`

// COCO 17 點骨架的常見連線 (index 對應 detect_skeleton 輸出順序)
// 0:nose 1:left_eye 2:right_eye 3:left_ear 4:right_ear
// 5:left_shoulder 6:right_shoulder 7:left_elbow 8:right_elbow
// 9:left_wrist 10:right_wrist 11:left_hip 12:right_hip
// 13:left_knee 14:right_knee 15:left_ankle 16:right_ankle
static const int SKELETON_PAIRS[][2] = {
    {5, 6},              // 左肩-右肩
    {5, 7},   {7, 9},    // 左肩-左肘-左腕
    {6, 8},   {8, 10},   // 右肩-右肘-右腕
    {5, 11},  {6, 12},   // 肩-髖
    {11, 12},            // 左髖-右髖
    {11, 13}, {13, 15},  // 左髖-左膝-左踝
    {12, 14}, {14, 16},  // 右髖-右膝-右踝
};
static const int NUM_PAIRS = sizeof(SKELETON_PAIRS) / sizeof(SKELETON_PAIRS[0]);

// 把 ROI 座標系的關鍵點,轉換成完整畫面座標系,並過濾掉無效點 (-1,-1)
std::vector<cv::Point> offsetKeypoints(const std::vector<cv::Point>& kpts, const cv::Rect& roi) {
    std::vector<cv::Point> result;
    result.reserve(kpts.size());
    for (const auto& p : kpts) {
        if (p.x < 0 || p.y < 0) {
            result.emplace_back(-1, -1);  // 保留無效標記,方便後面跳過連線
            continue;
        }
        result.emplace_back(p.x + roi.x, p.y + roi.y);
    }
    return result;
}

// 畫關鍵點 + 骨架連線
void drawSkeleton(cv::Mat& frame, const std::vector<cv::Point>& kpts) {
    if (kpts.size() < 17) return;

    // 先畫連線(畫在點的下面,視覺上比較乾淨)
    for (int i = 0; i < NUM_PAIRS; ++i) {
        const cv::Point& a = kpts[SKELETON_PAIRS[i][0]];
        const cv::Point& b = kpts[SKELETON_PAIRS[i][1]];
        if (a.x < 0 || a.y < 0 || b.x < 0 || b.y < 0) continue;  // 跳過無效點
        cv::line(frame, a, b, cv::Scalar(255, 200, 0), 2);
    }

    // 再畫每個關鍵點
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

    // 建立偵測器,設定最小面積為 1000 像素
    MotionDetector detector(500, 25.0, false, 1000.0);
    cv::Mat frame;

    while (true) {
        cap >> frame;  // 讀取影格
        if (frame.empty()) break;

        // 取得感興趣區塊
        std::vector<cv::Rect> rois = detector.get_rois(frame);

        for (const auto& roi : rois) {
            // 邊界保護: ROI 可能會超出畫面範圍,裁切前先夾住合法範圍
            cv::Rect safeRoi = roi & cv::Rect(0, 0, frame.cols, frame.rows);
            if (safeRoi.width <= 0 || safeRoi.height <= 0) continue;

            cv::Mat roiFrame = frame(safeRoi);

            // 跑骨架偵測 (目前每幀都做 full detect,之後可以換成 detector.update())
            detector.detect_skeleton(roiFrame);
            std::vector<cv::Point> kpts = detector.get_skeleton_point();

            // 轉換回完整畫面座標系
            std::vector<cv::Point> kptsOnFrame = offsetKeypoints(kpts, safeRoi);

            // 畫 ROI 框 + 骨架
            cv::rectangle(frame, safeRoi, cv::Scalar(0, 255, 0), 2);
            drawSkeleton(frame, kptsOnFrame);
        }

        cv::imshow("Reaction Cam - Skeleton Detection", frame);

        // 檢查是否按下 ESC 鍵離開 (30ms 延遲)
        if (cv::waitKey(30) == 27) {
            break;
        }
    }

    cap.release();
    cv::destroyAllWindows();
    return 0;
}
