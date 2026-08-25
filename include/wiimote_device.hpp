#ifndef WIIMOTE_DEVICE_HPP_
#define WIIMOTE_DEVICE_HPP_

#include "xwiimote.h"

#include <optional>
#include <string>

// RAII wrapper around a single xwiimote core interface.
//
// Owns the underlying struct xwii_iface* for its lifetime and guarantees
// it is closed/unref'd exactly once, even on early-return/exception paths.
// Move-only: there is exactly one owner of the underlying OS resource at
// any given time.
class WiimoteDevice
{
    static WiimoteDevice open();

    ~WiimoteDevice();

    WiimoteDevice(const WiimoteDevice &) = delete;
    WiimoteDevice &operator=(const WiimoteDevice &) = delete;

    WiimoteDevice(WiimoteDevice &&other) noexcept;
    WiimoteDevice &operator=(WiimoteDevice &&other) noexcept;

    // File descriptor suitable for poll()/select().
    int fd() const noexcept { return fd_; }

    // Returns true if at least one event is ready to be read within
    // timeout_ms (0 = return immediately without blocking).
    // Throws std::runtime_error on a genuine poll() failure.
    bool has_events(int timeout_ms = 0) const;

    // Reads and returns the next pending event, or std::nullopt once there
    // are no more events to read right now (i.e. would-block).
    // Throws std::runtime_error on a genuine read error.
    std::optional<xwii_event> next_event();

private:
    explicit WiimoteDevice(struct xwii_iface *iface);

    // Finds the device path of the first available Wiimote, or nullopt if
    // none is currently connected.
    static std::optional<std::string> find_device_path();

    // Releases the underlying resource, if any, and clears members so the
    // destructor becomes a no-op. Used by the destructor and move ops.
    void release() noexcept;

    struct xwii_iface *iface_ = nullptr;
    int fd_ = -1;
};

#endif