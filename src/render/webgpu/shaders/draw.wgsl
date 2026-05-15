struct Frame {
    drawable_size: vec2<f32>,
    screen_size: vec2<f32>,
    rect_count: u32,
    glyph_count: u32,
    icon_count: u32,
    _pad1: u32,
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

@group(0) @binding(0)
var<uniform> frame: Frame;

fn tone_for(value: f32) -> vec4<f32> {
    if value < 0.5 {
        return vec4<f32>(0.0, 0.0, 0.0, 1.0);
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

fn font_row_bits(code: u32, row: u32) -> u32 {
    let blank = array<u32, 7>(0u, 0u, 0u, 0u, 0u, 0u, 0u);
    let a = array<u32, 7>(14u, 17u, 17u, 31u, 17u, 17u, 17u);
    let b = array<u32, 7>(30u, 17u, 17u, 30u, 17u, 17u, 30u);
    let c = array<u32, 7>(14u, 17u, 16u, 16u, 16u, 17u, 14u);
    let d = array<u32, 7>(30u, 17u, 17u, 17u, 17u, 17u, 30u);
    let e = array<u32, 7>(31u, 16u, 16u, 30u, 16u, 16u, 31u);
    let f = array<u32, 7>(31u, 16u, 16u, 30u, 16u, 16u, 16u);
    let g = array<u32, 7>(14u, 17u, 16u, 23u, 17u, 17u, 15u);
    let h = array<u32, 7>(17u, 17u, 17u, 31u, 17u, 17u, 17u);
    let i = array<u32, 7>(14u, 4u, 4u, 4u, 4u, 4u, 14u);
    let k = array<u32, 7>(17u, 18u, 20u, 24u, 20u, 18u, 17u);
    let l = array<u32, 7>(16u, 16u, 16u, 16u, 16u, 16u, 31u);
    let m = array<u32, 7>(17u, 27u, 21u, 21u, 17u, 17u, 17u);
    let n = array<u32, 7>(17u, 25u, 21u, 19u, 17u, 17u, 17u);
    let o = array<u32, 7>(14u, 17u, 17u, 17u, 17u, 17u, 14u);
    let p = array<u32, 7>(30u, 17u, 17u, 30u, 16u, 16u, 16u);
    let r = array<u32, 7>(30u, 17u, 17u, 30u, 20u, 18u, 17u);
    let s = array<u32, 7>(15u, 16u, 16u, 14u, 1u, 1u, 30u);
    let t = array<u32, 7>(31u, 4u, 4u, 4u, 4u, 4u, 4u);
    let u = array<u32, 7>(17u, 17u, 17u, 17u, 17u, 17u, 14u);
    let v = array<u32, 7>(17u, 17u, 17u, 17u, 17u, 10u, 4u);
    let w = array<u32, 7>(17u, 17u, 17u, 21u, 21u, 21u, 10u);
    let y = array<u32, 7>(17u, 17u, 10u, 4u, 4u, 4u, 4u);
    let z = array<u32, 7>(31u, 1u, 2u, 4u, 8u, 16u, 31u);

    switch code {
        case 65u: { return a[row]; }
        case 66u: { return b[row]; }
        case 67u: { return c[row]; }
        case 68u: { return d[row]; }
        case 69u: { return e[row]; }
        case 70u: { return f[row]; }
        case 71u: { return g[row]; }
        case 72u: { return h[row]; }
        case 73u: { return i[row]; }
        case 75u: { return k[row]; }
        case 76u: { return l[row]; }
        case 77u: { return m[row]; }
        case 78u: { return n[row]; }
        case 79u: { return o[row]; }
        case 80u: { return p[row]; }
        case 82u: { return r[row]; }
        case 83u: { return s[row]; }
        case 84u: { return t[row]; }
        case 85u: { return u[row]; }
        case 86u: { return v[row]; }
        case 87u: { return w[row]; }
        case 89u: { return y[row]; }
        case 90u: { return z[row]; }
        default: { return blank[row]; }
    }
}

fn icon_row_bits(code: u32, row: u32) -> u32 {
    let pen = array<u32, 8>(0xc0u, 0xe0u, 0x50u, 0x28u, 0x14u, 0x0au, 0x04u, 0x00u);
    let brush = array<u32, 8>(0xe0u, 0xd0u, 0xa8u, 0x44u, 0x22u, 0x12u, 0x0cu, 0x00u);
    let line = array<u32, 8>(0xc0u, 0xb8u, 0x48u, 0x48u, 0x78u, 0x04u, 0x02u, 0x00u);
    let magic = array<u32, 8>(0xa8u, 0x50u, 0x88u, 0x50u, 0xa8u, 0x04u, 0x02u, 0x00u);
    let rect = array<u32, 8>(0xfcu, 0x84u, 0x84u, 0x80u, 0x84u, 0xeau, 0x04u, 0x00u);
    let zoom = array<u32, 8>(0x30u, 0x48u, 0x84u, 0x84u, 0x48u, 0x34u, 0x02u, 0x00u);
    let erase = array<u32, 8>(0x00u, 0x3cu, 0x46u, 0x4au, 0x52u, 0x62u, 0x3cu, 0x00u);

    switch code {
        case 0u: { return pen[row]; }
        case 1u: { return brush[row]; }
        case 2u: { return line[row]; }
        case 3u: { return magic[row]; }
        case 4u: { return rect[row]; }
        case 5u: { return zoom[row]; }
        case 6u: { return erase[row]; }
        default: { return 0u; }
    }
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
    return tone_for(in.tone);
}

@vertex
fn vs_glyph(@builtin(vertex_index) vertex_index: u32,
    @location(0) origin: vec4<f32>,
    @location(1) attrs: vec4<f32>) -> GlyphVertex {
    let scale = max(origin.z, 1.0);
    let size = vec2<f32>(5.0 * scale, 7.0 * scale);
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
    if gx >= 5u || gy >= 7u {
        discard;
    }

    let bits = font_row_bits(u32(in.code + 0.5), gy);
    if ((bits >> (4u - gx)) & 1u) == 0u {
        discard;
    }
    return tone_for(in.tone);
}

@vertex
fn vs_icon(@builtin(vertex_index) vertex_index: u32,
    @location(0) origin: vec4<f32>,
    @location(1) attrs: vec4<f32>) -> IconVertex {
    let scale = max(origin.z, 1.0);
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
    let scale = max(in.scale, 1.0);
    let ix = u32(floor(in.local.x / scale));
    let iy = u32(floor(in.local.y / scale));
    if ix >= 8u || iy >= 8u {
        discard;
    }

    let bits = icon_row_bits(u32(in.code + 0.5), iy);
    if ((bits >> (7u - ix)) & 1u) == 0u {
        discard;
    }
    return tone_for(in.tone);
}
