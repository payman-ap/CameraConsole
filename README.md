# Multithreaded Camera Console (OpenCV + Qt)

A real-time camera and audio monitoring application built to explore modern C++, concurrent programming, OpenCV, ALSA, and Qt GUI development.

---

## Features

### Video
- Real-time camera capture using OpenCV
- Multi-threaded processing pipeline
- Thread-safe bounded frame queues
- Configurable queue overflow policies
- Live image filtering:
  - None
  - Grayscale
  - Gaussian Blur
  - Edge Detection
- Runtime filter switching
- Real-time performance monitoring:
  - Display FPS
  - Capture FPS
  - End-to-end latency
  - Queue depth
  - Raw frame drops
  - Processed frame drops

### Audio
- Full-duplex audio monitoring via ALSA
- Lock-free SPSC ring buffer between capture and playback threads
- Device enumeration for input and output via ALSA control API
- Runtime device selection through the Audio Settings page
- Adjustable gain with real-time VU meters (left/right channels)
- Mute toggle
- Real-time audio visualization:
  - Amplitude envelope
  - Waveform display
  - Spectrogram (scrolling, color-mapped)
  - FFT spectrum (Hann-windowed, dBFS scale)

### GUI
- Qt Designer based layout
- Dashboard page with live video feed, video controls, and audio monitoring strip
- Audio Settings page with device dropdowns, waveform/spectrum plots, and apply/back navigation
- Dark theme throughout

---

## Pipeline Architecture

### Video Pipeline

```
Camera
│
▼
Capture Thread
│
▼
Raw Frame Queue
│
▼
Processing Thread
│
▼
Processed Frame Queue
│
▼
Qt GUI Timer
│
▼
Live Display

```

### Audio Pipeline

```
ALSA Capture Device
│
▼
CaptureThread  (producer)
│
▼
SPSC Ring Buffer  (lock-free, power-of-two capacity)
│
▼
PlaybackThread  (consumer)
│
▼
ALSA Playback Device
│
▼  (callback)
AudioPipeline  →  Visualization samples  →  Qt plots
```

The GUI is intentionally separated from from both pipelines. Worker threads handle acquisition and processing, while the Qt event loop is responsible only for presentation and user interaction.

---


## Audio Engine

