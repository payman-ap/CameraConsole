#ifndef QT_UTILS_HPP
#define QT_UTILS_HPP

#include <QPixmap>
#include <QLabel>
#include <opencv2/opencv.hpp>



namespace QtUtils
{
    QPixmap matToPixmap(const cv::Mat& mat);

    QPixmap scaleToLabel(
        const QPixmap& pix,
        const QLabel* label
    );
}

#endif // QT_UTILS_HPP
