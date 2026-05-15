#include "private.hpp"

namespace mira {
namespace impl = gui;

std::string_view layername(const Layer &layer) { return impl::fixname(layer.name); }

b8 layervisible(const Layer &layer) { return impl::layerflag(layer, kLayerVisible); }

b8 layerlocked(const Layer &layer) { return impl::layerflag(layer, kLayerLocked); }

b8 layerselected(const Layer &layer) { return impl::layerflag(layer, kLayerSelected); }

bool layerrename(GuiState *state, u8 index, std::string_view name) {
    if (state == nullptr || index >= state->layers.size() ||
        impl::iscanvas(state->layers[index])) {
        return false;
    }
    impl::copy_name(&state->layers[index].name, name);
    return true;
}

bool layeradd(GuiState *state, std::string_view name) {
    if (state == nullptr) {
        return false;
    }
    if (!state->initialized) {
        guiinit(state);
    }
    if (state->layers.size() >= state->layers.capacity()) {
        state->layers.overflowed = true;
        return false;
    }
    const u8 slot = impl::freeslot(*state);
    if (slot == kNoLayer) {
        state->layers.overflowed = true;
        return false;
    }

    const u32 id = state->next_layer_id;
    ++state->next_layer_id;
    Layer layer =
        impl::mklayer(id, name.empty() ? "Layer" : name, LayerKind::kInk, 255, true, false,
                         false, slot, false);
    if (name.empty()) {
        impl::genlayername(&layer.name, id);
    }

    const usize insert_at = impl::layerinsert(*state);
    if (!state->layers.insert(insert_at, layer)) {
        return false;
    }
    impl::markclear(state, slot);
    impl::select_layer(state, static_cast<u8>(insert_at));
    return true;
}

bool layerdel(GuiState *state) {
    if (state == nullptr || state->curlayer >= state->layers.size()) {
        return false;
    }
    const usize remove_at = state->curlayer;
    const Layer &layer = state->layers[remove_at];
    if (impl::iscanvas(layer)) {
        return false;
    }
    const u8 slot = layer.texture_slot;
    if (!state->layers.erase(remove_at)) {
        return false;
    }
    impl::markclear(state, slot);
    if (state->layers.empty()) {
        state->curlayer = kNoLayer;
        state->renaming_layer = kNoLayer;
        return true;
    }
    const usize next = std::min(remove_at, state->layers.size() - 1U);
    impl::select_layer(state, static_cast<u8>(next));
    state->renaming_layer = kNoLayer;
    state->rename_replace = false;
    return true;
}

bool layeredit(GuiState *state) {
    if (state == nullptr || state->curlayer >= state->layers.size() ||
        impl::iscanvas(state->layers[state->curlayer])) {
        return false;
    }
    state->renaming_layer = state->curlayer;
    state->rename_replace = true;
    return true;
}

const Layer *layercur(const GuiState &state) {
    if (state.curlayer >= state.layers.size()) {
        return nullptr;
    }
    return &state.layers[state.curlayer];
}

} // namespace mira

