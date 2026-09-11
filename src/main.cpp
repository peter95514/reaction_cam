#include <MotionDetector.h>

#include <iostream>

int main() {
    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        std::cerr << "錯誤: 無法開啟攝影機！" << std::endl;
        return -1;
    }

    // 建立偵測器，設定最小面積為 1000 像素
    MotionDetector detector(500, 25.0, false, 1000.0);
    cv::Mat frame;

    while (true) {
        cap >> frame;  // 讀取影格
        if (frame.empty()) break;

        // 取得感興趣區塊
        std::vector<cv::Rect> rois = detector.get_rois(frame);

        // 在畫面上繪製綠色的 Bounding Box
        for (const auto& roi : rois) {
            cv::rectangle(frame, roi, cv::Scalar(0, 255, 0), 2);
        }

        cv::imshow("Reaction Cam - C++ Motion ROI", frame);

        // 檢查是否按下 ESC 鍵離開 (30ms 延遲)
        if (cv::waitKey(30) == 27) {
            break;
        }
    }

    cap.release();
    cv::destroyAllWindows();
    return 0;
}
