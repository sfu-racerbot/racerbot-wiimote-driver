#ifndef WIIMOTE_DEVICE_HPP_
#define WIIMOTE_DEVICE_HPP_

#include "xwiimote.h"

#include <optional>
#include <string>
#include <sys/poll.h>
#include <unordered_map>

// Represents the current state of the Wiimote's accelerometer
struct WiimoteAccelerometerData {
  int x;
  int y;
  int z;
};

// RAII wrapper around a single xwiimote core interface.
//
// Owns the underlying struct xwii_iface* for its lifetime and guarantees
// it is closed/unref'd exactly once, even on early-return/exception paths.
// Move-only: there is exactly one owner of the underlying OS resource at
// any given time.
class WiimoteDevice {
public:
  static WiimoteDevice open();

  ~WiimoteDevice();

  WiimoteDevice(const WiimoteDevice &) = delete;
  WiimoteDevice &operator=(const WiimoteDevice &) = delete;

  WiimoteDevice(WiimoteDevice &&other) noexcept;
  WiimoteDevice &operator=(WiimoteDevice &&other) noexcept;

  // Reads any pending input from the device and updates internal button
  // state accordingly. Call this once per poll cycle before checking
  // is_button_down(). Cheap to call even when nothing is pending.
  // Throws std::runtime_error on a genuine read/poll error.
  void update();

  // Returns whether the given button is currently held down, as of the
  // last call to update(). key_code is one of the XWII_KEY_* constants
  // from xwiimote.h (e.g. XWII_KEY_A). Unknown/never-seen codes report
  // as not pressed.
  bool is_button_down(unsigned int key_code) const;

  // Sets the specified LED (0-3, from left to right) on/off.
  void set_led(unsigned int led_number, bool on);

  // Returns the current battery level of the Wiimote as a percentage (integer
  // between 0-100)
  uint8_t get_battery() const;

  // Returns the current values of the Wiimote's accelerometer
  WiimoteAccelerometerData get_accelerometer_state() const;

  // Sets the Wiimote's rumble feature on/off
  void set_rumble(bool enabled);

private:
  explicit WiimoteDevice(struct xwii_iface *core_iface,
                         struct xwii_iface *accelerometer_iface);

  // Finds the device path of the first available Wiimote, or nullopt if
  // none is currently connected.
  static std::optional<std::string> find_device_path();

  // Releases the underlying resource, if any, and clears members so the
  // destructor becomes a no-op. Used by the destructor and move ops.
  void release() noexcept;

  // True if at least one event is ready to be read right now.
  // Throws std::runtime_error on a genuine poll() failure.
  bool has_events();

  void drain_events(xwii_iface *iface);

  // Applies a single dispatched event to button_state_, if relevant.
  void apply_event(const xwii_event &event);

  struct xwii_iface *core_iface_ = nullptr;
  struct xwii_iface *accelerometer_iface_ = nullptr;

  int core_fd_ = -1;
  int accelerometer_fd_ = -1;

  struct pollfd pfds_[2] = {};
  std::unordered_map<unsigned int, bool> button_state_;
  bool led_status_[4] = {};
  WiimoteAccelerometerData accelerometer_data_;
};

#endif