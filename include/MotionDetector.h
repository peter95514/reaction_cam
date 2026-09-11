#pragma once
#include <opencv2/opencv.hpp>
#include <vector>

class MotionDetector {
private:
    cv::Ptr<cv::BackgroundSubtractorMOG2> bg_subtractor;
    double min_area;

public:
    MotionDetector(int history = 500, double varThreshold = 25.0, bool detectShadows = false, double minArea = 500);
    std::vector<cv::Rect> get_rois(const cv::Mat& frame);
    ~MotionDetector();
};
