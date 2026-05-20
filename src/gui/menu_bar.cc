#include "private.hpp"

namespace mira {
namespace impl = gui;

MenuAction menuaction(const GuiState &state, HitRecord hit) {
    if (hit.kind == HitKind::kContextAction) {
        const std::span<const impl::MenuCommand> commands =
            impl::contextcommands(contextkind(state));
        if (hit.index >= commands.size()) {
            return MenuAction::kNone;
        }
        return commands[hit.index].action;
    }
    if (hit.kind != HitKind::kMenuAction) {
        return MenuAction::kNone;
    }
    const std::span<const impl::MenuCommand> commands = impl::menucommands(state.active_menu);
    if (hit.index >= commands.size()) {
        return MenuAction::kNone;
    }
    return commands[hit.index].action;
}

} // namespace mira

namespace mira::gui {

void doaction(GuiState *state, MenuAction action) {
    switch (action) {
    case MenuAction::kNone:
        break;
    case MenuAction::kMiraAbout:
        aboutopen(state);
        break;
    case MenuAction::kUndo:
        (void)historyundo(state);
        break;
    case MenuAction::kRedo:
        (void)historyredo(state);
        break;
    case MenuAction::kFileNew:
        dialogopen(state);
        break;
    case MenuAction::kFileImport:
        pushaction(state, GuiActionKind::kOpenImagePicker);
        break;
    case MenuAction::kFileExport:
        pushaction(state, GuiActionKind::kExportPng);
        break;
    case MenuAction::kLayerNew:
        if (layeradd(state, {})) {
            (void)layeredit(state);
        }
        break;
    case MenuAction::kLayerDelete:
        (void)layerdel(state);
        break;
    case MenuAction::kLayerRename:
        (void)layeredit(state);
        break;
    }
}

void menulayout(GuiState *state) {
    f32 menu_x = 4.0F;
    for (const MenuItem item : kMenuItems) {
        const Rect rect = menurect(item.text, menu_x);
        addhit(state, rect, HitKind::kMenu, item.index, 100);
        menu_x += rect.width + 1.0F;
    }
    const std::span<const MenuCommand> commands = menucommands(state->active_menu);
    for (u8 index = 0; index < static_cast<u8>(commands.size()); ++index) {
        addhit(state, menucmdrect(state->active_menu, index), HitKind::kMenuAction, index, 120);
    }
}

b8 menumouse(GuiState *state, HitRecord hit) {
    if (hit.kind == HitKind::kMenu) {
        layerdone(state);
        state->context_open = false;
        state->active_menu = state->active_menu == hit.index ? kNoMenu : hit.index;
        return true;
    }

    const MenuAction action = mira::menuaction(*state, hit);
    if (action == MenuAction::kNone) {
        return false;
    }

    doaction(state, action);
    state->active_menu = kNoMenu;
    return true;
}

void menudraw(const GuiState &state, DrawList *draws) {
    drawrect(draws, state.layout.menu_bar, Tone::kWhite);
    drawstroke(draws,
               {.x = 0.0F,
                .y = state.layout.menu_bar.height - 1.0F,
                .width = state.layout.window.width,
                .height = 1.0F},
               Tone::kBlack);

    f32 menu_x = 4.0F;
    for (const MenuItem item : kMenuItems) {
        const Rect rect = menurect(item.text, menu_x);
        const b8 active = state.active_menu == item.index;
        const b8 hot = state.hot_kind == HitKind::kMenu && state.hot_index == item.index;
        if (active || hot) {
            drawrect(draws, {.x = rect.x, .y = rect.y + 2.0F, .width = rect.width, .height = 14.0F},
                     active ? Tone::kBlack : Tone::kLight);
            if (hot && !active) {
                drawstroke(draws,
                           {.x = rect.x, .y = rect.y + 2.0F, .width = rect.width, .height = 14.0F},
                           Tone::kBlack);
            }
        }
        drawtext(draws, item.text, rect.x + 5.0F, 3.0F, active ? Tone::kWhite : Tone::kBlack);
        menu_x += rect.width + 1.0F;
    }

    const std::span<const MenuCommand> commands = menucommands(state.active_menu);
    if (commands.empty()) {
        return;
    }

    drawplane(draws, DrawPlane::kMenu);
    const Rect menu = {
        .x = menux(state.active_menu),
        .y = 18.0F,
        .width = menucmdwidth(state.active_menu),
        .height = static_cast<f32>(commands.size()) * 15.0F,
    };
    drawrect(draws, menu, Tone::kWhite);
    drawstroke(draws, menu, Tone::kBlack);
    for (u8 index = 0; index < static_cast<u8>(commands.size()); ++index) {
        const Rect item = menucmdrect(state.active_menu, index);
        const b8 hot = state.hot_kind == HitKind::kMenuAction && state.hot_index == index;
        if (hot) {
            drawrect(draws, item, Tone::kBlack);
        }
        drawtext(draws, commands[index].text, item.x + 5.0F, item.y + 1.0F,
                 hot ? Tone::kWhite : Tone::kBlack);
    }
}

} // namespace mira::gui
