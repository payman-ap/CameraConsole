#include "ui/pages/audio_settings_page.hpp"


void MainWindow::showAudioSettingsPage()
{
    ui->stackedWidgetMain->setCurrentWidget(
        ui->pageAudioSettings);
}

void MainWindow::showDashboardPage()
{
    ui->stackedWidgetMain->setCurrentWidget(
        ui->pageDashboard);
}


void MainWindow::setupAudioSettingsPage()
{
    setupAudioPlots();
    populateAudioDevices();

    connect(
        ui->btnAudioSettings,
        &QPushButton::clicked,
        this,
        &MainWindow::showAudioSettingsPage
        );

    connect(
        ui->btnBackAudioSettings,
        &QPushButton::clicked,
        this,
        &MainWindow::showDashboardPage
        );

    connect(
        ui->btnApplyAudioDevices,
        &QPushButton::clicked,
        this,
        [this]()
        {
            int inIdx  = ui->cbInputDevice->currentIndex();
            int outIdx = ui->cbOutputDevice->currentIndex();

            if (inIdx < 0 || outIdx < 0) return;

            std::string inName  = ui->cbInputDevice->itemData(inIdx).toString().toStdString();
            std::string outName = ui->cbOutputDevice->itemData(outIdx).toString().toStdString();

            audio_pipeline_.stop();
            audio_pipeline_.setInputDevice(inName);
            audio_pipeline_.setOutputDevice(outName);
            audio_pipeline_.start();

            QString inDesc  = ui->cbInputDevice->itemText(inIdx);
            QString outDesc = ui->cbOutputDevice->itemText(outIdx);
            ui->lblAudioInputDevice->setText(inDesc);
            ui->lblAudioOutputDevice->setText(outDesc);
        });
}

void MainWindow::populateAudioDevices()
{
    ui->cbInputDevice->clear();
    ui->cbOutputDevice->clear();

    auto inputs = AudioDeviceManager::enumerateInputDevices();
    for (const auto& dev : inputs)
        ui->cbInputDevice->addItem(
            QString::fromStdString(dev.description),
            QString::fromStdString(dev.name));

    auto outputs = AudioDeviceManager::enumerateOutputDevices();
    for (const auto& dev : outputs)
        ui->cbOutputDevice->addItem(
            QString::fromStdString(dev.description),
            QString::fromStdString(dev.name));

    // Pre-select the device the pipeline is already using
    int inIdx = ui->cbInputDevice->findData(
        QString::fromStdString(audio_pipeline_.getInputDevice()));
    if (inIdx >= 0) ui->cbInputDevice->setCurrentIndex(inIdx);

    int outIdx = ui->cbOutputDevice->findData(
        QString::fromStdString(audio_pipeline_.getOutputDevice()));
    if (outIdx >= 0) ui->cbOutputDevice->setCurrentIndex(outIdx);
}


