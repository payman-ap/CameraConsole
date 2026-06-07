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
    void startPipeline();
    void stopPipeline();

    cv::VideoCapture cap_;

    SharedState state_;

    std::thread capture_worker_;
    std::thread processing_worker_;

    QTimer* poll_timer_;

    QButtonGroup* filter_group_;
    uint64_t raw_drop_baseline_{0};
    uint64_t proc_drop_baseline_{0};

    Ui::MainWindow *ui;

private slots:
    void onPollFrame();
    void onFilterChanged(int id);
    void onResetStats();
};


#endif // MAINWINDOW_H
