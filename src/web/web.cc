#include "mira/web/web.hpp"

#include "draw_shader.hpp"

#include <emscripten/html5.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace mira {
namespace {

constexpr char kCanvasSelector[] = "#canvas";
constexpr usize kDrawBufferBytes = kMaxDraws * sizeof(Draw);
constexpr usize kClipBufferBytes = kMaxClips * sizeof(Clip);
constexpr usize kTextBufferBytes = kMaxTextWords * sizeof(u32);
constexpr usize kSampleBufferBytes = kMaxSamples * sizeof(Sample);

[[nodiscard]] auto prefer_format(const wgpu::SurfaceCapabilities &capabilities)
    -> wgpu::TextureFormat {
    for (usize index = 0; index < capabilities.formatCount; ++index) {
        if (capabilities.formats[index] == wgpu::TextureFormat::BGRA8Unorm) {
            return capabilities.formats[index];
        }
    }
    return capabilities.formats[0];
}

[[nodiscard]] auto prefer_present_mode(const wgpu::SurfaceCapabilities &capabilities)
    -> wgpu::PresentMode {
    for (usize index = 0; index < capabilities.presentModeCount; ++index) {
        if (capabilities.presentModes[index] == wgpu::PresentMode::Fifo) {
            return capabilities.presentModes[index];
        }
    }
    return capabilities.presentModes[0];
}

[[nodiscard]] auto make_buffer(const wgpu::Device &device, usize size) -> wgpu::Buffer {
    wgpu::BufferDescriptor descriptor = {};
    descriptor.size = size;
    descriptor.usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst;
    return device.CreateBuffer(&descriptor);
}

} // namespace

auto Web::init() -> b8 {
    wgpu::InstanceDescriptor instance_descriptor = {};
    static constexpr auto kTimedWaitAny = wgpu::InstanceFeatureName::TimedWaitAny;
    instance_descriptor.requiredFeatureCount = 1;
    instance_descriptor.requiredFeatures = &kTimedWaitAny;
    instance_ = wgpu::CreateInstance(&instance_descriptor);
    if (instance_ == nullptr) {
        return false;
    }

    wgpu::RequestAdapterOptions adapter_options = {};
    instance_.WaitAny(instance_.RequestAdapter(&adapter_options, wgpu::CallbackMode::WaitAnyOnly,
                                               [this](wgpu::RequestAdapterStatus status,
                                                      wgpu::Adapter adapter, wgpu::StringView) {
                                                   if (status ==
                                                       wgpu::RequestAdapterStatus::Success) {
                                                       adapter_ = adapter;
                                                   }
                                               }),
                      std::numeric_limits<u64>::max());
    if (adapter_ == nullptr) {
        return false;
    }

    wgpu::DeviceDescriptor device_descriptor = {};
    device_descriptor.SetDeviceLostCallback(wgpu::CallbackMode::AllowSpontaneous,
                                            &Web::on_device_lost, this);
    device_descriptor.SetUncapturedErrorCallback(&Web::on_error, this);
    instance_.WaitAny(adapter_.RequestDevice(&device_descriptor, wgpu::CallbackMode::WaitAnyOnly,
                                             [this](wgpu::RequestDeviceStatus status,
                                                    wgpu::Device device, wgpu::StringView) {
                                                 if (status == wgpu::RequestDeviceStatus::Success) {
                                                     device_ = device;
                                                 }
                                             }),
                      std::numeric_limits<u64>::max());
    if (device_ == nullptr || device_lost_) {
        return false;
    }
    queue_ = device_.GetQueue();
    if (queue_ == nullptr) {
        return false;
    }

    wgpu::EmscriptenSurfaceSourceCanvasHTMLSelector canvas_descriptor = {};
    canvas_descriptor.selector = kCanvasSelector;
    wgpu::SurfaceDescriptor surface_descriptor = {};
    surface_descriptor.nextInChain = &canvas_descriptor;
    surface_ = instance_.CreateSurface(&surface_descriptor);
    if (surface_ == nullptr || !choose_surface()) {
        return false;
    }

    emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, this, false, &Web::on_resize);
    return resize() && make_pipeline();
}

