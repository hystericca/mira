#pragma once

#include <array>

#include "mira/draw/draw.hpp"
#include "mira/types.hpp"

namespace mira {

enum class ToolKind : u8 {
    kPen,
    kBrush,
    kLine,
    kMagic,
    kRect,
    kZoom,
    kErase,
};

enum class InkKind : u8 {
    kNone,
    kBlack,
    kWhite,
};

enum class StrokeKind : u8 {
    kNone,
    kFree,
    kLine,
    kRect,
};

enum class TraceKind : u8 {
    kNone,
    kAccumulate,
    kLast,
};

enum class ToolAction : u8 {
    kNone,
    kZoom,
};

constexpr u8 kToolUsesSize = 1U << 0U;
constexpr u8 kToolUsesTip = 1U << 1U;
constexpr u8 kToolUsesCoverage = 1U << 2U;

struct ToolDef {
    ToolKind kind = ToolKind::kPen;
    Icon icon = Icon::kPen;
    InkKind ink = InkKind::kNone;
    StrokeKind stroke = StrokeKind::kNone;
    TraceKind trace = TraceKind::kNone;
    ToolAction action = ToolAction::kNone;
    u8 flags = 0;
    u8 size = 0;
    u8 tip = 0;
};
static_assert(sizeof(ToolDef) == 9);

inline constexpr std::array<ToolDef, 7> kToolDefs = {{
    {.kind = ToolKind::kPen,
     .icon = Icon::kPen,
     .ink = InkKind::kBlack,
     .stroke = StrokeKind::kFree,
     .trace = TraceKind::kAccumulate,
     .size = 0,
     .tip = 0},
    {.kind = ToolKind::kBrush,
     .icon = Icon::kBrush,
     .ink = InkKind::kBlack,
     .stroke = StrokeKind::kFree,
     .trace = TraceKind::kAccumulate,
     .flags = kToolUsesSize | kToolUsesTip | kToolUsesCoverage},
    {.kind = ToolKind::kLine,
     .icon = Icon::kLine,
     .ink = InkKind::kBlack,
     .stroke = StrokeKind::kLine,
     .trace = TraceKind::kLast,
     .flags = kToolUsesSize | kToolUsesTip | kToolUsesCoverage},
    {.kind = ToolKind::kMagic,
     .icon = Icon::kMagic,
     .ink = InkKind::kWhite,
     .stroke = StrokeKind::kFree,
     .trace = TraceKind::kAccumulate,
     .size = 7,
     .tip = 7},
    {.kind = ToolKind::kRect,
     .icon = Icon::kRect,
     .ink = InkKind::kBlack,
     .stroke = StrokeKind::kRect,
     .trace = TraceKind::kLast,
     .flags = kToolUsesSize | kToolUsesTip | kToolUsesCoverage},
    {.kind = ToolKind::kZoom, .icon = Icon::kZoom, .action = ToolAction::kZoom},
    {.kind = ToolKind::kErase,
     .icon = Icon::kErase,
     .ink = InkKind::kWhite,
     .stroke = StrokeKind::kFree,
     .trace = TraceKind::kAccumulate,
     .flags = kToolUsesSize | kToolUsesTip | kToolUsesCoverage},
}};
static_assert(static_cast<usize>(ToolKind::kErase) + 1U == kToolDefs.size());

[[nodiscard]] constexpr auto toolindex(ToolKind kind) -> usize {
    return static_cast<usize>(kind);
}

[[nodiscard]] constexpr auto tooldef(ToolKind kind) -> ToolDef {
    const usize index = toolindex(kind);
    return index < kToolDefs.size() ? kToolDefs[index] : kToolDefs[0];
}

[[nodiscard]] constexpr auto toolpaints(ToolDef def) -> b8 { return def.ink != InkKind::kNone; }

[[nodiscard]] constexpr auto toolfreehand(ToolDef def) -> b8 {
    return def.stroke == StrokeKind::kFree;
}

[[nodiscard]] constexpr auto tooldraft(ToolDef def) -> b8 { return def.trace == TraceKind::kLast; }

[[nodiscard]] constexpr auto tooluses(ToolDef def, u8 flags) -> b8 {
    return (def.flags & flags) == flags;
}

} // namespace mira
