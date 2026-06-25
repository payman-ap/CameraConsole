#include "ui/mainwindow.h"
#include "ui_mainwindow.h"

#include <cmath>
#include <algorithm>
#include <QDebug>

#include "ui/mainwindow_styles.hpp"



void MainWindow::startAudioPipeline()
{
    ui->lblAudioInputDevice->setText("Baseus Mic (plughw:CARD=Audio)");
    ui->lblAudioOutputDevice->setText("USB Speaker (plughw:CARD=Device)");
}

void MainWindow::stopAudioPipeline()
{
    if (audio_pipeline_.control().running)
    {
        audio_pipeline_.stop();
        resetAudioMeters();
    }
}

void MainWindow::onAudioStartStop()
{
    auto& control = audio_pipeline_.control();

    if(!control.running)
    {
        if(audio_pipeline_.start())
        {
            ui->btnAudioStart->setText("Stop");
            ui->btnAudioStart->setStyleSheet(
                UiStyle::audioStop()
                );
            qDebug() << "Audio pipeline started";
        }

    }
    else
    {
        audio_pipeline_.stop();
        ui->btnAudioStart->setText("Start");
        ui->btnAudioStart->setStyleSheet(
            UiStyle::audioStart()
            );
        qDebug() << "Audio pipeline stopped";
    }
}

void MainWindow::onAudioMute()
{
    auto& control = audio_pipeline_.control();
    control.muted = !control.muted; // so GUI and pipeline stay synchronized

    if(control.muted)
    {
        ui->btnMute->setText("Unmute");
        ui->btnMute->setStyleSheet(
            UiStyle::mute()
            );
        qDebug() << "Audio muted";
    }
    else
    {
        ui->btnMute->setText("Mute");
        ui->btnMute->setStyleSheet(
            UiStyle::unmute()
            );
        qDebug() << "Audio unmuted";
    }
}

void MainWindow::onAudioGainChanged(int value)
{
    audio_pipeline_.control().gain_percent = value;
    
    QString dbStr;
    if (value == 0) {
        dbStr = "-inf dB";
    } else {
        double db = 20.0 * std::log10(value / 100.0);
        dbStr = QString::asprintf("%.1f dB", db);
    }
    
    ui->lblSliderAudioGain->setText(QString("%1% (%2)").arg(value).arg(dbStr));

    qDebug()
        << "Gain:"
        << audio_pipeline_.control().gain_percent.load();
}

void MainWindow::updateAudioMeters(
    int left,
    int right
    )
{
    left = std::clamp(left, 0, 100);
    right = std::clamp(right, 0, 100);

    ui->audioLevelLeft->setValue(left);
    ui->audioLevelRight->setValue(right);
}

void MainWindow::resetAudioMeters()
{
    ui->audioLevelLeft->setValue(0);
    ui->audioLevelRight->setValue(0);
}


void MainWindow::onMicDevChanged(const QString &text)
{
    Q_UNUSED(text);
}

void MainWindow::onLoopbackChanged(int value)
{
    Q_UNUSED(value);
}

void MainWindow::onAecChanged(int value)
{
    Q_UNUSED(value);
}

void MainWindow::onNsChanged(int value)
{
    Q_UNUSED(value);
}

void MainWindow::onAgcChanged(int value)
{
    Q_UNUSED(value);
}

