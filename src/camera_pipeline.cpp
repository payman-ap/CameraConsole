#include "camera_pipeline.h"

// Definition of the global shared state instance
SharedState shared_state;

void capture_thread(cv::VideoCapture& cap, SharedState& state)
{
    std::cout << "Capture started\n" << std::endl;
    while (state.control.running && !state.control.pipeline_ready) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    FramePacket packet;
    uint64_t global_frame_counter = 0;
    auto last_time = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    std::chrono::duration<double, std::ratio<1,1>> delta_t;
    int frame_count = 0;
    double capture_fps = 0.0;

    while(state.control.running)
    {
        cap.read(packet.image);
        if (packet.image.empty()) {
            std::cerr << "Error: empty frame\n";
            state.control.running = false;
            state.raw_frame_queue.notify_shutdown();
            break;
        }

        packet.capture_time = std::chrono::steady_clock::now();
        packet.sequence = ++global_frame_counter;

        state.raw_frame_queue.push(packet);

        frame_count++;
        if (frame_count % 30 == 0)
        {
            now = std::chrono::steady_clock::now();
            delta_t = now - last_time;
            capture_fps = 30.0 / delta_t.count();
            state.capture_fps.store(capture_fps);
            last_time = now;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void processing_thread(SharedState& state)
{
    std::cout << "Processing started\n" << std::endl;
    FramePacket packet;
    cv::Mat frame_processed;

    while(state.raw_frame_queue.pop(packet))
    {
        packet.processing_start = std::chrono::steady_clock::now();

        switch (state.control.filter_type.load())
        {
        case FilterType::none:
            frame_processed = packet.image;
            break;

        case FilterType::grayscale:
            cv::cvtColor(packet.image, frame_processed, cv::COLOR_BGR2GRAY);
            cv::cvtColor(frame_processed, frame_processed, cv::COLOR_GRAY2BGR);
            break;

        case FilterType::blur:
            cv::GaussianBlur(packet.image, frame_processed, cv::Size(15,15), 0);
            break;

        case FilterType::edge:
            cv::Canny(packet.image, frame_processed, 50, 150);
            cv::cvtColor(frame_processed, frame_processed, cv::COLOR_GRAY2BGR);
            break;
        }

        packet.image = frame_processed;
        packet.processing_end = std::chrono::steady_clock::now();
        state.processed_frame_queue.push(packet);
    }

    state.processed_frame_queue.notify_shutdown();
}

