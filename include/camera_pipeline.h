#ifndef CAMERA_PIPELINE_H
#define CAMERA_PIPELINE_H


#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <queue>
#include <condition_variable>
#include <string_view>


enum class FilterType
{
    none,
    grayscale,
    blur,
    edge
};

enum class OverflowPolicy
{
    block,
    drop_oldest,
    drop_newest
};

template<typename T>
class FrameQueue
{
public:
    FrameQueue(size_t max_size = 2, OverflowPolicy policy = OverflowPolicy::drop_oldest)
        : max_size_(max_size), policy_(policy), shutdown_(false) {}

    void push(const T& frame)
    {
        std::unique_lock<std::mutex> lock(mutex_);

        while(frames_.size() >= max_size_)
        {
            switch (policy_)
            {
            case OverflowPolicy::block:
                cv_produce_.wait(lock, std::bind(&FrameQueue::is_space_available_or_shutdown, this));
                if (shutdown_) return;
                break;
            case OverflowPolicy::drop_newest:
                dropped_frames_++;
                return;

            case OverflowPolicy::drop_oldest:
                frames_.pop();
                dropped_frames_++;
                break;
            }
        }
        frames_.push(frame);
        pushed_frames_++;
        cv_consume_.notify_one();
    }

    bool pop(T& frame)
    {
        std::unique_lock<std::mutex> lock(mutex_);

        cv_consume_.wait(lock, std::bind(&FrameQueue::is_data_available_or_shutdown, this));
        if (frames_.empty() && shutdown_)
        {
            return false;
        }
        frame = frames_.front();
        frames_.pop();

        cv_produce_.notify_one();
        return true;
    }

    void notify_shutdown()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        shutdown_ = true;
        cv_consume_.notify_all();
        cv_produce_.notify_all();
    }

    size_t size()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return frames_.size();
    }

    uint64_t dropped_frames() const
    {
        return dropped_frames_.load();
    }
    uint64_t pushed_frames() const
    {
        return pushed_frames_.load();
    }

private:
    bool is_space_available_or_shutdown() {
        return (frames_.size() < max_size_) || shutdown_;
    }

    bool is_data_available_or_shutdown() {
        return !frames_.empty() || shutdown_;
    }

    std::queue<T> frames_;
    std::mutex mutex_;
    std::condition_variable cv_produce_;
    std::condition_variable cv_consume_;
    size_t max_size_;
    OverflowPolicy policy_;
    bool shutdown_;
    std::atomic<uint64_t> dropped_frames_{0};
    std::atomic<uint64_t> pushed_frames_{0};
};

struct RuntimeControl
{
    std::atomic<bool> running{true};
    std::atomic<bool> pipeline_ready{false};
    std::atomic<FilterType> filter_type = FilterType::none;
};

struct FramePacket
{
    cv::Mat image;
    uint64_t sequence;
    std::chrono::steady_clock::time_point capture_time;
    std::chrono::steady_clock::time_point processing_start;
    std::chrono::steady_clock::time_point processing_end;
};

struct SharedState
{
    FrameQueue<FramePacket> raw_frame_queue{2, OverflowPolicy::drop_oldest};
    FrameQueue<FramePacket> processed_frame_queue{2, OverflowPolicy::drop_oldest};
    RuntimeControl control;
    std::atomic<double> capture_fps{0.0};
    std::atomic<size_t> current_queue_depth{0};
};

// Declared extern here so it can be safely defined once in the .cpp file
// extern SharedState shared_state;

constexpr std::string_view filter_name(FilterType filter) {
    switch (filter) {
    case FilterType::none:      return "none";
    case FilterType::grayscale: return "grayscale";
    case FilterType::blur:      return "blur";
    case FilterType::edge:      return "edge";
    }
    return "unknown";
}

// Thread function declarations
void capture_thread(cv::VideoCapture& cap, SharedState& state);
void processing_thread(SharedState& state);


#endif // CAMERA_CONSOLE_H
