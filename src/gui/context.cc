#include "private.hpp"

namespace mira {

ContextKind contextkind(const GuiState &state) {
    if (!state.context_open) {
        return ContextKind::kNone;
    }
    if (state.context_target != kNoLayer) {
        return ContextKind::kLayer;
    }
    if (gui::contains(state.layout.layers, state.context_x, state.context_y)) {
        return ContextKind::kLayer;
    }
    if (gui::contains(state.layout.viewport, state.context_x, state.context_y)) {
        return ContextKind::kWorkspace;
    }
    return ContextKind::kApp;
}

} // namespace mira

namespace mira::gui {
namespace {

[[nodiscard]] auto layerhit(HitKind kind) -> b8 {
    return kind == HitKind::kLayerRow || kind == HitKind::kLayerVisibility ||
           kind == HitKind::kLayerLock || kind == HitKind::kLayerOpacity;
}

[[nodiscard]] auto commandwidth(std::span<const MenuCommand> commands) -> f32 {
    f32 width = 54.0F;
    for (const MenuCommand command : commands) {
        width = std::max(width, static_cast<f32>(command.text.size()) * kFontWidth + 10.0F);
    }
    return width;
}

[[nodiscard]] auto commandrect(const GuiState &state, u8 index) -> Rect {
    return {
        .x = state.layout.context.x,
        .y = state.layout.context.y + static_cast<f32>(index) * 15.0F,
        .width = state.layout.context.width,
        .height = 15.0F,
    };
}

void close(GuiState *state) {
    state->context_open = false;
    state->context_target = kNoLayer;
}

} // namespace

void contextopen(GuiState *state, HitRecord hit, i32 x, i32 y) {
    layerdone(state);
    state->active_menu = kNoMenu;
    state->context_open = true;
    state->context_x = x;
    state->context_y = y;
    state->context_target = kNoLayer;
    if (layerhit(hit.kind) && hit.index < state->layers.size()) {
        state->context_target = hit.index;
        select_layer(state, hit.index);
    }
}

void contextlayout(GuiState *state) {
    if (!state->context_open) {
        return;
    }

    const std::span<const MenuCommand> commands = contextcommands(mira::contextkind(*state));
    if (commands.empty()) {
        close(state);
        return;
    }

    const f32 width = commandwidth(commands);
    const f32 height = static_cast<f32>(commands.size()) * 15.0F;
    const f32 x = std::clamp(static_cast<f32>(state->context_x), 0.0F,
                             std::max(0.0F, state->layout.window.width - width));
    const f32 y = std::clamp(static_cast<f32>(state->context_y), 0.0F,
                             std::max(0.0F, state->layout.window.height - height));
    state->layout.context = {.x = x, .y = y, .width = width, .height = height};

    addhit(state, state->layout.context, HitKind::kContextMenu, 0, 150);
    for (u8 index = 0; index < static_cast<u8>(commands.size()); ++index) {
        addhit(state, commandrect(*state, index), HitKind::kContextAction, index, 170);
    }
}

b8 contextmouse(GuiState *state, HitRecord hit) {
    if (!state->context_open) {
        return false;
    }
    if (hit.kind == HitKind::kContextAction) {
        doaction(state, mira::menuaction(*state, hit));
    }
    close(state);
    return true;
}

b8 contextkey(GuiState *state, Key key) {
    if (!state->context_open) {
        return false;
    }
    if (key == Key::kEscape) {
        close(state);
    }
    return true;
}

void contextdraw(const GuiState &state, DrawList *draws) {
    if (!state.context_open) {
        return;
    }

    const std::span<const MenuCommand> commands = contextcommands(mira::contextkind(state));
    if (commands.empty()) {
        return;
    }

    drawplane(draws, DrawPlane::kMenu);
    drawrect(draws, state.layout.context, Tone::kWhite);
    drawstroke(draws, state.layout.context, Tone::kBlack);
    for (u8 index = 0; index < static_cast<u8>(commands.size()); ++index) {
        const Rect item = commandrect(state, index);
        const b8 hot = state.hot_kind == HitKind::kContextAction && state.hot_index == index;
        if (hot) {
            drawrect(draws, item, Tone::kBlack);
        }
        drawtext(draws, commands[index].text, item.x + 5.0F, item.y + 1.0F,
                 hot ? Tone::kWhite : Tone::kBlack);
    }
}

} // namespace mira::gui
