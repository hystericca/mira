#include "private.hpp"

namespace mira {
namespace impl = gui;

void guilayout(GuiState *state, Screen screen) {
    if (!state->initialized) {
        guiinit(state);
    }

    const f32 width = static_cast<f32>(std::max(1, screen.width));
    const f32 height = static_cast<f32>(std::max(1, screen.height));
    const f32 menu_height = std::min(18.0F, height);
    const f32 toolbar_width = std::min(92.0F, std::max(84.0F, width * 0.15F));
    const f32 layers_width = std::min(160.0F, std::max(124.0F, width * 0.25F));
    const f32 viewport_x = toolbar_width;
    const f32 viewport_width = std::max(1.0F, width - toolbar_width - layers_width);
    const f32 layers_x = viewport_x + viewport_width;
    const f32 body_height = std::max(1.0F, height - menu_height);

    state->layout = {
        .window = {.x = 0.0F, .y = 0.0F, .width = width, .height = height},
        .menu_bar = {.x = 0.0F, .y = 0.0F, .width = width, .height = menu_height},
        .toolbar = {.x = 0.0F, .y = menu_height, .width = toolbar_width, .height = body_height},
        .tools = {.x = 8.0F,
                  .y = menu_height + 8.0F,
                  .width = 20.0F,
                  .height = std::max(1.0F, body_height - 16.0F)},
        .tips = {.x = 36.0F,
                 .y = menu_height + 8.0F,
                 .width = 20.0F,
                 .height = std::max(1.0F, body_height - 16.0F)},
        .brush_button = {.x = 64.0F,
                         .y = menu_height + 8.0F,
                         .width = 20.0F,
                         .height = 21.0F},
        .brush_panel = {},
        .coverages = {},
        .viewport = {.x = viewport_x,
                     .y = menu_height,
                     .width = viewport_width,
                     .height = body_height},
        .document = {},
        .layers = {.x = layers_x, .y = menu_height, .width = layers_width, .height = body_height},
        .layerrows = {.x = layers_x + 7.0F,
                      .y = menu_height + 30.0F,
                      .width = std::max(1.0F, layers_width - 14.0F),
                      .height = std::max(1.0F, body_height - 37.0F)},
    };

    state->hits.clear();
    impl::worklayout(state);
    impl::toollayout(state);
    impl::layerlayout(state);
    impl::menulayout(state);
    impl::contextlayout(state);
    impl::dialoglayout(state);
}

HitRecord guihit(const GuiState &state, i32 x, i32 y) {
    HitRecord result = {};
    for (const HitRecord hit : state.hits.span()) {
        if (impl::contains(hit.rect, x, y) && hit.priority >= result.priority) {
            result = hit;
        }
    }
    return result;
}

void guievent(GuiState *state, std::span<const InputEvent> input) {
    if (!state->initialized) {
        guiinit(state);
    }

    for (const InputEvent event : input) {
        state->mouse_x = event.x;
        state->mouse_y = event.y;
        const HitRecord hit = guihit(*state, event.x, event.y);
        state->hot_kind = hit.kind;
        state->hot_index = hit.index;

        if (state->about_dialog || state->new_dialog) {
            if (event.kind == InputKind::kKeyDown) {
                (void)impl::dialogkey(state, static_cast<Key>(event.button));
            } else if (event.kind == InputKind::kText) {
                (void)impl::dialogtext(state, static_cast<char>(event.dx));
            } else if (event.kind == InputKind::kMouseDown) {
                state->active_kind = hit.kind;
                state->active_index = hit.index;
                (void)impl::dialogmouse(state, hit);
            } else if (event.kind == InputKind::kMouseUp) {
                state->active_kind = HitKind::kNone;
                state->active_index = 0;
            }
        } else if (state->context_open) {
            if (event.kind == InputKind::kKeyDown) {
                (void)impl::contextkey(state, static_cast<Key>(event.button));
            } else if (event.kind == InputKind::kMouseDown) {
                state->active_kind = hit.kind;
                state->active_index = hit.index;
                if (event.button == 2) {
                    impl::contextopen(state, hit, event.x, event.y);
                } else {
                    (void)impl::contextmouse(state, hit);
                }
            } else if (event.kind == InputKind::kMouseUp) {
                state->active_kind = HitKind::kNone;
                state->active_index = 0;
            }
        } else if (event.kind == InputKind::kWheel) {
            (void)impl::workwheel(state, event.x, event.y, event.dx, event.dy, event.mods,
                                  hit.kind);
        } else if (event.kind == InputKind::kKeyDown) {
            const auto key = static_cast<Key>(event.button);
            if (impl::toolkey(state, key)) {
                continue;
            }
            if (!impl::layerkey(state, key)) {
                if (key == Key::kUndo) {
                    (void)impl::historyundo(state);
                } else if (key == Key::kRedo) {
                    (void)impl::historyredo(state);
                } else if (key == Key::kDelete) {
                    (void)layerdel(state);
                }
            }
        } else if (event.kind == InputKind::kText) {
            (void)impl::layertext(state, static_cast<char>(event.dx));
        } else if (event.kind == InputKind::kMouseDown) {
            impl::workcancel(state);
            state->setting_opacity = false;
            state->active_kind = hit.kind;
            state->active_index = hit.index;

            if (event.button == 2) {
                impl::contextopen(state, hit, event.x, event.y);
                continue;
            }

            if (impl::toolmodalmouse(state, hit)) {
                continue;
            }

            if (impl::workmouse(state, hit, event.x, event.y, event.button) ||
                impl::menumouse(state, hit) || impl::layermouse(state, hit, event.x) ||
                impl::toolmouse(state, hit)) {
                continue;
            }

            impl::layerdone(state);
            state->active_menu = kNoMenu;
        } else if (event.kind == InputKind::kMouseMove) {
            impl::workmove(state, event.x, event.y, event.buttons);
            impl::layermove(state, event.x);
        } else if (event.kind == InputKind::kMouseUp) {
            impl::workup(state, event.x, event.y);
            state->painting = false;
            state->panning = false;
            state->setting_opacity = false;
            state->active_kind = HitKind::kNone;
            state->active_index = 0;
        }
    }
}

void guidraw(const GuiState &state, DrawList *draws) {
    draws->clear();
    impl::workdraw(state, draws);
    impl::tooldraw(state, draws);
    impl::layerdraw(state, draws);
    impl::menudraw(state, draws);
    impl::toolpopupdraw(state, draws);
    impl::contextdraw(state, draws);
    impl::dialogdraw(state, draws);
}

void guiframe(GuiState *state, Screen screen, std::span<const InputEvent> input, DrawList *draws) {
    state->paint_stamps.clear();
    state->clear_slots.clear();
    guilayout(state, screen);
    guievent(state, input);
    if (!input.empty()) {
        guilayout(state, screen);
    }
    impl::tooltick(state);
    impl::historyreplay(state);
    impl::update_document_rect(state);
    guidraw(*state, draws);
}

b8 guianimating(const GuiState &state) { return impl::toolanimating(state); }

} // namespace mira
