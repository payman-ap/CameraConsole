#include "ui/mainwindow.h"
#include "ui_mainwindow.h"


void MainWindow::onResetStats()
{
    raw_drop_baseline_= state_.raw_frame_queue.dropped_frames();
    proc_drop_baseline_ = state_.processed_frame_queue.dropped_frames();
    display_fps_ = 0.0;
    display_frame_count_ = 0;
    display_last_time_ = std::chrono::steady_clock::now();
}
