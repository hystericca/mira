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
constexpr usize kMaxSizes = 8;
constexpr usize kMaxPaintStamps = 4096;
constexpr usize kMaxInputEvents = 128;
constexpr usize kMaxHitRecords = 128;
constexpr usize kLayerNameBytes = 16;
constexpr usize kToolNameBytes = 8;
constexpr u8 kNoLayer = 0xFFU;
constexpr u8 kNoMenu = 0xFFU;
constexpr u8 kCanvasTextureSlot = static_cast<u8>(kMaxLayers - 1U);

constexpr u8 kLayerVisible = 1U << 0U;
constexpr u8 kLayerLocked = 1U << 1U;
constexpr u8 kLayerSelected = 1U << 2U;
constexpr u8 kLayerBottom = 1U << 3U;

enum class LayerKind : u8 {
    kInk,
    kReference,
    kPaper,
};

struct Layer {
    u32 id = 0;
    std::array<char, kLayerNameBytes> name = {};
    u8 flags = kLayerVisible;
    u8 opacity_u8 = 255;
    u8 texture_slot = 0;
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

struct Size {
    u8 index = 0;
    u8 selected = 0;
    u16 _pad = 0;
};
static_assert(sizeof(Size) == 4);

struct PaintStamp {
    f32 x = 0.0F;
    f32 y = 0.0F;
    f32 size = 0.0F;
    f32 tone = 0.0F;
    f32 layer = 0.0F;
    f32 _pad0 = 0.0F;
    f32 _pad1 = 0.0F;
    f32 _pad2 = 0.0F;
};
static_assert(sizeof(PaintStamp) == 32);

enum class InputKind : u8 {
    kMouseMove,
    kMouseDown,
    kMouseUp,
    kWheel,
    kKeyDown,
    kText,
};

constexpr u8 kInputCtrl = 1U << 0U;
constexpr u8 kInputShift = 1U << 1U;

enum class Key : u8 {
    kNone,
    kEnter,
    kEscape,
    kBackspace,
    kDelete,
};

struct InputEvent {
    InputKind kind = InputKind::kMouseMove;
    u8 button = 0;
    u8 buttons = 0;
    u8 mods = 0;
    i32 x = 0;
    i32 y = 0;
    i32 dx = 0;
    i32 dy = 0;
};
static_assert(sizeof(InputEvent) == 20);

enum class HitKind : u8 {
    kNone,
    kViewport,
    kToolbar,
    kSidebar,
    kMenu,
    kTool,
    kSize,
    kLayerRow,
    kLayerVisibility,
    kLayerLock,
    kLayerOpacity,
    kMenuAction,
};

enum class MenuAction : u8 {
    kLayerNew,
    kLayerDelete,
    kLayerRename,
};

struct HitRecord {
    Rect rect;
    HitKind kind = HitKind::kNone;
    u8 index = 0;
    u16 priority = 0;
    u32 _pad = 0;
};
static_assert(sizeof(HitRecord) == 24);

struct Document {
    // The drawable page size, measured in document pixels.
    i32 width = 320;
    i32 height = 240;
};
static_assert(sizeof(Document) == 8);

struct View {
    // Document coordinate at the viewport origin, plus the current scale.
    f32 x = 0.0F;
    f32 y = 0.0F;
    f32 zoom = 1.0F;
};
static_assert(sizeof(View) == 12);

struct GuiLayout {
    // Window is the browser surface. Viewport is the window onto the document.
    Rect window;
    Rect menu_bar;
    Rect toolbar;
    Rect tools;
    Rect sizes;
    Rect viewport;
    Rect document;
    Rect layers;
    Rect layerrows;
};

struct GuiState {
    Table<Layer, kMaxLayers> layers;
    Table<Tool, kMaxTools> tools;
    Table<Size, kMaxSizes> sizes;
    Table<PaintStamp, kMaxPaintStamps> paint_stamps;
    Table<u8, kMaxLayers> clear_slots;
    Table<HitRecord, kMaxHitRecords> hits;
    Document document;
    View view;
    GuiLayout layout;
    i32 mouse_x = -1;
    i32 mouse_y = -1;
    HitKind hot_kind = HitKind::kNone;
    u8 hot_index = 0;
    HitKind active_kind = HitKind::kNone;
    u8 active_index = 0;
    u8 curlayer = 0;
    u8 curtool = 0;
    u8 cursize = 3;
    u8 active_menu = kNoMenu;
    u8 renaming_layer = kNoLayer;
    u32 next_layer_id = 1;
    f32 last_paint_x = 0.0F;
    f32 last_paint_y = 0.0F;
    i16 last_pan_x = 0;
    i16 last_pan_y = 0;
    b8 initialized = false;
    b8 view_initialized = false;
    b8 painting = false;
    b8 panning = false;
    b8 setting_opacity = false;
    b8 rename_replace = false;
};

[[nodiscard]] std::string_view layername(const Layer &layer);
[[nodiscard]] std::string_view toolname(const Tool &tool);
[[nodiscard]] b8 layervisible(const Layer &layer);
[[nodiscard]] b8 layerlocked(const Layer &layer);
[[nodiscard]] b8 layerselected(const Layer &layer);
[[nodiscard]] const Size *sizecur(const GuiState &state);
[[nodiscard]] Icon sizeicon(u8 index);
[[nodiscard]] Icon brushicon(u8 index);
void guiinit(GuiState *state);
void guilayout(GuiState *state, Screen screen);
void guievent(GuiState *state, std::span<const InputEvent> input);
void guidraw(const GuiState &state, DrawList *draws);
void guiframe(GuiState *state, Screen screen, std::span<const InputEvent> input,
                     DrawList *draws);
[[nodiscard]] b8 layeradd(GuiState *state, std::string_view name);
[[nodiscard]] b8 layerdel(GuiState *state);
[[nodiscard]] b8 layerrename(GuiState *state, u8 index, std::string_view name);
[[nodiscard]] b8 layeredit(GuiState *state);
[[nodiscard]] HitRecord guihit(const GuiState &state, i32 x, i32 y);
[[nodiscard]] const Layer *layercur(const GuiState &state);
[[nodiscard]] const Tool *toolcur(const GuiState &state);

} // namespace mira
