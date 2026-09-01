#ifndef  __SENDER_RUNNER_HDR__
#define __SENDER_RUNNER_HDR__

#include <sl/Fusion.hpp>
#include <sl/Camera.hpp>

#include <thread>

class SenderRunner {
public:
    SenderRunner();
    ~SenderRunner();

    bool open(sl::InputType input, sl::BODY_FORMAT body_format);
    void start();
    void stop();
    
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
    
    // Add member variables for the parameters
    sl::BODY_TRACKING_MODEL body_tracking_model = sl::BODY_TRACKING_MODEL::HUMAN_BODY_ACCURATE;
    bool enable_tracking = false;
    bool enable_body_fitting = false;
    float detection_confidence = 40;
};

#endif // ! __SENDER_RUNNER_HDR__
