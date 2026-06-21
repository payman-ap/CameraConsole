#include "ui/qt_utils.hpp"

#include <QImage>


QPixmap QtUtils::matToPixmap(const cv::Mat& mat)
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

QPixmap QtUtils::scaleToLabel(
    const QPixmap& pix,
    const QLabel* label
    )
{
    return pix.scaled(
        label->size(),
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation
        );
}