void MainWindow::setupAudioPlots()
{
    // Common helper lambda to style a plot for the modern dark theme
    auto stylePlotDark = [](QCustomPlot* plot) {
        plot->setBackground(QBrush(QColor("#181b1f"))); // container background

        plot->xAxis->basePen().setColor(QColor("#4b5563"));
        plot->xAxis->tickPen().setColor(QColor("#4b5563"));
        plot->xAxis->subTickPen().setColor(QColor("#4b5563"));
        plot->xAxis->setTickLabelColor(QColor("#cbd5e1"));
        plot->xAxis->setLabelColor(QColor("#cbd5e1"));
        plot->xAxis->grid()->setPen(QPen(QColor("#2d3139"), 1, Qt::DotLine));
        plot->xAxis->grid()->setVisible(true);

        plot->yAxis->basePen().setColor(QColor("#4b5563"));
        plot->yAxis->tickPen().setColor(QColor("#4b5563"));
        plot->yAxis->subTickPen().setColor(QColor("#4b5563"));
        plot->yAxis->setTickLabelColor(QColor("#cbd5e1"));
        plot->yAxis->setLabelColor(QColor("#cbd5e1"));
        plot->yAxis->grid()->setPen(QPen(QColor("#2d3139"), 1, Qt::DotLine));
        plot->yAxis->grid()->setVisible(true);
    };

    //
    // Envelope
    //
    envelope_plot_ = new QCustomPlot();
    auto* envelopeLayout = new QVBoxLayout(ui->widgetEnvelope);
    envelopeLayout->setContentsMargins(0, 0, 0, 0);
    envelopeLayout->addWidget(envelope_plot_);

    envelope_plot_->addGraph();
    envelope_plot_->xAxis->setLabel("Time");
    envelope_plot_->yAxis->setLabel("Amplitude");
    envelope_plot_->xAxis->setRange(0, 100);
    envelope_plot_->yAxis->setRange(0, 100);

    stylePlotDark(envelope_plot_);
    envelope_plot_->graph(0)->setPen(QPen(QColor("#10b981"), 2)); // Modern Emerald Green, thick line
    
    // Add a beautiful semi-transparent fading area fill
    QLinearGradient envelopeGrad(0, 0, 0, 100);
    envelopeGrad.setColorAt(0, QColor(16, 185, 129, 80));
    envelopeGrad.setColorAt(1, QColor(16, 185, 129, 0));
    envelope_plot_->graph(0)->setBrush(QBrush(envelopeGrad));

    //
    // Waveform
    //
    waveform_plot_ = new QCustomPlot();
    auto* waveformLayout = new QVBoxLayout(ui->widgetWaveform);
    waveformLayout->setContentsMargins(0, 0, 0, 0);
    waveformLayout->addWidget(waveform_plot_);

    waveform_plot_->addGraph();
    waveform_plot_->xAxis->setLabel("Samples");
    waveform_plot_->yAxis->setLabel("Amplitude");
    waveform_plot_->xAxis->setRange(0, 24000);
    waveform_plot_->yAxis->setRange(-1.0, 1.0);

    stylePlotDark(waveform_plot_);
    waveform_plot_->graph(0)->setPen(QPen(QColor("#38bdf8"), 1.5)); // Modern Sky Blue
    
    // Add a beautiful symmetric fill to center line
    QLinearGradient waveformGrad(0, 0, 0, 150);
    waveformGrad.setColorAt(0, QColor(56, 189, 248, 30));
    waveformGrad.setColorAt(1, QColor(56, 189, 248, 0));
    waveform_plot_->graph(0)->setBrush(QBrush(waveformGrad));

    //
    // Spectrogram & Spectrum Layout
    //
    // Create a single layout to cleanly manage both plots without collision
    auto* spectrumLayout = new QVBoxLayout(ui->widgetSpectrum);
    spectrumLayout->setContentsMargins(0, 0, 0, 0);
    spectrumLayout->setSpacing(0);

    //
    // Spectrogram
    //
    spectrogram_plot_ = new QCustomPlot();
    spectrumLayout->addWidget(spectrogram_plot_);

    spectrogram_map_ = new QCPColorMap(spectrogram_plot_->xAxis, spectrogram_plot_->yAxis);
    spectrogram_map_->data()->setSize(SPEC_TIME_BINS, SPEC_FREQ_BINS);
    
    // Set the mapping value range matching actual FFT frequencies (0 to 23812.5 Hz)
    spectrogram_map_->data()->setRange(
        QCPRange(0, SPEC_TIME_BINS),
        QCPRange(0, 23812.5));

    spectrogram_plot_->xAxis->setLabel("Time");
    spectrogram_plot_->yAxis->setLabel("Frequency (Hz)");
    spectrogram_plot_->xAxis->setRange(0, SPEC_TIME_BINS);
    spectrogram_plot_->yAxis->setRange(0, 8000); // Standard human vocal/speech frequency zoom

    stylePlotDark(spectrogram_plot_);

    auto* colorScale = new QCPColorScale(spectrogram_plot_);
    spectrogram_plot_->plotLayout()->addElement(0, 1, colorScale);
    spectrogram_map_->setColorScale(colorScale);
    
    // Style the color scale
    colorScale->axis()->setLabel("Energy (dBFS)");
    colorScale->axis()->setTickLabelColor(QColor("#cbd5e1"));
    colorScale->axis()->setLabelColor(QColor("#cbd5e1"));
    colorScale->axis()->basePen().setColor(QColor("#4b5563"));
    colorScale->axis()->tickPen().setColor(QColor("#4b5563"));
    colorScale->axis()->subTickPen().setColor(QColor("#4b5563"));

    // Set fixed professional range floor (-60 dBFS) to peak (0 dBFS)
    spectrogram_map_->setDataRange(QCPRange(-60.0, 0.0));

    // Create a premium custom gradient that transitions from dark background up to bright amber
    QCPColorGradient specGrad;
    specGrad.clearColorStops();
    specGrad.setColorStopAt(0.0, QColor("#181b1f")); // fades into background
    specGrad.setColorStopAt(0.2, QColor("#1e3a8a")); // deep blue
    specGrad.setColorStopAt(0.4, QColor("#3b82f6")); // bright blue
    specGrad.setColorStopAt(0.6, QColor("#8b5cf6")); // purple
    specGrad.setColorStopAt(0.8, QColor("#ec4899")); // magenta/pink
    specGrad.setColorStopAt(1.0, QColor("#fbbf24")); // bright yellow/amber
    spectrogram_map_->setGradient(specGrad);

    //
    // Spectrum (hidden by default)
    //
    spectrum_plot_ = new QCustomPlot();
    spectrumLayout->addWidget(spectrum_plot_);

    spectrum_plot_->addGraph();
    spectrum_plot_->xAxis->setLabel("Frequency (Hz)");
    spectrum_plot_->yAxis->setLabel("Magnitude (dBFS)");
    spectrum_plot_->xAxis->setRange(0, 8000);
    spectrum_plot_->yAxis->setRange(-80, 0);

    stylePlotDark(spectrum_plot_);
    spectrum_plot_->graph(0)->setPen(QPen(QColor("#f87171"), 2)); // Modern Coral Red
    
    // Fading spectrum area fill
    QLinearGradient spectrumLineGrad(0, 0, 0, 160);
    spectrumLineGrad.setColorAt(0, QColor(248, 113, 113, 60));
    spectrumLineGrad.setColorAt(1, QColor(248, 113, 113, 0));
    spectrum_plot_->graph(0)->setBrush(QBrush(spectrumLineGrad));

    // Hide spectrum_plot_ since its update is currently commented out in render loop
    spectrum_plot_->setVisible(false);
}


