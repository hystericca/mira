#include "private.hpp"

namespace mira::gui {
namespace {

[[nodiscard]] auto number(std::array<char, 8> *out, i32 value) -> std::string_view {
    out->fill('\0');
    value = std::max(0, value);
    std::array<char, 8> digits = {};
    usize count = 0;
    do {
        digits[count] = static_cast<char>('0' + (value % 10));
        value /= 10;
        ++count;
    } while (value != 0 && count < digits.size());

    usize index = 0;
    while (count != 0 && index < out->size()) {
        --count;
        (*out)[index] = digits[count];
        ++index;
    }
    return {out->data(), index};
}

[[nodiscard]] auto fieldvalue(GuiState *state) -> i32 * {
    return state->new_field == 0 ? &state->new_width : &state->new_height;
}

[[nodiscard]] auto dialogactive(const GuiState &state) -> b8 {
    return state.about_dialog || state.new_dialog;
}

void closedialog(GuiState *state) {
    state->about_dialog = false;
    state->new_dialog = false;
}

void accept(GuiState *state) { docnew(state, state->new_width, state->new_height); }

void drawfield(const GuiState &state, DrawList *draws, Rect field, u8 index, i32 value) {
    const b8 active = state.new_field == index;
    drawrect(draws, field, active ? Tone::kBlack : Tone::kWhite);
    drawstroke(draws, field, Tone::kBlack);

    std::array<char, 8> text = {};
    const std::string_view value_text = number(&text, value);
    drawtext(draws, value_text, field.x + 5.0F, field.y + 3.0F,
             active ? Tone::kWhite : Tone::kBlack);
}

void drawbutton(const GuiState &state, DrawList *draws, Rect rect, u8 index,
                std::string_view text) {
    const b8 hot = state.hot_kind == HitKind::kDialogButton && state.hot_index == index;
    drawrect(draws, rect, hot ? Tone::kBlack : Tone::kWhite);
    drawstroke(draws, rect, Tone::kBlack);
    drawtext(draws, text, rect.x + 5.0F, rect.y + 3.0F, hot ? Tone::kWhite : Tone::kBlack);
}

} // namespace

void aboutopen(GuiState *state) {
    layerdone(state);
    state->active_menu = kNoMenu;
    state->about_dialog = true;
    state->new_dialog = false;
}

void dialogopen(GuiState *state) {
    layerdone(state);
    state->active_menu = kNoMenu;
    state->about_dialog = false;
    state->new_dialog = true;
    state->new_replace = true;
    state->new_field = 0;
    state->new_width = state->document.width;
    state->new_height = state->document.height;
}

void dialoglayout(GuiState *state) {
    if (!dialogactive(*state)) {
        return;
    }

    if (state->about_dialog) {
        const f32 width = std::min(248.0F, std::max(160.0F, state->layout.window.width - 12.0F));
        const f32 height = 116.0F;
        const f32 x = std::floor((state->layout.window.width - width) * 0.5F);
        const f32 y = std::floor((state->layout.window.height - height) * 0.5F);
        state->layout.dialog = {.x = x, .y = y, .width = width, .height = height};
        state->layout.dialog_logo = {
            .x = x + 14.0F,
            .y = y + 32.0F,
            .width = 42.0F,
            .height = 42.0F,
        };
        state->layout.dialog_ok = {
            .x = x + width - 46.0F,
            .y = y + height - 27.0F,
            .width = 32.0F,
            .height = 17.0F,
        };

        addhit(state, state->layout.dialog, HitKind::kDialog, 0, 180);
        addhit(state, state->layout.dialog_ok, HitKind::kDialogButton, 0, 220);
        return;
    }

    const f32 width = std::min(176.0F, std::max(120.0F, state->layout.window.width - 12.0F));
    const f32 height = 104.0F;
    const f32 x = std::floor((state->layout.window.width - width) * 0.5F);
    const f32 y = std::floor((state->layout.window.height - height) * 0.5F);
    state->layout.dialog = {.x = x, .y = y, .width = width, .height = height};
    state->layout.dialog_width = {.x = x + 58.0F, .y = y + 28.0F, .width = 58.0F, .height = 16.0F};
    state->layout.dialog_height = {
        .x = x + 58.0F,
        .y = y + 50.0F,
        .width = 58.0F,
        .height = 16.0F,
    };
    state->layout.dialog_ok = {.x = x + 58.0F, .y = y + 76.0F, .width = 32.0F, .height = 17.0F};
    state->layout.dialog_cancel = {
        .x = x + 96.0F,
        .y = y + 76.0F,
        .width = 50.0F,
        .height = 17.0F,
    };

    addhit(state, state->layout.dialog, HitKind::kDialog, 0, 180);
    addhit(state, state->layout.dialog_width, HitKind::kDialogField, 0, 220);
    addhit(state, state->layout.dialog_height, HitKind::kDialogField, 1, 220);
    addhit(state, state->layout.dialog_ok, HitKind::kDialogButton, 0, 220);
    addhit(state, state->layout.dialog_cancel, HitKind::kDialogButton, 1, 220);
}

b8 dialogmouse(GuiState *state, Hit hit) {
    if (!dialogactive(*state)) {
        return false;
    }
    if (state->about_dialog) {
        if (hit.kind == HitKind::kDialogButton) {
            closedialog(state);
        }
        return true;
    }
    if (hit.kind == HitKind::kDialogField) {
        state->new_field = hit.index > 1 ? 0 : hit.index;
        state->new_replace = true;
        return true;
    }
    if (hit.kind == HitKind::kDialogButton) {
        if (hit.index == 0) {
            accept(state);
        } else {
            closedialog(state);
        }
        return true;
    }
    return true;
}

b8 dialogkey(GuiState *state, Key key) {
    if (!dialogactive(*state)) {
        return false;
    }
    if (state->about_dialog) {
        if (key == Key::kEscape || key == Key::kEnter) {
            closedialog(state);
        }
        return true;
    }
    if (key == Key::kEscape) {
        closedialog(state);
        return true;
    }
    if (key == Key::kTab) {
        state->new_field = state->new_field == 0 ? 1 : 0;
        state->new_replace = true;
        return true;
    }
    if (key == Key::kEnter) {
        accept(state);
        return true;
    }
    if (key == Key::kBackspace) {
        i32 *value = fieldvalue(state);
        *value /= 10;
        state->new_replace = false;
        return true;
    }
    return true;
}

b8 dialogtext(GuiState *state, char c) {
    if (!dialogactive(*state)) {
        return false;
    }
    if (state->about_dialog) {
        return true;
    }
    if (c < '0' || c > '9') {
        return true;
    }
    i32 *value = fieldvalue(state);
    if (state->new_replace) {
        *value = 0;
        state->new_replace = false;
    }
    *value = std::clamp((*value * 10) + static_cast<i32>(c - '0'), 0, 4096);
    return true;
}

void dialogdraw(const GuiState &state, DrawList *draws) {
    if (!dialogactive(state)) {
        return;
    }

    drawplane(draws, DrawPlane::kModal);
    if (state.about_dialog) {
        drawrect(draws, state.layout.dialog, Tone::kWhite);
        drawstroke(draws, state.layout.dialog, Tone::kBlack);
        drawrect(draws,
                 {.x = state.layout.dialog.x,
                  .y = state.layout.dialog.y,
                  .width = state.layout.dialog.width,
                  .height = 18.0F},
                 Tone::kBlack);
        drawtext(draws, "about", state.layout.dialog.x + 5.0F, state.layout.dialog.y + 4.0F,
                 Tone::kWhite);
        drawrect(draws, state.layout.dialog_logo, Tone::kBlack);
        const f32 text_x = state.layout.dialog_logo.x + state.layout.dialog_logo.width + 11.0F;
        const f32 text_y = state.layout.dialog_logo.y + 1.0F;
        drawtext(draws, "THIS PROJECT IS", text_x, text_y, Tone::kBlack);
        drawtext(draws, "PERSONAL", text_x, text_y + 12.0F, Tone::kBlack);
        drawtext(draws, "DONT BOTHER ME", text_x, text_y + 24.0F, Tone::kBlack);
        drawtext(draws, "I LOVE MY COMPUTER", text_x, text_y + 36.0F, Tone::kBlack);
        drawbutton(state, draws, state.layout.dialog_ok, 0, "ok");
        return;
    }

    drawrect(draws, state.layout.dialog, Tone::kWhite);
    drawstroke(draws, state.layout.dialog, Tone::kBlack);
    drawrect(draws,
             {.x = state.layout.dialog.x,
              .y = state.layout.dialog.y,
              .width = state.layout.dialog.width,
              .height = 18.0F},
             Tone::kBlack);
    drawtext(draws, "new document", state.layout.dialog.x + 5.0F, state.layout.dialog.y + 4.0F,
             Tone::kWhite);
    drawtext(draws, "w", state.layout.dialog.x + 18.0F, state.layout.dialog_width.y + 4.0F,
             Tone::kBlack);
    drawtext(draws, "h", state.layout.dialog.x + 18.0F, state.layout.dialog_height.y + 4.0F,
             Tone::kBlack);
    drawfield(state, draws, state.layout.dialog_width, 0, state.new_width);
    drawfield(state, draws, state.layout.dialog_height, 1, state.new_height);
    drawbutton(state, draws, state.layout.dialog_ok, 0, "ok");
    drawbutton(state, draws, state.layout.dialog_cancel, 1, "cancel");
}

} // namespace mira::gui
