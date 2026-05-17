struct Frame {
    drawable_size: vec2<f32>,
    screen_size: vec2<f32>,
    rect_count: u32,
    glyph_count: u32,
    icon_count: u32,
    layer_count: u32,
    document_rect: vec4<f32>,
    viewport_rect: vec4<f32>,
    document_size: vec2<f32>,
    _pad0: vec2<f32>,
}

struct Layers {
    rows: array<vec4<f32>, 16>,
}

struct RectVertex {
    @builtin(position) position: vec4<f32>,
    @location(0) tone: f32,
}

struct GlyphVertex {
    @builtin(position) position: vec4<f32>,
    @location(0) local: vec2<f32>,
    @location(1) code: f32,
    @location(2) tone: f32,
    @location(3) scale: f32,
}

struct IconVertex {
    @builtin(position) position: vec4<f32>,
    @location(0) local: vec2<f32>,
    @location(1) code: f32,
    @location(2) tone: f32,
    @location(3) scale: f32,
}

struct CompositeVertex {
    @builtin(position) position: vec4<f32>,
    @location(0) screen: vec2<f32>,
    @location(1) uv: vec2<f32>,
}

struct StampVertex {
    @builtin(position) position: vec4<f32>,
    @location(0) diameter: f32,
    @location(1) tone: f32,
    @location(2) tip: f32,
    @location(3) coverage: f32,
    @location(4) doc: vec2<f32>,
    @location(5) stamp: vec2<f32>,
}

@group(0) @binding(0)
var<uniform> frame: Frame;

@group(1) @binding(0)
var<uniform> layers: Layers;

@group(1) @binding(1)
var layer_tex: texture_2d_array<f32>;

@group(1) @binding(2)
var layer_sampler: sampler;

fn bayer_rank(x: u32, y: u32) -> u32 {
    let cells = array<u32, 16>(
        0u, 8u, 2u, 10u,
        12u, 4u, 14u, 6u,
        3u, 11u, 1u, 9u,
        15u, 7u, 13u, 5u
    );
    return cells[((y & 3u) * 4u) + (x & 3u)];
}

fn bayer4(p: vec2<f32>) -> f32 {
    return (f32(bayer_rank(u32(floor(p.x)), u32(floor(p.y)))) + 0.5) / 16.0;
}

fn mono(luma: f32, p: vec2<f32>) -> vec4<f32> {
    let bit = select(0.0, 1.0, clamp(luma, 0.0, 1.0) >= bayer4(p));
    return vec4<f32>(bit, bit, bit, 1.0);
}

fn tone_for(value: f32, p: vec2<f32>) -> vec4<f32> {
    let tone = u32(value + 0.5);
    if tone == 0u {
        return vec4<f32>(0.0, 0.0, 0.0, 1.0);
    }
    if tone == 1u {
        return vec4<f32>(0.62, 0.62, 0.62, 1.0);
    }
    if tone == 2u {
        return vec4<f32>(0.78, 0.78, 0.78, 1.0);
    }
    if tone == 3u {
        return vec4<f32>(0.94, 0.94, 0.94, 1.0);
    }
    return vec4<f32>(1.0, 1.0, 1.0, 1.0);
}

fn screen_scale() -> vec2<f32> {
    return max(frame.drawable_size / max(frame.screen_size, vec2<f32>(1.0)), vec2<f32>(1.0));
}

fn logical_to_clip(p: vec2<f32>) -> vec4<f32> {
    let physical = p * screen_scale();
    let ndc = vec2<f32>(
        (physical.x / max(frame.drawable_size.x, 1.0)) * 2.0 - 1.0,
        1.0 - (physical.y / max(frame.drawable_size.y, 1.0)) * 2.0
    );
    return vec4<f32>(ndc, 0.0, 1.0);
}

fn document_to_screen(p: vec2<f32>) -> vec2<f32> {
    let span = frame.document_rect.zw - frame.document_rect.xy;
    return frame.document_rect.xy + ((p / max(frame.document_size, vec2<f32>(1.0))) * span);
}

