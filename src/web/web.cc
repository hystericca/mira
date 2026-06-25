#include "mira/web/web.hpp"

#include "draw_shader.hpp"

#include <emscripten/emscripten.h>
#include <emscripten/html5.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <string_view>

namespace mira {
namespace {

constexpr char kCanvasSelector[] = "#canvas";
constexpr usize kRectBufferBytes{kMaxRects * sizeof(RectDraw)};
constexpr usize kGlyphBufferBytes{kMaxGlyphs * sizeof(GlyphDraw)};
constexpr usize kIconBufferBytes{kMaxIcons * sizeof(IconDraw)};
constexpr usize kPreviewStampBufferBytes{kMaxPreviewStamps *
                                         sizeof(PaintStamp)};
constexpr usize kStampBufferBytes{kMaxPaintDelta * sizeof(PaintStamp)};
constexpr usize kLayerBufferBytes{kMaxLayers * sizeof(f32) * 4U};
constexpr usize kInputTerminalReserve{8};

[[nodiscard]] auto prefer_format(const wgpu::SurfaceCapabilities &capabilities)
    -> wgpu::TextureFormat {
  for (usize index{0}; index < capabilities.formatCount; ++index) {
    if (capabilities.formats[index] == wgpu::TextureFormat::BGRA8Unorm) {
      return capabilities.formats[index];
    }
  }
  return capabilities.formats[0];
}

[[nodiscard]] auto
prefer_present_mode(const wgpu::SurfaceCapabilities &capabilities)
    -> wgpu::PresentMode {
  for (usize index{0}; index < capabilities.presentModeCount; ++index) {
    if (capabilities.presentModes[index] == wgpu::PresentMode::Fifo) {
      return capabilities.presentModes[index];
    }
  }
  return capabilities.presentModes[0];
}

[[nodiscard]] auto make_vertex_buffer(const wgpu::Device &device, usize size)
    -> wgpu::Buffer {
  wgpu::BufferDescriptor descriptor = {};
  descriptor.size = size;
  descriptor.usage = wgpu::BufferUsage::Vertex | wgpu::BufferUsage::CopyDst;
  return device.CreateBuffer(&descriptor);
}

[[nodiscard]] auto layer_flags_for_gpu(const Layer &layer) -> f32 {
  return static_cast<f32>(layer.flags);
}

void draw_rows(wgpu::RenderPassEncoder pass,
               const wgpu::RenderPipeline &pipeline, const wgpu::Buffer &buffer,
               usize first, usize count, usize stride) {
  if (count == 0) {
    return;
  }
  pass.SetPipeline(pipeline);
  pass.SetVertexBuffer(0, buffer, static_cast<u64>(first * stride),
                       static_cast<u64>(count * stride));
  pass.Draw(6, static_cast<u32>(count));
}

[[nodiscard]] auto key_is(const EmscriptenKeyboardEvent &event,
                          std::string_view key) -> b8 {
  return std::string_view(event.key) == key;
}

[[nodiscard]] auto motion_input(InputKind kind) -> b8 {
  return kind == InputKind::kMouseMove || kind == InputKind::kWheel;
}

[[nodiscard]] auto clamp_delta(i32 value) -> i32 {
  return std::clamp(value, -160, 160);
}

void focus_canvas() {
  emscripten_run_script("document.getElementById('canvas').focus()");
}

void suppress_context_menu() {
  emscripten_run_script("document.getElementById('canvas').addEventListener('"
                        "contextmenu',function(e){"
                        "e.preventDefault();"
                        "});");
}

} // namespace

auto Web::init() -> b8 {
  install_file_import();
  wgpu::InstanceDescriptor instance_descriptor = {};
  static constexpr auto kTimedWaitAny = wgpu::InstanceFeatureName::TimedWaitAny;
  instance_descriptor.requiredFeatureCount = 1;
  instance_descriptor.requiredFeatures = &kTimedWaitAny;
  instance_ = wgpu::CreateInstance(&instance_descriptor);
  if (instance_ == nullptr) {
    return false;
  }

  wgpu::RequestAdapterOptions adapter_options = {};
  instance_.WaitAny(instance_.RequestAdapter(
                        &adapter_options, wgpu::CallbackMode::WaitAnyOnly,
                        [this](wgpu::RequestAdapterStatus status,
                               wgpu::Adapter adapter, wgpu::StringView) {
                          if (status == wgpu::RequestAdapterStatus::Success) {
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
  instance_.WaitAny(adapter_.RequestDevice(
                        &device_descriptor, wgpu::CallbackMode::WaitAnyOnly,
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

  emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, this, false,
                                 &Web::on_resize);
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

  if (device_lost_ || !resize() || rect_pipeline_ == nullptr ||
      glyph_pipeline_ == nullptr || icon_pipeline_ == nullptr ||
      preview_stamp_pipeline_ == nullptr || composite_pipeline_ == nullptr ||
      stamp_pipeline_ == nullptr || bind_group_ == nullptr ||
      layer_bind_group_ == nullptr) {
    return;
  }
  if (!draw_dirty_) {
    return;
  }
  if (!upload_draws()) {
    return;
  }

  wgpu::CommandEncoder encoder = device_.CreateCommandEncoder();
  if (encoder == nullptr) {
    ++surface_error_count_;
    draw_dirty_ = true;
    return;
  }
  encode_layer_clears(encoder);
  encode_paint(encoder);
  const b8 has_offscreen_work{!gui_.clear_slots.empty() ||
                              !gui_.paint_delta.empty()};

  wgpu::SurfaceTexture surface_texture = {};
  surface_.GetCurrentTexture(&surface_texture);
  if (!can_render(surface_texture)) {
    if (has_offscreen_work) {
      wgpu::CommandBuffer paint_commands = encoder.Finish();
      if (paint_commands != nullptr) {
        queue_.Submit(1, &paint_commands);
      }
    }
    if (pending_export_) {
      pending_export_ = false;
      export_png();
    }
    return;
  }
  wgpu::TextureView view = surface_texture.texture.CreateView();
  if (view == nullptr) {
    ++surface_error_count_;
    draw_dirty_ = true;
    if (has_offscreen_work) {
      wgpu::CommandBuffer paint_commands = encoder.Finish();
      if (paint_commands != nullptr) {
        queue_.Submit(1, &paint_commands);
      }
    }
    return;
  }

  wgpu::RenderPassColorAttachment color_attachment = {};
  color_attachment.view = view;
  color_attachment.loadOp = wgpu::LoadOp::Clear;
  color_attachment.storeOp = wgpu::StoreOp::Store;
  color_attachment.clearValue = wgpu::Color{0.0, 0.0, 0.0, 1.0};

  wgpu::RenderPassDescriptor render_pass = {};
  render_pass.colorAttachmentCount = 1;
  render_pass.colorAttachments = &color_attachment;

  wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&render_pass);
  if (pass == nullptr) {
    ++surface_error_count_;
    draw_dirty_ = true;
    if (has_offscreen_work) {
      wgpu::CommandBuffer paint_commands = encoder.Finish();
      if (paint_commands != nullptr) {
        queue_.Submit(1, &paint_commands);
      }
    }
    return;
  }
  encode_gui(pass);
  pass.End();

  wgpu::CommandBuffer commands = encoder.Finish();
  if (commands == nullptr) {
    ++surface_error_count_;
    draw_dirty_ = true;
    return;
  }
  queue_.Submit(1, &commands);
  draw_dirty_ = guianimating(gui_);
  if (pending_export_) {
    pending_export_ = false;
    export_png();
  }
}

auto Web::read_canvas_size() -> CanvasPixelSize {
  double css_width{0.0};
  double css_height{0.0};
  emscripten_get_element_css_size(kCanvasSelector, &css_width, &css_height);
  css_width_ = static_cast<f32>(std::max(1.0, css_width));
  css_height_ = static_cast<f32>(std::max(1.0, css_height));
  const double scale{std::max(emscripten_get_device_pixel_ratio(), 1.0)};
  return {
      .width = static_cast<u32>(std::max(1.0, std::floor(css_width_ * scale))),
      .height =
          static_cast<u32>(std::max(1.0, std::floor(css_height_ * scale))),
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

auto Web::on_mouse_move(int, const EmscriptenMouseEvent *event, void *user_data)
    -> bool {
  auto *app = static_cast<Web *>(user_data);
  if (app != nullptr && event != nullptr) {
    app->push_mouse_event(InputKind::kMouseMove, *event);
  }
  return false;
}

auto Web::on_mouse_down(int, const EmscriptenMouseEvent *event, void *user_data)
    -> bool {
  auto *app = static_cast<Web *>(user_data);
  if (app != nullptr && event != nullptr) {
    focus_canvas();
    app->push_mouse_event(InputKind::kMouseDown, *event);
  }
  return true;
}

auto Web::on_mouse_up(int, const EmscriptenMouseEvent *event, void *user_data)
    -> bool {
  auto *app = static_cast<Web *>(user_data);
  if (app != nullptr && event != nullptr) {
    app->push_mouse_event(InputKind::kMouseUp, *event);
  }
  return true;
}

auto Web::on_wheel(int, const EmscriptenWheelEvent *event, void *user_data)
    -> bool {
  auto *app = static_cast<Web *>(user_data);
  if (app != nullptr && event != nullptr) {
    app->push_wheel_event(*event);
  }
  return true;
}

auto Web::on_key_down(int, const EmscriptenKeyboardEvent *event,
                      void *user_data) -> bool {
  auto *app = static_cast<Web *>(user_data);
  if (app != nullptr && event != nullptr) {
    app->push_key_down_event(*event);
  }
  return true;
}

void Web::on_device_lost(const wgpu::Device &, wgpu::DeviceLostReason,
                         wgpu::StringView, Web *app) {
  if (app != nullptr) {
    app->device_lost_ = true;
  }
}

void Web::on_error(const wgpu::Device &, wgpu::ErrorType, wgpu::StringView,
                   Web *app) {
  if (app != nullptr) {
    ++app->uncaptured_error_count_;
  }
}

void Web::install_input() {
  suppress_context_menu();
  emscripten_set_mousemove_callback(kCanvasSelector, this, false,
                                    &Web::on_mouse_move);
  emscripten_set_mousedown_callback(kCanvasSelector, this, true,
                                    &Web::on_mouse_down);
  emscripten_set_mouseup_callback(kCanvasSelector, this, true,
                                  &Web::on_mouse_up);
  emscripten_set_wheel_callback(kCanvasSelector, this, true, &Web::on_wheel);
  emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, this, true,
                                  &Web::on_key_down);
}

auto Web::coalesce_input(InputEvent input) -> b8 {
  if (!motion_input(input.kind)) {
    return false;
  }
  for (usize offset{0}; offset < input_events_.size(); ++offset) {
    const usize index{input_events_.size() - 1U - offset};
    InputEvent &queued = input_events_[index];
    if (queued.kind != input.kind) {
      continue;
    }
    if (queued.kind == InputKind::kWheel && queued.mods != input.mods) {
      continue;
    }
    queued.x = input.x;
    queued.y = input.y;
    queued.dx = clamp_delta(queued.dx + input.dx);
    queued.dy = clamp_delta(queued.dy + input.dy);
    queued.button = input.button;
    queued.buttons = input.buttons;
    queued.mods = input.mods;
    return true;
  }
  return false;
}

auto Web::drop_motion_input() -> b8 {
  for (usize offset{0}; offset < input_events_.size(); ++offset) {
    const usize index{input_events_.size() - 1U - offset};
    if (motion_input(input_events_[index].kind)) {
      return input_events_.erase(index);
    }
  }
  return false;
}

auto Web::push_input_event(InputEvent input) -> b8 {
  if (motion_input(input.kind)) {
    if (input_events_.size() + kInputTerminalReserve >=
        input_events_.capacity()) {
      if (coalesce_input(input)) {
        return true;
      }
      input_events_.overflowed = true;
      return false;
    }
    return input_events_.push(input);
  }

  if (input_events_.push(input)) {
    return true;
  }
  if (drop_motion_input()) {
    return input_events_.push(input);
  }
  input_events_.overflowed = true;
  return false;
}

auto Web::mouse_point(const EmscriptenMouseEvent &event) const -> MousePoint {
  if (css_width_ <= 0.0F || css_height_ <= 0.0F || screen_.scale <= 0) {
    return {};
  }

  const f32 physical_x{static_cast<f32>(
      event.targetX * static_cast<double>(width_) / css_width_)};
  const f32 physical_y{static_cast<f32>(
      event.targetY * static_cast<double>(height_) / css_height_)};
  const i32 x{
      std::clamp(static_cast<i32>(std::floor(physical_x / screen_.scale)), 0,
                 std::max(0, screen_.width - 1))};
  const i32 y{
      std::clamp(static_cast<i32>(std::floor(physical_y / screen_.scale)), 0,
                 std::max(0, screen_.height - 1))};
  return {.x = x, .y = y, .ok = true};
}

void Web::push_mouse_event(InputKind kind, const EmscriptenMouseEvent &event) {
  const MousePoint point{mouse_point(event)};
  if (!point.ok) {
    return;
  }
  InputEvent input = {};
  input.kind = kind;
  input.button = static_cast<u8>(event.button);
  input.buttons = static_cast<u8>(event.buttons & 0xFFU);
  input.mods = static_cast<u8>((event.ctrlKey ? kInputCtrl : 0U) |
                               (event.shiftKey ? kInputShift : 0U));
  input.x = point.x;
  input.y = point.y;
  input.dx = static_cast<i32>(std::round(static_cast<double>(event.movementX) /
                                         static_cast<double>(screen_.scale)));
  input.dy = static_cast<i32>(std::round(static_cast<double>(event.movementY) /
                                         static_cast<double>(screen_.scale)));
  if (!push_input_event(input) && kind == InputKind::kMouseUp) {
    gui_.painting = false;
    gui_.panning = false;
    gui_.setting_opacity = false;
  }
  draw_dirty_ = true;
}

void Web::push_wheel_event(const EmscriptenWheelEvent &event) {
  const MousePoint point{mouse_point(event.mouse)};
  if (!point.ok) {
    return;
  }

  double unit{1.0};
  if (event.deltaMode == DOM_DELTA_LINE) {
    unit = 8.0;
  } else if (event.deltaMode == DOM_DELTA_PAGE) {
    unit = std::max(1.0, static_cast<double>(height_) * 0.5);
  }

  i32 dx{static_cast<i32>(
      std::round((event.deltaX * unit) / static_cast<double>(screen_.scale)))};
  i32 dy{static_cast<i32>(
      std::round((event.deltaY * unit) / static_cast<double>(screen_.scale)))};
  if (dx == 0 && event.deltaX != 0.0) {
    dx = event.deltaX < 0.0 ? -1 : 1;
  }
  if (dy == 0 && event.deltaY != 0.0) {
    dy = event.deltaY < 0.0 ? -1 : 1;
  }

  InputEvent input = {};
  input.kind = InputKind::kWheel;
  input.button = static_cast<u8>(event.mouse.button);
  input.buttons = static_cast<u8>(event.mouse.buttons & 0xFFU);
  input.mods = static_cast<u8>((event.mouse.ctrlKey ? kInputCtrl : 0U) |
                               (event.mouse.shiftKey ? kInputShift : 0U));
  input.x = point.x;
  input.y = point.y;
  input.dx = std::clamp(dx, -80, 80);
  input.dy = std::clamp(dy, -80, 80);
  (void)push_input_event(input);
  draw_dirty_ = true;
}

void Web::push_key_down_event(const EmscriptenKeyboardEvent &event) {
  Key key = Key::kNone;
  const b8 command{event.ctrlKey || event.metaKey};
  if (command && event.key[1] == '\0') {
    const char c = event.key[0] >= 'A' && event.key[0] <= 'Z'
                       ? static_cast<char>(event.key[0] + ('a' - 'A'))
                       : event.key[0];
    if (c == 'z') {
      key = event.shiftKey ? Key::kRedo : Key::kUndo;
    } else if (c == 'y') {
      key = Key::kRedo;
    }
  } else if (key_is(event, "Enter")) {
    key = Key::kEnter;
  } else if (key_is(event, "Escape")) {
    key = Key::kEscape;
  } else if (key_is(event, "Backspace")) {
    key = Key::kBackspace;
  } else if (key_is(event, "Delete")) {
    key = Key::kDelete;
  } else if (key_is(event, "Tab")) {
    key = Key::kTab;
  }
  if (key == Key::kNone) {
    const char c = event.key[0];
    if (event.key[1] == '\0' && c >= 32 && c <= 126 && !event.ctrlKey &&
        !event.metaKey) {
      InputEvent text = {};
      text.kind = InputKind::kText;
      text.dx = static_cast<i32>(static_cast<unsigned char>(c));
      text.mods = static_cast<u8>((event.ctrlKey ? kInputCtrl : 0U) |
                                  (event.shiftKey ? kInputShift : 0U));
      (void)push_input_event(text);
      draw_dirty_ = true;
    }
    return;
  }
  InputEvent input = {};
  input.kind = InputKind::kKeyDown;
  input.button = static_cast<u8>(key);
  input.mods = static_cast<u8>((command ? kInputCtrl : 0U) |
                               (event.shiftKey ? kInputShift : 0U));
  (void)push_input_event(input);
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
  const Screen next_screen =
      screen_for(static_cast<i32>(width_), static_cast<i32>(height_));
  if (next_screen.scale != screen_.scale ||
      next_screen.width != screen_.width ||
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
  wgpu::ShaderSourceWGSL wgsl = {};
  wgsl.code =
      wgpu::StringView(kDrawShaderSource, sizeof(kDrawShaderSource) - 1);

  wgpu::ShaderModuleDescriptor shader_descriptor = {};
  shader_descriptor.nextInChain = &wgsl;
  wgpu::ShaderModule shader = device_.CreateShaderModule(&shader_descriptor);
  if (shader == nullptr) {
    return false;
  }

  wgpu::BufferDescriptor uniform_buffer_descriptor = {};
  uniform_buffer_descriptor.size = sizeof(Frame);
  uniform_buffer_descriptor.usage =
      wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst;
  uniform_buffer_ = device_.CreateBuffer(&uniform_buffer_descriptor);
  if (uniform_buffer_ == nullptr) {
    return false;
  }
  wgpu::BufferDescriptor font_buffer_descriptor = {};
  font_buffer_descriptor.size = sizeof(Font);
  font_buffer_descriptor.usage =
      wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst;
  font_buffer_ = device_.CreateBuffer(&font_buffer_descriptor);
  if (font_buffer_ == nullptr) {
    return false;
  }
  const Font &text_font = font();
  queue_.WriteBuffer(font_buffer_, 0, &text_font, sizeof(Font));

  rect_buffer_ = make_vertex_buffer(device_, kRectBufferBytes);
  glyph_buffer_ = make_vertex_buffer(device_, kGlyphBufferBytes);
  icon_buffer_ = make_vertex_buffer(device_, kIconBufferBytes);
  preview_stamp_buffer_ = make_vertex_buffer(device_, kPreviewStampBufferBytes);
  stamp_buffer_ = make_vertex_buffer(device_, kStampBufferBytes);
  wgpu::BufferDescriptor layer_buffer_descriptor = {};
  layer_buffer_descriptor.size = kLayerBufferBytes;
  layer_buffer_descriptor.usage =
      wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst;
  layer_buffer_ = device_.CreateBuffer(&layer_buffer_descriptor);
  if (rect_buffer_ == nullptr || glyph_buffer_ == nullptr ||
      icon_buffer_ == nullptr || preview_stamp_buffer_ == nullptr ||
      stamp_buffer_ == nullptr || layer_buffer_ == nullptr) {
    return false;
  }
  if (!make_layer_texture()) {
    return false;
  }

  std::array<wgpu::BindGroupLayoutEntry, 2> bind_group_layout_entries = {};
  bind_group_layout_entries[0].binding = 0;
  bind_group_layout_entries[0].visibility =
      wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
  bind_group_layout_entries[0].buffer.type = wgpu::BufferBindingType::Uniform;
  bind_group_layout_entries[0].buffer.minBindingSize = sizeof(Frame);
  bind_group_layout_entries[1].binding = 1;
  bind_group_layout_entries[1].visibility =
      wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
  bind_group_layout_entries[1].buffer.type =
      wgpu::BufferBindingType::ReadOnlyStorage;
  bind_group_layout_entries[1].buffer.minBindingSize = sizeof(Font);

  wgpu::BindGroupLayoutDescriptor bind_group_layout_descriptor = {};
  bind_group_layout_descriptor.entryCount = bind_group_layout_entries.size();
  bind_group_layout_descriptor.entries = bind_group_layout_entries.data();
  bind_group_layout_ =
      device_.CreateBindGroupLayout(&bind_group_layout_descriptor);
  if (bind_group_layout_ == nullptr) {
    return false;
  }

  std::array<wgpu::BindGroupLayoutEntry, 3> layer_layout_entries = {};
  layer_layout_entries[0].binding = 0;
  layer_layout_entries[0].visibility = wgpu::ShaderStage::Fragment;
  layer_layout_entries[0].buffer.type = wgpu::BufferBindingType::Uniform;
  layer_layout_entries[0].buffer.minBindingSize = kLayerBufferBytes;
  layer_layout_entries[1].binding = 1;
  layer_layout_entries[1].visibility = wgpu::ShaderStage::Fragment;
  layer_layout_entries[1].texture.sampleType = wgpu::TextureSampleType::Float;
  layer_layout_entries[1].texture.viewDimension =
      wgpu::TextureViewDimension::e2DArray;
  layer_layout_entries[1].texture.multisampled = false;
  layer_layout_entries[2].binding = 2;
  layer_layout_entries[2].visibility = wgpu::ShaderStage::Fragment;
  layer_layout_entries[2].sampler.type = wgpu::SamplerBindingType::Filtering;

  wgpu::BindGroupLayoutDescriptor layer_layout_descriptor = {};
  layer_layout_descriptor.entryCount = layer_layout_entries.size();
  layer_layout_descriptor.entries = layer_layout_entries.data();
  layer_bind_group_layout_ =
      device_.CreateBindGroupLayout(&layer_layout_descriptor);
  if (layer_bind_group_layout_ == nullptr) {
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

  std::array<wgpu::BindGroupLayout, 2> composite_layouts = {
      bind_group_layout_, layer_bind_group_layout_};
  wgpu::PipelineLayoutDescriptor composite_layout_descriptor = {};
  composite_layout_descriptor.bindGroupLayoutCount = composite_layouts.size();
  composite_layout_descriptor.bindGroupLayouts = composite_layouts.data();
  wgpu::PipelineLayout composite_layout =
      device_.CreatePipelineLayout(&composite_layout_descriptor);
  if (composite_layout == nullptr) {
    return false;
  }

  std::array<wgpu::BindGroupEntry, 2> bind_group_entries = {};
  bind_group_entries[0].binding = 0;
  bind_group_entries[0].buffer = uniform_buffer_;
  bind_group_entries[0].offset = 0;
  bind_group_entries[0].size = sizeof(Frame);
  bind_group_entries[1].binding = 1;
  bind_group_entries[1].buffer = font_buffer_;
  bind_group_entries[1].offset = 0;
  bind_group_entries[1].size = sizeof(Font);

  wgpu::BindGroupDescriptor bind_group_descriptor = {};
  bind_group_descriptor.layout = bind_group_layout_;
  bind_group_descriptor.entryCount = bind_group_entries.size();
  bind_group_descriptor.entries = bind_group_entries.data();
  bind_group_ = device_.CreateBindGroup(&bind_group_descriptor);
  if (bind_group_ == nullptr) {
    return false;
  }

  if (!make_layer_bind_group()) {
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

  auto make_render_pipeline =
      [&](const char *vertex_entry,
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
    pipeline_descriptor.primitive.topology =
        wgpu::PrimitiveTopology::TriangleList;
    pipeline_descriptor.multisample.count = 1;
    pipeline_descriptor.fragment = &fragment;
    return device_.CreateRenderPipeline(&pipeline_descriptor);
  };

  rect_pipeline_ = make_render_pipeline("vs_rect", "fs_rect");
  glyph_pipeline_ = make_render_pipeline("vs_glyph", "fs_glyph");
  icon_pipeline_ = make_render_pipeline("vs_icon", "fs_icon");
  preview_stamp_pipeline_ =
      make_render_pipeline("vs_screen_stamp", "fs_screen_stamp");

  wgpu::ColorTargetState composite_color_target = {};
  composite_color_target.format = surface_format_;
  wgpu::FragmentState composite_fragment = {};
  composite_fragment.module = shader;
  composite_fragment.entryPoint = "fs_composite";
  composite_fragment.targetCount = 1;
  composite_fragment.targets = &composite_color_target;
  wgpu::RenderPipelineDescriptor composite_descriptor = {};
  composite_descriptor.layout = composite_layout;
  composite_descriptor.vertex.module = shader;
  composite_descriptor.vertex.entryPoint = "vs_composite";
  composite_descriptor.primitive.topology =
      wgpu::PrimitiveTopology::TriangleList;
  composite_descriptor.multisample.count = 1;
  composite_descriptor.fragment = &composite_fragment;
  composite_pipeline_ = device_.CreateRenderPipeline(&composite_descriptor);

  std::array<wgpu::VertexAttribute, 2> stamp_attributes = {};
  stamp_attributes[0].format = wgpu::VertexFormat::Float32x4;
  stamp_attributes[0].offset = 0;
  stamp_attributes[0].shaderLocation = 0;
  stamp_attributes[1].format = wgpu::VertexFormat::Float32x4;
  stamp_attributes[1].offset = sizeof(f32) * 4U;
  stamp_attributes[1].shaderLocation = 1;
  wgpu::VertexBufferLayout stamp_buffer = {};
  stamp_buffer.arrayStride = sizeof(PaintStamp);
  stamp_buffer.stepMode = wgpu::VertexStepMode::Instance;
  stamp_buffer.attributeCount = stamp_attributes.size();
  stamp_buffer.attributes = stamp_attributes.data();
  wgpu::ColorTargetState stamp_color_target = {};
  stamp_color_target.format = wgpu::TextureFormat::R8Unorm;
  wgpu::FragmentState stamp_fragment = {};
  stamp_fragment.module = shader;
  stamp_fragment.entryPoint = "fs_stamp";
  stamp_fragment.targetCount = 1;
  stamp_fragment.targets = &stamp_color_target;
  wgpu::RenderPipelineDescriptor stamp_descriptor = {};
  stamp_descriptor.layout = pipeline_layout;
  stamp_descriptor.vertex.module = shader;
  stamp_descriptor.vertex.entryPoint = "vs_stamp";
  stamp_descriptor.vertex.bufferCount = 1;
  stamp_descriptor.vertex.buffers = &stamp_buffer;
  stamp_descriptor.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
  stamp_descriptor.multisample.count = 1;
  stamp_descriptor.fragment = &stamp_fragment;
  stamp_pipeline_ = device_.CreateRenderPipeline(&stamp_descriptor);

  return rect_pipeline_ != nullptr && glyph_pipeline_ != nullptr &&
         icon_pipeline_ != nullptr && preview_stamp_pipeline_ != nullptr &&
         composite_pipeline_ != nullptr && stamp_pipeline_ != nullptr;
}

auto Web::make_layer_texture() -> b8 {
  const u32 document_width{static_cast<u32>(std::max(1, gui_.document.width))};
  const u32 document_height{
      static_cast<u32>(std::max(1, gui_.document.height))};

  wgpu::TextureBindingViewDimension binding_dimension = {};
  binding_dimension.textureBindingViewDimension =
      wgpu::TextureViewDimension::e2DArray;
  wgpu::TextureDescriptor texture_descriptor = {};
  texture_descriptor.nextInChain = &binding_dimension;
  texture_descriptor.size = {document_width, document_height,
                             static_cast<u32>(kMaxLayers)};
  texture_descriptor.dimension = wgpu::TextureDimension::e2D;
  texture_descriptor.format = wgpu::TextureFormat::R8Unorm;
  texture_descriptor.usage = wgpu::TextureUsage::RenderAttachment |
                             wgpu::TextureUsage::TextureBinding |
                             wgpu::TextureUsage::CopyDst |
                             wgpu::TextureUsage::CopySrc;
  layer_texture_ = device_.CreateTexture(&texture_descriptor);
  if (layer_texture_ == nullptr) {
    return false;
  }

  wgpu::TextureViewDescriptor array_view = {};
  array_view.format = wgpu::TextureFormat::R8Unorm;
  array_view.dimension = wgpu::TextureViewDimension::e2DArray;
  array_view.baseArrayLayer = 0;
  array_view.arrayLayerCount = static_cast<u32>(kMaxLayers);
  layer_texture_view_ = layer_texture_.CreateView(&array_view);
  if (layer_texture_view_ == nullptr) {
    return false;
  }

  for (usize index{0}; index < layer_slice_views_.size(); ++index) {
    wgpu::TextureViewDescriptor slice_view = {};
    slice_view.format = wgpu::TextureFormat::R8Unorm;
    slice_view.dimension = wgpu::TextureViewDimension::e2D;
    slice_view.baseArrayLayer = static_cast<u32>(index);
    slice_view.arrayLayerCount = 1;
    layer_slice_views_[index] = layer_texture_.CreateView(&slice_view);
    if (layer_slice_views_[index] == nullptr) {
      return false;
    }
  }

  wgpu::SamplerDescriptor sampler_descriptor = {};
  sampler_descriptor.minFilter = wgpu::FilterMode::Nearest;
  sampler_descriptor.magFilter = wgpu::FilterMode::Nearest;
  sampler_descriptor.mipmapFilter = wgpu::MipmapFilterMode::Nearest;
  sampler_descriptor.addressModeU = wgpu::AddressMode::ClampToEdge;
  sampler_descriptor.addressModeV = wgpu::AddressMode::ClampToEdge;
  sampler_descriptor.addressModeW = wgpu::AddressMode::ClampToEdge;
  layer_sampler_ = device_.CreateSampler(&sampler_descriptor);
  if (layer_sampler_ == nullptr) {
    return false;
  }

  wgpu::CommandEncoder encoder = device_.CreateCommandEncoder();
  if (encoder == nullptr) {
    return false;
  }
  for (const wgpu::TextureView &view : layer_slice_views_) {
    wgpu::RenderPassColorAttachment attachment = {};
    attachment.view = view;
    attachment.loadOp = wgpu::LoadOp::Clear;
    attachment.storeOp = wgpu::StoreOp::Store;
    attachment.clearValue = wgpu::Color{0.0, 0.0, 0.0, 1.0};
    wgpu::RenderPassDescriptor pass_descriptor = {};
    pass_descriptor.colorAttachmentCount = 1;
    pass_descriptor.colorAttachments = &attachment;
    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&pass_descriptor);
    if (pass == nullptr) {
      return false;
    }
    pass.End();
  }
  wgpu::CommandBuffer commands = encoder.Finish();
  if (commands == nullptr) {
    return false;
  }
  queue_.Submit(1, &commands);
  return true;
}

auto Web::make_layer_bind_group() -> b8 {
  if (layer_bind_group_layout_ == nullptr || layer_buffer_ == nullptr ||
      layer_texture_view_ == nullptr || layer_sampler_ == nullptr) {
    return false;
  }

  std::array<wgpu::BindGroupEntry, 3> layer_bind_group_entries = {};
  layer_bind_group_entries[0].binding = 0;
  layer_bind_group_entries[0].buffer = layer_buffer_;
  layer_bind_group_entries[0].offset = 0;
  layer_bind_group_entries[0].size = kLayerBufferBytes;
  layer_bind_group_entries[1].binding = 1;
  layer_bind_group_entries[1].textureView = layer_texture_view_;
  layer_bind_group_entries[2].binding = 2;
  layer_bind_group_entries[2].sampler = layer_sampler_;

  wgpu::BindGroupDescriptor layer_bind_group_descriptor = {};
  layer_bind_group_descriptor.layout = layer_bind_group_layout_;
  layer_bind_group_descriptor.entryCount = layer_bind_group_entries.size();
  layer_bind_group_descriptor.entries = layer_bind_group_entries.data();
  layer_bind_group_ = device_.CreateBindGroup(&layer_bind_group_descriptor);
  return layer_bind_group_ != nullptr;
}

auto Web::upload_draws() -> b8 {
  if (!draw_dirty_) {
    return true;
  }

  guiframe(&gui_, screen_, input_events_.span(), &draws_);
  input_events_.clear();
  if (gui_.recreate_layers) {
    if (!make_layer_texture() || !make_layer_bind_group()) {
      return false;
    }
    gui_.recreate_layers = false;
    gui_.clear_slots.clear();
  }
  for (const GuiAction action : gui_.actions.span()) {
    switch (action.kind) {
    case GuiActionKind::kOpenImagePicker:
      open_image_picker();
      break;
    case GuiActionKind::kExportPng:
      pending_export_ = true;
      break;
    }
  }

  const Frame uniforms = {
      .width = static_cast<f32>(width_),
      .height = static_cast<f32>(height_),
      .screen_width = static_cast<f32>(screen_.width),
      .screen_height = static_cast<f32>(screen_.height),
      .rect_count = static_cast<u32>(draws_.rects.size()),
      .glyph_count = static_cast<u32>(draws_.glyphs.size()),
      .icon_count = static_cast<u32>(draws_.icons.size()),
      .layer_count = static_cast<u32>(gui_.layers.size()),
      .document_x = gui_.layout.document.x,
      .document_y = gui_.layout.document.y,
      .document_x1 = gui_.layout.document.x + gui_.layout.document.width,
      .document_y1 = gui_.layout.document.y + gui_.layout.document.height,
      .viewport_x = gui_.layout.viewport.x,
      .viewport_y = gui_.layout.viewport.y,
      .viewport_x1 = gui_.layout.viewport.x + gui_.layout.viewport.width,
      .viewport_y1 = gui_.layout.viewport.y + gui_.layout.viewport.height,
      .document_width = static_cast<f32>(gui_.document.width),
      .document_height = static_cast<f32>(gui_.document.height),
      ._pad0 = 0.0F,
      ._pad1 = 0.0F,
  };
  queue_.WriteBuffer(uniform_buffer_, 0, &uniforms, sizeof(uniforms));
  std::array<LayerGpu, kMaxLayers> layer_rows = {};
  for (usize index{0}; index < gui_.layers.size() && index < layer_rows.size();
       ++index) {
    const Layer &layer = gui_.layers[index];
    layer_rows[index] = {
        .flags = layer_flags_for_gpu(layer),
        .opacity = static_cast<f32>(layer.opacity_u8) / 255.0F,
        .layer_slot = static_cast<f32>(layer.layer_slot),
        .kind = static_cast<f32>(static_cast<u8>(layer.kind)),
    };
  }
  queue_.WriteBuffer(layer_buffer_, 0, layer_rows.data(), sizeof(layer_rows));
  if (!draws_.rects.empty()) {
    queue_.WriteBuffer(rect_buffer_, 0, draws_.rects.data(),
                       draws_.rects.byte_size());
  }
  if (!draws_.glyphs.empty()) {
    queue_.WriteBuffer(glyph_buffer_, 0, draws_.glyphs.data(),
                       draws_.glyphs.byte_size());
  }
  if (!draws_.icons.empty()) {
    queue_.WriteBuffer(icon_buffer_, 0, draws_.icons.data(),
                       draws_.icons.byte_size());
  }
  if (!draws_.preview_stamps.empty()) {
    queue_.WriteBuffer(preview_stamp_buffer_, 0, draws_.preview_stamps.data(),
                       draws_.preview_stamps.byte_size());
  }
  if (!gui_.paint_delta.empty()) {
    queue_.WriteBuffer(stamp_buffer_, 0, gui_.paint_delta.data(),
                       gui_.paint_delta.byte_size());
  }
  return true;
}

void Web::encode_layer_clears(wgpu::CommandEncoder encoder) {
  for (const u8 slot : gui_.clear_slots.span()) {
    if (slot >= layer_slice_views_.size() ||
        layer_slice_views_[slot] == nullptr) {
      continue;
    }
    wgpu::RenderPassColorAttachment attachment = {};
    attachment.view = layer_slice_views_[slot];
    attachment.loadOp = wgpu::LoadOp::Clear;
    attachment.storeOp = wgpu::StoreOp::Store;
    attachment.clearValue = wgpu::Color{0.0, 0.0, 0.0, 1.0};
    wgpu::RenderPassDescriptor descriptor = {};
    descriptor.colorAttachmentCount = 1;
    descriptor.colorAttachments = &attachment;
    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&descriptor);
    if (pass != nullptr) {
      pass.End();
    }
  }
}

void Web::encode_paint(wgpu::CommandEncoder encoder) {
  if (gui_.paint_delta.empty() || stamp_pipeline_ == nullptr ||
      stamp_buffer_ == nullptr) {
    return;
  }

  usize first{0};
  while (first < gui_.paint_delta.size()) {
    const u32 layer{
        static_cast<u32>(gui_.paint_delta[first].layer_slot + 0.5F)};
    usize count{1};
    while (first + count < gui_.paint_delta.size() &&
           static_cast<u32>(gui_.paint_delta[first + count].layer_slot +
                            0.5F) == layer) {
      ++count;
    }
    if (layer < layer_slice_views_.size() &&
        layer_slice_views_[layer] != nullptr) {
      wgpu::RenderPassColorAttachment attachment = {};
      attachment.view = layer_slice_views_[layer];
      attachment.loadOp = wgpu::LoadOp::Load;
      attachment.storeOp = wgpu::StoreOp::Store;
      wgpu::RenderPassDescriptor descriptor = {};
      descriptor.colorAttachmentCount = 1;
      descriptor.colorAttachments = &attachment;
      wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&descriptor);
      if (pass != nullptr) {
        pass.SetBindGroup(0, bind_group_);
        pass.SetPipeline(stamp_pipeline_);
        pass.SetVertexBuffer(0, stamp_buffer_, 0, gui_.paint_delta.byte_size());
        pass.Draw(6, static_cast<u32>(count), 0, static_cast<u32>(first));
        pass.End();
      }
    }
    first += count;
  }
}

void Web::encode_gui(wgpu::RenderPassEncoder pass) {
  pass.SetBindGroup(0, bind_group_);
  pass.SetBindGroup(1, layer_bind_group_);
  pass.SetPipeline(composite_pipeline_);
  pass.Draw(6, 1);

  pass.SetBindGroup(0, bind_group_);
  for (usize plane{0}; plane < draws_.plane_count(); ++plane) {
    const DrawPlane draw_plane = static_cast<DrawPlane>(plane);
    const DrawPlaneStart begin = draws_.plane_begin(draw_plane);
    const DrawPlaneStart end = draws_.plane_end(draw_plane);
    draw_rows(pass, rect_pipeline_, rect_buffer_, begin.rect,
              end.rect - begin.rect, sizeof(RectDraw));
    draw_rows(pass, glyph_pipeline_, glyph_buffer_, begin.glyph,
              end.glyph - begin.glyph, sizeof(GlyphDraw));
    draw_rows(pass, icon_pipeline_, icon_buffer_, begin.icon,
              end.icon - begin.icon, sizeof(IconDraw));
    draw_rows(pass, preview_stamp_pipeline_, preview_stamp_buffer_,
              begin.preview_stamp, end.preview_stamp - begin.preview_stamp,
              sizeof(PaintStamp));
  }
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
