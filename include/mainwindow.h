#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <thread>
#include <QTimer>
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
    void startPipeline();
    void stopPipeline();

    cv::VideoCapture cap_;

    SharedState state_;

    std::thread capture_worker_;
    std::thread processing_worker_;

    QTimer* poll_timer_;

    Ui::MainWindow *ui;

private slots:
    void onPollFrame();
    // void onFilterChanged();
    // void onResetStats();
};


#endif // MAINWINDOW_H
