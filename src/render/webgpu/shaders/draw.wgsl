struct Frame {
    drawable_size: vec2<f32>,
    screen_size: vec2<f32>,
    draw_count: u32,
    clip_count: u32,
    sample_count: u32,
    _pad: u32,
}

struct Clip {
    x0: f32,
    y0: f32,
    x1: f32,
    y1: f32,
}

struct Draw {
    x0: f32,
    y0: f32,
    x1: f32,
    y1: f32,
    p0: f32,
    p1: f32,
    data0: u32,
    data1: u32,
}

struct Sample {
    y: f32,
    flags: u32,
}

@group(0) @binding(0)
var<uniform> frame: Frame;

@group(0) @binding(1)
var<storage, read> draws: array<Draw>;

@group(0) @binding(2)
var<storage, read> clips: array<Clip>;

@group(0) @binding(3)
var<storage, read> text_words: array<u32>;

@group(0) @binding(4)
var<storage, read> samples: array<Sample>;

const KIND_FILL: u32 = 0u;
const KIND_STROKE: u32 = 1u;
const KIND_DASH: u32 = 2u;
const KIND_TEXT: u32 = 3u;
const KIND_GRAPH: u32 = 4u;

// Repeating 4x4 Bayer threshold matrix for ordered grayscale dithering.
fn bayer4(cell: vec2<u32>) -> f32 {
    let table = array<f32, 16>(
        0.0, 8.0, 2.0, 10.0,
        12.0, 4.0, 14.0, 6.0,
        3.0, 11.0, 1.0, 9.0,
        15.0, 7.0, 13.0, 5.0
    );
    // & 3u is modulo 4 for unsigned integers, mapping any cell into the tile.
    let index = ((cell.y & 3u) * 4u) + (cell.x & 3u);
    return (table[index] + 0.5) / 16.0;
}

fn draw_kind(d: Draw) -> u32 {
    return (d.data1 >> 16u) & 255u;
}

fn draw_clip_index(d: Draw) -> u32 {
    return (d.data1 >> 8u) & 255u;
}

fn draw_luma(d: Draw) -> f32 {
    return f32((d.data1 >> 24u) & 255u) / 255.0;
}

fn text_offset(d: Draw) -> u32 {
    return d.data0 & 65535u;
}

fn text_length(d: Draw) -> u32 {
    return (d.data0 >> 16u) & 65535u;
}

fn text_byte(offset: u32) -> u32 {
    let word = text_words[offset >> 2u];
    return (word >> ((offset & 3u) * 8u)) & 255u;
}

fn in_clip(p: vec2<f32>, clip_index: u32) -> bool {
    if clip_index >= frame.clip_count {
        return false;
    }
    let clip = clips[clip_index];
    return p.x >= clip.x0 && p.x < clip.x1 && p.y >= clip.y0 && p.y < clip.y1;
}

// Signed-distance helpers keep draw evaluation compact in the fragment pass.
fn sd_box(p: vec2<f32>, center: vec2<f32>, half_size: vec2<f32>) -> f32 {
    let d = abs(p - center) - half_size;
    return length(max(d, vec2<f32>(0.0))) + min(max(d.x, d.y), 0.0);
}

fn fill(distance: f32) -> f32 {
    return 1.0 - smoothstep(-0.5, 0.5, distance);
}

fn stroke(distance: f32, width: f32) -> f32 {
    return 1.0 - smoothstep(width, width + 1.0, abs(distance));
}

fn sd_segment(p: vec2<f32>, a: vec2<f32>, b: vec2<f32>) -> f32 {
    let pa = p - a;
    let ba = b - a;
    let h = clamp(dot(pa, ba) / max(dot(ba, ba), 0.0001), 0.0, 1.0);
    return length(pa - (ba * h));
}

fn dashed_segment(p: vec2<f32>, a: vec2<f32>, b: vec2<f32>, width: f32, dash: f32) -> f32 {
    let ba = b - a;
    let length_ba = max(length(ba), 0.0001);
    let axis = ba / length_ba;
    let along = dot(p - a, axis);
    let pattern = select(0.0, 1.0, fract(along / max(dash, 1.0)) < 0.55);
    let inside = step(0.0, along) * step(along, length_ba);
    return (1.0 - smoothstep(width, width + 1.0, sd_segment(p, a, b))) * pattern * inside;
}

fn font_row_bits(code: u32, row: u32) -> u32 {
    let blank = array<u32, 7>(0u, 0u, 0u, 0u, 0u, 0u, 0u);
    let a = array<u32, 7>(14u, 17u, 17u, 31u, 17u, 17u, 17u);
    let d = array<u32, 7>(30u, 17u, 17u, 17u, 17u, 17u, 30u);
    let i = array<u32, 7>(14u, 4u, 4u, 4u, 4u, 4u, 14u);
    let l = array<u32, 7>(16u, 16u, 16u, 16u, 16u, 16u, 31u);
    let m = array<u32, 7>(17u, 27u, 21u, 21u, 17u, 17u, 17u);
    let r = array<u32, 7>(30u, 17u, 17u, 30u, 20u, 18u, 17u);

    switch code {
        case 65u: { return a[row]; }
        case 68u: { return d[row]; }
        case 73u: { return i[row]; }
        case 76u: { return l[row]; }
        case 77u: { return m[row]; }
        case 82u: { return r[row]; }
        default: { return blank[row]; }
    }
}