void Web::frame() {
    if (device_lost_ || !resize() || pipeline_ == nullptr || bind_group_ == nullptr ||
        !upload_draws()) {
        return;
    }

    wgpu::SurfaceTexture surface_texture = {};
    surface_.GetCurrentTexture(&surface_texture);
    if (!can_render(surface_texture)) {
        return;
    }

    wgpu::TextureView view = surface_texture.texture.CreateView();
    if (view == nullptr) {
        ++surface_error_count_;
        return;
    }

    wgpu::RenderPassColorAttachment color_attachment = {};
    color_attachment.view = view;
    color_attachment.loadOp = wgpu::LoadOp::Clear;
    color_attachment.storeOp = wgpu::StoreOp::Store;
    color_attachment.clearValue = wgpu::Color{0.055, 0.075, 0.060, 1.0};

    wgpu::RenderPassDescriptor render_pass = {};
    render_pass.colorAttachmentCount = 1;
    render_pass.colorAttachments = &color_attachment;

    wgpu::CommandEncoder encoder = device_.CreateCommandEncoder();
    if (encoder == nullptr) {
        ++surface_error_count_;
        return;
    }

    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&render_pass);
    if (pass == nullptr) {
        ++surface_error_count_;
        return;
    }
    pass.SetPipeline(pipeline_);
    pass.SetBindGroup(0, bind_group_);
    pass.Draw(6);
    pass.End();

    wgpu::CommandBuffer commands = encoder.Finish();
    if (commands == nullptr) {
        ++surface_error_count_;
        return;
    }
    queue_.Submit(1, &commands);
}

auto Web::read_canvas_size() -> CanvasPixelSize {
    double css_width = 0.0;
    double css_height = 0.0;
    emscripten_get_element_css_size(kCanvasSelector, &css_width, &css_height);
    const double scale = std::max(emscripten_get_device_pixel_ratio(), 1.0);
    return {
        .width = static_cast<u32>(std::max(1.0, std::floor(css_width * scale))),
        .height = static_cast<u32>(std::max(1.0, std::floor(css_height * scale))),
    };
}

auto Web::on_resize(int, const EmscriptenUiEvent *, void *user_data) -> bool {
    auto *app = static_cast<Web *>(user_data);
    if (app != nullptr) {
        app->needs_canvas_read_ = true;
        app->needs_configure_ = true;
    }
    return false;
}

void Web::on_device_lost(const wgpu::Device &, wgpu::DeviceLostReason, wgpu::StringView, Web *app) {
    if (app != nullptr) {
        app->device_lost_ = true;
    }
}

void Web::on_error(const wgpu::Device &, wgpu::ErrorType, wgpu::StringView, Web *app) {
    if (app != nullptr) {
        ++app->uncaptured_error_count_;
    }
}

auto Web::choose_surface() -> b8 {
    wgpu::SurfaceCapabilities capabilities = {};
    surface_.GetCapabilities(adapter_, &capabilities);
    if (capabilities.formatCount == 0 || capabilities.presentModeCount == 0) {
        return false;
    }
    surface_format_ = prefer_format(capabilities);
    present_mode_ = prefer_present_mode(capabilities);
    return true;
}

auto Web::resize() -> b8 {
    if (!needs_canvas_read_ && !needs_configure_) {
        return true;
    }

    const CanvasPixelSize size = read_canvas_size();
    needs_canvas_read_ = false;
    if (size.width == width_ && size.height == height_ && !needs_configure_) {
        return true;
    }

    width_ = size.width;
    height_ = size.height;
    const Screen next_screen = screen_for(static_cast<i32>(width_), static_cast<i32>(height_));
    if (next_screen.scale != screen_.scale || next_screen.width != screen_.width ||
        next_screen.height != screen_.height) {
        screen_ = next_screen;
        draw_dirty_ = true;
    }
    emscripten_set_canvas_element_size(kCanvasSelector, static_cast<int>(width_),
                                       static_cast<int>(height_));

    wgpu::SurfaceConfiguration configuration = {};
    configuration.device = device_;
    configuration.format = surface_format_;
    configuration.usage = wgpu::TextureUsage::RenderAttachment;
    configuration.width = width_;
    configuration.height = height_;
    configuration.presentMode = present_mode_;
    configuration.alphaMode = wgpu::CompositeAlphaMode::Opaque;
    surface_.Configure(&configuration);
    needs_configure_ = false;
    return true;
}

