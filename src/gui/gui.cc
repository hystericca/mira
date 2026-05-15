#include "mira/gui/gui.hpp"

#include <algorithm>

namespace mira {
namespace {

struct MenuItem {
    std::string_view text;
    u8 index;
};

constexpr MenuItem kMenuItems[] = {
    {"MIRA", 0}, {"FILE", 1}, {"EDIT", 2}, {"LAYER", 3}, {"VIEW", 4},
};

[[nodiscard]] auto contains(Rect rect, i32 x, i32 y) -> b8 {
    const f32 px = static_cast<f32>(x);
    const f32 py = static_cast<f32>(y);
    return px >= rect.x && py >= rect.y && px < rect.x + rect.width && py < rect.y + rect.height;
}

[[nodiscard]] auto menu_rect(std::string_view text, f32 x) -> Rect {
    return {
        .x = x,
        .y = 0.0F,
        .width = static_cast<f32>(text.size() * 6U + 12U),
        .height = 14.0F,
    };
}

template <usize N> void copy_name(std::array<char, N> *out, std::string_view name) {
    usize index = 0;
    for (char &slot : *out) {
        slot = index < name.size() ? name[index] : '\0';
        ++index;
    }
}

[[nodiscard]] auto fixed_name(std::span<const char> name) -> std::string_view {
    usize length = 0;
    for (const char c : name) {
        if (c == '\0') {
            break;
        }
        ++length;
    }
    return {name.data(), length};
}

void push_layer(GuiState *state, u32 id, std::string_view name, LayerKind kind, u8 opacity,
                b8 visible, b8 selected) {
    Layer layer = {};
    layer.id = id;
    layer.visible = visible ? 1U : 0U;
    layer.opacity_u8 = opacity;
    layer.selected = selected ? 1U : 0U;
    layer.kind = kind;
    copy_name(&layer.name, name);
    (void)state->layers.push(layer);
}

void push_tool(GuiState *state, u32 id, std::string_view name, ToolKind kind, b8 selected) {
    Tool tool = {};
    tool.id = id;
    tool.selected = selected ? 1U : 0U;
    tool.kind = kind;
    copy_name(&tool.name, name);
    (void)state->tools.push(tool);
}

void select_layer(GuiState *state, u8 index) {
    if (index >= state->layers.size()) {
        return;
    }
    state->selected_layer = index;
    for (usize layer_index = 0; layer_index < state->layers.size(); ++layer_index) {
        state->layers[layer_index].selected = layer_index == index ? 1U : 0U;
    }
}

void select_tool(GuiState *state, u8 index) {
    if (index >= state->tools.size()) {
        return;
    }
    state->selected_tool = index;
    for (usize tool_index = 0; tool_index < state->tools.size(); ++tool_index) {
        state->tools[tool_index].selected = tool_index == index ? 1U : 0U;
    }
}

void add_hit(GuiState *state, Rect rect, HitKind kind, u8 index, u16 priority) {
    (void)state->hits.push({.rect = rect, .kind = kind, .index = index, .priority = priority});
}

void draw_rect(DrawList *draws, Rect rect, Tone tone) { (void)add_rect(draws, rect, tone); }

void draw_stroke(DrawList *draws, Rect rect, Tone tone, f32 width = 1.0F) {
    (void)add_stroke(draws, rect, tone, width);
}

void draw_text(DrawList *draws, std::string_view text, f32 x, f32 y, Tone tone, f32 scale = 1.0F) {
    (void)add_text(draws, text, x, y, tone, scale);
}

void draw_icon(DrawList *draws, Icon icon, f32 x, f32 y, Tone tone, f32 scale = 1.0F) {
    (void)add_icon(draws, icon, x, y, tone, scale);
}

void draw_layer_name(DrawList *draws, const Layer &layer, f32 x, f32 y, Tone tone) {
    std::array<char, kLayerNameBytes> upper = {};
    const std::string_view name = layer_name(layer);
    for (usize index = 0; index < name.size() && index < kLayerNameBytes; ++index) {
        const char c = name[index];
        upper[index] = c >= 'a' && c <= 'z' ? static_cast<char>(c - ('a' - 'A')) : c;
    }
    draw_text(draws, std::string_view(upper.data(), name.size()), x, y, tone);
}

[[nodiscard]] Icon icon_for(ToolKind kind) {
    switch (kind) {
    case ToolKind::kBrush:
        return Icon::kBrush;
    case ToolKind::kLine:
        return Icon::kLine;
    case ToolKind::kMagic:
        return Icon::kMagic;
    case ToolKind::kRect:
        return Icon::kRect;
    case ToolKind::kZoom:
        return Icon::kZoom;
    case ToolKind::kErase:
        return Icon::kErase;
    case ToolKind::kPen:
    default:
        return Icon::kPen;
    }
}

} // namespace

std::string_view layer_name(const Layer &layer) { return fixed_name(layer.name); }

std::string_view tool_name(const Tool &tool) { return fixed_name(tool.name); }

void init_gui(GuiState *state) {
    state->layers.clear();
    state->tools.clear();
    state->hits.clear();
    state->mouse_x = -1;
    state->mouse_y = -1;
    state->hot_kind = HitKind::kNone;
    state->active_kind = HitKind::kNone;
    state->hot_index = 0;
    state->active_index = 0;
    state->selected_layer = 0;
    state->selected_tool = 0;
    push_tool(state, 1, "Pen", ToolKind::kPen, true);
    push_tool(state, 2, "Brush", ToolKind::kBrush, false);
    push_tool(state, 3, "Line", ToolKind::kLine, false);
    push_tool(state, 4, "Magic", ToolKind::kMagic, false);
    push_tool(state, 5, "Rect", ToolKind::kRect, false);
    push_tool(state, 6, "Zoom", ToolKind::kZoom, false);
    push_tool(state, 7, "Erase", ToolKind::kErase, false);
    push_layer(state, 1, "Ink", LayerKind::kInk, 255, true, true);
    push_layer(state, 2, "Reference", LayerKind::kReference, 112, true, false);
    push_layer(state, 3, "Paper", LayerKind::kPaper, 255, true, false);
    state->initialized = true;
}

void layout_gui(GuiState *state, Screen screen) {
    if (!state->initialized) {
        init_gui(state);
    }

    const f32 width = static_cast<f32>(std::max(1, screen.width));
    const f32 height = static_cast<f32>(std::max(1, screen.height));
    const f32 menu_height = std::min(14.0F, height);
    const f32 toolbar_width = std::min(28.0F, std::max(24.0F, width * 0.06F));
    const f32 sidebar_width = std::min(112.0F, std::max(72.0F, width * 0.25F));
    const f32 canvas_x = toolbar_width;
    const f32 canvas_width = std::max(1.0F, width - toolbar_width - sidebar_width);
    const f32 sidebar_x = canvas_x + canvas_width;
    const f32 body_height = std::max(1.0F, height - menu_height);

    state->layout = {
        .screen = {.x = 0.0F, .y = 0.0F, .width = width, .height = height},
        .menu_bar = {.x = 0.0F, .y = 0.0F, .width = width, .height = menu_height},
        .toolbar = {.x = 0.0F, .y = menu_height, .width = toolbar_width, .height = body_height},
        .tool_list = {.x = 4.0F,
                      .y = menu_height + 4.0F,
                      .width = std::max(1.0F, toolbar_width - 8.0F),
                      .height = std::max(1.0F, body_height - 8.0F)},
        .canvas = {.x = canvas_x, .y = menu_height, .width = canvas_width, .height = body_height},
        .work_area = {.x = canvas_x + 8.0F,
                      .y = menu_height + 8.0F,
                      .width = std::max(1.0F, canvas_width - 16.0F),
                      .height = std::max(1.0F, body_height - 16.0F)},
        .sidebar = {.x = sidebar_x,
                    .y = menu_height,
                    .width = sidebar_width,
                    .height = body_height},
        .layer_list = {.x = sidebar_x + 5.0F,
                       .y = menu_height + 22.0F,
                       .width = std::max(1.0F, sidebar_width - 10.0F),
                       .height = std::max(1.0F, body_height - 27.0F)},
    };

    state->hits.clear();
    add_hit(state, state->layout.canvas, HitKind::kCanvas, 0, 10);
    add_hit(state, state->layout.toolbar, HitKind::kToolbar, 0, 20);
    add_hit(state, state->layout.sidebar, HitKind::kSidebar, 0, 20);

    f32 menu_x = 4.0F;
    for (const MenuItem item : kMenuItems) {
        const Rect rect = menu_rect(item.text, menu_x);
        add_hit(state, rect, HitKind::kMenu, item.index, 100);
        menu_x += rect.width + 1.0F;
    }

    for (usize index = 0; index < state->layers.size(); ++index) {
        const f32 row_y = state->layout.layer_list.y + static_cast<f32>(index) * 24.0F;
        const Rect row = {
            .x = state->layout.layer_list.x,
            .y = row_y,
            .width = state->layout.layer_list.width,
            .height = 21.0F,
        };
        const Rect visible = {
            .x = row.x + 5.0F,
            .y = row.y + 6.0F,
            .width = 7.0F,
            .height = 7.0F,
        };
        add_hit(state, row, HitKind::kLayerRow, static_cast<u8>(index), 80);
        add_hit(state, visible, HitKind::kLayerVisibility, static_cast<u8>(index), 90);
    }

    for (usize index = 0; index < state->tools.size(); ++index) {
        const f32 row_y = state->layout.tool_list.y + static_cast<f32>(index) * 18.0F;
        const Rect row = {
            .x = state->layout.tool_list.x,
            .y = row_y,
            .width = state->layout.tool_list.width,
            .height = 15.0F,
        };
        add_hit(state, row, HitKind::kTool, static_cast<u8>(index), 85);
    }
}

HitRecord hit_at(const GuiState &state, i32 x, i32 y) {
    HitRecord result = {};
    for (const HitRecord hit : state.hits.span()) {
        if (contains(hit.rect, x, y) && hit.priority >= result.priority) {
            result = hit;
        }
    }
    return result;
}

void reduce_gui(GuiState *state, std::span<const InputEvent> input) {
    if (!state->initialized) {
        init_gui(state);
    }

    for (const InputEvent event : input) {
        state->mouse_x = event.x;
        state->mouse_y = event.y;
        const HitRecord hit = hit_at(*state, event.x, event.y);
        state->hot_kind = hit.kind;
        state->hot_index = hit.index;

        if (event.kind == InputKind::kMouseDown) {
            state->active_kind = hit.kind;
            state->active_index = hit.index;
            if (hit.kind == HitKind::kLayerVisibility && hit.index < state->layers.size()) {
                Layer &layer = state->layers[hit.index];
                layer.visible = layer.visible == 0 ? 1U : 0U;
            } else if (hit.kind == HitKind::kLayerRow) {
                select_layer(state, hit.index);
            } else if (hit.kind == HitKind::kTool) {
                select_tool(state, hit.index);
            }
        } else if (event.kind == InputKind::kMouseUp) {
            state->active_kind = HitKind::kNone;
            state->active_index = 0;
        }
    }
}

void emit_gui(const GuiState &state, DrawList *draws) {
    draws->clear();

    draw_rect(draws, state.layout.screen, Tone::kBlack);
    draw_rect(draws, state.layout.menu_bar, Tone::kBlack);
    draw_stroke(draws,
                {.x = 0.0F,
                 .y = state.layout.menu_bar.height - 1.0F,
                 .width = state.layout.screen.width,
                 .height = 1.0F},
                Tone::kWhite);

    f32 menu_x = 4.0F;
    for (const MenuItem item : kMenuItems) {
        const Rect rect = menu_rect(item.text, menu_x);
        if (state.hot_kind == HitKind::kMenu && state.hot_index == item.index) {
            draw_rect(draws,
                      {.x = rect.x, .y = rect.y + 1.0F, .width = rect.width, .height = 12.0F},
                      Tone::kWhite);
        }
        draw_text(draws, item.text, rect.x + 5.0F, 4.0F,
                  state.hot_kind == HitKind::kMenu && state.hot_index == item.index ? Tone::kBlack
                                                                                    : Tone::kWhite);
        menu_x += rect.width + 1.0F;
    }

    draw_rect(draws, state.layout.canvas, Tone::kWhite);
    draw_stroke(draws, state.layout.canvas, Tone::kBlack);

    draw_rect(draws, state.layout.toolbar, Tone::kBlack);
    draw_stroke(draws, state.layout.toolbar, Tone::kWhite);

    for (usize index = 0; index < state.tools.size(); ++index) {
        const Tool &tool = state.tools[index];
        const f32 row_y = state.layout.tool_list.y + static_cast<f32>(index) * 18.0F;
        const Rect row = {
            .x = state.layout.tool_list.x,
            .y = row_y,
            .width = state.layout.tool_list.width,
            .height = 15.0F,
        };
        const b8 hot_row = state.hot_kind == HitKind::kTool && state.hot_index == index;
        const b8 selected = tool.selected != 0;
        const b8 inverted = selected || hot_row;
        if (inverted) {
            draw_rect(draws, row, Tone::kWhite);
        }
        const f32 icon_x = row.x + std::max(0.0F, (row.width - 8.0F) * 0.5F);
        draw_icon(draws, icon_for(tool.kind), icon_x, row.y + 4.0F,
                  inverted ? Tone::kBlack : Tone::kWhite);
    }

    draw_rect(draws, state.layout.sidebar, Tone::kBlack);
    draw_stroke(draws, state.layout.sidebar, Tone::kWhite);
    draw_text(draws, "LAYERS", state.layout.sidebar.x + 7.0F, state.layout.sidebar.y + 7.0F,
              Tone::kWhite);

    for (usize index = 0; index < state.layers.size(); ++index) {
        const Layer &layer = state.layers[index];
        const f32 row_y = state.layout.layer_list.y + static_cast<f32>(index) * 24.0F;
        const Rect row = {
            .x = state.layout.layer_list.x,
            .y = row_y,
            .width = state.layout.layer_list.width,
            .height = 21.0F,
        };
        const b8 hot_row =
            (state.hot_kind == HitKind::kLayerRow || state.hot_kind == HitKind::kLayerVisibility) &&
            state.hot_index == index;
        const b8 selected = layer.selected != 0;
        const b8 inverted = selected || hot_row;
        if (selected || hot_row) {
            draw_rect(draws, row, Tone::kWhite);
        }

        const Rect eye = {.x = row.x + 5.0F, .y = row.y + 6.0F, .width = 7.0F, .height = 7.0F};
        if (layer.visible != 0) {
            draw_rect(draws, eye, inverted ? Tone::kBlack : Tone::kWhite);
        }
        draw_stroke(draws, eye, inverted ? Tone::kBlack : Tone::kWhite);
        draw_layer_name(draws, layer, row.x + 17.0F, row.y + 4.0F,
                        inverted ? Tone::kBlack : Tone::kWhite);
    }
}

void build_gui_frame(GuiState *state, Screen screen, std::span<const InputEvent> input,
                     DrawList *draws) {
    layout_gui(state, screen);
    reduce_gui(state, input);
    emit_gui(*state, draws);
}

const Layer *selected_layer(const GuiState &state) {
    if (state.selected_layer >= state.layers.size()) {
        return nullptr;
    }
    return &state.layers[state.selected_layer];
}

const Tool *selected_tool(const GuiState &state) {
    if (state.selected_tool >= state.tools.size()) {
        return nullptr;
    }
    return &state.tools[state.selected_tool];
}

} // namespace mira