fn corner(vertex_index: u32) -> vec2<f32> {
    let corners = array<vec2<f32>, 6>(
        vec2<f32>(0.0, 0.0),
        vec2<f32>(1.0, 0.0),
        vec2<f32>(0.0, 1.0),
        vec2<f32>(0.0, 1.0),
        vec2<f32>(1.0, 0.0),
        vec2<f32>(1.0, 1.0)
    );
    return corners[vertex_index];
}

struct FontGlyph {
    metrics: vec4<u32>,
    rows: array<u32, 16>,
}

struct Font {
    metrics: vec4<u32>,
    glyphs: array<FontGlyph, 95>,
}

@group(0) @binding(1)
var<storage, read> font: Font;

fn font_bit(code: u32, x: u32, y: u32) -> bool {
    let first = font.metrics.x;
    let count = font.metrics.y;
    let width = font.metrics.z;
    let height = font.metrics.w;
    if code < first || code >= first + count || x >= width || y >= height {
        return false;
    }
    let row = font.glyphs[code - first].rows[y];
    return ((row >> ((width - 1u) - x)) & 1u) != 0u;
}

fn coverage_row_bits(code: u32, row: u32) -> u32 {
    let first = min(code, 15u);
    var bits = 0u;
    for (var col = 0u; col < 8u; col = col + 1u) {
        if bayer_rank(col, row) >= first {
            bits = bits | (1u << (7u - col));
        }
    }
    return bits;
}

fn icon_row_bits(code: u32, row: u32) -> u32 {
    let pen = array<u32, 8>(0xc0u, 0xe0u, 0x50u, 0x28u, 0x14u, 0x0au, 0x04u, 0x00u);
    let brush = array<u32, 8>(0xe0u, 0xd0u, 0xa8u, 0x44u, 0x22u, 0x12u, 0x0cu, 0x00u);
    let line = array<u32, 8>(0xc0u, 0xb8u, 0x48u, 0x48u, 0x78u, 0x04u, 0x02u, 0x00u);
    let magic = array<u32, 8>(0xa8u, 0x50u, 0x88u, 0x50u, 0xa8u, 0x04u, 0x02u, 0x00u);
    let rect = array<u32, 8>(0xfcu, 0x84u, 0x84u, 0x80u, 0x84u, 0xeau, 0x04u, 0x00u);
    let zoom = array<u32, 8>(0x30u, 0x48u, 0x84u, 0x84u, 0x48u, 0x34u, 0x02u, 0x00u);
    let erase = array<u32, 8>(0x00u, 0x3cu, 0x46u, 0x4au, 0x52u, 0x62u, 0x3cu, 0x00u);
    let size1 = array<u32, 8>(0x00u, 0x00u, 0x00u, 0x10u, 0x00u, 0x00u, 0x00u, 0x00u);
    let size2 = array<u32, 8>(0x00u, 0x00u, 0x10u, 0x38u, 0x10u, 0x00u, 0x00u, 0x00u);
    let size3 = array<u32, 8>(0x00u, 0x00u, 0x38u, 0x38u, 0x38u, 0x00u, 0x00u, 0x00u);
    let size4 = array<u32, 8>(0x00u, 0x10u, 0x38u, 0x7cu, 0x38u, 0x10u, 0x00u, 0x00u);
    let size5 = array<u32, 8>(0x00u, 0x38u, 0x7cu, 0x7cu, 0x7cu, 0x38u, 0x00u, 0x00u);
    let size6 = array<u32, 8>(0x10u, 0x38u, 0x7cu, 0xfeu, 0x7cu, 0x38u, 0x10u, 0x00u);
    let size7 = array<u32, 8>(0x38u, 0x7cu, 0xfeu, 0xfeu, 0xfeu, 0x7cu, 0x38u, 0x00u);
    let size8 = array<u32, 8>(0x7cu, 0xfeu, 0xfeu, 0xfeu, 0xfeu, 0xfeu, 0x7cu, 0x00u);
    let lock_open = array<u32, 8>(0x0cu, 0x12u, 0x10u, 0x7eu, 0x42u, 0x5au, 0x42u, 0x7eu);
    let lock_closed = array<u32, 8>(0x18u, 0x24u, 0x24u, 0x7eu, 0x42u, 0x5au, 0x42u, 0x7eu);

    switch code {
        case 0u: { return pen[row]; }
        case 1u: { return brush[row]; }
        case 2u: { return line[row]; }
        case 3u: { return magic[row]; }
        case 4u: { return rect[row]; }
        case 5u: { return zoom[row]; }
        case 6u: { return erase[row]; }
        case 7u: { return size1[row]; }
        case 8u: { return size2[row]; }
        case 9u: { return size3[row]; }
        case 10u: { return size4[row]; }
        case 11u: { return size5[row]; }
        case 12u: { return size6[row]; }
        case 13u: { return size7[row]; }
        case 14u: { return size8[row]; }
        case 15u: { return coverage_row_bits(0u, row); }
        case 16u: { return coverage_row_bits(1u, row); }
        case 17u: { return coverage_row_bits(2u, row); }
        case 18u: { return coverage_row_bits(3u, row); }
        case 19u: { return coverage_row_bits(4u, row); }
        case 20u: { return coverage_row_bits(5u, row); }
        case 21u: { return coverage_row_bits(6u, row); }
        case 22u: { return coverage_row_bits(7u, row); }
        case 23u: { return coverage_row_bits(8u, row); }
        case 24u: { return coverage_row_bits(9u, row); }
        case 25u: { return coverage_row_bits(10u, row); }
        case 26u: { return coverage_row_bits(11u, row); }
        case 27u: { return coverage_row_bits(12u, row); }
        case 28u: { return coverage_row_bits(13u, row); }
        case 29u: { return coverage_row_bits(14u, row); }
        case 30u: { return coverage_row_bits(15u, row); }
        case 31u: { return lock_open[row]; }
        case 32u: { return lock_closed[row]; }
        case 33u: { return tip_row_bits(0u, row); }
        case 34u: { return tip_row_bits(1u, row); }
        case 35u: { return tip_row_bits(2u, row); }
        case 36u: { return tip_row_bits(3u, row); }
        case 37u: { return tip_row_bits(4u, row); }
        case 38u: { return tip_row_bits(5u, row); }
        case 39u: { return tip_row_bits(6u, row); }
        case 40u: { return tip_row_bits(7u, row); }
        default: { return 0u; }
    }
}

