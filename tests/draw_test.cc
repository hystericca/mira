#include "mira/draw/draw.hpp"
#include "test_support.hpp"

#include <array>
#include <string_view>
#include <type_traits>

namespace {

[[nodiscard]] auto kindof(mira::Draw draw) -> mira::DrawKind {
    return static_cast<mira::DrawKind>((draw.data1 >> 16U) & 0xFFU);
}

} // namespace

auto main() -> int {
    static_assert(sizeof(mira::Draw) == 32);
    static_assert(sizeof(mira::Clip) == 16);
    static_assert(sizeof(mira::Sample) == 8);
    static_assert(!std::is_copy_constructible_v<mira::DrawList>);

    const mira::Screen small = mira::screen_for(900, 600);
    const mira::Screen normal = mira::screen_for(1120, 720);
    const mira::Screen large = mira::screen_for(1800, 1200);
    MIRA_TEST(small.scale == 1);
    MIRA_TEST(normal.scale == 2);
    MIRA_TEST(normal.width == 560);
    MIRA_TEST(normal.height == 360);
    MIRA_TEST(large.scale == 3);

    mira::Table<mira::i32, 2> table;
    MIRA_TEST(table.push(1));
    MIRA_TEST(table.push(2));
    MIRA_TEST(!table.push(3));
    MIRA_TEST(table.overflowed);
    MIRA_TEST(table.size() == 2);
    table.clear();
    MIRA_TEST(table.empty());
    MIRA_TEST(!table.overflowed);

    mira::DrawList list;
    mira::build_demo(&list, normal);
    MIRA_TEST(list.clips.size() == 1);
    MIRA_TEST(list.draws.size() >= 5);
    MIRA_TEST(list.text.size() == 4);
    MIRA_TEST(list.samples.size() > 0);
    MIRA_TEST(list.upload_bytes() == list.draws.byte_size() + list.clips.byte_size() +
                                         list.text.byte_size() + list.samples.byte_size());
    MIRA_TEST(list.overflow_count() == 0);

    std::array<bool, 5> seen_kind = {};
    for (const mira::Draw draw : list.draws.span()) {
        seen_kind[static_cast<mira::usize>(kindof(draw))] = true;
    }
    MIRA_TEST(seen_kind[static_cast<mira::usize>(mira::DrawKind::kFill)]);
    MIRA_TEST(seen_kind[static_cast<mira::usize>(mira::DrawKind::kStroke)]);
    MIRA_TEST(seen_kind[static_cast<mira::usize>(mira::DrawKind::kDash)]);
    MIRA_TEST(seen_kind[static_cast<mira::usize>(mira::DrawKind::kText)]);
    MIRA_TEST(seen_kind[static_cast<mira::usize>(mira::DrawKind::kGraph)]);

    mira::DrawList tiny_list;
    mira::build_demo(&tiny_list, mira::screen_for(1, 1));
    MIRA_TEST(tiny_list.draws.size() >= 5);
    MIRA_TEST(tiny_list.draws[1].x1 >= tiny_list.draws[1].x0);
    MIRA_TEST(tiny_list.draws[1].y1 >= tiny_list.draws[1].y0);

    const mira::DrawView view = mira::view(list);
    MIRA_TEST(view.draws.size() == list.draws.size());
    MIRA_TEST(view.clips.size() == list.clips.size());
    MIRA_TEST(std::string_view(view.text.data(), view.text.size()) == "MIRA");

    list.clear();
    std::array<char, mira::kMaxTextBytes + 100> long_text = {};
    long_text.fill('x');
    const mira::TextRange text =
        mira::add_text(&list, std::string_view(long_text.data(), long_text.size()));
    MIRA_TEST(text.length == mira::kMaxTextBytes);
    MIRA_TEST(list.text.overflowed);
    MIRA_TEST(list.overflow_count() == 1);
    return 0;
}
