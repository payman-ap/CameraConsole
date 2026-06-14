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

    setupUI();

    setupFilterGroup();

    setupButtons();

    setupTimer();

    setupStats();

    display_last_time_ = std::chrono::steady_clock::now();

    startPipeline();
}

void MainWindow::setupUI()
{

}
void MainWindow::setupButtons()
{
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

    connect(
        ui->btnVideoStart,
        &QPushButton::clicked,
        this,
        &MainWindow::onVideoStartStop
        );

    connect(
        ui->btnAudioStart,
        &QPushButton::clicked,
        this,
        &MainWindow::onAudioStartStop
        );

    connect(
        ui->btnMute,
        &QPushButton::clicked,
        this,
        &MainWindow::onAudioMute
        );

    connect(
        ui->sliderAudioGain,
        &QSlider::valueChanged,
        this,
        &MainWindow::onAudioGainChanged
        );
}
void MainWindow::setupTimer()
{
    // --- Create QTimer -----------------------
    poll_timer_ = new QTimer(this); // The "this" makes MainWindow the parent. ownership model will automatically destroy the timer when the window is destroyed
    connect(
        poll_timer_,
        &QTimer::timeout,
        this,
        &MainWindow::onPollFrame
        );
}

void MainWindow::setupFilterGroup()
{
    filter_group_ = new QButtonGroup(this);
    filter_group_->addButton(ui->rbNone, 0);
    filter_group_->addButton(ui->rbGray, 1);
    filter_group_->addButton(ui->rbBlur, 2);
    filter_group_->addButton(ui->rbEdge, 3);
}
void MainWindow::setupStats()
{

}

void MainWindow::startPipeline()
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

void MainWindow::stopPipeline()
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


void MainWindow::onPollFrame()
{
    // FramePacket packet;
    // if(state_.processed_frame_queue.size()==0) return; // try getting data
    // if(!state_.processed_frame_queue.pop(packet)) {
    //     poll_timer_->stop();
    //     return;
    // }
    FramePacket packet;
    if(!state_.processed_frame_queue.try_pop(packet))
    {
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

    // Latency
    int64_t latency_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                             now - packet.capture_time).count();
    ui->lblLatency->setText(
        QString::number(latency_ms)
    );


    // Render frame
    if(packet.image.empty()){
        return; // empty frame check
    }
    QPixmap pix = matToPixmap(packet.image); // convert
    QPixmap scaled_pix = pix.scaled(ui->feedLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation); // scale to qt
    ui->feedLabel->setPixmap(scaled_pix); // display in the label


    // Test audio meter fake
    static int level = 0;
    static bool up = true;
    if(up)
    {
        level += 2;
        if(level >= 100)
            up = false;
    }
    else
    {
        level -= 2;
        if(level <= 0)
            up = true;
    }
    ui->audioLevelLeft->setValue(level);
    ui->audioLevelRight->setValue(100 - level);
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

void MainWindow::onResetStats()
{
    raw_drop_baseline_= state_.raw_frame_queue.dropped_frames();
    proc_drop_baseline_ = state_.processed_frame_queue.dropped_frames();
    display_fps_ = 0.0;
    display_frame_count_ = 0;
    display_last_time_ = std::chrono::steady_clock::now();
}

void MainWindow::onVideoStartStop()
{
    video_running_ = !video_running_;

    if(video_running_)
    {
        ui->btnVideoStart->setText("Stop Video");
        ui->btnVideoStart->setStyleSheet(
            "QPushButton {"
            "background:#38bdf8;"
            "color:#111417;"
            "border:1px solid #38bdf8;"
            "border-radius:4px;"
            "padding:5px 14px;"
            "font-weight:bold;"
            "}"
            "QPushButton:hover {"
            "background:#7dd3fc;"
            "}"
            "QPushButton:pressed {"
            "background:#0ea5e9;"
            "}"
            );
        qDebug() << "Video started";
    }
    else
    {
        ui->btnVideoStart->setText("Start Video");
        ui->btnVideoStart->setStyleSheet(
            "QPushButton {"
            "background:#1e2126;"
            "color:#38bdf8;"
            "border:1px solid #38bdf8;"
            "border-radius:4px;"
            "padding:5px 14px;"
            "font-weight:bold;"
            "}"
            "QPushButton:hover {"
            "background:#38bdf8;"
            "color:#111417;"
            "}"
            "QPushButton:pressed {"
            "background:#0ea5e9;"
            "}"
            );
        qDebug() << "Video stopped";
    }
}

void MainWindow::onAudioStartStop()
{
    audio_running_ = !audio_running_;

    if(audio_running_)
    {
        ui->btnAudioStart->setText("Stop");
        ui->btnAudioStart->setStyleSheet(
            "QPushButton{"
            "background:#f59e0b;"
            "color:#111417;"
            "border:1px solid #f59e0b;"
            "border-radius:4px;"
            "font-weight:bold;"
            "}"
            );
        qDebug() << "Audio started";
    }
    else
    {
        ui->btnAudioStart->setText("Start");
        ui->btnAudioStart->setStyleSheet(
            "QPushButton{"
            "background:#1e2126;"
            "color:#22c55e;"
            "border:1px solid #22c55e;"
            "border-radius:4px;"
            "font-weight:bold;"
            "}"
            );
        qDebug() << "Audio stopped";
    }
}

void MainWindow::onAudioMute()
{
    audio_muted_ = !audio_muted_;

    if(audio_muted_)
    {
        ui->btnMute->setText("Unmute");
        ui->btnMute->setStyleSheet(
            "QPushButton{"
            "background:#64748b;"
            "color:white;"
            "border:1px solid #64748b;"
            "border-radius:4px;"
            "}"
            );
        qDebug() << "Audio muted";
    }
    else
    {
        ui->btnMute->setText("Mute");
        ui->btnMute->setStyleSheet(
            "QPushButton{"
            "background:#1e2126;"
            "color:#cbd5e1;"
            "border:1px solid #2a2d32;"
            "border-radius:4px;"
            "}"
            );
        qDebug() << "Audio unmuted";
    }
}

void MainWindow::onAudioGainChanged(int value)
{
    ui->lblSliderAudioGain->setText(QString::number(value));
    qDebug() << "Audio Gain =" << value;
}





MainWindow::~MainWindow()
{
    stopPipeline();
    delete ui;
}
