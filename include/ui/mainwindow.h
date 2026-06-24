#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <thread>
#include <algorithm>
#include <complex>
#include <QTimer>
#include <QPixmap>
#include <QFont>
#include <QButtonGroup>

#include <opencv2/videoio.hpp>
#include "video/camera_pipeline.h"
#include "audio/audio_pipeline.hpp"
#include "ui/qcustomplot.h"
#include <fftw3.h>

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
    void startVideoPipeline();
    void stopVideoPipeline();
    void startAudioPipeline();
    void stopAudioPipeline();
    void setupAudioSettingsPage();
    void setupAudioPlots();
    void updateAudioEnvelope();
    void updateWaveform();
    void updateSpectrum();
    void updateSpectrogram();

    // Video members
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

    // ── Stats shared for UI buttons and etc . ───
    bool video_running_ = true;
    // bool audio_running_ = false;
    // bool audio_muted_ = false;

    // ── Privacy flag
    bool privacy_mode_ = false;

    Ui::MainWindow *ui;


    // Audio members
    AudioPipeline audio_pipeline_;

    // ── Audio meters
    void updateAudioMeters(int left, int right);
    void resetAudioMeters();

    // ── Audio plots
    QCustomPlot* envelope_plot_{nullptr};
    QCustomPlot* spectrum_plot_{nullptr};
    QCustomPlot* waveform_plot_{nullptr};
    QVector<double> envelope_x_;
    QVector<double> envelope_y_;
    int envelope_index_{0};
    QCustomPlot* spectrogram_plot_{nullptr};
    QCPColorMap* spectrogram_map_{nullptr};
    static constexpr int SPEC_TIME_BINS = 200;
    static constexpr int SPEC_FREQ_BINS = 128;
    int spectrogram_column_{0};

    // For FFT calculation
    QVector<double> computeSpectrum(
        const std::vector<int16_t>& samples);
    void initFFT(int size);
    void cleanupFFT();
    double* fft_in_{nullptr};
    fftw_complex* fft_out_{nullptr};
    fftw_plan fft_plan_{nullptr};
    int fft_size_{0};

private slots:
    void onPollFrame();
    void onFilterChanged(int id);
    void onResetStats();
    void onVideoStartStop();
    void onAudioStartStop();
    void onAudioMute();
    void onAudioGainChanged(int value);

    // ================= Audio slots =================
    void onMicDevChanged(const QString &text);
    void onLoopbackChanged(int value);    // 0..100
    void onAecChanged(int value);         // 0..100
    void onNsChanged(int value);          // 0..100
    void onAgcChanged(int value);         // 0..100

    // ================= Page slots =================
    void showAudioSettingsPage();
    void showDashboardPage();

};

#endif // MAINWINDOW_H
