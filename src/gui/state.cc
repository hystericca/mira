#include "private.hpp"

namespace mira {
namespace impl = gui;

void guiinit(GuiState *state) {
    state->layers.clear();
    state->tools.clear();
    state->sizes.clear();
    state->paint_stamps.clear();
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

    impl::pushtool(state, 1, "Pen", ToolKind::kPen, true);
    impl::pushtool(state, 2, "Brush", ToolKind::kBrush, false);
    impl::pushtool(state, 3, "Line", ToolKind::kLine, false);
    impl::pushtool(state, 4, "Magic", ToolKind::kMagic, false);
    impl::pushtool(state, 5, "Rect", ToolKind::kRect, false);
    impl::pushtool(state, 6, "Zoom", ToolKind::kZoom, false);
    impl::pushtool(state, 7, "Erase", ToolKind::kErase, false);
    for (u8 index = 0; index < 8; ++index) {
        impl::pushsize(state, index, index == state->cursize);
    }

    impl::pushlayer(state, state->next_layer_id, "Ink", LayerKind::kInk, 255, true, false, true,
                     0, false);
    ++state->next_layer_id;
    impl::pushlayer(state, state->next_layer_id, "Canvas", LayerKind::kPaper, 255, true, true,
                     false, kCanvasTextureSlot, true);
    ++state->next_layer_id;
    state->initialized = true;
}

} // namespace mira