void MainWindow::updateAudioEnvelope()
{
    int level =
        audio_pipeline_
            .control()
            .level_left
            .load(std::memory_order_relaxed);

    envelope_x_.push_back(envelope_index_);
    envelope_y_.push_back(level);

    envelope_index_++;

    constexpr int MAX_POINTS = 200;

    if (envelope_x_.size() > MAX_POINTS)
    {
        envelope_x_.removeFirst();
        envelope_y_.removeFirst();
    }

    envelope_plot_->graph(0)->setData(
        envelope_x_,
        envelope_y_);

    envelope_plot_->xAxis->setRange(
        std::max(0, envelope_index_ - MAX_POINTS),
        envelope_index_);

    envelope_plot_->replot(
        QCustomPlot::rpQueuedReplot);
}


void MainWindow::updateWaveform()
{
    auto samples = audio_pipeline_.latestSamples();

    if (samples.empty())
        return;

    QVector<double> x(samples.size());
    QVector<double> y(samples.size());

    for (int i = 0; i < static_cast<int>(samples.size()); ++i)
    {
        x[i] = i;

        y[i] =
            static_cast<double>(samples[i]) /
            32768.0;
    }

    waveform_plot_->graph(0)->setData(x, y);

    waveform_plot_->xAxis->setRange(
        0,
        samples.size());

    waveform_plot_->yAxis->setRange(
        -1.0,
        1.0);

    waveform_plot_->replot(
        QCustomPlot::rpQueuedReplot);
}



