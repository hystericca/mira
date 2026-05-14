#pragma once

#include "mira/draw/draw.hpp"
#include "mira/types.hpp"

#include <array>

#include <emscripten/html5.h>
#include <webgpu/webgpu_cpp.h>

namespace mira {

class Web {
  public:
    [[nodiscard]] auto init() -> b8;
    void frame();

  private:
    struct CanvasPixelSize {
        u32 width = 1;
        u32 height = 1;
    };

    struct Frame {
        f32 width = 1.0F;
        f32 height = 1.0F;
        f32 screen_width = 1.0F;
        f32 screen_height = 1.0F;
        u32 draw_count = 0;
        u32 clip_count = 0;
        u32 sample_count = 0;
        u32 _pad = 0;
    };
    static_assert(sizeof(Frame) == 32);

    [[nodiscard]] static auto read_canvas_size() -> CanvasPixelSize;
    static auto on_resize(int, const EmscriptenUiEvent *, void *user_data) -> bool;
    static void on_device_lost(const wgpu::Device &, wgpu::DeviceLostReason, wgpu::StringView,
                               Web *app);
    static void on_error(const wgpu::Device &, wgpu::ErrorType, wgpu::StringView, Web *app);

    [[nodiscard]] auto choose_surface() -> b8;
    [[nodiscard]] auto resize() -> b8;
    [[nodiscard]] auto make_pipeline() -> b8;
    [[nodiscard]] auto can_render(wgpu::SurfaceTexture surface_texture) -> b8;
    [[nodiscard]] auto upload_draws() -> b8;

    wgpu::Instance instance_;
    wgpu::Adapter adapter_;
    wgpu::Device device_;
    wgpu::Queue queue_;
    wgpu::Surface surface_;
    wgpu::Buffer uniform_buffer_;
    wgpu::Buffer draw_buffer_;
    wgpu::Buffer clip_buffer_;
    wgpu::Buffer text_buffer_;
    wgpu::Buffer sample_buffer_;
    wgpu::BindGroupLayout bind_group_layout_;
    wgpu::BindGroup bind_group_;
    wgpu::RenderPipeline pipeline_;
    DrawList draws_;
    Screen screen_;
    std::array<u32, kMaxTextWords> text_words_ = {};
    wgpu::TextureFormat surface_format_ = wgpu::TextureFormat::BGRA8Unorm;
    wgpu::PresentMode present_mode_ = wgpu::PresentMode::Fifo;
    u32 width_ = 0;
    u32 height_ = 0;
    u32 surface_error_count_ = 0;
    u32 surface_skip_count_ = 0;
    u32 uncaptured_error_count_ = 0;
    b8 needs_configure_ = true;
    b8 needs_canvas_read_ = true;
    b8 draw_dirty_ = true;
    b8 device_lost_ = false;
};

} // namespace mira
