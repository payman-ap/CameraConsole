#include "ui/mainwindow.h"
#include "ui_mainwindow.h"

#include "ui/qt_utils.hpp"

#include <QPixmap>


void MainWindow::onPollFrame()
{
    // FramePacket packet;
    // if(state_.processed_frame_queue.size()==0) return; // try getting data
    // if(!state_.processed_frame_queue.pop(packet)) {
    //     poll_timer_->stop();
    //     return;
    // }
    // Update real audio meters or reset them if pipeline is stopped
    if (audio_pipeline_.control().running.load(std::memory_order_relaxed))
    {
        updateAudioMeters(
            audio_pipeline_.control().level_left.load(std::memory_order_relaxed),
            audio_pipeline_.control().level_right.load(std::memory_order_relaxed)
        );
    }
    else
    {
        resetAudioMeters();
    }

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
    QPixmap pix = QtUtils::matToPixmap(packet.image); // convert
    QPixmap scaled_pix = QtUtils::scaleToLabel(pix, ui->feedLabel); // scale to qt
    ui->feedLabel->setPixmap(scaled_pix); // display in the label

    updateAudioEnvelope();
    updateWaveform();
    // updateSpectrum();
    updateSpectrogram();

}


