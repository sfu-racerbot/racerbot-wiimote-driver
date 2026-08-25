#include "wiimote_device.hpp"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <sys/poll.h>

namespace
{
    // Local RAII helper so struct xwii_monitor* is always released, even if
    // find_device_path() returns early.
    struct MonitorDeleter
    {
        void operator()(struct xwii_monitor *mon) const noexcept
        {
            if (mon)
            {
                xwii_monitor_unref(mon);
            }
        }
    };
    using MonitorPtr = std::unique_ptr<struct xwii_monitor, MonitorDeleter>;
}

std::optional<std::string> WiimoteDevice::find_device_path()
{
    MonitorPtr mon(xwii_monitor_new(false, false));
    if (!mon)
    {
        return std::nullopt;
    }

    char *ent = xwii_monitor_poll(mon.get());
    if (!ent)
    {
        return std::nullopt;
    }

    std::string path(ent);
    free(ent);
    return path;
}

WiimoteDevice WiimoteDevice::open()
{
    auto path = find_device_path();
    if (!path)
    {
        throw std::runtime_error("No Wiimote found");
    }

    struct xwii_iface *iface = nullptr;
    int err = xwii_iface_new(&iface, path->c_str());
    if (err)
    {
        throw std::runtime_error(
            "Failed to create Wiimote interface (error code: " +
            std::to_string(err) + ")");
    }

    err = xwii_iface_open(iface, XWII_IFACE_CORE);
    if (err)
    {
        xwii_iface_unref(iface);
        throw std::runtime_error(
            "Failed to open core interface (error code: " +
            std::to_string(err) + ")");
    }

    return WiimoteDevice(iface);
}

WiimoteDevice::WiimoteDevice(struct xwii_iface *iface)
    : iface_(iface), fd_(xwii_iface_get_fd(iface))
{
}

WiimoteDevice::WiimoteDevice(WiimoteDevice &&other) noexcept
    : iface_(other.iface_), fd_(other.fd_)
{
    other.release();
}

WiimoteDevice &WiimoteDevice::operator=(WiimoteDevice &&other) noexcept
{
    if (this != &other)
    {
        // Free whatever this instance currently owns before taking ownership
        // of other's resource.
        if (iface_)
        {
            xwii_iface_close(iface_, XWII_IFACE_CORE);
            xwii_iface_unref(iface_);
        }
        iface_ = other.iface_;
        fd_ = other.fd_;
        other.release();
    }
    return *this;
}

WiimoteDevice::~WiimoteDevice()
{
    if (iface_)
    {
        xwii_iface_close(iface_, XWII_IFACE_CORE);
        xwii_iface_unref(iface_);
    }
}

void WiimoteDevice::release() noexcept
{
    iface_ = nullptr;
    fd_ = -1;
}

bool WiimoteDevice::has_events(int timeout_ms) const
{
    struct pollfd pfd{};
    pfd.fd = fd_;
    pfd.events = POLLIN;

    int ready = poll(&pfd, 1, timeout_ms);
    if (ready < 0)
    {
        if (errno == EINTR)
        {
            return false;
        }
        throw std::runtime_error("Failed to poll Wiimote file descriptor");
    }
    return ready > 0;
}

std::optional<xwii_event> WiimoteDevice::next_event()
{
    struct xwii_event event{};
    int err = xwii_iface_dispatch(iface_, &event, sizeof(event));
    if (err == 0)
    {
        return event;
    }
    if (err == -EAGAIN)
    {
        return std::nullopt;
    }
    throw std::runtime_error("Error reading Wiimote event: " +
                             std::to_string(err));
}