namespace mira::gui {
namespace {

void opacityat(GuiState *state, u8 index, i32 x) {
    if (index >= state->layers.size()) {
        return;
    }
    const f32 row_y = state->layout.layerrows.y + static_cast<f32>(index) * 28.0F;
    const Rect bar = {
        .x = state->layout.layerrows.x + 17.0F,
        .y = row_y + 18.0F,
        .width = std::max(1.0F, state->layout.layerrows.width - 24.0F),
        .height = 3.0F,
    };
    const f32 t = std::clamp((static_cast<f32>(x) - bar.x) / bar.width, 0.0F, 1.0F);
    state->layers[index].opacity_u8 = static_cast<u8>(std::round(t * 255.0F));
}

void nameback(GuiState *state) {
    if (state->renaming_layer >= state->layers.size()) {
        layerdone(state);
        return;
    }
    Layer &layer = state->layers[state->renaming_layer];
    if (state->rename_replace) {
        layer.name.fill('\0');
        state->rename_replace = false;
        return;
    }
    std::string_view name = layername(layer);
    if (name.empty()) {
        return;
    }
    layer.name[name.size() - 1U] = '\0';
}

void nameadd(GuiState *state, char c) {
    if (state->renaming_layer >= state->layers.size() || c < 32 || c > 126) {
        return;
    }
    Layer &layer = state->layers[state->renaming_layer];
    if (iscanvas(layer)) {
        layerdone(state);
        return;
    }
    if (state->rename_replace) {
        layer.name.fill('\0');
        state->rename_replace = false;
    }
    std::string_view name = layername(layer);
    if (name.size() + 1U >= kLayerNameBytes) {
        return;
    }
    layer.name[name.size()] = c;
}

} // namespace

void layerdone(GuiState *state) {
    if (state->renaming_layer < state->layers.size() &&
        layername(state->layers[state->renaming_layer]).empty()) {
        genlayername(&state->layers[state->renaming_layer].name,
                                   state->layers[state->renaming_layer].id);
    }
    state->renaming_layer = kNoLayer;
    state->rename_replace = false;
}

void layerlayout(GuiState *state) {
    addhit(state, state->layout.layers, HitKind::kSidebar, 0, 20);

    for (usize index = 0; index < state->layers.size(); ++index) {
        const f32 row_y = state->layout.layerrows.y + static_cast<f32>(index) * 28.0F;
        const Rect row = {
            .x = state->layout.layerrows.x,
            .y = row_y,
            .width = state->layout.layerrows.width,
            .height = 25.0F,
        };
        const Rect visible = {
            .x = row.x + 5.0F,
            .y = row.y + 6.0F,
            .width = 7.0F,
            .height = 7.0F,
        };
        const Rect lock = {
            .x = row.x + 15.0F,
            .y = row.y + 5.0F,
            .width = 9.0F,
            .height = 9.0F,
        };
        const Rect opacity = {
            .x = row.x + 17.0F,
            .y = row.y + 18.0F,
            .width = std::max(1.0F, row.width - 24.0F),
            .height = 3.0F,
        };
        addhit(state, row, HitKind::kLayerRow, static_cast<u8>(index), 80);
        addhit(state, visible, HitKind::kLayerVisibility, static_cast<u8>(index), 95);
        addhit(state, lock, HitKind::kLayerLock, static_cast<u8>(index), 95);
        addhit(state, opacity, HitKind::kLayerOpacity, static_cast<u8>(index), 95);
    }
}

b8 layermouse(GuiState *state, HitRecord hit, i32 x) {
    if (hit.kind == HitKind::kLayerVisibility && hit.index < state->layers.size()) {
        layerdone(state);
        state->active_menu = kNoMenu;
        Layer &layer = state->layers[hit.index];
        setlayerflag(&layer, kLayerVisible, !layervisible(layer));
        return true;
    }
    if (hit.kind == HitKind::kLayerLock && hit.index < state->layers.size()) {
        layerdone(state);
        state->active_menu = kNoMenu;
        Layer &layer = state->layers[hit.index];
        if (!iscanvas(layer)) {
            setlayerflag(&layer, kLayerLocked, !layerlocked(layer));
        }
        return true;
    }
    if (hit.kind == HitKind::kLayerOpacity) {
        layerdone(state);
        state->active_menu = kNoMenu;
        state->setting_opacity = true;
        state->active_kind = HitKind::kLayerOpacity;
        state->active_index = hit.index;
        opacityat(state, hit.index, x);
        return true;
    }
    if (hit.kind == HitKind::kLayerRow) {
        layerdone(state);
        state->active_menu = kNoMenu;
        select_layer(state, hit.index);
        return true;
    }
    return false;
}

b8 layerkey(GuiState *state, Key key) {
    if (state->renaming_layer == kNoLayer) {
        return false;
    }
    if (key == Key::kEnter || key == Key::kEscape) {
        layerdone(state);
        return true;
    }
    if (key == Key::kBackspace) {
        nameback(state);
        return true;
    }
    return true;
}

b8 layertext(GuiState *state, char c) {
    if (state->renaming_layer == kNoLayer) {
        return false;
    }
    nameadd(state, c);
    return true;
}

void layermove(GuiState *state, i32 x) {
    if (state->setting_opacity) {
        opacityat(state, state->active_index, x);
    }
}

void layerdraw(const GuiState &state, DrawList *draws) {
    drawrect(draws, state.layout.layers, Tone::kBlack);
    drawstroke(draws, state.layout.layers, Tone::kWhite);
    drawtext(draws, "LAYERS", state.layout.layers.x + 7.0F, state.layout.layers.y + 7.0F,
              Tone::kWhite);

    for (usize index = 0; index < state.layers.size(); ++index) {
        const Layer &layer = state.layers[index];
        const f32 row_y = state.layout.layerrows.y + static_cast<f32>(index) * 28.0F;
        const Rect row = {
            .x = state.layout.layerrows.x,
            .y = row_y,
            .width = state.layout.layerrows.width,
            .height = 25.0F,
        };
        const b8 hot_row =
            (state.hot_kind == HitKind::kLayerRow || state.hot_kind == HitKind::kLayerVisibility ||
             state.hot_kind == HitKind::kLayerLock || state.hot_kind == HitKind::kLayerOpacity) &&
            state.hot_index == index;
        const b8 selected = layerselected(layer);
        const b8 inverted = selected || hot_row;
        if (selected || hot_row) {
            drawrect(draws, row, Tone::kWhite);
        }

        const Rect eye = {.x = row.x + 5.0F, .y = row.y + 6.0F, .width = 7.0F, .height = 7.0F};
        if (layervisible(layer)) {
            drawrect(draws, eye, inverted ? Tone::kBlack : Tone::kWhite);
        }
        drawstroke(draws, eye, inverted ? Tone::kBlack : Tone::kWhite);
        drawicon(draws, layerlocked(layer) ? Icon::kLockClosed : Icon::kLockOpen,
                  row.x + 15.0F, row.y + 5.0F, inverted ? Tone::kBlack : Tone::kWhite);
        layernametext(draws, layer, row.x + 27.0F, row.y + 4.0F,
                        inverted ? Tone::kBlack : Tone::kWhite);
        if (state.renaming_layer == index) {
            const f32 cursor_x = row.x + 27.0F + static_cast<f32>(layername(layer).size()) * 6.0F;
            drawrect(draws,
                      {.x = cursor_x, .y = row.y + 4.0F, .width = 1.0F, .height = 7.0F},
                      inverted ? Tone::kBlack : Tone::kWhite);
        }
        const Rect bar = {
            .x = row.x + 17.0F,
            .y = row.y + 18.0F,
            .width = std::max(1.0F, row.width - 24.0F),
            .height = 3.0F,
        };
        drawstroke(draws, bar, inverted ? Tone::kBlack : Tone::kWhite);
        drawrect(
            draws,
            {.x = bar.x,
             .y = bar.y,
             .width = std::max(1.0F, bar.width * (static_cast<f32>(layer.opacity_u8) / 255.0F)),
             .height = bar.height},
            inverted ? Tone::kBlack : Tone::kWhite);
    }
}

} // namespace mira::gui
