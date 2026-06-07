# Multithreaded Camera Console (OpenCV + Qt)

A real-time camera application built to explore modern C++, concurrent programming, OpenCV, and Qt GUI development.

## Features

- Real-time camera capture using OpenCV
- Multi-threaded processing pipeline
- Thread-safe bounded frame queues
- Configurable queue overflow policies
- Live image filtering:
  - None
  - Grayscale
  - Gaussian Blur
  - Edge Detection
- Qt Designer based GUI
- Real-time performance monitoring:
  - Display FPS
  - Capture FPS
  - End-to-end latency
  - Queue depth
  - Raw frame drops
  - Processed frame drops
- Runtime filter switching
- Graceful shutdown of all worker threads

---

## Pipeline Architecture

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

The GUI is intentionally separated from the image-processing pipeline. Worker threads handle acquisition and processing, while the Qt event loop is responsible only for presentation and user interaction.

---

## Technologies

- C++20
- OpenCV
- Qt 6
- std::thread
- std::mutex
- std::condition_variable
- std::atomic
- CMake

---

## Design Goals

This project was developed as a practical exercise in:

- Producer-consumer architectures
- Thread synchronization
- Lock-based concurrent queues
- Low-latency image processing
- GUI integration without coupling processing logic to the UI
- Modern C++ design practices

The original OpenCV pipeline was preserved while migrating the display layer to Qt, demonstrating separation of concerns between the processing backend and the graphical frontend.

---

## Future Extensions

Potential improvements include:

- Video recording
- Snapshot capture
- Camera selection
- Dynamic resolution control
- Additional image filters
- Queue policy selection from the GUI
- Multi-camera support
