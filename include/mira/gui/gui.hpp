#pragma once

#include <array>
#include <span>
#include <string_view>

#include "mira/base/table.hpp"
#include "mira/draw/draw.hpp"
#include "mira/gui/tool.hpp"
#include "mira/types.hpp"

namespace mira {

constexpr usize kMaxLayers = 16;
constexpr usize kMaxTools = 8;
constexpr usize kMaxTips = 8;
constexpr usize kMaxSizes = 8;
constexpr usize kMaxCoverages = 16;
constexpr usize kMaxPaintDelta = 32768;
constexpr usize kMaxDraftStamps = 32768;
constexpr usize kMaxStrokeActions = 1024;
constexpr usize kMaxHistoryStamps = 32768;
constexpr usize kMaxInputEvents = 128;
constexpr usize kMaxHitRecords = 128;
constexpr usize kMaxGuiActions = 16;
constexpr usize kLayerNameBytes = 16;
constexpr usize kToolNameBytes = 8;
constexpr u8 kNoLayer = 0xFFU;
constexpr u8 kNoMenu = 0xFFU;
constexpr u8 kBackgroundTextureSlot = static_cast<u8>(kMaxLayers - 1U);
constexpr i32 kDefaultDocumentWidth = 1360;
constexpr i32 kDefaultDocumentHeight = 736;
constexpr f32 kInitialViewZoom = 0.5F;

constexpr u8 kLayerVisible = 1U << 0U;
constexpr u8 kLayerLocked = 1U << 1U;
constexpr u8 kLayerSelected = 1U << 2U;
constexpr u8 kLayerBottom = 1U << 3U;

enum class LayerKind : u8 {
    kInk,
    kImage,
    kBackground,
};

struct Layer {
    u32 id = 0;
    std::array<char, kLayerNameBytes> name = {};
    u8 flags = kLayerVisible;
    u8 opacity_u8 = 255;
    u8 layer_slot = 0;
    LayerKind kind = LayerKind::kInk;
};
static_assert(sizeof(Layer) == 24);

struct Tool {
    u32 id = 0;
    std::array<char, kToolNameBytes> name = {};
    u8 selected = 0;
    ToolKind kind = ToolKind::kPen;
    u16 _pad = 0;
};
static_assert(sizeof(Tool) == 16);

struct Tip {
    u8 index = 0;
    u8 selected = 0;
    u16 _pad = 0;
};
static_assert(sizeof(Tip) == 4);

struct Size {
    u8 index = 0;
    u8 selected = 0;
    u16 _pad = 0;
};
static_assert(sizeof(Size) == 4);

struct Coverage {
    u8 index = 0;
    u8 selected = 0;
    u16 _pad = 0;
};
static_assert(sizeof(Coverage) == 4);

struct StrokeAction {
    u32 layer_id = 0;
    u32 first_stamp = 0;
    u32 stamp_count = 0;
    Rect affected = {};
};
static_assert(sizeof(StrokeAction) == 28);

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
    kTab,
    kUndo,
    kRedo,
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
    kTip,
    kSize,
    kBrushButton,
    kBrushPanel,
    kBrushTitle,
    kBrushClose,
    kCoverage,
    kLayerRow,
    kLayerVisibility,
    kLayerLock,
    kLayerOpacity,
    kMenuAction,
    kContextMenu,
    kContextAction,
    kDialog,
    kDialogField,
    kDialogButton,
};

enum class MenuAction : u8 {
    kNone,
    kMiraAbout,
    kUndo,
    kRedo,
    kFileNew,
    kFileImport,
    kFileExport,
    kLayerNew,
    kLayerDelete,
    kLayerRename,
};

enum class ContextKind : u8 {
    kNone,
    kApp,
    kWorkspace,
    kLayer,
};

enum class GuiActionKind : u8 {
    kOpenImagePicker,
    kExportPng,
};

struct GuiAction {
    GuiActionKind kind = GuiActionKind::kOpenImagePicker;
    u8 _pad0 = 0;
    u16 _pad1 = 0;
};
static_assert(sizeof(GuiAction) == 4);

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
    i32 width = kDefaultDocumentWidth;
    i32 height = kDefaultDocumentHeight;
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
    Rect tips;
    Rect brush_button;
    Rect brush_panel;
    Rect coverages;
    Rect viewport;
    Rect document;
    Rect layers;
    Rect layerrows;
    Rect context;
    Rect dialog;
    Rect dialog_logo;
    Rect dialog_width;
    Rect dialog_height;
    Rect dialog_ok;
    Rect dialog_cancel;
};

struct GuiDocumentState {
    Table<Layer, kMaxLayers> layers;
    Document document;
    u32 next_layer_id = 1;
    u8 curlayer = 0;
    b8 recreate_layers = false;
};

