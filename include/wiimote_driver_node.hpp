#ifndef WIIMOTE_DRIVER_NODE_HPP_
#define WIIMOTE_DRIVER_NODE_HPP_

#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/joy.hpp"
#include "wiimote_device.hpp"

class WiimoteDriverNode : public rclcpp::Node
{
public:
    WiimoteDriverNode();

private:
    // Button indices within the published Joy message's buttons array.
    static constexpr int kAccelerateButton = 1;
    static constexpr int kBrakeButton = 2;
    static constexpr int kDeadmanButton = 4;
    static constexpr size_t kNumButtons = 5; // large enough for the highest index above

    WiimoteDevice device_;

    // Previous button states, used only to detect press/release transitions
    // for logging. The published Joy state is always read fresh from
    // device_ in publish_joy_state().
    bool a_button_down_ = false;
    bool b_button_down_ = false;
    bool two_button_down_ = false;

    rclcpp::TimerBase::SharedPtr timer_;
    rclcpp::Publisher<sensor_msgs::msg::Joy>::SharedPtr joy_publisher_;

    void poll_wiimote();
    void publish_joy_state();
    void log_transition(const char *label, unsigned int key_code, bool &was_down);
};

#endif // WIIMOTE_DRIVER_NODE_HPP_