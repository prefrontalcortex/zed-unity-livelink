#ifndef  __SENDER_RUNNER_HDR__
#define __SENDER_RUNNER_HDR__

#include <sl/Fusion.hpp>
#include <sl/Camera.hpp>

#include <thread>
#include <atomic>
#include <chrono>

class SenderRunner {
public:
    SenderRunner();
    ~SenderRunner();

    bool open(sl::InputType input, sl::BODY_FORMAT body_format);
    void start();
    void stop();

    // Whether the grab thread was actually started (i.e. the camera opened successfully).
    bool isRunning() const { return running; }

    // How long it has been since this camera last delivered a frame successfully.
    // Only meaningful once isRunning() is true.
    std::chrono::milliseconds timeSinceLastSuccessfulGrab() const;

    // Add setters for the parameters
    void setDepthMode(sl::DEPTH_MODE mode) { init_params.depth_mode = mode; }
    void setBodyModel(sl::BODY_TRACKING_MODEL model) { body_tracking_model = model; }
    void setBodyTracking(bool enable) { enable_tracking = enable; }
    void setBodyFitting(bool enable) { enable_body_fitting = enable; }
    void setDetectionConfidence(float threshold) { detection_confidence = threshold; }
    void setPredictionTimeout(float seconds) { body_tracking_parameters.prediction_timeout_s = seconds; }

private:
    sl::Camera zed;
    sl::InitParameters init_params;
	sl::BodyTrackingParameters body_tracking_parameters;
    void work();
    std::thread runner;
    bool running;

    // Timestamp (steady_clock, ms) of the last successful grab() call; updated from the
    // worker thread, read from main() to detect a camera that stopped delivering frames
    // without the SDK's own internal recovery ever giving us a hard error/crash for it.
    std::atomic<long long> lastSuccessfulGrabMs{0};

    // Add member variables for the parameters
    sl::BODY_TRACKING_MODEL body_tracking_model = sl::BODY_TRACKING_MODEL::HUMAN_BODY_ACCURATE;
    bool enable_tracking = false;
    bool enable_body_fitting = false;
    float detection_confidence = 40;
};

#endif // ! __SENDER_RUNNER_HDR__
