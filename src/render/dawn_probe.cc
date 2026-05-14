#include "mira/render/dawn_probe.hpp"

#include <webgpu/webgpu_cpp.h>

#include <cstdint>
#include <cstdlib>

namespace mira {

auto ProbeDawn() -> int {
    static constexpr auto kTimedWaitAny = wgpu::InstanceFeatureName::TimedWaitAny;
    wgpu::InstanceDescriptor descriptor{
        .requiredFeatureCount = 1,
        .requiredFeatures = &kTimedWaitAny,
    };
    wgpu::Instance instance = wgpu::CreateInstance(&descriptor);
    if (instance == nullptr) {
        return EXIT_FAILURE;
    }

    wgpu::RequestAdapterOptions options = {};
    wgpu::Adapter adapter;
    auto adapter_callback = [&adapter](wgpu::RequestAdapterStatus status, wgpu::Adapter found,
                                       wgpu::StringView) {
        if (status == wgpu::RequestAdapterStatus::Success) {
            adapter = found;
        }
    };
    instance.WaitAny(
        instance.RequestAdapter(&options, wgpu::CallbackMode::WaitAnyOnly, adapter_callback),
        UINT64_MAX);
    if (adapter == nullptr) {
        return EXIT_FAILURE;
    }

    wgpu::DeviceDescriptor device_descriptor = {};
    device_descriptor.SetDeviceLostCallback(
        wgpu::CallbackMode::AllowSpontaneous,
        [](const wgpu::Device &, wgpu::DeviceLostReason, wgpu::StringView) {});
    wgpu::Device device;
    auto device_callback = [&device](wgpu::RequestDeviceStatus status, wgpu::Device found,
                                     wgpu::StringView) {
        if (status == wgpu::RequestDeviceStatus::Success) {
            device = found;
        }
    };
    instance.WaitAny(
        adapter.RequestDevice(&device_descriptor, wgpu::CallbackMode::WaitAnyOnly, device_callback),
        UINT64_MAX);
    return device == nullptr ? EXIT_FAILURE : EXIT_SUCCESS;
}

} // namespace mira
