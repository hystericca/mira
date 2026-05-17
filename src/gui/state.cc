#include "private.hpp"

namespace mira {
namespace impl = gui;

void guiinit(GuiState *state) {
    state->layers.clear();
    state->tools.clear();
    state->tips.clear();
    state->sizes.clear();
    state->coverages.clear();
    state->paint_stamps.clear();
    state->draft_stamps.clear();
    state->strokes.clear();
    state->stroke_stamps.clear();
    state->clear_slots.clear();
    state->hits.clear();
    state->mouse_x = -1;
    state->mouse_y = -1;
    state->new_width = kDefaultDocumentWidth;
    state->new_height = kDefaultDocumentHeight;
    state->hot_kind = HitKind::kNone;
    state->active_kind = HitKind::kNone;
    state->hot_index = 0;
    state->active_index = 0;
    state->curlayer = 0;
    state->curtool = 0;
    state->curtip = 3;
    state->cursize = 3;
    state->curcoverage = 0;
    state->new_field = 0;
    state->active_menu = kNoMenu;
    state->context_target = kNoLayer;
    state->renaming_layer = kNoLayer;
    state->context_x = 0;
    state->context_y = 0;
    state->next_layer_id = 1;
    state->stroke_cursor = 0;
    state->active_paint_first = 0;
    state->active_stroke_first = 0;
    state->active_stroke_count = 0;
    state->active_stroke_layer = 0;
    state->active_stroke_rect = {};
    state->draft_start_x = 0.0F;
    state->draft_start_y = 0.0F;
    state->draft_x = 0.0F;
    state->draft_y = 0.0F;
    state->document = {.width = kDefaultDocumentWidth, .height = kDefaultDocumentHeight};
    state->last_paint_x = 0.0F;
    state->last_paint_y = 0.0F;
    state->brush_t = 0.0F;
    state->last_pan_x = 0;
    state->last_pan_y = 0;
    state->view.x = 0.0F;
    state->view.y = 0.0F;
    state->view.zoom = kInitialViewZoom;
    state->view_initialized = false;
    state->painting = false;
    state->panning = false;
    state->setting_opacity = false;
    state->rename_replace = false;
    state->context_open = false;
    state->brush_open = false;
    state->about_dialog = false;
    state->new_dialog = false;
    state->new_replace = false;
    state->document_changed = false;
    state->export_requested = false;
    state->recording_stroke = false;
    state->replay_strokes = false;
    state->draft_active = false;
    state->draft_kind = ToolKind::kPen;
    state->draft_texture_slot = 0;

    for (usize index = 0; index < kToolDefs.size(); ++index) {
        impl::pushtool(state, static_cast<u32>(index + 1U), impl::kToolNames[index],
                       kToolDefs[index].kind, index == 0);
    }
    for (u8 index = 0; index < 8; ++index) {
        impl::pushtip(state, index, index == state->curtip);
    }
    for (u8 index = 0; index < 8; ++index) {
        impl::pushsize(state, index, index == state->cursize);
    }
    for (u8 index = 0; index < kMaxCoverages; ++index) {
        impl::pushcoverage(state, index, index == state->curcoverage);
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
    state->document_changed = true;
}

} // namespace mira