fn tip_row_bits(code: u32, row: u32) -> u32 {
    let round = array<u32, 8>(0x00u, 0x38u, 0x7cu, 0x7cu, 0x7cu, 0x38u, 0x00u, 0x00u);
    let square = array<u32, 8>(0x00u, 0x7cu, 0x7cu, 0x7cu, 0x7cu, 0x7cu, 0x00u, 0x00u);
    let slash_r = array<u32, 8>(0xc0u, 0xe0u, 0x70u, 0x38u, 0x1cu, 0x0eu, 0x06u, 0x00u);
    let slash_l = array<u32, 8>(0x06u, 0x0eu, 0x1cu, 0x38u, 0x70u, 0xe0u, 0xc0u, 0x00u);
    let flat_h = array<u32, 8>(0x00u, 0x00u, 0x00u, 0xfeu, 0xfeu, 0x00u, 0x00u, 0x00u);
    let flat_v = array<u32, 8>(0x18u, 0x18u, 0x18u, 0x18u, 0x18u, 0x18u, 0x18u, 0x00u);
    let rake = array<u32, 8>(0x92u, 0x92u, 0x92u, 0x92u, 0x92u, 0x92u, 0x92u, 0x00u);
    let scatter = array<u32, 8>(0x44u, 0x10u, 0x82u, 0x28u, 0x01u, 0x44u, 0x10u, 0x00u);

    switch code {
        case 0u: { return round[row]; }
        case 1u: { return square[row]; }
        case 2u: { return slash_r[row]; }
        case 3u: { return slash_l[row]; }
        case 4u: { return flat_h[row]; }
        case 5u: { return flat_v[row]; }
        case 6u: { return rake[row]; }
        case 7u: { return scatter[row]; }
        default: { return round[row]; }
    }
}

fn coverage_on(code: u32, x: i32, y: i32) -> bool {
    return bayer_rank(u32(x & 3), u32(y & 3)) >= min(code, 15u);
}

