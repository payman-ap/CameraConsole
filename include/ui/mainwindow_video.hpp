#ifndef MAINWINDOW_VIDEO_HPP
#define MAINWINDOW_VIDEO_HPP

#include <opencv2/opencv.hpp>
#include "video/camera_pipeline.h"

namespace VideoHelpers
{
    bool initializeCamera(
        cv::VideoCapture& cap
    );
}



#endif // MAINWINDOW_VIDEO_HPP