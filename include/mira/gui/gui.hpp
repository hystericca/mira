#pragma once

#include <array>
#include <span>
#include <string_view>

#include "mira/base/table.hpp"
#include "mira/draw/draw.hpp"
#include "mira/types.hpp"

namespace mira {

constexpr usize kMaxLayers = 16;
constexpr usize kMaxTools = 8;
constexpr usize kMaxInputEvents = 128;
constexpr usize kMaxHitRecords = 128;
constexpr usize kLayerNameBytes = 16;
constexpr usize kToolNameBytes = 8;

enum class LayerKind : u8 {
    kInk,
    kReference,
    kPaper,
};

struct Layer {
    u32 id = 0;
    std::array<char, kLayerNameBytes> name = {};
    u8 visible = 1;
    u8 opacity_u8 = 255;
    u8 selected = 0;
    LayerKind kind = LayerKind::kInk;
};
static_assert(sizeof(Layer) == 24);

enum class ToolKind : u8 {
    kPen,
    kBrush,
    kLine,
    kMagic,
    kRect,
    kZoom,
    kErase,
};

struct Tool {
    u32 id = 0;
    std::array<char, kToolNameBytes> name = {};
    u8 selected = 0;
    ToolKind kind = ToolKind::kPen;
    u16 _pad = 0;
};
static_assert(sizeof(Tool) == 16);

enum class InputKind : u8 {
    kMouseMove,
    kMouseDown,
    kMouseUp,
};

struct InputEvent {
    InputKind kind = InputKind::kMouseMove;
    u8 button = 0;
    u16 _pad = 0;
    i32 x = 0;
    i32 y = 0;
};
static_assert(sizeof(InputEvent) == 12);

enum class HitKind : u8 {
    kNone,
    kCanvas,
    kToolbar,
    kSidebar,
    kMenu,
    kTool,
    kLayerRow,
    kLayerVisibility,
};

struct HitRecord {
    Rect rect;
    HitKind kind = HitKind::kNone;
    u8 index = 0;
    u16 priority = 0;
    u32 _pad = 0;
};
static_assert(sizeof(HitRecord) == 24);

struct GuiLayout {
    Rect screen;
    Rect menu_bar;
    Rect toolbar;
    Rect tool_list;
    Rect canvas;
    Rect work_area;
    Rect sidebar;
    Rect layer_list;
};

struct GuiState {
    Table<Layer, kMaxLayers> layers;
    Table<Tool, kMaxTools> tools;
    Table<HitRecord, kMaxHitRecords> hits;
    GuiLayout layout;
    i32 mouse_x = -1;
    i32 mouse_y = -1;
    HitKind hot_kind = HitKind::kNone;
    u8 hot_index = 0;
    HitKind active_kind = HitKind::kNone;
    u8 active_index = 0;
    u8 selected_layer = 0;
    u8 selected_tool = 0;
    b8 initialized = false;
};

[[nodiscard]] std::string_view layer_name(const Layer &layer);
[[nodiscard]] std::string_view tool_name(const Tool &tool);
void init_gui(GuiState *state);
void layout_gui(GuiState *state, Screen screen);
void reduce_gui(GuiState *state, std::span<const InputEvent> input);
void emit_gui(const GuiState &state, DrawList *draws);
void build_gui_frame(GuiState *state, Screen screen, std::span<const InputEvent> input,
                     DrawList *draws);
[[nodiscard]] HitRecord hit_at(const GuiState &state, i32 x, i32 y);
[[nodiscard]] const Layer *selected_layer(const GuiState &state);
[[nodiscard]] const Tool *selected_tool(const GuiState &state);

} // namespace mira
