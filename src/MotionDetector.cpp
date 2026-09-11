#include <MotionDetector.h>

MotionDetector::MotionDetector(int history, double varThreshold, bool detectShadows, double minArea) {
    bg_subtractor = cv::createBackgroundSubtractorMOG2(history, varThreshold, detectShadows);
    min_area = minArea;
}

std::vector<cv::Rect> MotionDetector::get_rois(const cv::Mat &frame) {
    std::vector<cv::Rect> rois;
    cv::Mat gray, blurred, fg_mask, dilated;

    cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
    cv::GaussianBlur(gray, blurred, cv::Size(5, 5), 0);

    bg_subtractor->apply(blurred, fg_mask);

    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5));
    cv::dilate(fg_mask, dilated, kernel);

    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;
    cv::findContours(dilated, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    for (size_t i = 0; i < contours.size(); i++) {
        if (cv::contourArea(contours[i]) >= min_area) {
            rois.push_back(cv::boundingRect(contours[i]));
        }
    }

    return rois;
}
MotionDetector::~MotionDetector() {}
