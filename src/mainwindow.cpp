#include "mainwindow.h"
#include "ui_mainwindow.h"


QPixmap matToPixmap(const cv::Mat& mat)
{
    cv::Mat rgb;

    cv::cvtColor(
        mat,
        rgb,
        cv::COLOR_BGR2RGB
        );
    QImage img(
        rgb.data,
        rgb.cols,
        rgb.rows,
        rgb.step,
        QImage::Format_RGB888
        );
    return QPixmap::fromImage(
        img.copy()
        ); //return copy Because QImage points to rgb.data
    // which disappears when the function exits, .copy() allocates Qt's own storage.
}



MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    filter_group_ = new QButtonGroup(this);
    filter_group_->addButton(ui->rbNone, 0);
    filter_group_->addButton(ui->rbGray, 1);
    filter_group_->addButton(ui->rbBlur, 2);
    filter_group_->addButton(ui->rbEdge, 3);

    connect(
        filter_group_,
        &QButtonGroup::idClicked,
        this,
        &MainWindow::onFilterChanged
    );

    connect(
        ui->btnExit,
        &QPushButton::clicked,
        this,
        &MainWindow::close
        );

    connect(
        ui->btnReset,
        &QPushButton::clicked,
        this,
        &MainWindow::onResetStats
        );



    // --- Create QTimer -----------------------
    poll_timer_ = new QTimer(this); // The "this" makes MainWindow the parent. ownership model will automatically destroy the timer when the window is destroyed
    connect(
        poll_timer_,
        &QTimer::timeout,
        this,
        &MainWindow::onPollFrame
    );

    startPipeline();

    display_last_time_ = std::chrono::steady_clock::now();



    state_.control.pipeline_ready = true;
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
    FramePacket packet;
    if(state_.processed_frame_queue.size()==0) return; // try getting data
    if(!state_.processed_frame_queue.pop(packet)) {
        poll_timer_->stop();
        return;
    }

    auto now = std::chrono::steady_clock::now();

    // Display-side FPS
    if (++display_frame_count_ % 30 == 0) {
        std::chrono::duration<double> dt = now - display_last_time_;
        display_fps_ = 30.0 / dt.count();
        display_last_time_ = now;
        display_frame_count_ = 0;
    }
    // Update FPS labels
    ui->lblCaptureFps->setText(
        QString::number(static_cast<int>(state_.capture_fps.load()))
    );
    ui->lblDisplayFps->setText(
        QString::number(static_cast<int>(display_fps_))
    );


    // // Latency
    // int64_t latency_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
    //                          now - packet.capture_time).count();

    // // Queue depth
    // state_.current_queue_depth = state_.processed_frame_queue.size();





    // Render frame
    if(packet.image.empty()) return; // empty frame check
    QPixmap pix = matToPixmap(packet.image); // convert
    QPixmap scaled_pix = pix.scaled(ui->feedLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation); // scale to qt
    ui->feedLabel->setPixmap(scaled_pix); // display in the label
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
    }
}

void MainWindow::onResetStats()
{
    std::cout << "Reset button pushed" << std::endl;
}



MainWindow::~MainWindow()
{
    stopPipeline();
    delete ui;
}