struct GuiToolState {
    Table<Tool, kMaxTools> tools;
    Table<Tip, kMaxTips> tips;
    Table<Size, kMaxSizes> sizes;
    Table<Coverage, kMaxCoverages> coverages;
    u8 curtool = 0;
    u8 curtip = 3;
    u8 cursize = 3;
    u8 curcoverage = 0;
};

struct GuiHistoryState {
    Table<StrokeAction, kMaxStrokeActions> strokes;
    Table<PaintStamp, kMaxHistoryStamps> history_stamps;
    u32 stroke_cursor = 0;
    u32 active_paint_first = 0;
    u32 active_stroke_first = 0;
    u32 active_stroke_count = 0;
    u32 active_stroke_layer = 0;
    Rect active_stroke_rect = {};
    b8 recording_stroke = false;
    b8 replay_strokes = false;
};

struct GuiFrameState {
    Table<PaintStamp, kMaxPaintDelta> paint_delta;
    Table<PaintStamp, kMaxDraftStamps> draft_stamps;
    Table<u8, kMaxLayers> clear_slots;
    Table<HitRecord, kMaxHitRecords> hits;
    Table<GuiAction, kMaxGuiActions> actions;
    GuiLayout layout;
    i32 mouse_x = -1;
    i32 mouse_y = -1;
    HitKind hot_kind = HitKind::kNone;
    u8 hot_index = 0;
    HitKind active_kind = HitKind::kNone;
    u8 active_index = 0;
};

struct GuiSessionState {
    View view;
    i32 new_width = kDefaultDocumentWidth;
    i32 new_height = kDefaultDocumentHeight;
    u8 new_field = 0;
    u8 active_menu = kNoMenu;
    u8 context_target = kNoLayer;
    u8 renaming_layer = kNoLayer;
    i32 context_x = 0;
    i32 context_y = 0;
    f32 draft_start_x = 0.0F;
    f32 draft_start_y = 0.0F;
    f32 draft_x = 0.0F;
    f32 draft_y = 0.0F;
    f32 last_paint_x = 0.0F;
    f32 last_paint_y = 0.0F;
    f32 brush_t = 0.0F;
    f32 brush_x = 0.0F;
    f32 brush_y = 0.0F;
    i16 last_pan_x = 0;
    i16 last_pan_y = 0;
    i16 brush_drag_x = 0;
    i16 brush_drag_y = 0;
    b8 initialized = false;
    b8 view_initialized = false;
    b8 painting = false;
    b8 panning = false;
    b8 setting_opacity = false;
    b8 rename_replace = false;
    b8 context_open = false;
    b8 brush_open = false;
    b8 brush_placed = false;
    b8 moving_brush = false;
    b8 about_dialog = false;
    b8 new_dialog = false;
    b8 new_replace = false;
    b8 draft_active = false;
    ToolKind draft_kind = ToolKind::kPen;
    u8 draft_layer_slot = 0;
};

struct GuiState : GuiDocumentState,
                  GuiToolState,
                  GuiHistoryState,
                  GuiFrameState,
                  GuiSessionState {
};

[[nodiscard]] std::string_view layername(const Layer &layer);
[[nodiscard]] std::string_view toolname(const Tool &tool);
[[nodiscard]] b8 layervisible(const Layer &layer);
[[nodiscard]] b8 layerlocked(const Layer &layer);
[[nodiscard]] b8 layerselected(const Layer &layer);
[[nodiscard]] const Tip *tipcur(const GuiState &state);
[[nodiscard]] const Size *sizecur(const GuiState &state);
[[nodiscard]] const Coverage *coveragecur(const GuiState &state);
[[nodiscard]] Icon tipicon(u8 index);
[[nodiscard]] Icon sizeicon(u8 index);
[[nodiscard]] Icon coverageicon(u8 index);
void guiinit(GuiState *state);
void docnew(GuiState *state, i32 width = kDefaultDocumentWidth,
            i32 height = kDefaultDocumentHeight);
void guilayout(GuiState *state, Screen screen);
void guievent(GuiState *state, std::span<const InputEvent> input);
void guidraw(const GuiState &state, DrawList *draws);
void guiframe(GuiState *state, Screen screen, std::span<const InputEvent> input, DrawList *draws);
[[nodiscard]] b8 guianimating(const GuiState &state);
[[nodiscard]] b8 layeradd(GuiState *state, std::string_view name);
[[nodiscard]] u8 layerimage(GuiState *state, std::string_view name, u8 opacity = 255);
[[nodiscard]] b8 layerdel(GuiState *state);
[[nodiscard]] b8 layerrename(GuiState *state, u8 index, std::string_view name);
[[nodiscard]] b8 layeredit(GuiState *state);
[[nodiscard]] HitRecord guihit(const GuiState &state, i32 x, i32 y);
[[nodiscard]] MenuAction menuaction(const GuiState &state, HitRecord hit);
[[nodiscard]] ContextKind contextkind(const GuiState &state);
[[nodiscard]] const Layer *layercur(const GuiState &state);
[[nodiscard]] const Tool *toolcur(const GuiState &state);

} // namespace mira
