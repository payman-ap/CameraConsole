#include "ui/mainwindow.h"
#include "ui_mainwindow.h"

#include "ui/mainwindow_video.hpp"
#include "ui/mainwindow_audio.hpp"
#include "ui/mainwindow_styles.hpp"
#include "ui/pages/audio_settings_page.hpp"
#include "ui/pages/video_settings_page.hpp"


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

    startVideoPipeline();
    startAudioPipeline();
}

MainWindow::~MainWindow()
{
    stopVideoPipeline();
    stopAudioPipeline(); // ?
    delete ui;
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

    ui->lblSliderAudioGain->setMinimumWidth(120);
    ui->lblSliderAudioGain->setMaximumWidth(150);
    onAudioGainChanged(ui->sliderAudioGain->value());
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