fn stamp_local(doc: vec2<f32>, stamp: vec2<f32>, diameter: f32) -> vec2<f32> {
    let d = max(diameter, 1.0);
    let center = stamp + vec2<f32>(0.5, 0.5);
    let origin = center - vec2<f32>(d * 0.5, d * 0.5);
    let pixel_center = floor(doc) + vec2<f32>(0.5, 0.5);
    return ((pixel_center - origin) / d) * 8.0;
}

fn stamp_on(in: StampVertex) -> bool {
    let local = stamp_local(in.doc, in.stamp, in.diameter);
    if local.x < 0.0 || local.y < 0.0 || local.x >= 8.0 || local.y >= 8.0 {
        return false;
    }

    let ix = u32(floor(local.x));
    let iy = u32(floor(local.y));
    if in.diameter > 1.5 {
        let bits = tip_row_bits(u32(in.tip + 0.5), iy);
        if ((bits >> (7u - ix)) & 1u) == 0u {
            return false;
        }
    }
    return coverage_on(u32(in.coverage + 0.5), i32(floor(in.doc.x)), i32(floor(in.doc.y)));
}

@vertex
fn vs_composite(@builtin(vertex_index) vertex_index: u32) -> CompositeVertex {
    let c = corner(vertex_index);
    let screen = mix(frame.document_rect.xy, frame.document_rect.zw, c);
    var out: CompositeVertex;
    out.position = logical_to_clip(screen);
    out.screen = screen;
    out.uv = c;
    return out;
}

@fragment
fn fs_composite(in: CompositeVertex) -> @location(0) vec4<f32> {
    let inside_viewport =
        in.screen.x >= frame.viewport_rect.x &&
        in.screen.y >= frame.viewport_rect.y &&
        in.screen.x < frame.viewport_rect.z &&
        in.screen.y < frame.viewport_rect.w;
    if inside_viewport == false {
        discard;
    }

    var luma = 0.0;
    for (var step = 0u; step < 16u; step = step + 1u) {
        if step >= frame.layer_count {
            continue;
        }
        let index = frame.layer_count - 1u - step;
        let row = layers.rows[index];
        let visible = (u32(row.x + 0.5) & 1u) != 0u;
        if visible == false {
            continue;
        }
        let opacity = clamp(row.y, 0.0, 1.0);
        let slot = i32(row.z + 0.5);
        let kind = u32(row.w + 0.5);
        if kind == 2u {
            luma = mix(luma, 1.0, opacity);
        } else if kind == 1u {
            let image = textureSampleLevel(layer_tex, layer_sampler, in.uv, slot, 0.0).r;
            luma = mix(luma, image, opacity);
        } else {
            let ink = textureSampleLevel(layer_tex, layer_sampler, in.uv, slot, 0.0).r;
            luma = mix(luma, 0.0, ink * opacity);
        }
    }
    return mono(luma, in.uv * frame.document_size);
}

@vertex
fn vs_stamp(@builtin(vertex_index) vertex_index: u32,
    @location(0) stamp: vec4<f32>,
    @location(1) attrs: vec4<f32>) -> StampVertex {
    let c = corner(vertex_index);
    let diameter = max(stamp.z, 1.0);
    let center = stamp.xy + vec2<f32>(0.5, 0.5);
    let p = center + ((c * diameter) - vec2<f32>(diameter * 0.5));
    let ndc = vec2<f32>(
        (p.x / max(frame.document_size.x, 1.0)) * 2.0 - 1.0,
        1.0 - (p.y / max(frame.document_size.y, 1.0)) * 2.0
    );
    var out: StampVertex;
    out.position = vec4<f32>(ndc, 0.0, 1.0);
    out.diameter = stamp.z;
    out.tone = stamp.w;
    out.tip = attrs.y;
    out.coverage = attrs.z;
    out.doc = p;
    out.stamp = stamp.xy;
    return out;
}

@fragment
fn fs_stamp(in: StampVertex) -> @location(0) vec4<f32> {
    if !stamp_on(in) {
        discard;
    }
    return vec4<f32>(1.0 - clamp(in.tone, 0.0, 1.0), 0.0, 0.0, 1.0);
}