auto Web::make_pipeline() -> b8 {
    // The shader is authored as WGSL and embedded by GN into a generated header.
    wgpu::ShaderSourceWGSL wgsl = {};
    wgsl.code = wgpu::StringView(kDrawShaderSource, sizeof(kDrawShaderSource) - 1);

    wgpu::ShaderModuleDescriptor shader_descriptor = {};
    shader_descriptor.nextInChain = &wgsl;
    wgpu::ShaderModule shader = device_.CreateShaderModule(&shader_descriptor);
    if (shader == nullptr) {
        return false;
    }

    wgpu::BufferDescriptor uniform_buffer_descriptor = {};
    uniform_buffer_descriptor.size = sizeof(Frame);
    uniform_buffer_descriptor.usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst;
    uniform_buffer_ = device_.CreateBuffer(&uniform_buffer_descriptor);
    if (uniform_buffer_ == nullptr) {
        return false;
    }

    draw_buffer_ = make_buffer(device_, kDrawBufferBytes);
    clip_buffer_ = make_buffer(device_, kClipBufferBytes);
    text_buffer_ = make_buffer(device_, kTextBufferBytes);
    sample_buffer_ = make_buffer(device_, kSampleBufferBytes);
    if (draw_buffer_ == nullptr || clip_buffer_ == nullptr || text_buffer_ == nullptr ||
        sample_buffer_ == nullptr) {
        return false;
    }

    std::array<wgpu::BindGroupLayoutEntry, 5> bind_group_layout_entries = {};
    bind_group_layout_entries[0].binding = 0;
    bind_group_layout_entries[0].visibility = wgpu::ShaderStage::Fragment;
    bind_group_layout_entries[0].buffer.type = wgpu::BufferBindingType::Uniform;
    bind_group_layout_entries[0].buffer.minBindingSize = sizeof(Frame);
    for (u32 binding = 1; binding < bind_group_layout_entries.size(); ++binding) {
        bind_group_layout_entries[binding].binding = binding;
        bind_group_layout_entries[binding].visibility = wgpu::ShaderStage::Fragment;
        bind_group_layout_entries[binding].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;
    }
    bind_group_layout_entries[1].buffer.minBindingSize = kDrawBufferBytes;
    bind_group_layout_entries[2].buffer.minBindingSize = kClipBufferBytes;
    bind_group_layout_entries[3].buffer.minBindingSize = kTextBufferBytes;
    bind_group_layout_entries[4].buffer.minBindingSize = kSampleBufferBytes;

    wgpu::BindGroupLayoutDescriptor bind_group_layout_descriptor = {};
    bind_group_layout_descriptor.entryCount = bind_group_layout_entries.size();
    bind_group_layout_descriptor.entries = bind_group_layout_entries.data();
    bind_group_layout_ = device_.CreateBindGroupLayout(&bind_group_layout_descriptor);
    if (bind_group_layout_ == nullptr) {
        return false;
    }

    wgpu::PipelineLayoutDescriptor pipeline_layout_descriptor = {};
    pipeline_layout_descriptor.bindGroupLayoutCount = 1;
    pipeline_layout_descriptor.bindGroupLayouts = &bind_group_layout_;
    wgpu::PipelineLayout pipeline_layout =
        device_.CreatePipelineLayout(&pipeline_layout_descriptor);
    if (pipeline_layout == nullptr) {
        return false;
    }

    std::array<wgpu::BindGroupEntry, 5> bind_group_entries = {};
    bind_group_entries[0].binding = 0;
    bind_group_entries[0].buffer = uniform_buffer_;
    bind_group_entries[0].offset = 0;
    bind_group_entries[0].size = sizeof(Frame);
    bind_group_entries[1].binding = 1;
    bind_group_entries[1].buffer = draw_buffer_;
    bind_group_entries[1].size = kDrawBufferBytes;
    bind_group_entries[2].binding = 2;
    bind_group_entries[2].buffer = clip_buffer_;
    bind_group_entries[2].size = kClipBufferBytes;
    bind_group_entries[3].binding = 3;
    bind_group_entries[3].buffer = text_buffer_;
    bind_group_entries[3].size = kTextBufferBytes;
    bind_group_entries[4].binding = 4;
    bind_group_entries[4].buffer = sample_buffer_;
    bind_group_entries[4].size = kSampleBufferBytes;

    wgpu::BindGroupDescriptor bind_group_descriptor = {};
    bind_group_descriptor.layout = bind_group_layout_;
    bind_group_descriptor.entryCount = bind_group_entries.size();
    bind_group_descriptor.entries = bind_group_entries.data();
    bind_group_ = device_.CreateBindGroup(&bind_group_descriptor);
    if (bind_group_ == nullptr) {
        return false;
    }

    wgpu::ColorTargetState color_target = {};
    color_target.format = surface_format_;

    wgpu::FragmentState fragment = {};
    fragment.module = shader;
    fragment.entryPoint = "fs_main";
    fragment.targetCount = 1;
    fragment.targets = &color_target;

    wgpu::RenderPipelineDescriptor pipeline_descriptor = {};
    pipeline_descriptor.layout = pipeline_layout;
    pipeline_descriptor.vertex.module = shader;
    pipeline_descriptor.vertex.entryPoint = "vs_main";
    pipeline_descriptor.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
    pipeline_descriptor.multisample.count = 1;
    pipeline_descriptor.fragment = &fragment;

    pipeline_ = device_.CreateRenderPipeline(&pipeline_descriptor);
    return pipeline_ != nullptr;
}

