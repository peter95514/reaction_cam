#include <iostream>
#include <opencv4/opencv2/opencv.hpp>
#include <ostream>

int main() {
    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        std::cerr << "err!! cant find the cam!!" << std::endl;
        return -1;
    }
    std::cout << "find the cam!!" << std::endl;
}
