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
constexpr usize kRectBufferBytes = kMaxRects * sizeof(RectDraw);
constexpr usize kGlyphBufferBytes = kMaxGlyphs * sizeof(GlyphDraw);
constexpr usize kIconBufferBytes = kMaxIcons * sizeof(IconDraw);

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

[[nodiscard]] auto make_vertex_buffer(const wgpu::Device &device, usize size) -> wgpu::Buffer {
    wgpu::BufferDescriptor descriptor = {};
    descriptor.size = size;
    descriptor.usage = wgpu::BufferUsage::Vertex | wgpu::BufferUsage::CopyDst;
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
    install_input();
    if (!resize()) {
        return false;
    }
    if (!make_pipeline()) {
        return false;
    }
    return true;
}

void Web::frame() {
    if (startup_frames_ != 0) {
        --startup_frames_;
        needs_canvas_read_ = true;
        draw_dirty_ = true;
    }

    if (device_lost_ || !resize() || rect_pipeline_ == nullptr || glyph_pipeline_ == nullptr ||
        icon_pipeline_ == nullptr || bind_group_ == nullptr) {
        return;
    }
    if (!draw_dirty_) {
        return;
    }
    if (!upload_draws()) {
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
        draw_dirty_ = true;
        return;
    }

    wgpu::RenderPassColorAttachment color_attachment = {};
    color_attachment.view = view;
    color_attachment.loadOp = wgpu::LoadOp::Clear;
    color_attachment.storeOp = wgpu::StoreOp::Store;
    color_attachment.clearValue = wgpu::Color{1.0, 1.0, 1.0, 1.0};

    wgpu::RenderPassDescriptor render_pass = {};
    render_pass.colorAttachmentCount = 1;
    render_pass.colorAttachments = &color_attachment;

    wgpu::CommandEncoder encoder = device_.CreateCommandEncoder();
    if (encoder == nullptr) {
        ++surface_error_count_;
        draw_dirty_ = true;
        return;
    }

    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&render_pass);
    if (pass == nullptr) {
        ++surface_error_count_;
        draw_dirty_ = true;
        return;
    }
    pass.SetBindGroup(0, bind_group_);
    if (!draws_.rects.empty()) {
        pass.SetPipeline(rect_pipeline_);
        pass.SetVertexBuffer(0, rect_buffer_, 0, draws_.rects.byte_size());
        pass.Draw(6, static_cast<u32>(draws_.rects.size()));
    }
    if (!draws_.glyphs.empty()) {
        pass.SetPipeline(glyph_pipeline_);
        pass.SetVertexBuffer(0, glyph_buffer_, 0, draws_.glyphs.byte_size());
        pass.Draw(6, static_cast<u32>(draws_.glyphs.size()));
    }
    if (!draws_.icons.empty()) {
        pass.SetPipeline(icon_pipeline_);
        pass.SetVertexBuffer(0, icon_buffer_, 0, draws_.icons.byte_size());
        pass.Draw(6, static_cast<u32>(draws_.icons.size()));
    }
    pass.End();

    wgpu::CommandBuffer commands = encoder.Finish();
    if (commands == nullptr) {
        ++surface_error_count_;
        draw_dirty_ = true;
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

auto Web::on_mouse_move(int, const EmscriptenMouseEvent *event, void *user_data) -> bool {
    auto *app = static_cast<Web *>(user_data);
    if (app != nullptr && event != nullptr) {
        app->push_mouse_event(InputKind::kMouseMove, *event);
    }
    return false;
}

auto Web::on_mouse_down(int, const EmscriptenMouseEvent *event, void *user_data) -> bool {
    auto *app = static_cast<Web *>(user_data);
    if (app != nullptr && event != nullptr) {
        app->push_mouse_event(InputKind::kMouseDown, *event);
    }
    return true;
}

auto Web::on_mouse_up(int, const EmscriptenMouseEvent *event, void *user_data) -> bool {
    auto *app = static_cast<Web *>(user_data);
    if (app != nullptr && event != nullptr) {
        app->push_mouse_event(InputKind::kMouseUp, *event);
    }
    return true;
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

void Web::install_input() {
    emscripten_set_mousemove_callback(kCanvasSelector, this, false, &Web::on_mouse_move);
    emscripten_set_mousedown_callback(kCanvasSelector, this, true, &Web::on_mouse_down);
    emscripten_set_mouseup_callback(kCanvasSelector, this, true, &Web::on_mouse_up);
}

void Web::push_mouse_event(InputKind kind, const EmscriptenMouseEvent &event) {
    double css_width = 0.0;
    double css_height = 0.0;
    emscripten_get_element_css_size(kCanvasSelector, &css_width, &css_height);
    if (css_width <= 0.0 || css_height <= 0.0 || screen_.scale <= 0) {
        return;
    }

    const f32 physical_x =
        static_cast<f32>(event.targetX * static_cast<double>(width_) / css_width);
    const f32 physical_y =
        static_cast<f32>(event.targetY * static_cast<double>(height_) / css_height);
    const i32 x = std::clamp(static_cast<i32>(std::floor(physical_x / screen_.scale)), 0,
                             std::max(0, screen_.width - 1));
    const i32 y = std::clamp(static_cast<i32>(std::floor(physical_y / screen_.scale)), 0,
                             std::max(0, screen_.height - 1));
    InputEvent input = {};
    input.kind = kind;
    input.button = static_cast<u8>(event.button);
    input.x = x;
    input.y = y;
    (void)input_events_.push(input);
    draw_dirty_ = true;
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
    draw_dirty_ = true;
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

    rect_buffer_ = make_vertex_buffer(device_, kRectBufferBytes);
    glyph_buffer_ = make_vertex_buffer(device_, kGlyphBufferBytes);
    icon_buffer_ = make_vertex_buffer(device_, kIconBufferBytes);
    if (rect_buffer_ == nullptr || glyph_buffer_ == nullptr || icon_buffer_ == nullptr) {
        return false;
    }

    std::array<wgpu::BindGroupLayoutEntry, 1> bind_group_layout_entries = {};
    bind_group_layout_entries[0].binding = 0;
    bind_group_layout_entries[0].visibility =
        wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
    bind_group_layout_entries[0].buffer.type = wgpu::BufferBindingType::Uniform;
    bind_group_layout_entries[0].buffer.minBindingSize = sizeof(Frame);

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

    std::array<wgpu::BindGroupEntry, 1> bind_group_entries = {};
    bind_group_entries[0].binding = 0;
    bind_group_entries[0].buffer = uniform_buffer_;
    bind_group_entries[0].offset = 0;
    bind_group_entries[0].size = sizeof(Frame);

    wgpu::BindGroupDescriptor bind_group_descriptor = {};
    bind_group_descriptor.layout = bind_group_layout_;
    bind_group_descriptor.entryCount = bind_group_entries.size();
    bind_group_descriptor.entries = bind_group_entries.data();
    bind_group_ = device_.CreateBindGroup(&bind_group_descriptor);
    if (bind_group_ == nullptr) {
        return false;
    }

    std::array<wgpu::VertexAttribute, 2> attributes = {};
    attributes[0].format = wgpu::VertexFormat::Float32x4;
    attributes[0].offset = 0;
    attributes[0].shaderLocation = 0;
    attributes[1].format = wgpu::VertexFormat::Float32x4;
    attributes[1].offset = sizeof(f32) * 4U;
    attributes[1].shaderLocation = 1;

    wgpu::VertexBufferLayout instance_buffer = {};
    instance_buffer.arrayStride = sizeof(RectDraw);
    instance_buffer.stepMode = wgpu::VertexStepMode::Instance;
    instance_buffer.attributeCount = attributes.size();
    instance_buffer.attributes = attributes.data();

    auto make_render_pipeline = [&](const char *vertex_entry,
                                    const char *fragment_entry) -> wgpu::RenderPipeline {
        wgpu::ColorTargetState color_target = {};
        color_target.format = surface_format_;

        wgpu::FragmentState fragment = {};
        fragment.module = shader;
        fragment.entryPoint = fragment_entry;
        fragment.targetCount = 1;
        fragment.targets = &color_target;

        wgpu::RenderPipelineDescriptor pipeline_descriptor = {};
        pipeline_descriptor.layout = pipeline_layout;
        pipeline_descriptor.vertex.module = shader;
        pipeline_descriptor.vertex.entryPoint = vertex_entry;
        pipeline_descriptor.vertex.bufferCount = 1;
        pipeline_descriptor.vertex.buffers = &instance_buffer;
        pipeline_descriptor.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
        pipeline_descriptor.multisample.count = 1;
        pipeline_descriptor.fragment = &fragment;
        return device_.CreateRenderPipeline(&pipeline_descriptor);
    };

    rect_pipeline_ = make_render_pipeline("vs_rect", "fs_rect");
    glyph_pipeline_ = make_render_pipeline("vs_glyph", "fs_glyph");
    icon_pipeline_ = make_render_pipeline("vs_icon", "fs_icon");
    return rect_pipeline_ != nullptr && glyph_pipeline_ != nullptr && icon_pipeline_ != nullptr;
}

auto Web::upload_draws() -> b8 {
    if (!draw_dirty_) {
        return true;
    }

    build_gui_frame(&gui_, screen_, input_events_.span(), &draws_);
    input_events_.clear();

    const Frame uniforms = {
        .width = static_cast<f32>(width_),
        .height = static_cast<f32>(height_),
        .screen_width = static_cast<f32>(screen_.width),
        .screen_height = static_cast<f32>(screen_.height),
        .rect_count = static_cast<u32>(draws_.rects.size()),
        .glyph_count = static_cast<u32>(draws_.glyphs.size()),
        .icon_count = static_cast<u32>(draws_.icons.size()),
        ._pad1 = 0,
    };
    queue_.WriteBuffer(uniform_buffer_, 0, &uniforms, sizeof(uniforms));
    if (!draws_.rects.empty()) {
        queue_.WriteBuffer(rect_buffer_, 0, draws_.rects.data(), draws_.rects.byte_size());
    }
    if (!draws_.glyphs.empty()) {
        queue_.WriteBuffer(glyph_buffer_, 0, draws_.glyphs.data(), draws_.glyphs.byte_size());
    }
    if (!draws_.icons.empty()) {
        queue_.WriteBuffer(icon_buffer_, 0, draws_.icons.data(), draws_.icons.byte_size());
    }
    draw_dirty_ = false;
    return true;
}

auto Web::can_render(wgpu::SurfaceTexture surface_texture) -> b8 {
    // Browser surfaces can become outdated after resize or tab/device changes.
    switch (surface_texture.status) {
    case wgpu::SurfaceGetCurrentTextureStatus::SuccessOptimal:
    case wgpu::SurfaceGetCurrentTextureStatus::SuccessSuboptimal:
        if (surface_texture.texture != nullptr) {
            return true;
        }
        draw_dirty_ = true;
        ++surface_error_count_;
        return false;
    case wgpu::SurfaceGetCurrentTextureStatus::Outdated:
    case wgpu::SurfaceGetCurrentTextureStatus::Lost:
        needs_configure_ = true;
        draw_dirty_ = true;
        ++surface_skip_count_;
        return false;
    case wgpu::SurfaceGetCurrentTextureStatus::Timeout:
        draw_dirty_ = true;
        ++surface_skip_count_;
        return false;
    case wgpu::SurfaceGetCurrentTextureStatus::Error:
    default:
        draw_dirty_ = true;
        ++surface_error_count_;
        return false;
    }
}

} // namespace mira
