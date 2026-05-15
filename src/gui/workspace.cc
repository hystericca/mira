#include "private.hpp"

namespace mira::gui {
namespace {

void zoomat(GuiState *state, i32 x, i32 y, i32 wheel_delta) {
    if (!contains(state->layout.viewport, x, y) || wheel_delta == 0) {
        return;
    }
    const f32 document_x = screen_to_document_x(*state, static_cast<f32>(x));
    const f32 document_y = screen_to_document_y(*state, static_cast<f32>(y));
    const f32 factor = wheel_delta < 0 ? 1.125F : 1.0F / 1.125F;
    state->view.zoom = clamp_zoom(state->view.zoom * factor);
    state->view.x =
        document_x - ((static_cast<f32>(x) - state->layout.viewport.x) / state->view.zoom);
    state->view.y =
        document_y - ((static_cast<f32>(y) - state->layout.viewport.y) / state->view.zoom);
}

void scrollview(GuiState *state, i32 dx, i32 dy) {
    state->view.x += static_cast<f32>(dx) / state->view.zoom;
    state->view.y += static_cast<f32>(dy) / state->view.zoom;
}

void panview(GuiState *state, i32 dx, i32 dy) {
    state->view.x -= static_cast<f32>(dx) / state->view.zoom;
    state->view.y -= static_cast<f32>(dy) / state->view.zoom;
}

void mark(GuiState *state, f32 document_x, f32 document_y, ToolKind kind) {
    const Layer *layer = paintlayer(*state);
    if (!painttool(kind) || layer == nullptr || !in_document(*state, document_x, document_y)) {
        return;
    }
    (void)state->paint_stamps.push({
        .x = document_x,
        .y = document_y,
        .size = static_cast<f32>(stampsize(*state, kind)),
        .tone = static_cast<f32>(tone_value(painttone(kind))),
        .layer = static_cast<f32>(layer->texture_slot),
    });
}

void markline(GuiState *state, f32 x0, f32 y0, f32 x1, f32 y1, ToolKind kind) {
    if (!painttool(kind)) {
        return;
    }
    const f32 dx = x1 - x0;
    const f32 dy = y1 - y0;
    const i32 steps = static_cast<i32>(std::max(f32abs(dx), f32abs(dy)));
    if (steps == 0) {
        return;
    }
    for (i32 step = 1; step <= steps; ++step) {
        const f32 t = static_cast<f32>(step) / static_cast<f32>(steps);
        mark(state, x0 + (dx * t), y0 + (dy * t), kind);
    }
}

} // namespace

void worklayout(GuiState *state) {
    if (!state->view_initialized) {
        center_document(state);
    } else {
        update_document_rect(state);
    }
    addhit(state, state->layout.viewport, HitKind::kViewport, 0, 10);
}

b8 workwheel(GuiState *state, i32 x, i32 y, i32 dx, i32 dy, u8 mods,
                          HitKind hit_kind) {
    if ((mods & kInputCtrl) != 0) {
        zoomat(state, x, y, dy);
        return true;
    }
    if (hit_kind == HitKind::kViewport) {
        const b8 shift_scroll = (mods & kInputShift) != 0 && dx == 0;
        scrollview(state, shift_scroll ? dy : dx, shift_scroll ? 0 : dy);
        return true;
    }
    return false;
}

b8 workmouse(GuiState *state, HitRecord hit, i32 x, i32 y, u8 button) {
    if (button == 1 && hit.kind == HitKind::kViewport) {
        layerdone(state);
        state->active_menu = kNoMenu;
        state->panning = true;
        state->last_pan_x = static_cast<i16>(x);
        state->last_pan_y = static_cast<i16>(y);
        return true;
    }
    if (hit.kind != HitKind::kViewport) {
        return false;
    }

    layerdone(state);
    state->active_menu = kNoMenu;
    const ToolKind kind = toolkind(*state);
    if (painttool(kind) && paintlayer(*state) != nullptr) {
        const f32 document_x = screen_to_paint_x(*state, x);
        const f32 document_y = screen_to_paint_y(*state, y);
        if (in_document(*state, document_x, document_y)) {
            state->painting = true;
            state->last_paint_x = document_x;
            state->last_paint_y = document_y;
            mark(state, document_x, document_y, kind);
        }
    }
    return true;
}

void workmove(GuiState *state, i32 x, i32 y) {
    if (state->panning) {
        panview(state, x - state->last_pan_x, y - state->last_pan_y);
        state->last_pan_x = static_cast<i16>(x);
        state->last_pan_y = static_cast<i16>(y);
        return;
    }
    if (!state->painting || !contains(state->layout.viewport, x, y)) {
        return;
    }
    const ToolKind kind = toolkind(*state);
    const f32 document_x = screen_to_paint_x(*state, x);
    const f32 document_y = screen_to_paint_y(*state, y);
    markline(state, state->last_paint_x, state->last_paint_y, document_x, document_y, kind);
    state->last_paint_x = document_x;
    state->last_paint_y = document_y;
}

void workup(GuiState *state, i32 x, i32 y) {
    if (!state->painting || !contains(state->layout.viewport, x, y)) {
        return;
    }
    const ToolKind kind = toolkind(*state);
    const f32 document_x = screen_to_paint_x(*state, x);
    const f32 document_y = screen_to_paint_y(*state, y);
    markline(state, state->last_paint_x, state->last_paint_y, document_x, document_y, kind);
}

void workdraw(const GuiState &state, DrawList *draws) {
    const ToolKind active_tool = toolkind(state);
    if (state.hot_kind == HitKind::kViewport && painttool(active_tool)) {
        const f32 document_x = screen_to_paint_x(state, state.mouse_x);
        const f32 document_y = screen_to_paint_y(state, state.mouse_y);
        const f32 preview_x = document_to_screen_x(state, document_x) - (4.0F * state.view.zoom);
        const f32 preview_y = document_to_screen_y(state, document_y) - (4.0F * state.view.zoom);
        const f32 preview_size = 8.0F * state.view.zoom;
        const Rect preview = {
            .x = preview_x,
            .y = preview_y,
            .width = preview_size,
            .height = preview_size,
        };
        if (paintlayer(state) != nullptr && in_document(state, document_x, document_y) &&
            containsrect(state.layout.viewport, preview) &&
            containsrect(state.layout.document, preview)) {
            drawicon(draws, brushicon(stampsize(state, active_tool)), preview_x,
                      preview_y, Tone::kBlack, state.view.zoom);
        }
    }

    drawstroke(draws, state.layout.viewport, Tone::kWhite);
    const Rect visible_document = intersectrect(state.layout.viewport, state.layout.document);
    if (!empty(visible_document)) {
        drawstroke(draws, visible_document, Tone::kWhite);
    }
}

} // namespace mira::gui
