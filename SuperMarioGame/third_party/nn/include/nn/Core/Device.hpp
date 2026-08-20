#pragma once

#include "nn/Compute/Backend.hpp"
#include <memory>
#include <string>

namespace nn {

enum class DeviceType {
    CPU,
    Metal
};

// Abstract Factory for creating backends
class Device {
public:
    virtual ~Device() = default;

    virtual DeviceType type() const = 0;
    virtual std::unique_ptr<Backend> createBackend() const = 0;
    virtual std::string name() const = 0;

    // Singleton accessors for specific devices
    static Device* cpu();
    static Device* metal();

    // Global active device management
    static Device* active();
    static void setActive(Device* device);
    
    // Get a persistent backend instance for the active device
    static Backend* activeBackend();
};

} // namespace nn
