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

[[nodiscard]] Rect stamprect(PaintStamp stamp) {
    return {
        .x = stamp.x - 4.0F,
        .y = stamp.y - 4.0F,
        .width = 8.0F,
        .height = 8.0F,
    };
}

[[nodiscard]] Rect unionrect(Rect a, Rect b) {
    const f32 x0 = std::min(a.x, b.x);
    const f32 y0 = std::min(a.y, b.y);
    const f32 x1 = std::max(a.x + a.width, b.x + b.width);
    const f32 y1 = std::max(a.y + a.height, b.y + b.height);
    return {
        .x = x0,
        .y = y0,
        .width = x1 - x0,
        .height = y1 - y0,
    };
}

void mark(GuiState *state, f32 document_x, f32 document_y, ToolKind kind) {
    const Layer *layer = paintlayer(*state);
    if (!painttool(kind) || layer == nullptr || !in_document(*state, document_x, document_y)) {
        return;
    }
    const PaintStamp stamp = {
        .x = document_x,
        .y = document_y,
        .size = static_cast<f32>(stampsize(*state, kind)),
        .tone = static_cast<f32>(tone_value(painttone(kind))),
        .layer = static_cast<f32>(layer->texture_slot),
    };
    (void)state->paint_stamps.push(stamp);
    historymark(state, stamp);
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

void historyclear(GuiState *state) {
    state->strokes.clear();
    state->stroke_stamps.clear();
    state->stroke_cursor = 0;
    state->active_stroke_first = 0;
    state->active_stroke_count = 0;
    state->active_stroke_layer = 0;
    state->active_stroke_rect = {};
    state->recording_stroke = false;
    state->replay_strokes = false;
}

void historystart(GuiState *state, const Layer &layer) {
    if (state->stroke_cursor < state->strokes.size()) {
        const usize stamp_size = state->strokes[state->stroke_cursor].first_stamp;
        state->strokes.truncate(state->stroke_cursor);
        state->stroke_stamps.truncate(stamp_size);
    }

    state->active_stroke_first = static_cast<u32>(state->stroke_stamps.size());
    state->active_stroke_count = 0;
    state->active_stroke_layer = layer.id;
    state->active_stroke_rect = {};
    state->recording_stroke = state->strokes.size() < state->strokes.capacity();
    if (!state->recording_stroke) {
        state->strokes.overflowed = true;
    }
}

void historymark(GuiState *state, PaintStamp stamp) {
    if (!state->recording_stroke) {
        return;
    }
    if (!state->stroke_stamps.push(stamp)) {
        state->stroke_stamps.truncate(state->active_stroke_first);
        state->active_stroke_count = 0;
        state->recording_stroke = false;
        return;
    }

    const Rect bounds = stamprect(stamp);
    state->active_stroke_rect =
        state->active_stroke_count == 0 ? bounds : unionrect(state->active_stroke_rect, bounds);
    ++state->active_stroke_count;
}

void historyfinish(GuiState *state) {
    if (!state->recording_stroke) {
        state->recording_stroke = false;
        return;
    }
    if (state->active_stroke_count == 0) {
        state->recording_stroke = false;
        return;
    }
    const StrokeAction action = {
        .layer_id = state->active_stroke_layer,
        .first_stamp = state->active_stroke_first,
        .stamp_count = state->active_stroke_count,
        .affected = state->active_stroke_rect,
    };
    if (!state->strokes.push(action)) {
        state->stroke_stamps.truncate(state->active_stroke_first);
        state->recording_stroke = false;
        return;
    }
    state->stroke_cursor = static_cast<u32>(state->strokes.size());
    state->recording_stroke = false;
}

b8 historyundo(GuiState *state) {
    if (state->stroke_cursor == 0) {
        return false;
    }
    --state->stroke_cursor;
    state->replay_strokes = true;
    return true;
}

b8 historyredo(GuiState *state) {
    if (state->stroke_cursor >= state->strokes.size()) {
        return false;
    }
    ++state->stroke_cursor;
    state->replay_strokes = true;
    return true;
}

void historyreplay(GuiState *state) {
    if (!state->replay_strokes) {
        return;
    }

    for (const StrokeAction action : state->strokes.span()) {
        if (action.stamp_count == 0 || action.first_stamp >= state->stroke_stamps.size()) {
            continue;
        }
        const PaintStamp stamp = state->stroke_stamps[action.first_stamp];
        markclear(state, static_cast<u8>(stamp.layer + 0.5F));
    }

    for (usize action_index = 0; action_index < state->stroke_cursor; ++action_index) {
        const StrokeAction action = state->strokes[action_index];
        for (u32 stamp_index = 0; stamp_index < action.stamp_count; ++stamp_index) {
            const usize index = static_cast<usize>(action.first_stamp) + stamp_index;
            if (index >= state->stroke_stamps.size()) {
                break;
            }
            (void)state->paint_stamps.push(state->stroke_stamps[index]);
        }
    }
    state->replay_strokes = false;
}

void worklayout(GuiState *state) {
    if (!state->view_initialized) {
        center_document(state);
    } else {
        update_document_rect(state);
    }
    addhit(state, state->layout.viewport, HitKind::kViewport, 0, 10);
}

b8 workwheel(GuiState *state, i32 x, i32 y, i32 dx, i32 dy, u8 mods, HitKind hit_kind) {
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
    const Layer *layer = paintlayer(*state);
    if (painttool(kind) && layer != nullptr) {
        const f32 document_x = screen_to_paint_x(*state, x);
        const f32 document_y = screen_to_paint_y(*state, y);
        if (in_document(*state, document_x, document_y)) {
            state->painting = true;
            state->last_paint_x = document_x;
            state->last_paint_y = document_y;
            historystart(state, *layer);
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
    if (!state->painting) {
        return;
    }
    if (contains(state->layout.viewport, x, y)) {
        const ToolKind kind = toolkind(*state);
        const f32 document_x = screen_to_paint_x(*state, x);
        const f32 document_y = screen_to_paint_y(*state, y);
        markline(state, state->last_paint_x, state->last_paint_y, document_x, document_y, kind);
    }
    historyfinish(state);
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
            drawicon(draws, brushicon(stampsize(state, active_tool)), preview_x, preview_y,
                     Tone::kBlack, state.view.zoom);
        }
    }

    const Rect visible_document = intersectrect(state.layout.viewport, state.layout.document);
    if (!empty(visible_document)) {
        drawrect(draws,
                 {.x = state.layout.viewport.x,
                  .y = state.layout.viewport.y,
                  .width = state.layout.viewport.width,
                  .height = visible_document.y - state.layout.viewport.y},
                 Tone::kLight);
        drawrect(draws,
                 {.x = state.layout.viewport.x,
                  .y = visible_document.y + visible_document.height,
                  .width = state.layout.viewport.width,
                  .height = (state.layout.viewport.y + state.layout.viewport.height) -
                            (visible_document.y + visible_document.height)},
                 Tone::kLight);
        drawrect(draws,
                 {.x = state.layout.viewport.x,
                  .y = visible_document.y,
                  .width = visible_document.x - state.layout.viewport.x,
                  .height = visible_document.height},
                 Tone::kLight);
        drawrect(draws,
                 {.x = visible_document.x + visible_document.width,
                  .y = visible_document.y,
                  .width = (state.layout.viewport.x + state.layout.viewport.width) -
                           (visible_document.x + visible_document.width),
                  .height = visible_document.height},
                 Tone::kLight);
    } else {
        drawrect(draws, state.layout.viewport, Tone::kLight);
    }
    drawstroke(draws, state.layout.viewport, Tone::kBlack);
    const Rect tag = {
        .x = state.layout.viewport.x + 1.0F,
        .y = state.layout.viewport.y + 1.0F,
        .width = std::max(1.0F, state.layout.viewport.width - 2.0F),
        .height = 21.0F,
    };
    drawrect(draws, tag, Tone::kWhite);
    drawrect(draws,
             {.x = state.layout.viewport.x,
              .y = state.layout.viewport.y + 22.0F,
              .width = state.layout.viewport.width,
              .height = 1.0F},
             Tone::kBlack);
    drawtext(draws, "canvas", state.layout.viewport.x + 6.0F, state.layout.viewport.y + 5.0F,
             Tone::kBlack);
    if (!empty(visible_document)) {
        drawstroke(draws, visible_document, Tone::kBlack);
    }
}

} // namespace mira::gui
