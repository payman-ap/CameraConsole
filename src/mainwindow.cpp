#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

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

    // --- Create QTimer -----------------------
    poll_timer_ = new QTimer(this); // The "this" makes MainWindow the parent. ownership model will automatically destroy the timer when the window is destroyed
    connect(
        poll_timer_,
        &QTimer::timeout,
        this,
        &MainWindow::onPollFrame
    );






}


void MainWindow::onPollFrame()
{

}



MainWindow::~MainWindow()
{
    if(cap_.isOpened())
    {
        cap_.release();
    }
    delete ui;
}