void MainWindow::updateSpectrum()
{
    auto samples =
        audio_pipeline_.latestSamples();

    constexpr int N = 256;

    auto magnitudes =
        computeSpectrum(samples);

    if (magnitudes.isEmpty())
        return;

    QVector<double> freq(magnitudes.size());

    for (int k = 0; k < magnitudes.size(); ++k)
    {
        freq[k] =
            k *
            (48000.0 / N);
    }

    spectrum_plot_->graph(0)->setData(
        freq,
        magnitudes);

    spectrum_plot_->xAxis->setRange(
        0,
        8000);

    spectrum_plot_->yAxis->setRange(
        -80,
        0);

    spectrum_plot_->replot(
        QCustomPlot::rpQueuedReplot);

}

void MainWindow::updateSpectrogram()
{
    auto samples =
        audio_pipeline_.latestSamples();

    constexpr int N = 256;

    auto magnitudes =
        computeSpectrum(samples);

    if (magnitudes.isEmpty())
        return;

    // Scroll the image. move all existing columns left
    for (int x = 0;
         x < SPEC_TIME_BINS - 1;
         ++x)
    {
        for (int y = 0;
             y < SPEC_FREQ_BINS;
             ++y)
        {
            double value =
                spectrogram_map_
                    ->data()
                    ->cell(x + 1, y);

            spectrogram_map_
                ->data()
                ->setCell(
                    x,
                    y,
                    value);
        }
    }

    // Insert newest FFT at right edge
    int copy_bins = std::min(static_cast<int>(magnitudes.size()), SPEC_FREQ_BINS);
    for (int y = 0;
         y < copy_bins;
         ++y)
    {
        spectrogram_map_
            ->data()
            ->setCell(
                SPEC_TIME_BINS - 1,
                y,
                magnitudes[y]);
    }

    // Since we set a fixed data range for consistent, premium coloring in setupAudioPlots,
    // we do not call rescaleDataRange() here, which would overwrite it dynamically.

    spectrogram_plot_->replot(
        QCustomPlot::rpQueuedReplot);

}










// void MainWindow::updateSpectrum()
// {
//     auto samples = audio_pipeline_.latestSamples();

//     if (samples.size() < 256)
//         return;

//     constexpr int N = 256;

//     QVector<double> freq(N / 2);
//     QVector<double> mag(N / 2);

//     for (int k = 0; k < N / 2; ++k)
//     {
//         std::complex<double> X(0.0, 0.0);

//         for (int n = 0; n < N; ++n)
//         {
//             double sample =
//                 static_cast<double>(samples[n]) / 32768.0;

//             double window =
//                 0.5 *
//                 (1.0 - std::cos(
//                      2.0 * M_PI * n / (N - 1)));

//             sample *= window;

//             double angle =
//                 -2.0 * M_PI * k * n / N;

//             X += sample *
//                  std::complex<double>(
//                      std::cos(angle),
//                      std::sin(angle));
//         }

//         mag[k] = 20.0 * std::log10(std::abs(X) + 1e-9);

//         freq[k] =
//             k *
//             (48000.0 / N);
//     }

//     spectrum_plot_->graph(0)->setData(
//         freq,
//         mag);

//     spectrum_plot_->xAxis->setRange(0, 4000); // For speech
//     spectrum_plot_->yAxis->setRange(-100, 20);

//     // spectrum_plot_->rescaleAxes(true);

//     spectrum_plot_->replot(
//         QCustomPlot::rpQueuedReplot);
// }

// void MainWindow::updateSpectrogram()
// {
//     auto samples =
//         audio_pipeline_.latestSamples();