@vertex
fn vs_screen_stamp(@builtin(vertex_index) vertex_index: u32,
    @location(0) stamp: vec4<f32>,
    @location(1) attrs: vec4<f32>) -> StampVertex {
    let c = corner(vertex_index);
    let diameter = max(stamp.z, 1.0);
    let center = stamp.xy + vec2<f32>(0.5, 0.5);
    let p = center + ((c * diameter) - vec2<f32>(diameter * 0.5));
    var out: StampVertex;
    out.position = logical_to_clip(document_to_screen(p));
    out.diameter = stamp.z;
    out.tone = stamp.w;
    out.tip = attrs.y;
    out.coverage = attrs.z;
    out.doc = p;
    out.stamp = stamp.xy;
    return out;
}

@fragment
fn fs_screen_stamp(in: StampVertex) -> @location(0) vec4<f32> {
    let screen = document_to_screen(in.doc);
    let in_view =
        screen.x >= frame.viewport_rect.x &&
        screen.y >= frame.viewport_rect.y &&
        screen.x < frame.viewport_rect.z &&
        screen.y < frame.viewport_rect.w;
    let in_doc =
        in.doc.x >= 0.0 &&
        in.doc.y >= 0.0 &&
        in.doc.x < frame.document_size.x &&
        in.doc.y < frame.document_size.y;
    if !in_view || !in_doc {
        discard;
    }

    if !stamp_on(in) {
        discard;
    }
    return tone_for(in.tone, in.position.xy);
}

@vertex
fn vs_rect(@builtin(vertex_index) vertex_index: u32,
    @location(0) bounds: vec4<f32>,
    @location(1) attrs: vec4<f32>) -> RectVertex {
    let c = corner(vertex_index);
    let p = mix(bounds.xy, bounds.zw, c);
    var out: RectVertex;
    out.position = logical_to_clip(p);
    out.tone = attrs.x;
    return out;
}

@fragment
fn fs_rect(in: RectVertex) -> @location(0) vec4<f32> {
    return tone_for(in.tone, in.position.xy);
}

@vertex
fn vs_glyph(@builtin(vertex_index) vertex_index: u32,
    @location(0) origin: vec4<f32>,
    @location(1) attrs: vec4<f32>) -> GlyphVertex {
    let scale = max(origin.z, 1.0);
    let size = vec2<f32>(f32(font.metrics.z) * scale, f32(font.metrics.w) * scale);
    let local = corner(vertex_index) * size;
    let p = origin.xy + local;
    var out: GlyphVertex;
    out.position = logical_to_clip(p);
    out.local = local;
    out.code = attrs.x;
    out.tone = attrs.y;
    out.scale = scale;
    return out;
}

@fragment
fn fs_glyph(in: GlyphVertex) -> @location(0) vec4<f32> {
    let scale = max(in.scale, 1.0);
    let gx = u32(floor(in.local.x / scale));
    let gy = u32(floor(in.local.y / scale));
    if !font_bit(u32(in.code + 0.5), gx, gy) {
        discard;
    }
    return tone_for(in.tone, in.position.xy);
}

@vertex
fn vs_icon(@builtin(vertex_index) vertex_index: u32,
    @location(0) origin: vec4<f32>,
    @location(1) attrs: vec4<f32>) -> IconVertex {
    let scale = max(origin.z, 0.125);
    let size = vec2<f32>(8.0 * scale, 8.0 * scale);
    let local = corner(vertex_index) * size;
    let p = origin.xy + local;
    var out: IconVertex;
    out.position = logical_to_clip(p);
    out.local = local;
    out.code = attrs.x;
    out.tone = attrs.y;
    out.scale = scale;
    return out;
}

@fragment
fn fs_icon(in: IconVertex) -> @location(0) vec4<f32> {
    let scale = max(in.scale, 0.125);
    let ix = u32(floor(in.local.x / scale));
    let iy = u32(floor(in.local.y / scale));
    if ix >= 8u || iy >= 8u {
        discard;
    }

    let bits = icon_row_bits(u32(in.code + 0.5), iy);
    if ((bits >> (7u - ix)) & 1u) == 0u {
        discard;
    }
    return tone_for(in.tone, in.position.xy);
}
