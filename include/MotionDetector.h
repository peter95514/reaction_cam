#pragma once
#include <opencv2/dnn/dnn.hpp>
#include <opencv2/opencv.hpp>
#include <vector>

class MotionDetector {
private:
    cv::dnn::Net net;

    cv::Ptr<cv::BackgroundSubtractorMOG2> bg_subtractor;
    double min_area;

    bool is_tracking_skeleton_or_not;
    cv::Rect skeleton_bbox;

public:
    MotionDetector(int history = 500, double varThreshold = 25.0, bool detectShadows = false, double minArea = 500);
    std::vector<cv::Rect> get_rois(const cv::Mat& frame);
    bool is_tracking_skeleton();
    void tracking_skeleton();
    cv::Rect get_skeleton();
    ~MotionDetector();
};
