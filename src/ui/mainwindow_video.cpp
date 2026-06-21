#include "ui/mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>

#include "ui/mainwindow_styles.hpp"


void MainWindow::startVideoPipeline()
{
    // --- Camera init -----------------------
    constexpr int deviceID = 1;
    constexpr int apiID = cv::CAP_V4L2;
    cap_.open(deviceID, apiID);
    if(!cap_.isOpened())
    {
        cap_.open(0, apiID); // check diffult cam connection
    }
    if (!cap_.isOpened()) {
        ui->feedLabel->setText("Camera not found");
        return;
    }
    else
    {
        ui->feedLabel->setText("Camera ready");
    }
    cap_.set(
        cv::CAP_PROP_FOURCC,
        cv::VideoWriter::fourcc('M','J','P','G')
        );
    cap_.set(
        cv::CAP_PROP_FRAME_WIDTH,
        1280
        );
    cap_.set(
        cv::CAP_PROP_FRAME_HEIGHT,
        720
        );

    capture_worker_ = std::thread(capture_thread, std::ref(cap_), std::ref(state_));
    processing_worker_ = std::thread(processing_thread, std::ref(state_));

    poll_timer_->start(16); // giving roughly 60 GUI updates per second. 1000 / 60 ≈ 16 ms

    state_.control.pipeline_ready = true;

    // Set labels
    ui->lblVideoDeviceName->setText(
        QString::fromStdString(cap_.getBackendName())
        );
    ui->lblAudioInputDevice->setText("No Input");
    ui->lblAudioOutputDevice->setText("No Output");

    std::cout << "Pipeline started\n" << std::endl;

}

void MainWindow::stopVideoPipeline()
{
    if(poll_timer_) poll_timer_->stop();
    state_.control.running = false;
    // Wake queues:
    state_.raw_frame_queue.notify_shutdown();
    state_.processed_frame_queue.notify_shutdown();

    if(capture_worker_.joinable())
        capture_worker_.join();
    if(processing_worker_.joinable())
        processing_worker_.join();

    if(cap_.isOpened())
        cap_.release();

}



void MainWindow::onVideoStartStop()
{
    video_running_ = !video_running_;

    if(video_running_)
    {
        ui->btnVideoStart->setText("Stop Video");
        ui->btnVideoStart->setStyleSheet(
            UiStyle::videoStop()
        );
        qDebug() << "Video started";
    }
    else
    {
        ui->btnVideoStart->setText("Start Video");
        ui->btnVideoStart->setStyleSheet(
            UiStyle::videoStart()
        );
        qDebug() << "Video stopped";
    }
}



void MainWindow::onFilterChanged(int id)
{
    switch(id)
    {
    case 0:
        state_.control.filter_type = FilterType::none;
        break;

    case 1:
        state_.control.filter_type = FilterType::grayscale;
        break;

    case 2:
        state_.control.filter_type = FilterType::blur;
        break;

    case 3:
        state_.control.filter_type = FilterType::edge;
        break;
    default:
        break;
    }
}

