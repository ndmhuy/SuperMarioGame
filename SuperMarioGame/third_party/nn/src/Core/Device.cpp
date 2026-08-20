#include "nn/Core/Device.hpp"
#include "nn/Compute/CPUBackend.hpp"
#include <stdexcept>
#include <iostream>

// ---------------------------------------------------------------------------
// VENDORED, WITH ONE PATCH. See third_party/nn/README.md.
//
// Upstream (CS200-Cpp) also builds a Metal GPU backend. SuperMarioGame vendors
// the CPU path only: the policy network is ~123K parameters, which the NEON
// CPU backend handles in microseconds per step, and pulling MetalBackend.mm in
// would add an Objective-C++ language requirement to this project's CMake for
// no measured benefit. Device::metal() therefore returns the CPU device here.
//
// Keep this patch when re-syncing from upstream.
// ---------------------------------------------------------------------------

namespace nn {

namespace {
    Device* g_activeDevice = nullptr;
    std::unique_ptr<Backend> g_activeBackend = nullptr;
}

class CPUDevice : public Device {
public:
    DeviceType type() const override { return DeviceType::CPU; }
    std::unique_ptr<Backend> createBackend() const override {
        return std::make_unique<CPUBackend>();
    }
    std::string name() const override { return "CPU"; }
};

// PATCHED: Metal backend is not vendored — this is the CPU device wearing the
// Metal name, so any caller asking for GPU transparently gets CPU rather than
// failing to link.
class MetalDevice : public Device {
public:
    DeviceType type() const override { return DeviceType::CPU; }
    std::unique_ptr<Backend> createBackend() const override {
        return std::make_unique<CPUBackend>();
    }
    std::string name() const override { return "CPU (Metal not vendored)"; }
};

Device* Device::cpu() {
    static CPUDevice instance;
    return &instance;
}

Device* Device::metal() {
    static MetalDevice instance;
    return &instance;
}

Device* Device::active() {
    if (!g_activeDevice) {
        g_activeDevice = cpu();
    }
    return g_activeDevice;
}

void Device::setActive(Device* device) {
    g_activeDevice = device;
    g_activeBackend = device->createBackend();
}

Backend* Device::activeBackend() {
    if (!g_activeBackend) {
        // Initialize if not set
        setActive(active());
    }
    return g_activeBackend.get();
}

} // namespace nn
