#pragma once

#include "mira/gui/gui.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <string_view>

namespace mira::gui {

struct MenuItem {
    std::string_view text;
    u8 index;
};

struct MenuCommand {
    std::string_view text;
    MenuAction action;
};

inline constexpr std::array<MenuItem, 5> kMenuItems = {{
    {"mira", 0},
    {"file", 1},
    {"edit", 2},
    {"layer", 3},
    {"view", 4},
}};

inline constexpr u8 kMiraMenu = 0;
inline constexpr u8 kFileMenu = 1;
inline constexpr u8 kEditMenu = 2;
inline constexpr u8 kLayerMenu = 3;

inline constexpr std::array<MenuCommand, 1> kMiraMenuCommands = {{
    {"about", MenuAction::kMiraAbout},
}};

inline constexpr std::array<MenuCommand, 3> kAppContextCommands = {{
    {"new", MenuAction::kFileNew},
    {"import", MenuAction::kFileImport},
    {"export", MenuAction::kFileExport},
}};

inline constexpr std::array<MenuCommand, 3> kWorkspaceContextCommands = {{
    {"undo", MenuAction::kUndo},
    {"redo", MenuAction::kRedo},
    {"export", MenuAction::kFileExport},
}};

inline constexpr std::array<MenuCommand, 3> kLayerContextCommands = {{
    {"new", MenuAction::kLayerNew},
    {"del", MenuAction::kLayerDelete},
    {"name", MenuAction::kLayerRename},
}};

inline constexpr std::array<MenuCommand, 3> kFileMenuCommands = {{
    {"new", MenuAction::kFileNew},
    {"import", MenuAction::kFileImport},
    {"export", MenuAction::kFileExport},
}};

inline constexpr std::array<MenuCommand, 2> kEditMenuCommands = {{
    {"undo", MenuAction::kUndo},
    {"redo", MenuAction::kRedo},
}};

inline constexpr std::array<MenuCommand, 3> kLayerMenuCommands = {{
    {"new", MenuAction::kLayerNew},
    {"del", MenuAction::kLayerDelete},
    {"name", MenuAction::kLayerRename},
}};

inline constexpr std::array<std::string_view, kToolDefs.size()> kToolNames = {{
    "pen",
    "brush",
    "line",
    "magic",
    "rect",
    "zoom",
    "erase",
}};
static_assert(kToolDefs.size() <= kMaxTools);
static_assert(kToolNames.size() == kToolDefs.size());

[[nodiscard]] inline auto menucommands(u8 menu) -> std::span<const MenuCommand> {
    if (menu == kMiraMenu) {
        return std::span<const MenuCommand>(kMiraMenuCommands);
    }
    if (menu == kFileMenu) {
        return std::span<const MenuCommand>(kFileMenuCommands);
    }
    if (menu == kEditMenu) {
        return std::span<const MenuCommand>(kEditMenuCommands);
    }
    if (menu == kLayerMenu) {
        return std::span<const MenuCommand>(kLayerMenuCommands);
    }
    return {};
}

[[nodiscard]] inline auto contextcommands(ContextKind kind) -> std::span<const MenuCommand> {
    if (kind == ContextKind::kWorkspace) {
        return std::span<const MenuCommand>(kWorkspaceContextCommands);
    }
    if (kind == ContextKind::kLayer) {
        return std::span<const MenuCommand>(kLayerContextCommands);
    }
    if (kind == ContextKind::kApp) {
        return std::span<const MenuCommand>(kAppContextCommands);
    }
    return {};
}

[[nodiscard]] inline auto contains(Rect rect, i32 x, i32 y) -> b8 {
    const f32 px = static_cast<f32>(x);
    const f32 py = static_cast<f32>(y);
    return px >= rect.x && py >= rect.y && px < rect.x + rect.width && py < rect.y + rect.height;
}

[[nodiscard]] inline auto containsrect(Rect outer, Rect inner) -> b8 {
    return inner.x >= outer.x && inner.y >= outer.y &&
           inner.x + inner.width <= outer.x + outer.width &&
           inner.y + inner.height <= outer.y + outer.height;
}

[[nodiscard]] inline auto intersectrect(Rect a, Rect b) -> Rect {
    const f32 x0 = std::max(a.x, b.x);
    const f32 y0 = std::max(a.y, b.y);
    const f32 x1 = std::min(a.x + a.width, b.x + b.width);
    const f32 y1 = std::min(a.y + a.height, b.y + b.height);
    return {
        .x = x0,
        .y = y0,
        .width = std::max(0.0F, x1 - x0),
        .height = std::max(0.0F, y1 - y0),
    };
}

[[nodiscard]] inline auto empty(Rect rect) -> b8 {
    return rect.width <= 0.0F || rect.height <= 0.0F;
}

[[nodiscard]] inline auto menurect(std::string_view text, f32 x) -> Rect {
    return {
        .x = x,
        .y = 0.0F,
        .width = static_cast<f32>(text.size()) * kFontWidth + 12.0F,
        .height = 18.0F,
    };
}

[[nodiscard]] inline auto menux(u8 menu) -> f32 {
    f32 menu_x = 4.0F;
    for (const MenuItem item : kMenuItems) {
        if (item.index == menu) {
            return menu_x;
        }
        menu_x += menurect(item.text, menu_x).width + 1.0F;
    }
    return 4.0F;
}

[[nodiscard]] inline auto menucmdwidth(u8 menu) -> f32 {
    f32 width = 45.0F;
    for (const MenuCommand command : menucommands(menu)) {
        width = std::max(width, static_cast<f32>(command.text.size()) * kFontWidth + 10.0F);
    }
    return width;
}

[[nodiscard]] inline auto menucmdrect(u8 menu, u8 index) -> Rect {
    return {
        .x = menux(menu),
        .y = 18.0F + static_cast<f32>(index) * 15.0F,
        .width = menucmdwidth(menu),
        .height = 15.0F,
    };
}

template <usize N> void copy_name(std::array<char, N> *out, std::string_view name) {
    out->fill('\0');
    usize index = 0;
    while (index + 1U < N && index < name.size()) {
        (*out)[index] = name[index];
        ++index;
    }
}

[[nodiscard]] inline auto fixname(std::span<const char> name) -> std::string_view {
    usize length = 0;
    for (const char c : name) {
        if (c == '\0') {
            break;
        }
        ++length;
    }
    return {name.data(), length};
}

[[nodiscard]] inline auto layerflag(const Layer &layer, u8 flag) -> b8 {
    return (layer.flags & flag) != 0;
}

inline void setlayerflag(Layer *layer, u8 flag, b8 enabled) {
    if (enabled) {
        layer->flags = static_cast<u8>(layer->flags | flag);
    } else {
        layer->flags = static_cast<u8>(layer->flags & ~flag);
    }
}

[[nodiscard]] inline auto isbackground(const Layer &layer) -> b8 {
    return layerflag(layer, kLayerBottom) || layer.kind == LayerKind::kBackground;
}

[[nodiscard]] inline auto mklayer(u32 id, std::string_view name, LayerKind kind, u8 opacity,
                                  b8 visible, b8 locked, b8 selected, u8 layer_slot, b8 bottom)
    -> Layer {
    Layer layer = {};
    layer.id = id;
    layer.flags = 0;
    layer.opacity_u8 = opacity;
    layer.layer_slot = layer_slot;
    layer.kind = kind;
    setlayerflag(&layer, kLayerVisible, visible);
    setlayerflag(&layer, kLayerLocked, locked);
    setlayerflag(&layer, kLayerSelected, selected);
    setlayerflag(&layer, kLayerBottom, bottom);
    copy_name(&layer.name, name);
    return layer;
}

inline void pushlayer(GuiState *state, u32 id, std::string_view name, LayerKind kind, u8 opacity,
                      b8 visible, b8 locked, b8 selected, u8 layer_slot, b8 bottom) {
    (void)state->layers.push(
        mklayer(id, name, kind, opacity, visible, locked, selected, layer_slot, bottom));
}

inline void markclear(GuiState *state, u8 slot) {
    for (const u8 queued : state->clear_slots.span()) {
        if (queued == slot) {
            return;
        }
    }
    (void)state->clear_slots.push(slot);
}

[[nodiscard]] inline auto slotused(const GuiState &state, u8 slot) -> b8 {
    for (const Layer &layer : state.layers.span()) {
        if (layer.layer_slot == slot) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] inline auto freeslot(const GuiState &state) -> u8 {
    for (u8 slot = 0; slot + 1U < kMaxLayers; ++slot) {
        if (!slotused(state, slot)) {
            return slot;
        }
    }
    return kNoLayer;
}

inline void genlayername(std::array<char, kLayerNameBytes> *name, u32 id) {
    copy_name(name, "layer ");
    std::array<char, 10> digits = {};
    usize count = 0;
    u32 value = id;
    do {
        digits[count] = static_cast<char>('0' + (value % 10U));
        value /= 10U;
        ++count;
    } while (value != 0U && count < sizeof(digits));

    usize out = 6;
    while (count != 0U && out + 1U < kLayerNameBytes) {
        --count;
        (*name)[out] = digits[count];
        ++out;
    }
}

[[nodiscard]] inline auto layerinsert(const GuiState &state) -> usize {
    if (state.layers.empty()) {
        return 0;
    }
    if (state.curlayer < state.layers.size()) {
        return state.curlayer;
    }
    return state.layers.size() - 1U;
}

inline void pushtool(GuiState *state, u32 id, std::string_view name, ToolKind kind, b8 selected) {
    Tool tool = {};
    tool.id = id;
    tool.selected = selected ? 1U : 0U;
    tool.kind = kind;
    copy_name(&tool.name, name);
    (void)state->tools.push(tool);
}

inline void pushtip(GuiState *state, u8 index, b8 selected) {
    (void)state->tips.push({
        .index = index,
        .selected = static_cast<u8>(selected ? 1U : 0U),
    });
}

inline void pushsize(GuiState *state, u8 index, b8 selected) {
    (void)state->sizes.push({
        .index = index,
        .selected = static_cast<u8>(selected ? 1U : 0U),
    });
}

inline void pushcoverage(GuiState *state, u8 index, b8 selected) {
    (void)state->coverages.push({
        .index = index,
        .selected = static_cast<u8>(selected ? 1U : 0U),
    });
}

inline void select_layer(GuiState *state, u8 index) {
    if (index >= state->layers.size()) {
        return;
    }
    state->curlayer = index;
    for (usize layer_index = 0; layer_index < state->layers.size(); ++layer_index) {
        setlayerflag(&state->layers[layer_index], kLayerSelected, layer_index == index);
    }
}

inline void select_tool(GuiState *state, u8 index) {
    if (index >= state->tools.size()) {
        return;
    }
    state->curtool = index;
    for (usize tool_index = 0; tool_index < state->tools.size(); ++tool_index) {
        state->tools[tool_index].selected = tool_index == index ? 1U : 0U;
    }
}

inline void select_tip(GuiState *state, u8 index) {
    if (index >= state->tips.size()) {
        return;
    }
    state->curtip = index;
    for (usize tip_index = 0; tip_index < state->tips.size(); ++tip_index) {
        state->tips[tip_index].selected = tip_index == index ? 1U : 0U;
    }
}

inline void select_size(GuiState *state, u8 index) {
    if (index >= state->sizes.size()) {
        return;
    }
    state->cursize = index;
    for (usize size_index = 0; size_index < state->sizes.size(); ++size_index) {
        state->sizes[size_index].selected = size_index == index ? 1U : 0U;
    }
}

inline void select_coverage(GuiState *state, u8 index) {
    if (index >= state->coverages.size()) {
        return;
    }
    state->curcoverage = index;
    for (usize coverage_index = 0; coverage_index < state->coverages.size(); ++coverage_index) {
        state->coverages[coverage_index].selected = coverage_index == index ? 1U : 0U;
    }
}

inline void addhit(GuiState *state, Rect rect, HitKind kind, u8 index, u16 priority) {
    (void)state->hits.push({.rect = rect, .kind = kind, .index = index, .priority = priority});
}

inline void pushaction(GuiState *state, GuiActionKind kind) {
    (void)state->actions.push({.kind = kind});
}

inline void drawrect(DrawList *draws, Rect rect, Tone tone) { (void)add_rect(draws, rect, tone); }

inline void drawplane(DrawList *draws, DrawPlane plane) { draws->begin_plane(plane); }

inline void drawstroke(DrawList *draws, Rect rect, Tone tone, f32 width = 1.0F) {
    (void)add_stroke(draws, rect, tone, width);
}

inline void drawtext(DrawList *draws, std::string_view text, f32 x, f32 y, Tone tone,
                     f32 scale = 1.0F) {
    (void)add_text(draws, text, x, y, tone, scale);
}

inline void drawicon(DrawList *draws, Icon icon, f32 x, f32 y, Tone tone, f32 scale = 1.0F) {
    (void)add_icon(draws, icon, x, y, tone, scale);
}

inline void layernametext(DrawList *draws, const Layer &layer, f32 x, f32 y, Tone tone) {
    drawtext(draws, layername(layer), x, y, tone);
}

[[nodiscard]] inline auto toolicon(ToolKind kind) -> Icon { return tooldef(kind).icon; }

[[nodiscard]] inline auto f32abs(f32 value) -> f32 { return value < 0.0F ? -value : value; }

[[nodiscard]] inline auto painttool(ToolKind kind) -> b8 { return toolpaints(tooldef(kind)); }

[[nodiscard]] inline auto freehandtool(ToolKind kind) -> b8 { return toolfreehand(tooldef(kind)); }

[[nodiscard]] inline auto drafttool(ToolKind kind) -> b8 { return tooldraft(tooldef(kind)); }

[[nodiscard]] inline auto sizetool(ToolKind kind) -> b8 {
    return tooluses(tooldef(kind), kToolUsesSize);
}

[[nodiscard]] inline auto tiptool(ToolKind kind) -> b8 {
    return tooluses(tooldef(kind), kToolUsesTip);
}

[[nodiscard]] inline auto coveragetool(ToolKind kind) -> b8 {
    return tooluses(tooldef(kind), kToolUsesCoverage);
}

[[nodiscard]] inline auto paintlayer(const GuiState &state) -> const Layer * {
    const Layer *layer = layercur(state);
    if (layer == nullptr || layerlocked(*layer) || layer->kind != LayerKind::kInk) {
        return nullptr;
    }
    return layer;
}

[[nodiscard]] inline auto painttone(ToolKind kind) -> Tone {
    return tooldef(kind).ink == InkKind::kWhite ? Tone::kWhite : Tone::kBlack;
}

[[nodiscard]] inline auto stampsize(const GuiState &state, ToolKind kind) -> u8 {
    const ToolDef def = tooldef(kind);
    return tooluses(def, kToolUsesSize) ? state.cursize : def.size;
}

[[nodiscard]] inline auto stamptip(const GuiState &state, ToolKind kind) -> u8 {
    const ToolDef def = tooldef(kind);
    return tooluses(def, kToolUsesTip) ? state.curtip : def.tip;
}

[[nodiscard]] inline auto stampcoverage(const GuiState &state, ToolKind kind) -> u8 {
    if (!coveragetool(kind)) {
        return 0;
    }
    return state.curcoverage;
}

[[nodiscard]] inline auto brushspec(const GuiState &state, ToolKind kind) -> BrushSpec {
    return {
        .diameter = static_cast<f32>(stampsize(state, kind)) + 1.0F,
        .tone = painttone(kind),
        .tip = stamptip(state, kind),
        .coverage = stampcoverage(state, kind),
    };
}

[[nodiscard]] inline auto toolkind(const GuiState &state) -> ToolKind {
    if (const Tool *tool = toolcur(state)) {
        return tool->kind;
    }
    return ToolKind::kPen;
}

[[nodiscard]] inline auto screen_to_document_x(const GuiState &state, f32 x) -> f32 {
    return state.view.x + ((x - state.layout.viewport.x) / state.view.zoom);
}

[[nodiscard]] inline auto screen_to_document_y(const GuiState &state, f32 y) -> f32 {
    return state.view.y + ((y - state.layout.viewport.y) / state.view.zoom);
}

[[nodiscard]] inline auto screen_to_paint_x(const GuiState &state, i32 x) -> f32 {
    return std::floor(screen_to_document_x(state, static_cast<f32>(x)));
}

[[nodiscard]] inline auto screen_to_paint_y(const GuiState &state, i32 y) -> f32 {
    return std::floor(screen_to_document_y(state, static_cast<f32>(y)));
}

[[nodiscard]] inline auto in_document(const GuiState &state, f32 x, f32 y) -> b8 {
    return x >= 0.0F && y >= 0.0F && x < static_cast<f32>(state.document.width) &&
           y < static_cast<f32>(state.document.height);
}

[[nodiscard]] inline auto document_to_screen_x(const GuiState &state, f32 x) -> f32 {
    return state.layout.viewport.x + ((x - state.view.x) * state.view.zoom);
}

[[nodiscard]] inline auto document_to_screen_y(const GuiState &state, f32 y) -> f32 {
    return state.layout.viewport.y + ((y - state.view.y) * state.view.zoom);
}

[[nodiscard]] inline auto clamp_zoom(f32 zoom) -> f32 { return std::clamp(zoom, 0.25F, 16.0F); }

inline void update_document_rect(GuiState *state) {
    state->layout.document = {
        .x = document_to_screen_x(*state, 0.0F),
        .y = document_to_screen_y(*state, 0.0F),
        .width = static_cast<f32>(state->document.width) * state->view.zoom,
        .height = static_cast<f32>(state->document.height) * state->view.zoom,
    };
}

inline void center_document(GuiState *state) {
    state->view.zoom = kInitialViewZoom;
    state->view.x = (static_cast<f32>(state->document.width) -
                     (state->layout.viewport.width / state->view.zoom)) *
                    0.5F;
    state->view.y = (static_cast<f32>(state->document.height) -
                     (state->layout.viewport.height / state->view.zoom)) *
                    0.5F;
    state->view_initialized = true;
    update_document_rect(state);
}

void menulayout(GuiState *state);
[[nodiscard]] b8 menumouse(GuiState *state, HitRecord hit);
void doaction(GuiState *state, MenuAction action);
void menudraw(const GuiState &state, DrawList *draws);

void contextopen(GuiState *state, HitRecord hit, i32 x, i32 y);
void contextlayout(GuiState *state);
[[nodiscard]] b8 contextmouse(GuiState *state, HitRecord hit);
[[nodiscard]] b8 contextkey(GuiState *state, Key key);
void contextdraw(const GuiState &state, DrawList *draws);

void aboutopen(GuiState *state);
void dialogopen(GuiState *state);
void dialoglayout(GuiState *state);
[[nodiscard]] b8 dialogmouse(GuiState *state, HitRecord hit);
[[nodiscard]] b8 dialogkey(GuiState *state, Key key);
[[nodiscard]] b8 dialogtext(GuiState *state, char c);
void dialogdraw(const GuiState &state, DrawList *draws);

void toollayout(GuiState *state);
void tooltick(GuiState *state);
[[nodiscard]] b8 toolanimating(const GuiState &state);
[[nodiscard]] b8 toolkey(GuiState *state, Key key);
[[nodiscard]] b8 toolmouse(GuiState *state, HitRecord hit);
void toolmove(GuiState *state, i32 x, i32 y, u8 buttons);
void toolup(GuiState *state);
void tooldraw(const GuiState &state, DrawList *draws);
void toolpopupdraw(const GuiState &state, DrawList *draws);

void layerlayout(GuiState *state);
[[nodiscard]] b8 layermouse(GuiState *state, HitRecord hit, i32 x);
[[nodiscard]] b8 layerkey(GuiState *state, Key key);
[[nodiscard]] b8 layertext(GuiState *state, char c);
void layermove(GuiState *state, i32 x);
void layerdone(GuiState *state);
void layerdraw(const GuiState &state, DrawList *draws);

void historyclear(GuiState *state);
[[nodiscard]] b8 historystart(GuiState *state, const Layer &layer);
void historymark(GuiState *state, PaintStamp stamp);
void historyfinish(GuiState *state);
[[nodiscard]] b8 historyundo(GuiState *state);
[[nodiscard]] b8 historyredo(GuiState *state);
void historyreplay(GuiState *state);

void worklayout(GuiState *state);
void workcancel(GuiState *state);
[[nodiscard]] b8 workwheel(GuiState *state, i32 x, i32 y, i32 dx, i32 dy, u8 mods,
                           HitKind hit_kind);
[[nodiscard]] b8 workmouse(GuiState *state, HitRecord hit, i32 x, i32 y, u8 button);
void workmove(GuiState *state, i32 x, i32 y, u8 buttons);
void workup(GuiState *state, i32 x, i32 y);
void workdraw(const GuiState &state, DrawList *draws);

} // namespace mira::gui
