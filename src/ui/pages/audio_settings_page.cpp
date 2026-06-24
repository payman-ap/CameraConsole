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
}


void MainWindow::setupAudioPlots()
{
    //
    // Envelope
    //
    envelope_plot_ = new QCustomPlot();

    auto* envelopeLayout =
        new QVBoxLayout(ui->widgetEnvelope);

    envelopeLayout->setContentsMargins(0,0,0,0);

    envelopeLayout->addWidget(envelope_plot_);

    envelope_plot_->addGraph();

    envelope_plot_->xAxis->setLabel("Time");
    envelope_plot_->yAxis->setLabel("Amplitude");

    envelope_plot_->xAxis->setRange(0, 100);
    envelope_plot_->yAxis->setRange(0, 100);

    envelope_plot_->graph(0)->setPen(QPen(Qt::green));

    //
    // Waveform
    //
    waveform_plot_ = new QCustomPlot();

    auto* waveformLayout =
        new QVBoxLayout(ui->widgetWaveform);

    waveformLayout->setContentsMargins(0,0,0,0);

    waveformLayout->addWidget(waveform_plot_);

    waveform_plot_->addGraph();

    waveform_plot_->xAxis->setLabel("Samples");
    waveform_plot_->yAxis->setLabel("Amplitude");

    waveform_plot_->xAxis->setRange(0, 24000);
    waveform_plot_->yAxis->setRange(0, 1);

    waveform_plot_->graph(0)->setPen(QPen(Qt::blue));


    //
    // Spectrogram
    //
    spectrogram_plot_ = new QCustomPlot();

    // auto* spectrogramLayout =
    //     new QVBoxLayout(ui->widgetSpectrogram);
    auto* spectrogramLayout =
        new QVBoxLayout(ui->widgetSpectrum);

    spectrogramLayout->setContentsMargins(0,0,0,0);

    spectrogramLayout->addWidget(spectrogram_plot_);

    spectrogram_map_ =
        new QCPColorMap(
            spectrogram_plot_->xAxis,
            spectrogram_plot_->yAxis);

    spectrogram_map_->data()->setSize(
        SPEC_TIME_BINS,
        SPEC_FREQ_BINS);

    spectrogram_map_->data()->setRange(
        QCPRange(0, SPEC_TIME_BINS),
        QCPRange(0, 8000));

    spectrogram_plot_->xAxis->setLabel("Time");

    spectrogram_plot_->yAxis->setLabel("Frequency (Hz)");

    spectrogram_plot_->xAxis->setRange(
        0,
        SPEC_TIME_BINS);

    spectrogram_plot_->yAxis->setRange(
        0,
        8000);

    auto* colorScale =
        new QCPColorScale(spectrogram_plot_);

    spectrogram_plot_->plotLayout()->addElement(
        0,
        1,
        colorScale);

    spectrogram_map_->setColorScale(colorScale);
    spectrogram_map_->setGradient(
        QCPColorGradient::gpThermal);


    //
    // Spectrum
    //
    spectrum_plot_ = new QCustomPlot();

    auto* spectrumLayout =
        new QVBoxLayout(ui->widgetSpectrum);

    spectrumLayout->setContentsMargins(0,0,0,0);

    spectrumLayout->addWidget(spectrum_plot_);

    spectrum_plot_->addGraph();

    spectrum_plot_->xAxis->setLabel("Frequency (Hz)");
    spectrum_plot_->yAxis->setLabel("Magnitude");

    spectrum_plot_->xAxis->setRange(0, 24000);
    spectrum_plot_->yAxis->setRange(0, 1);

    spectrum_plot_->graph(0)->setPen(QPen(Qt::red));

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
        4000);

    spectrum_plot_->yAxis->setRange(
        -100,
        20);

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
    for (int y = 0;
         y < SPEC_FREQ_BINS;
         ++y)
    {
        spectrogram_map_
            ->data()
            ->setCell(
                SPEC_TIME_BINS - 1,
                y,
                magnitudes[y]);
    }

    // update the display
    spectrogram_map_->rescaleDataRange();

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




QVector<double> MainWindow::computeSpectrum(
    const std::vector<int16_t>& samples)
{
    constexpr int N = 256;

    if (samples.size() < N)
        return {};

    double* in =
        fftw_alloc_real(N);

    fftw_complex* out =
        fftw_alloc_complex(N/2 + 1);

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

        in[i] = sample * window;
    }

    fftw_plan plan =
        fftw_plan_dft_r2c_1d(
            N,
            in,
            out,
            FFTW_ESTIMATE);

    fftw_execute(plan);

    QVector<double> magnitudes(N/2);

    for (int k = 0; k < N/2; ++k)
    {
        double real = out[k][0];
        double imag = out[k][1];

        double magnitude =
            std::sqrt(
                real*real +
                imag*imag);

        magnitudes[k] =
            20.0 *
            std::log10(
                magnitude + 1e-9);
    }

    fftw_destroy_plan(plan);

    fftw_free(in);
    fftw_free(out);

    return magnitudes;
}

