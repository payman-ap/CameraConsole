#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <thread>

#include <QTimer>
#include <QPixmap>
#include <QFont>
#include <QButtonGroup>

#include <opencv2/videoio.hpp>
#include "camera_pipeline.h"

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    void setupUI();
    void setupButtons();
    void setupTimer();
    void setupFilterGroup();
    void setupStats();
    void startPipeline();
    void stopPipeline();

    cv::VideoCapture cap_;

    SharedState state_;

    std::thread capture_worker_;
    std::thread processing_worker_;

    QTimer* poll_timer_;

    QButtonGroup* filter_group_;


    // ── Display-side FPS ────────────────────────
    int    display_frame_count_{0};
    double display_fps_{0.0};
    std::chrono::steady_clock::time_point display_last_time_;
    // ── Stats snapshot for reset ────────────────
    uint64_t raw_drop_baseline_{0};
    uint64_t proc_drop_baseline_{0};

    Ui::MainWindow *ui;

private slots:
    void onPollFrame();
    void onFilterChanged(int id);
    void onResetStats();
    void onVideoStartStop();
    void onAudioStartStop();
    void onAudioMute();
    void onAudioGainChanged(int value);
};


#endif // MAINWINDOW_H
