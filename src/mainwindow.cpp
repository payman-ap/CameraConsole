#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // --- Create QTimer -----------------------
    poll_timer_ = new QTimer(this); // The "this" makes MainWindow the parent. ownership model will automatically destroy the timer when the window is destroyed
    connect(
        poll_timer_,
        &QTimer::timeout,
        this,
        &MainWindow::onPollFrame
    );

    startPipeline();




}



void MainWindow::startPipeline()
{
    // --- Camera init -----------------------
    int deviceID = 1;
    int apiID = cv::CAP_V4L2;
    cap_.open(deviceID, apiID);
    if (!cap_.isOpened()) {
        ui->feedLabel->setText("Camera not found");
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

    capture_worker_ =
        std::thread(
            capture_thread,
            std::ref(cap_),
            std::ref(state_)
            );

    processing_worker_ =
        std::thread(
            processing_thread,
            std::ref(state_)
            );

    poll_timer_->start(16); // giving roughly 60 GUI updates per second. 1000 / 60 ≈ 16 ms

    state_.control.pipeline_ready = true;

    std::cout << "Pipeline started\n" << std::endl;

}

void MainWindow::stopPipeline()
{
    poll_timer_->stop();
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


void MainWindow::onPollFrame()
{

}



MainWindow::~MainWindow()
{
    stopPipeline();
    delete ui;
}