fn text_mask(d: Draw, p: vec2<f32>) -> f32 {
    let scale = max(d.p0, 1.0);
    let local = p - vec2<f32>(d.x0, d.y0);
    if local.x < 0.0 || local.y < 0.0 || local.y >= 7.0 * scale {
        return 0.0;
    }

    let advance = 6.0 * scale;
    let glyph_index = u32(floor(local.x / advance));
    if glyph_index >= text_length(d) {
        return 0.0;
    }

    let glyph_x = u32(floor((local.x - (f32(glyph_index) * advance)) / scale));
    let glyph_y = u32(floor(local.y / scale));
    if glyph_x >= 5u || glyph_y >= 7u {
        return 0.0;
    }

    let bits = font_row_bits(text_byte(text_offset(d) + glyph_index), glyph_y);
    return f32((bits >> (4u - glyph_x)) & 1u);
}

fn graph_curve_mask(d: Draw, p: vec2<f32>) -> f32 {
    if p.x < d.x0 || p.x >= d.x1 || p.y < d.y0 || p.y >= d.y1 {
        return 0.0;
    }

    let offset = d.data0 & 65535u;
    if offset >= frame.sample_count {
        return 0.0;
    }
    let count = min((d.data0 >> 16u) & 65535u, frame.sample_count - offset);
    if count < 2u {
        return 0.0;
    }

    let t = clamp((p.x - d.x0) / max(d.x1 - d.x0, 1.0), 0.0, 1.0);
    let sample_x = t * f32(count - 1u);
    let sample0 = min(u32(floor(sample_x)), count - 2u);
    let sample1 = sample0 + 1u;
    let y0 = samples[offset + sample0].y;
    let y1 = samples[offset + sample1].y;
    let y = mix(y0, y1, fract(sample_x));
    return 1.0 - smoothstep(max(d.p0, 1.0), max(d.p0, 1.0) + 1.0, abs(p.y - y));
}

@vertex
fn vs_main(@builtin(vertex_index) vertex_index: u32) -> @builtin(position) vec4<f32> {
    let positions = array<vec2<f32>, 6>(
        vec2<f32>(-1.0, -1.0),
        vec2<f32>(1.0, -1.0),
        vec2<f32>(-1.0, 1.0),
        vec2<f32>(-1.0, 1.0),
        vec2<f32>(1.0, -1.0),
        vec2<f32>(1.0, 1.0)
    );
    return vec4<f32>(positions[vertex_index], 0.0, 1.0);
}

@fragment
fn fs_main(@builtin(position) position: vec4<f32>) -> @location(0) vec4<f32> {
    let screen_scale = max(frame.drawable_size / max(frame.screen_size, vec2<f32>(1.0)), vec2<f32>(1.0));
    let cell_size = min(screen_scale.x, screen_scale.y);
    let cell = vec2<u32>(floor(position.xy / cell_size));
    let local = fract(position.xy / cell_size);
    let p = (vec2<f32>(cell) + vec2<f32>(0.5));

    var luma = 0.0;
    for (var index: u32 = 0u; index < frame.draw_count; index = index + 1u) {
        let d = draws[index];
        if !in_clip(p, draw_clip_index(d)) {
            continue;
        }

        let kind = draw_kind(d);
        let ink = draw_luma(d);
        if kind == KIND_FILL {
            let inside = select(0.0, 1.0,
                p.x >= d.x0 && p.x < d.x1 && p.y >= d.y0 && p.y < d.y1);
            luma = mix(luma, ink, inside);
        } else if kind == KIND_STROKE {
            let center = (vec2<f32>(d.x0, d.y0) + vec2<f32>(d.x1, d.y1)) * 0.5;
            let half_size = abs(vec2<f32>(d.x1 - d.x0, d.y1 - d.y0)) * 0.5;
            luma = max(luma, stroke(sd_box(p, center, half_size), max(d.p0, 1.0)) * ink);
        } else if kind == KIND_DASH {
            luma = max(luma, dashed_segment(p, vec2<f32>(d.x0, d.y0),
                vec2<f32>(d.x1, d.y1), max(d.p0, 1.0), max(d.p1, 1.0)) * ink);
        } else if kind == KIND_TEXT {
            luma = max(luma, text_mask(d, p) * ink);
        } else if kind == KIND_GRAPH {
            luma = max(luma, graph_curve_mask(d, p) * ink);
        }
    }

    let threshold = bayer4(cell);
    // Draw rows store gray luma; Bayer dither turns it into screen cells.
    var out_luma = select(0.06, 0.88, clamp(luma, 0.0, 1.0) > threshold);
    let border = step(0.80, max(abs(local.x - 0.5), abs(local.y - 0.5)) * 2.0);
    out_luma = mix(out_luma, out_luma * 0.62, border * 0.60);

    return vec4<f32>(vec3<f32>(out_luma), 1.0);
}