auto Web::upload_draws() -> b8 {
    if (!draw_dirty_) {
        return true;
    }

    build_demo(&draws_, screen_);
    const usize text_word_count = (draws_.text.size() + sizeof(u32) - 1U) / sizeof(u32);
    for (usize index = 0; index < text_word_count; ++index) {
        text_words_[index] = 0;
    }
    for (usize index = 0; index < draws_.text.size(); ++index) {
        const u32 byte = static_cast<u8>(draws_.text[index]);
        text_words_[index >> 2U] |= byte << ((index & 3U) * 8U);
    }

    const Frame uniforms = {
        .width = static_cast<f32>(width_),
        .height = static_cast<f32>(height_),
        .screen_width = static_cast<f32>(screen_.width),
        .screen_height = static_cast<f32>(screen_.height),
        .draw_count = static_cast<u32>(draws_.draws.size()),
        .clip_count = static_cast<u32>(draws_.clips.size()),
        .sample_count = static_cast<u32>(draws_.samples.size()),
        ._pad = 0,
    };
    queue_.WriteBuffer(uniform_buffer_, 0, &uniforms, sizeof(uniforms));
    if (!draws_.draws.empty()) {
        queue_.WriteBuffer(draw_buffer_, 0, draws_.draws.data(), draws_.draws.byte_size());
    }
    if (!draws_.clips.empty()) {
        queue_.WriteBuffer(clip_buffer_, 0, draws_.clips.data(), draws_.clips.byte_size());
    }
    if (text_word_count != 0) {
        queue_.WriteBuffer(text_buffer_, 0, text_words_.data(), text_word_count * sizeof(u32));
    }
    if (!draws_.samples.empty()) {
        queue_.WriteBuffer(sample_buffer_, 0, draws_.samples.data(), draws_.samples.byte_size());
    }
    draw_dirty_ = false;
    return true;
}

auto Web::can_render(wgpu::SurfaceTexture surface_texture) -> b8 {
    // Browser surfaces can become outdated after resize or tab/device changes.
    switch (surface_texture.status) {
    case wgpu::SurfaceGetCurrentTextureStatus::SuccessOptimal:
    case wgpu::SurfaceGetCurrentTextureStatus::SuccessSuboptimal:
        return surface_texture.texture != nullptr;
    case wgpu::SurfaceGetCurrentTextureStatus::Outdated:
    case wgpu::SurfaceGetCurrentTextureStatus::Lost:
        needs_configure_ = true;
        ++surface_skip_count_;
        return false;
    case wgpu::SurfaceGetCurrentTextureStatus::Timeout:
        ++surface_skip_count_;
        return false;
    case wgpu::SurfaceGetCurrentTextureStatus::Error:
    default:
        ++surface_error_count_;
        return false;
    }
}

} // namespace mira