//     if (samples.size() < 256)
//         return;

//     constexpr int N = 256;

//     QVector<double> magnitudes(N / 2);

//     for (int k = 0; k < N / 2; ++k)
//     {
//         std::complex<double> X(0.0, 0.0);

//         for (int n = 0; n < N; ++n)
//         {
//             double sample =
//                 static_cast<double>(samples[n])
//                 / 32768.0;

//             double window =
//                 0.5 *
//                 (1.0 - std::cos(
//                      2.0 * M_PI * n / (N - 1)));

//             sample *= window;

//             double angle =
//                 -2.0 * M_PI * k * n / N;

//             X += sample *
//                  std::complex<double>(
//                      std::cos(angle),
//                      std::sin(angle));
//         }

//         magnitudes[k] =
//             20.0 *
//             std::log10(
//                 std::abs(X) + 1e-9);
//     }

//     // Scroll the image. move all existing columns left
//     for (int x = 0;
//          x < SPEC_TIME_BINS - 1;
//          ++x)
//     {
//         for (int y = 0;
//              y < SPEC_FREQ_BINS;
//              ++y)
//         {
//             double value =
//                 spectrogram_map_
//                     ->data()
//                     ->cell(x + 1, y);

//             spectrogram_map_
//                 ->data()
//                 ->setCell(
//                     x,
//                     y,
//                     value);
//         }
//     }

//     // Insert newest FFT at right edge
//     for (int y = 0;
//          y < SPEC_FREQ_BINS;
//          ++y)
//     {
//         spectrogram_map_
//             ->data()
//             ->setCell(
//                 SPEC_TIME_BINS - 1,
//                 y,
//                 magnitudes[y]);
//     }

//     // update the display
//     spectrogram_map_->rescaleDataRange();

//     spectrogram_plot_->replot(
//         QCustomPlot::rpQueuedReplot);

// }




void MainWindow::initFFT(int size)
{
    cleanupFFT();
    fft_size_ = size;
    fft_in_ = fftw_alloc_real(size);
    fft_out_ = fftw_alloc_complex(size / 2 + 1);
    fft_plan_ = fftw_plan_dft_r2c_1d(size, fft_in_, fft_out_, FFTW_ESTIMATE);
}

void MainWindow::cleanupFFT()
{
    if (fft_plan_)
    {
        fftw_destroy_plan(fft_plan_);
        fft_plan_ = nullptr;
    }
    if (fft_in_)
    {
        fftw_free(fft_in_);
        fft_in_ = nullptr;
    }
    if (fft_out_)
    {
        fftw_free(fft_out_);
        fft_out_ = nullptr;
    }
    fft_size_ = 0;
}

QVector<double> MainWindow::computeSpectrum(
    const std::vector<int16_t>& samples)
{
    constexpr int N = 256;

    if (samples.size() < N)
        return {};

    if (!fft_plan_ || fft_size_ != N)
    {
        initFFT(N);
    }

    if (!fft_in_ || !fft_out_ || !fft_plan_)
    {
        return {};
    }

    //
    // Hann window
    //
    for (int i = 0; i < N; ++i)
    {
        double sample =
            static_cast<double>(samples[i]) / 32768.0;

        double window =
            0.5 *
            (1.0 - std::cos(
                 2.0 * M_PI * i / (N - 1)));

        fft_in_[i] = sample * window;
    }

    fftw_execute(fft_plan_);

    QVector<double> magnitudes(N/2);

    for (int k = 0; k < N/2; ++k)
    {
        double real = fft_out_[k][0];
        double imag = fft_out_[k][1];

        // Normalize FFT by N/2 (for real FFT) and correct for Hann window gain (0.5).
        // Hence, divide by (N / 2) * 0.5 = N / 4.0.
        double magnitude = std::sqrt(real*real + imag*imag) / (N / 4.0);

        magnitudes[k] =
            20.0 *
            std::log10(
                magnitude + 1e-9);
    }

    return magnitudes;
}

