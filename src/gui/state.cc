#include "private.hpp"

namespace mira {
namespace impl = gui;

void guiinit(GuiState *state) {
    state->layers.clear();
    state->tools.clear();
    state->sizes.clear();
    state->paint_stamps.clear();
    state->strokes.clear();
    state->stroke_stamps.clear();
    state->clear_slots.clear();
    state->hits.clear();
    state->mouse_x = -1;
    state->mouse_y = -1;
    state->hot_kind = HitKind::kNone;
    state->active_kind = HitKind::kNone;
    state->hot_index = 0;
    state->active_index = 0;
    state->curlayer = 0;
    state->curtool = 0;
    state->cursize = 3;
    state->active_menu = kNoMenu;
    state->renaming_layer = kNoLayer;
    state->next_layer_id = 1;
    state->stroke_cursor = 0;
    state->active_stroke_first = 0;
    state->active_stroke_count = 0;
    state->active_stroke_layer = 0;
    state->active_stroke_rect = {};
    state->document = {.width = 320, .height = 240};
    state->last_paint_x = 0.0F;
    state->last_paint_y = 0.0F;
    state->last_pan_x = 0;
    state->last_pan_y = 0;
    state->view.x = 0.0F;
    state->view.y = 0.0F;
    state->view.zoom = 1.0F;
    state->view_initialized = false;
    state->painting = false;
    state->panning = false;
    state->setting_opacity = false;
    state->rename_replace = false;
    state->recording_stroke = false;
    state->replay_strokes = false;

    impl::pushtool(state, 1, "pen", ToolKind::kPen, true);
    impl::pushtool(state, 2, "brush", ToolKind::kBrush, false);
    impl::pushtool(state, 3, "line", ToolKind::kLine, false);
    impl::pushtool(state, 4, "magic", ToolKind::kMagic, false);
    impl::pushtool(state, 5, "rect", ToolKind::kRect, false);
    impl::pushtool(state, 6, "zoom", ToolKind::kZoom, false);
    impl::pushtool(state, 7, "erase", ToolKind::kErase, false);
    for (u8 index = 0; index < 8; ++index) {
        impl::pushsize(state, index, index == state->cursize);
    }

    impl::pushlayer(state, state->next_layer_id, "ink", LayerKind::kInk, 255, true, false, true, 0,
                    false);
    ++state->next_layer_id;
    impl::pushlayer(state, state->next_layer_id, "background", LayerKind::kBackground, 255, true,
                    true, false, kBackgroundTextureSlot, true);
    ++state->next_layer_id;
    state->initialized = true;
}

void docnew(GuiState *state, i32 width, i32 height) {
    if (state == nullptr) {
        return;
    }
    guiinit(state);
    state->document = {
        .width = std::clamp(width, 1, 4096),
        .height = std::clamp(height, 1, 4096),
    };
    state->view_initialized = false;
    state->clear_slots.clear();
    for (u8 slot = 0; slot < kMaxLayers; ++slot) {
        impl::markclear(state, slot);
    }
}

} // namespace mira