The audio subsystem is derived from [alsa-spsc-audio-bridge](https://github.com/payman-ap/alsa-spsc-audio-bridge):

> A high-performance, low-latency C++ audio engine designed to bridge ALSA capture and playback using a lock-free Single-Producer Single-Consumer (SPSC) ring buffer. This project demonstrates real-time audio threading techniques to minimize jitter and avoid audible glitches during concurrent full-duplex audio processing.

Key properties carried over and extended here:
- Wait-free SPSC ring buffer with power-of-two capacity and false-sharing prevention (cache-line aligned atomics)
- Dedicated capture and playback threads with independent ALSA handles
- Period-size negotiation and clock readback after `snd_pcm_hw_params`
- Underrun recovery via `snd_pcm_prepare`
- Gain, mute, and peak-level metering integrated into the playback thread

> **Note on multi-device clock drift:** When the capture and playback devices are different physical hardware (e.g. a USB microphone and a separate USB headset), their clocks run at slightly different rates and will eventually cause playback underruns. Using the same device for both input and output (e.g. a USB headset with integrated mic) eliminates this. A future improvement would be drift compensation via libsamplerate or routing through PipeWire/PulseAudio (`"default"` device), which handles resampling transparently.

---

## Technologies

- C++20
- OpenCV
- Qt 6
- ALSA (`libasound`)
- FFTW3
- `std::thread`, `std::mutex`, `std::condition_variable`, `std::atomic`
- CMake + Ninja

---

## Project Structure

```
├── include/
│   ├── audio/          # AudioPipeline, AudioRecorder, AudioPlayer,
│   │                   # PlaybackThread, RingBuffer, AudioDeviceManager
│   ├── ui/             # MainWindow, page headers, QCustomPlot, styles
│   └── video/          # CameraPipeline, CaptureThread, VideoDeviceManager
├── src/
│   ├── audio/          # ALSA capture, playback, device enumeration, pipeline
│   ├── ui/             # MainWindow slots, audio/video controls, settings pages
│   └── video/          # Camera pipeline implementation
├── forms/
│   └── mainwindow.ui   # Qt Designer layout
└── CMakeLists.txt
```

---

## Design Goals

- Producer-consumer architectures with lock-free data structures
- Real-time audio threading and ALSA low-level programming
- Thread synchronization without coupling processing logic to the UI
- Low-latency image and audio processing in a single application
- Qt GUI integration with live plots and multi-page navigation
- Modern C++ design practices (RAII, smart pointers, atomics, move semantics)


The original OpenCV pipeline was preserved while migrating the display layer to Qt, demonstrating separation of concerns between the processing backend and the graphical frontend.

---

## Future Extensions

Potential improvements include:

- Video recording and snapshot capture
- Camera selection and dynamic resolution control
- Additional image filters
- PipeWire/PulseAudio device routing for transparent clock drift handling
- libsamplerate integration for multi-device drift compensation
- Multi-camera support
- Queue policy selection from the GUI
- Audio recording to file
Replace https://github.com/your-username/alsa-spsc-audio-bridge with your actual repo URL.

not like this. give the whole markdown syntax in a single codeblock. also: https://github.com/payman-ap/alsa-spsc-audio-bridge

4:03 PM
markdown
# Multithreaded Camera Console (OpenCV + Qt)

A real-time camera and audio monitoring application built to explore modern C++, concurrent programming, OpenCV, ALSA, and Qt GUI development.

---

## Features

### Video
- Real-time camera capture using OpenCV
- Multi-threaded processing pipeline
- Thread-safe bounded frame queues
- Configurable queue overflow policies
- Live image filtering:
  - None
  - Grayscale
  - Gaussian Blur
  - Edge Detection
- Runtime filter switching
- Real-time performance monitoring:
  - Display FPS
  - Capture FPS
  - End-to-end latency
  - Queue depth
  - Raw frame drops
  - Processed frame drops

### Audio
- Full-duplex audio monitoring via ALSA
- Lock-free SPSC ring buffer between capture and playback threads
- Device enumeration for input and output via ALSA control API
- Runtime device selection through the Audio Settings page
- Adjustable gain with real-time VU meters (left/right channels)
- Mute toggle
- Real-time audio visualization:
  - Amplitude envelope
  - Waveform display
  - Spectrogram (scrolling, color-mapped)
  - FFT spectrum (Hann-windowed, dBFS scale)

### GUI
- Qt Designer based layout
- Dashboard page with live video feed, video controls, and audio monitoring strip
- Audio Settings page with device dropdowns, waveform and spectrum plots, and apply/back navigation
- Dark theme throughout

---

## Pipeline Architecture

### Video Pipeline
```
Camera
│
▼
Capture Thread
│
▼
Raw Frame Queue
│
▼
Processing Thread
│
▼
Processed Frame Queue
│
▼
Qt GUI Timer
│
▼
Live Display
```

### Audio Pipeline

```
ALSA Capture Device
│
▼
CaptureThread (producer)
│
▼
SPSC Ring Buffer (lock-free, power-of-two capacity)
│
▼
PlaybackThread (consumer)
│
▼
ALSA Playback Device
│
└──(callback)──▶ AudioPipeline ──▶ Visualization samples ──▶ Qt plots
```

The GUI is intentionally separated from both pipelines. Worker threads handle acquisition and processing, while the Qt event loop is responsible only for presentation and user interaction.

---

## Audio Engine

The audio subsystem is derived from [alsa-spsc-audio-bridge](https://github.com/payman-ap/alsa-spsc-audio-bridge):

> A high-performance, low-latency C++ audio engine designed to bridge ALSA capture and playback using a lock-free Single-Producer Single-Consumer (SPSC) ring buffer. This project demonstrates real-time audio threading techniques to minimize jitter and avoid audible glitches during concurrent full-duplex audio processing.

Key properties carried over and extended here:

- Wait-free SPSC ring buffer with power-of-two capacity and false-sharing prevention via cache-line aligned atomics
- Dedicated capture and playback threads with independent ALSA handles
- Period-size negotiation and clock readback after `snd_pcm_hw_params`
- Underrun recovery via `snd_pcm_prepare`
- Gain, mute, and peak-level metering integrated into the playback thread
- Runtime device switching with clean thread teardown and ring flush between sessions

> **Note on multi-device clock drift:** When the capture and playback devices are different physical hardware (e.g. a USB microphone and a separate USB headset), their clocks run at slightly different rates and will eventually cause playback underruns. Using the same physical device for both input and output (e.g. a USB headset with an integrated mic) eliminates this entirely. A future improvement would be routing through PipeWire or PulseAudio using the `"default"` device name, which handles clock drift and resampling transparently at the OS level.

---

## Technologies

- C++20
- OpenCV
- Qt 6
- ALSA (`libasound`)
- FFTW3
- `std::thread`, `std::mutex`, `std::condition_variable`, `std::atomic`
- CMake + Ninja

---

## Project Structure

```
.
├── include/
│ ├── audio/ # AudioPipeline, AudioRecorder, AudioPlayer,
│ │ # PlaybackThread, RingBuffer, AudioDeviceManager
│ ├── ui/ # MainWindow, page headers, QCustomPlot, styles
│ └── video/ # CameraPipeline, CaptureThread, VideoDeviceManager
├── src/
│ ├── audio/ # ALSA capture, playback, device enumeration, pipeline
│ ├── ui/ # MainWindow slots, audio/video controls, settings pages
│ └── video/ # Camera pipeline implementation
├── forms/
│ └── mainwindow.ui # Qt Designer layout
└── CMakeLists.txt
```

---

## Design Goals

This project was developed as a practical exercise in:

- Producer-consumer architectures with lock-free data structures
- Real-time audio threading and low-level ALSA programming
- Thread synchronization without coupling processing logic to the UI
- Low-latency image and audio processing within a single application
- Qt GUI integration with live plots and multi-page navigation
- Modern C++ design practices (RAII, smart pointers, atomics, move semantics)

---

## Future Extensions

Potential improvements include:

- Video recording and snapshot capture
- Camera selection and dynamic resolution control
- Additional image filters
- PipeWire/PulseAudio device routing for transparent clock drift handling
- libsamplerate integration for multi-device drift compensation
- Multi-camera support
- Queue policy selection from the GUI
- Audio recording to file



