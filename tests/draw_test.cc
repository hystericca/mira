#include "mira/draw/draw.hpp"
#include "mira/gui/gui.hpp"
#include "test_support.hpp"

#include <array>
#include <string_view>
#include <type_traits>

auto main() -> int {
    const auto close = [](mira::f32 a, mira::f32 b) -> bool {
        const mira::f32 delta = a - b;
        return delta > -0.01F && delta < 0.01F;
    };

    static_assert(sizeof(mira::RectDraw) == 32);
    static_assert(sizeof(mira::GlyphDraw) == 32);
    static_assert(sizeof(mira::IconDraw) == 32);
    static_assert(sizeof(mira::Layer) == 24);
    static_assert(sizeof(mira::Tool) == 16);
    static_assert(sizeof(mira::Size) == 4);
    static_assert(sizeof(mira::PaintStamp) == 32);
    static_assert(sizeof(mira::Document) == 8);
    static_assert(sizeof(mira::View) == 12);
    static_assert(sizeof(mira::InputEvent) == 20);
    static_assert(sizeof(mira::HitRecord) == 24);
    static_assert(!std::is_copy_constructible_v<mira::DrawList>);
    static_assert(!std::is_copy_constructible_v<mira::GuiState>);

    const mira::Screen small = mira::screen_for(900, 600);
    const mira::Screen normal = mira::screen_for(1120, 720);
    const mira::Screen large = mira::screen_for(1800, 1200);
    MIRA_TEST(small.scale == 2);
    MIRA_TEST(small.width == 450);
    MIRA_TEST(small.height == 300);
    MIRA_TEST(normal.scale == 2);
    MIRA_TEST(normal.width == 560);
    MIRA_TEST(normal.height == 360);
    MIRA_TEST(large.scale == 2);
    MIRA_TEST(large.width == 900);
    MIRA_TEST(large.height == 600);

    mira::Table<mira::i32, 2> table;
    MIRA_TEST(table.push(1));
    MIRA_TEST(table.push(2));
    MIRA_TEST(!table.push(3));
    MIRA_TEST(table.overflowed);
    MIRA_TEST(table.size() == 2);
    table.clear();
    MIRA_TEST(table.empty());
    MIRA_TEST(!table.overflowed);
    mira::Table<mira::i32, 3> edit_table;
    MIRA_TEST(edit_table.push(1));
    MIRA_TEST(edit_table.push(3));
    MIRA_TEST(edit_table.insert(1, 2));
    MIRA_TEST(edit_table[0] == 1);
    MIRA_TEST(edit_table[1] == 2);
    MIRA_TEST(edit_table[2] == 3);
    MIRA_TEST(!edit_table.insert(0, 0));
    MIRA_TEST(edit_table.overflowed);
    MIRA_TEST(!edit_table.erase(4));
    MIRA_TEST(edit_table.erase(0));
    MIRA_TEST(edit_table[0] == 2);

    mira::DrawList packing;
    MIRA_TEST(
        mira::add_rect(&packing, {.x = 1.0F, .y = 2.0F, .width = 3.0F, .height = 4.0F},
                       mira::Tone::kWhite));
    MIRA_TEST(packing.rects[0].x0 == 1.0F);
    MIRA_TEST(packing.rects[0].y0 == 2.0F);
    MIRA_TEST(packing.rects[0].x1 == 4.0F);
    MIRA_TEST(packing.rects[0].y1 == 6.0F);
    MIRA_TEST(packing.rects[0].tone == mira::tone_value(mira::Tone::kWhite));
    MIRA_TEST(mira::add_text(&packing, "A", 5.0F, 6.0F, mira::Tone::kWhite));
    MIRA_TEST(packing.glyphs[0].x == 5.0F);
    MIRA_TEST(packing.glyphs[0].y == 6.0F);
    MIRA_TEST(packing.glyphs[0].code == 'A');
    MIRA_TEST(packing.glyphs[0].tone == mira::tone_value(mira::Tone::kWhite));
    MIRA_TEST(mira::add_icon(&packing, mira::Icon::kPen, 7.0F, 8.0F, mira::Tone::kBlack));
    MIRA_TEST(packing.icons[0].x == 7.0F);
    MIRA_TEST(packing.icons[0].y == 8.0F);
    MIRA_TEST(packing.icons[0].code == 0.0F);
    MIRA_TEST(packing.icons[0].tone == mira::tone_value(mira::Tone::kBlack));
    MIRA_TEST(mira::sizeicon(7) == mira::Icon::kSize8);
    MIRA_TEST(mira::brushicon(7) == mira::Icon::kBrushSize8);
    MIRA_TEST(mira::add_icon(&packing, mira::Icon::kBrushSize8, 9.0F, 10.0F,
                             mira::Tone::kBlack, 0.25F));
    MIRA_TEST(packing.icons[1].scale == 0.25F);
    MIRA_TEST(mira::add_icon(&packing, mira::Icon::kLockClosed, 11.0F, 12.0F,
                             mira::Tone::kWhite));
    MIRA_TEST(packing.icons[2].code == static_cast<mira::f32>(static_cast<mira::u8>(mira::Icon::kLockClosed)));

    mira::GuiState gui;
    mira::guiinit(&gui);
    MIRA_TEST(gui.tools.size() == 7);
    MIRA_TEST(mira::toolname(gui.tools[0]) == "Pen");
    MIRA_TEST(mira::toolname(gui.tools[1]) == "Brush");
    MIRA_TEST(mira::toolname(gui.tools[6]) == "Erase");
    MIRA_TEST(gui.tools[0].selected == 1);
    MIRA_TEST(mira::toolcur(gui) == &gui.tools[0]);
    MIRA_TEST(gui.sizes.size() == 8);
    MIRA_TEST(gui.sizes[3].selected == 1);
    MIRA_TEST(mira::sizecur(gui) == &gui.sizes[3]);
    MIRA_TEST(gui.document.width == 320);
    MIRA_TEST(gui.document.height == 240);
    MIRA_TEST(gui.layers.size() == 2);
    MIRA_TEST(mira::layername(gui.layers[0]) == "Ink");
    MIRA_TEST(mira::layername(gui.layers[1]) == "Canvas");
    MIRA_TEST(mira::layerselected(gui.layers[0]));
    MIRA_TEST(!mira::layerlocked(gui.layers[0]));
    MIRA_TEST(mira::layerlocked(gui.layers[1]));
    MIRA_TEST(gui.layers[0].texture_slot == 0);
    MIRA_TEST(gui.layers[1].texture_slot == mira::kCanvasTextureSlot);
    MIRA_TEST(gui.layers[1].opacity_u8 == 255);
    MIRA_TEST(gui.next_layer_id == 3);
    MIRA_TEST(mira::layercur(gui) == &gui.layers[0]);

    mira::guilayout(&gui, normal);
    MIRA_TEST(gui.layout.window.x == 0.0F);
    MIRA_TEST(gui.layout.window.y == 0.0F);
    MIRA_TEST(gui.layout.window.width == 560.0F);
    MIRA_TEST(gui.layout.window.height == 360.0F);
    MIRA_TEST(gui.layout.menu_bar.x == 0.0F);
    MIRA_TEST(gui.layout.menu_bar.y == 0.0F);
    MIRA_TEST(gui.layout.menu_bar.width == 560.0F);
    MIRA_TEST(gui.layout.menu_bar.height == 14.0F);
    MIRA_TEST(gui.layout.toolbar.x == 0.0F);
    MIRA_TEST(gui.layout.toolbar.width == 48.0F);
    MIRA_TEST(gui.layout.layers.x == 448.0F);
    MIRA_TEST(gui.layout.layers.width == 112.0F);
    MIRA_TEST(gui.layout.viewport.x == 48.0F);
    MIRA_TEST(gui.layout.viewport.y == 14.0F);
    MIRA_TEST(gui.layout.viewport.width == 400.0F);
    MIRA_TEST(gui.layout.document.x == 88.0F);
    MIRA_TEST(gui.layout.document.y == 67.0F);
    MIRA_TEST(gui.layout.document.width == 320.0F);
    MIRA_TEST(gui.layout.document.height == 240.0F);
    MIRA_TEST(close(gui.view.x, -40.0F));
    MIRA_TEST(close(gui.view.y, -53.0F));

    mira::GuiState nav_gui;
    mira::guiinit(&nav_gui);
    mira::guilayout(&nav_gui, normal);
    const mira::i32 nav_x = static_cast<mira::i32>(nav_gui.layout.viewport.x + 100.0F);
    const mira::i32 nav_y = static_cast<mira::i32>(nav_gui.layout.viewport.y + 60.0F);
    mira::InputEvent wheel_down = {};
    wheel_down.kind = mira::InputKind::kWheel;
    wheel_down.x = nav_x;
    wheel_down.y = nav_y;
    wheel_down.dy = 24;
    mira::guievent(&nav_gui, {&wheel_down, 1});
    MIRA_TEST(close(nav_gui.view.x, -40.0F));
    MIRA_TEST(close(nav_gui.view.y, -29.0F));

    mira::InputEvent wheel_side = wheel_down;
    wheel_side.mods = mira::kInputShift;
    wheel_side.dy = 16;
    mira::guievent(&nav_gui, {&wheel_side, 1});
    MIRA_TEST(close(nav_gui.view.x, -24.0F));
    MIRA_TEST(close(nav_gui.view.y, -29.0F));

    const mira::f32 document_x_before =
        nav_gui.view.x + ((static_cast<mira::f32>(nav_x) - nav_gui.layout.viewport.x) / nav_gui.view.zoom);
    const mira::f32 document_y_before =
        nav_gui.view.y + ((static_cast<mira::f32>(nav_y) - nav_gui.layout.viewport.y) / nav_gui.view.zoom);
    mira::InputEvent wheel_zoom = wheel_down;
    wheel_zoom.mods = mira::kInputCtrl;
    wheel_zoom.dy = -12;
    mira::guievent(&nav_gui, {&wheel_zoom, 1});
    MIRA_TEST(close(nav_gui.view.zoom, 1.125F));
    const mira::f32 document_x_after =
        nav_gui.view.x + ((static_cast<mira::f32>(nav_x) - nav_gui.layout.viewport.x) / nav_gui.view.zoom);
    const mira::f32 document_y_after =
        nav_gui.view.y + ((static_cast<mira::f32>(nav_y) - nav_gui.layout.viewport.y) / nav_gui.view.zoom);
    MIRA_TEST(close(document_x_before, document_x_after));
    MIRA_TEST(close(document_y_before, document_y_after));

    const mira::f32 pan_x_before = nav_gui.view.x;
    const mira::f32 pan_y_before = nav_gui.view.y;
    mira::InputEvent pan_down = {};
    pan_down.kind = mira::InputKind::kMouseDown;
    pan_down.button = 1;
    pan_down.x = nav_x;
    pan_down.y = nav_y;
    mira::InputEvent pan_move = {};
    pan_move.kind = mira::InputKind::kMouseMove;
    pan_move.button = 1;
    pan_move.buttons = 4;
    pan_move.x = nav_x + 18;
    pan_move.y = nav_y - 9;
    mira::InputEvent pan_up = pan_move;
    pan_up.kind = mira::InputKind::kMouseUp;
    pan_up.button = 1;
    const mira::InputEvent pan_events[] = {pan_down, pan_move, pan_up};
    mira::guievent(&nav_gui, pan_events);
    MIRA_TEST(close(nav_gui.view.x, pan_x_before - (18.0F / nav_gui.view.zoom)));
    MIRA_TEST(close(nav_gui.view.y, pan_y_before + (9.0F / nav_gui.view.zoom)));
    MIRA_TEST(!nav_gui.panning);

    const mira::HitRecord menu_hit = mira::guihit(gui, 8, 5);
    MIRA_TEST(menu_hit.kind == mira::HitKind::kMenu);
    MIRA_TEST(menu_hit.index == 0);
    mira::GuiState menu_gui;
    mira::guiinit(&menu_gui);
    mira::guilayout(&menu_gui, normal);
    mira::InputEvent open_layer_menu = {};
    open_layer_menu.kind = mira::InputKind::kMouseDown;
    open_layer_menu.x = 119;
    open_layer_menu.y = 5;
    mira::guievent(&menu_gui, {&open_layer_menu, 1});
    MIRA_TEST(menu_gui.active_menu == 3);
    mira::guilayout(&menu_gui, normal);
    const mira::HitRecord new_layer_hit = mira::guihit(menu_gui, 119, 16);
    MIRA_TEST(new_layer_hit.kind == mira::HitKind::kMenuAction);
    mira::InputEvent click_new_layer = {};
    click_new_layer.kind = mira::InputKind::kMouseDown;
    click_new_layer.x = 119;
    click_new_layer.y = 16;
    mira::guievent(&menu_gui, {&click_new_layer, 1});
    MIRA_TEST(menu_gui.layers.size() == 3);
    MIRA_TEST(menu_gui.active_menu == mira::kNoMenu);
    MIRA_TEST(menu_gui.renaming_layer == 0);
    MIRA_TEST(mira::layername(menu_gui.layers[0]) == "Layer 3");
    const mira::HitRecord tool_hit = mira::guihit(gui, 8, 24);
    MIRA_TEST(tool_hit.kind == mira::HitKind::kTool);
    MIRA_TEST(tool_hit.index == 0);
    const mira::HitRecord size_hit = mira::guihit(gui, 30, 24);
    MIRA_TEST(size_hit.kind == mira::HitKind::kSize);
    MIRA_TEST(size_hit.index == 0);
    const mira::HitRecord viewport_hit = mira::guihit(gui, 80, 40);
    MIRA_TEST(viewport_hit.kind == mira::HitKind::kViewport);
    const mira::HitRecord visibility_hit =
        mira::guihit(gui, static_cast<mira::i32>(gui.layout.layerrows.x + 6.0F),
                     static_cast<mira::i32>(gui.layout.layerrows.y + 7.0F));
    MIRA_TEST(visibility_hit.kind == mira::HitKind::kLayerVisibility);
    MIRA_TEST(visibility_hit.priority > 80);

    mira::InputEvent toggle_visibility = {};
    toggle_visibility.kind = mira::InputKind::kMouseDown;
    toggle_visibility.x = static_cast<mira::i32>(gui.layout.layerrows.x + 6.0F);
    toggle_visibility.y = static_cast<mira::i32>(gui.layout.layerrows.y + 7.0F);
    mira::guievent(&gui, {&toggle_visibility, 1});
    MIRA_TEST(!mira::layervisible(gui.layers[0]));

    mira::InputEvent select_canvas = {};
    select_canvas.kind = mira::InputKind::kMouseDown;
    select_canvas.x = static_cast<mira::i32>(gui.layout.layerrows.x + 20.0F);
    select_canvas.y = static_cast<mira::i32>(gui.layout.layerrows.y + 28.0F);
    mira::guievent(&gui, {&select_canvas, 1});
    MIRA_TEST(gui.curlayer == 1);
    MIRA_TEST(mira::layerselected(gui.layers[1]));
    MIRA_TEST(!mira::layerselected(gui.layers[0]));

    const mira::HitRecord canvas_lock_hit =
        mira::guihit(gui, static_cast<mira::i32>(gui.layout.layerrows.x + 17.0F),
                     static_cast<mira::i32>(gui.layout.layerrows.y + 35.0F));
    MIRA_TEST(canvas_lock_hit.kind == mira::HitKind::kLayerLock);
    mira::InputEvent unlock_canvas = {};
    unlock_canvas.kind = mira::InputKind::kMouseDown;
    unlock_canvas.x = canvas_lock_hit.rect.x + 1;
    unlock_canvas.y = canvas_lock_hit.rect.y + 1;
    mira::guievent(&gui, {&unlock_canvas, 1});
    MIRA_TEST(mira::layerlocked(gui.layers[1]));
    MIRA_TEST(!mira::layerdel(&gui));
    MIRA_TEST(gui.layers.size() == 2);

    MIRA_TEST(mira::layeradd(&gui, "Sketch"));
    MIRA_TEST(gui.layers.size() == 3);
    MIRA_TEST(gui.curlayer == 1);
    MIRA_TEST(mira::layername(gui.layers[1]) == "Sketch");
    MIRA_TEST(gui.layers[1].texture_slot == 1);
    MIRA_TEST(gui.clear_slots.size() == 1);
    MIRA_TEST(gui.clear_slots[0] == 1);
    MIRA_TEST(mira::layerrename(&gui, 1, "Line 2"));
    MIRA_TEST(mira::layername(gui.layers[1]) == "Line 2");
    MIRA_TEST(mira::layeredit(&gui));
    MIRA_TEST(gui.renaming_layer == 1);
    mira::InputEvent rename_text = {};
    rename_text.kind = mira::InputKind::kText;
    rename_text.dx = 'A';
    mira::InputEvent rename_enter = {};
    rename_enter.kind = mira::InputKind::kKeyDown;
    rename_enter.button = static_cast<mira::u8>(mira::Key::kEnter);
    const mira::InputEvent rename_events[] = {rename_text, rename_enter};
    mira::guievent(&gui, rename_events);
    MIRA_TEST(mira::layername(gui.layers[1]) == "A");
    MIRA_TEST(gui.renaming_layer == mira::kNoLayer);
    mira::guilayout(&gui, normal);

    const mira::HitRecord opacity_hit =
        mira::guihit(gui, static_cast<mira::i32>(gui.layout.layerrows.x + 40.0F),
                     static_cast<mira::i32>(gui.layout.layerrows.y + 46.0F));
    MIRA_TEST(opacity_hit.kind == mira::HitKind::kLayerOpacity);
    mira::InputEvent opacity_click = {};
    opacity_click.kind = mira::InputKind::kMouseDown;
    opacity_click.x = static_cast<mira::i32>(gui.layout.layerrows.x + 50.0F);
    opacity_click.y = static_cast<mira::i32>(gui.layout.layerrows.y + 46.0F);
    mira::guievent(&gui, {&opacity_click, 1});
    MIRA_TEST(gui.layers[1].opacity_u8 < 255);

    mira::InputEvent select_line = {};
    select_line.kind = mira::InputKind::kMouseDown;
    select_line.x = static_cast<mira::i32>(gui.layout.tools.x + 8.0F);
    select_line.y = static_cast<mira::i32>(gui.layout.tools.y + 40.0F);
    mira::guievent(&gui, {&select_line, 1});
    MIRA_TEST(gui.curtool == 2);
    MIRA_TEST(gui.tools[2].selected == 1);
    MIRA_TEST(gui.tools[0].selected == 0);

    mira::DrawList list;
    mira::guiframe(&gui, normal, {}, &list);
    MIRA_TEST(list.rects.size() >= 5);
    MIRA_TEST(list.glyphs.size() > 0);
    MIRA_TEST(list.icons.size() == 18);
    MIRA_TEST(list.upload_bytes() ==
              list.rects.byte_size() + list.glyphs.byte_size() + list.icons.byte_size());
    MIRA_TEST(list.overflow_count() == 0);
    MIRA_TEST(list.rects.size() <= 72);
    MIRA_TEST(list.glyphs.size() <= 64);

    mira::InputEvent select_size = {};
    select_size.kind = mira::InputKind::kMouseDown;
    select_size.x = static_cast<mira::i32>(gui.layout.sizes.x + 8.0F);
    select_size.y = static_cast<mira::i32>(gui.layout.sizes.y + 112.0F);
    mira::guievent(&gui, {&select_size, 1});
    MIRA_TEST(gui.cursize == 6);
    MIRA_TEST(gui.sizes[6].selected == 1);
    MIRA_TEST(gui.sizes[3].selected == 0);

    mira::InputEvent select_brush = {};
    select_brush.kind = mira::InputKind::kMouseDown;
    select_brush.x = static_cast<mira::i32>(gui.layout.tools.x + 8.0F);
    select_brush.y = static_cast<mira::i32>(gui.layout.tools.y + 22.0F);
    mira::guievent(&gui, {&select_brush, 1});
    MIRA_TEST(gui.curtool == 1);

    mira::InputEvent select_canvas_again = {};
    select_canvas_again.kind = mira::InputKind::kMouseDown;
    select_canvas_again.x = static_cast<mira::i32>(gui.layout.layerrows.x + 28.0F);
    select_canvas_again.y = static_cast<mira::i32>(gui.layout.layerrows.y + 64.0F);
    mira::guievent(&gui, {&select_canvas_again, 1});
    MIRA_TEST(gui.curlayer == 2);
    mira::InputEvent locked_canvas_down = {};
    locked_canvas_down.kind = mira::InputKind::kMouseDown;
    locked_canvas_down.x = static_cast<mira::i32>(gui.layout.document.x + 30.0F);
    locked_canvas_down.y = static_cast<mira::i32>(gui.layout.document.y + 30.0F);
    mira::guievent(&gui, {&locked_canvas_down, 1});
    MIRA_TEST(!gui.painting);
    MIRA_TEST(gui.paint_stamps.empty());

    mira::InputEvent select_ink = {};
    select_ink.kind = mira::InputKind::kMouseDown;
    select_ink.x = static_cast<mira::i32>(gui.layout.layerrows.x + 28.0F);
    select_ink.y = static_cast<mira::i32>(gui.layout.layerrows.y + 8.0F);
    mira::guievent(&gui, {&select_ink, 1});
    MIRA_TEST(gui.curlayer == 0);

    mira::InputEvent outside_document_down = {};
    outside_document_down.kind = mira::InputKind::kMouseDown;
    outside_document_down.x = static_cast<mira::i32>(gui.layout.viewport.x + 4.0F);
    outside_document_down.y = static_cast<mira::i32>(gui.layout.viewport.y + 4.0F);
    mira::guievent(&gui, {&outside_document_down, 1});
    MIRA_TEST(!gui.painting);
    MIRA_TEST(gui.paint_stamps.empty());

    const mira::i32 paint_y = static_cast<mira::i32>(gui.layout.document.y + 30.0F);
    mira::InputEvent paint_down = {};
    paint_down.kind = mira::InputKind::kMouseDown;
    paint_down.x = static_cast<mira::i32>(gui.layout.document.x + 40.0F);
    paint_down.y = paint_y;
    mira::InputEvent paint_move = {};
    paint_move.kind = mira::InputKind::kMouseMove;
    paint_move.x = static_cast<mira::i32>(gui.layout.document.x + 50.0F);
    paint_move.y = paint_y;
    mira::InputEvent paint_up = paint_move;
    paint_up.kind = mira::InputKind::kMouseUp;
    const mira::InputEvent paint_events[] = {paint_down, paint_move, paint_up};
    mira::guievent(&gui, paint_events);
    MIRA_TEST(gui.paint_stamps.size() == 11);
    MIRA_TEST(close(gui.paint_stamps[0].x, 40.0F));
    MIRA_TEST(close(gui.paint_stamps[0].y, 30.0F));
    MIRA_TEST(close(gui.paint_stamps[10].x, 50.0F));
    MIRA_TEST(close(gui.paint_stamps[10].y, 30.0F));
    MIRA_TEST(close(gui.paint_stamps[0].size, 6.0F));
    MIRA_TEST(close(gui.paint_stamps[0].tone, static_cast<mira::f32>(mira::tone_value(mira::Tone::kBlack))));
    MIRA_TEST(close(gui.paint_stamps[0].layer, 0.0F));

    mira::GuiState zoom_paint_gui;
    mira::guiinit(&zoom_paint_gui);
    mira::guilayout(&zoom_paint_gui, normal);
    zoom_paint_gui.view.zoom = 2.0F;
    mira::InputEvent zoom_select_size = {};
    zoom_select_size.kind = mira::InputKind::kMouseDown;
    zoom_select_size.x = static_cast<mira::i32>(zoom_paint_gui.layout.sizes.x + 8.0F);
    zoom_select_size.y = static_cast<mira::i32>(zoom_paint_gui.layout.sizes.y + 112.0F);
    mira::InputEvent zoom_select_brush = {};
    zoom_select_brush.kind = mira::InputKind::kMouseDown;
    zoom_select_brush.x = static_cast<mira::i32>(zoom_paint_gui.layout.tools.x + 8.0F);
    zoom_select_brush.y = static_cast<mira::i32>(zoom_paint_gui.layout.tools.y + 22.0F);
    mira::guievent(&zoom_paint_gui, {&zoom_select_size, 1});
    mira::guievent(&zoom_paint_gui, {&zoom_select_brush, 1});
    mira::InputEvent zoom_paint_down = {};
    zoom_paint_down.kind = mira::InputKind::kMouseDown;
    zoom_paint_down.x = static_cast<mira::i32>(
        zoom_paint_gui.layout.viewport.x + ((40.0F - zoom_paint_gui.view.x) * 2.0F));
    zoom_paint_down.y = static_cast<mira::i32>(
        zoom_paint_gui.layout.viewport.y + ((30.0F - zoom_paint_gui.view.y) * 2.0F));
    mira::InputEvent zoom_paint_move = {};
    zoom_paint_move.kind = mira::InputKind::kMouseMove;
    zoom_paint_move.x = static_cast<mira::i32>(
        zoom_paint_gui.layout.viewport.x + ((45.0F - zoom_paint_gui.view.x) * 2.0F));
    zoom_paint_move.y = zoom_paint_down.y;
    mira::InputEvent zoom_paint_up = zoom_paint_move;
    zoom_paint_up.kind = mira::InputKind::kMouseUp;
    const mira::InputEvent zoom_paint_events[] = {zoom_paint_down, zoom_paint_move,
                                                  zoom_paint_up};
    mira::guievent(&zoom_paint_gui, zoom_paint_events);
    MIRA_TEST(zoom_paint_gui.paint_stamps.size() == 6);
    MIRA_TEST(close(zoom_paint_gui.paint_stamps[0].x, 40.0F));
    MIRA_TEST(close(zoom_paint_gui.paint_stamps[5].x, 45.0F));
    MIRA_TEST(close(zoom_paint_gui.paint_stamps[0].size, 6.0F));

    mira::DrawList paint_list;
    mira::guiframe(&gui, normal, {}, &paint_list);
    MIRA_TEST(gui.paint_stamps.empty());
    MIRA_TEST(paint_list.icons.size() >= 18);

    mira::DrawList tiny_list;
    mira::GuiState tiny_gui;
    mira::guiframe(&tiny_gui, mira::screen_for(1, 1), {}, &tiny_list);
    MIRA_TEST(tiny_list.rects.size() >= 1);
    MIRA_TEST(tiny_list.rects[0].x1 >= tiny_list.rects[0].x0);
    MIRA_TEST(tiny_list.rects[0].y1 >= tiny_list.rects[0].y0);

    const mira::DrawView view = mira::view(list);
    MIRA_TEST(view.rects.size() == list.rects.size());
    MIRA_TEST(view.glyphs.size() == list.glyphs.size());
    MIRA_TEST(view.icons.size() == list.icons.size());

    list.clear();
    std::array<char, mira::kMaxGlyphs + 100> long_text = {};
    long_text.fill('x');
    MIRA_TEST(!mira::add_text(&list, std::string_view(long_text.data(), long_text.size()), 0.0F,
                              0.0F, mira::Tone::kWhite));
    MIRA_TEST(list.glyphs.overflowed);
    MIRA_TEST(list.overflow_count() == 1);
    return 0;
}
