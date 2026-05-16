#pragma once

#include "mira/gui/gui.hpp"
#include "mira/types.hpp"

#include <emscripten/html5.h>
#include <webgpu/webgpu_cpp.h>

#include <array>
#include <string_view>

namespace mira {

class Web {
  public:
    [[nodiscard]] auto init() -> b8;
    void frame();
    void import_image_ready(i32 width, i32 height, const u8 *pixels, usize byte_count,
                            std::string_view name);

  private:
    struct CanvasPixelSize {
        u32 width = 1;
        u32 height = 1;
    };

    struct MousePoint {
        i32 x = 0;
        i32 y = 0;
        b8 ok = false;
    };

    struct Frame {
        f32 width = 1.0F;
        f32 height = 1.0F;
        f32 screen_width = 1.0F;
        f32 screen_height = 1.0F;
        u32 rect_count = 0;
        u32 glyph_count = 0;
        u32 icon_count = 0;
        u32 layer_count = 0;
        f32 document_x = 0.0F;
        f32 document_y = 0.0F;
        f32 document_x1 = 1.0F;
        f32 document_y1 = 1.0F;
        f32 viewport_x = 0.0F;
        f32 viewport_y = 0.0F;
        f32 viewport_x1 = 1.0F;
        f32 viewport_y1 = 1.0F;
        f32 document_width = 1.0F;
        f32 document_height = 1.0F;
        f32 _pad0 = 0.0F;
        f32 _pad1 = 0.0F;
    };
    static_assert(sizeof(Frame) == 80);

    struct LayerGpu {
        f32 flags = 0.0F;
        f32 opacity = 1.0F;
        f32 texture_slot = 0.0F;
        f32 kind = 0.0F;
    };
    static_assert(sizeof(LayerGpu) == 16);

    [[nodiscard]] static auto read_canvas_size() -> CanvasPixelSize;
    static auto on_resize(int, const EmscriptenUiEvent *, void *user_data) -> bool;
    static auto on_mouse_move(int, const EmscriptenMouseEvent *event, void *user_data) -> bool;
    static auto on_mouse_down(int, const EmscriptenMouseEvent *event, void *user_data) -> bool;
    static auto on_mouse_up(int, const EmscriptenMouseEvent *event, void *user_data) -> bool;
    static auto on_wheel(int, const EmscriptenWheelEvent *event, void *user_data) -> bool;
    static auto on_key_down(int, const EmscriptenKeyboardEvent *event, void *user_data) -> bool;
    static void on_device_lost(const wgpu::Device &, wgpu::DeviceLostReason, wgpu::StringView,
                               Web *app);
    static void on_error(const wgpu::Device &, wgpu::ErrorType, wgpu::StringView, Web *app);

    void install_input();
    [[nodiscard]] auto mouse_point(const EmscriptenMouseEvent &event) const -> MousePoint;
    [[nodiscard]] auto menu_action_at(const EmscriptenMouseEvent &event) const -> MenuAction;
    void push_mouse_event(InputKind kind, const EmscriptenMouseEvent &event);
    void push_wheel_event(const EmscriptenWheelEvent &event);
    void push_key_down_event(const EmscriptenKeyboardEvent &event);
    void install_file_import();
    void open_image_picker();
    void export_png();
    [[nodiscard]] auto upload_layer(u8 slot, const u8 *pixels, usize byte_count) -> b8;
    [[nodiscard]] auto choose_surface() -> b8;
    [[nodiscard]] auto resize() -> b8;
    [[nodiscard]] auto make_pipeline() -> b8;
    [[nodiscard]] auto make_layer_texture() -> b8;
    [[nodiscard]] auto make_layer_bind_group() -> b8;
    [[nodiscard]] auto can_render(wgpu::SurfaceTexture surface_texture) -> b8;
    [[nodiscard]] auto upload_draws() -> b8;
    void encode_layer_clears(wgpu::CommandEncoder encoder);
    void encode_paint(wgpu::CommandEncoder encoder);
    void encode_gui(wgpu::RenderPassEncoder pass);

    wgpu::Instance instance_;
    wgpu::Adapter adapter_;
    wgpu::Device device_;
    wgpu::Queue queue_;
    wgpu::Surface surface_;
    wgpu::Buffer uniform_buffer_;
    wgpu::Buffer font_buffer_;
    wgpu::Buffer rect_buffer_;
    wgpu::Buffer glyph_buffer_;
    wgpu::Buffer icon_buffer_;
    wgpu::Buffer stamp_buffer_;
    wgpu::Buffer layer_buffer_;
    wgpu::BindGroupLayout bind_group_layout_;
    wgpu::BindGroupLayout layer_bind_group_layout_;
    wgpu::BindGroup bind_group_;
    wgpu::BindGroup layer_bind_group_;
    wgpu::RenderPipeline rect_pipeline_;
    wgpu::RenderPipeline glyph_pipeline_;
    wgpu::RenderPipeline icon_pipeline_;
    wgpu::RenderPipeline composite_pipeline_;
    wgpu::RenderPipeline stamp_pipeline_;
    wgpu::Texture layer_texture_;
    wgpu::TextureView layer_texture_view_;
    std::array<wgpu::TextureView, kMaxLayers> layer_slice_views_;
    wgpu::Sampler layer_sampler_;
    GuiState gui_;
    Table<InputEvent, kMaxInputEvents> input_events_;
    DrawList draws_;
    Screen screen_;
    wgpu::TextureFormat surface_format_ = wgpu::TextureFormat::BGRA8Unorm;
    wgpu::PresentMode present_mode_ = wgpu::PresentMode::Fifo;
    u32 width_ = 0;
    u32 height_ = 0;
    u32 surface_error_count_ = 0;
    u32 surface_skip_count_ = 0;
    u32 uncaptured_error_count_ = 0;
    u8 startup_frames_ = 8;
    b8 needs_configure_ = true;
    b8 needs_canvas_read_ = true;
    b8 draw_dirty_ = true;
    b8 pending_export_ = false;
    b8 device_lost_ = false;
};

} // namespace mira
