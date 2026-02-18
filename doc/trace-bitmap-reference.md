# Trace Bitmap Recreation Reference (Multicolor / Colors / Stack ON)

This document is a **single-file, implementation-oriented specification** for recreating the Inkscape Trace Bitmap behavior for:

- Multiple scans
- Detection mode: **Colors** (`MS_C` / `TRACE_QUANT_COLOR`)
- **Stack ON** (assumed always on in this reference)
- Potrace shaping settings:
  - `turdsize = 2` (Speckles)
  - `alphamax = 1.0` (Smooth corners)
  - `opticurve = true`, `opttolerance = 0.2` (Optimize)

The emphasis here is the exact manner in which:
1. the source image is quantized,
2. each color index is iterated,
3. the pass mask is built for each index under **Stack ON**,
4. each traced layer is appended in deterministic index order.

---

## 1) Ground truth mapping (UI -> engine)

Use this as your fixed parameter mapping for parity:

```cpp
traceType = TRACE_QUANT_COLOR;      // mode "Colors" in multiscans
multiScanNrColors = scans;          // requested layer count
multiScanStack = true;              // this reference assumes always ON
multiScanSmooth = true|false;       // gaussian pre-blur before quantization
multiScanRemoveBackground = false;  // optional final behavior

potraceParams->opticurve    = true;
potraceParams->opttolerance = 0.2;
potraceParams->alphamax     = 1.0;
potraceParams->turdsize     = 2;
```

---

## 2) Canonical pipeline overview

The flow is:

1. Convert source bitmap to RGB map.
2. If configured, apply Gaussian blur to RGB map (**before quantization**).
3. Quantize RGB map to an `IndexedMap` with `scans` palette entries.
4. Create one shared binary `GrayMap` mask (white initialized).
5. For each `colorIndex` in `[0, nrColors)`:
   - fill mask pixels belonging to that index with black,
   - because Stack is ON, leave non-matching pixels unchanged,
   - trace resulting mask with Potrace,
   - style traced path with palette color `clut[colorIndex]`,
   - append traced layer in loop order.
6. Optionally remove bottom-most layer if requested.

Important: with **Stack ON**, the mask is cumulative from one pass to the next.

---

## 3) Quantization details (exact behavior to reproduce)

### 3.1 Pre-quantization blur

If multiscans smooth is enabled, blur happens *before* quantization.
That reduces high-frequency edge stair-stepping and small isolated classes.

```cpp
RgbMap *rgb = gdkPixbufToRgbMap(pixbuf);
IndexedMap *indexed = nullptr;

if (smooth_preblur) {
    RgbMap *gauss = rgbMapGaussian(rgb);
    indexed = rgbMapQuantize(gauss, scans);
    gauss->destroy(gauss);
} else {
    indexed = rgbMapQuantize(rgb, scans);
}
rgb->destroy(rgb);
```

### 3.2 Quantizer output contract

Your indexed result must provide:
- `indexed->width`, `indexed->height`
- `indexed->nrColors`
- `indexed->getPixel(indexed, x, y)` -> integer class index in `[0, nrColors)`
- `indexed->clut[index]` -> the RGB color for style output

Avoid introducing dithering if you are pursuing strict parity.

---

## 4) Color iteration + mask building with Stack ON (deep detail)

This is the critical section.

### 4.1 Shared mask initialization

A single GrayMap is allocated once, then initialized all white.

```cpp
GrayMap *gm = GrayMapCreate(indexed->width, indexed->height);
for (int y = 0; y < gm->height; ++y) {
    for (int x = 0; x < gm->width; ++x) {
        gm->setPixel(gm, x, y, GRAYMAP_WHITE);
    }
}
```

### 4.2 Per-index update rule (Stack ON)

For each pass `colorIndex`:
- if pixel belongs to current index: set black
- else: **do nothing** (because Stack ON)

```cpp
for (int colorIndex = 0; colorIndex < indexed->nrColors; ++colorIndex) {
    for (int y = 0; y < indexed->height; ++y) {
        for (int x = 0; x < indexed->width; ++x) {
            int idx = (int)indexed->getPixel(indexed, x, y);
            if (idx == colorIndex) {
                gm->setPixel(gm, x, y, GRAYMAP_BLACK);
            }
            // Stack ON: no reset for non-matching idx.
        }
    }

    std::string d = grayMapToPath(gm, params);
    ... append layer ...
}
```

### 4.3 What cumulative means in practice

Because you never reset non-matching pixels to white,
black pixels from prior passes remain black.

That means:
- pass 0 traces class 0
- pass 1 traces class 0 ∪ class 1
- pass 2 traces class 0 ∪ class 1 ∪ class 2
- ... and so on

This is exactly why stack mode yields "no gaps" behavior visually.

---

## 5) Side-by-side micro example (2D index grid)

Given a quantized 3x3 map:

```text
0 0 1
0 2 1
3 2 1
```

Initialize `gm = WHITE everywhere`.

### Pass `colorIndex = 0`
Set class-0 cells black:

```text
B B W
B W W
W W W
```

Trace this -> append Layer[0] style = `clut[0]`.

### Pass `colorIndex = 1` (Stack ON)
Set class-1 cells black; keep existing black pixels:

```text
B B B
B W B
W W B
```

Trace this cumulative mask -> append Layer[1] style = `clut[1]`.

### Pass `colorIndex = 2`

```text
B B B
B B B
W B B
```

append Layer[2]

### Pass `colorIndex = 3`

```text
B B B
B B B
B B B
```

append Layer[3]

This demonstrates cumulative growth under Stack ON.

---

## 6) Tracing call specifics and style assignment

Per pass:
1. Convert GrayMap to Potrace bitmap.
2. Run Potrace using fixed params.
3. Convert Potrace output to path `d`.
4. Use `clut[colorIndex]` to generate `fill:#rrggbb` style.
5. Append layer to output vector.

```cpp
std::string d = grayMapToPath(gm, params);
if (!d.empty()) {
    RGB rgb = indexed->clut[colorIndex];
    char style_buf[32];
    snprintf(style_buf, sizeof(style_buf), "fill:#%02x%02x%02x", rgb.r, rgb.g, rgb.b);
    out.push_back({style_buf, d});
}
```

---

## 7) Exact append order and rendering implications

### 7.1 Append order contract

Layers are appended in strictly ascending `colorIndex`:

```cpp
for (int colorIndex = 0; colorIndex < indexed->nrColors; ++colorIndex) {
    ...
    out.push_back(layer_for_this_index);
}
```

So `out[0]` comes from `colorIndex=0`, `out[1]` from `colorIndex=1`, etc.

### 7.2 Paint order semantics

If your renderer paints in insertion order:
- `out[0]` is painted first (bottom)
- `out[n-1]` is painted last (top)

Because masks are cumulative under Stack ON, upper layers can visually dominate if not composited the same way as your reference system.

### 7.3 Do not reorder during parity work

For accurate recreation, do **not** sort by area, hue, or brightness unless explicitly intended.
Reordering breaks deterministic parity even if tracing itself is correct.

---

## 8) Fully consolidated reference function (Stack ON profile)

```cpp
struct TraceBitmapConfig {
    int scans = 8;                     // UI range typically 2..256
    bool smooth_preblur = true;        // multiscans Smooth
    bool remove_background = false;    // optional post step

    // fixed shaping profile requested:
    bool optimize = true;
    double optimize_tolerance = 0.2;
    double smooth_corners = 1.0;
    int speckles = 2;
};

struct Layer {
    std::string style; // e.g. fill:#000000
    std::string d;     // SVG path data
};

std::vector<Layer> trace_bitmap_multicolor_colors_stack_on(
    GdkPixbuf *pixbuf,
    TraceBitmapConfig const &cfg)
{
    std::vector<Layer> out;
    if (!pixbuf) return out;

    // 1) Quantize
    RgbMap *rgb = gdkPixbufToRgbMap(pixbuf);
    if (!rgb) return out;

    IndexedMap *indexed = nullptr;
    if (cfg.smooth_preblur) {
        RgbMap *gauss = rgbMapGaussian(rgb);
        indexed = rgbMapQuantize(gauss, cfg.scans);
        gauss->destroy(gauss);
    } else {
        indexed = rgbMapQuantize(rgb, cfg.scans);
    }
    rgb->destroy(rgb);
    if (!indexed) return out;

    // 2) Potrace params
    potrace_param_t *params = potrace_param_default();
    params->opticurve = cfg.optimize;
    params->opttolerance = cfg.optimize_tolerance;
    params->alphamax = cfg.smooth_corners;
    params->turdsize = cfg.speckles;

    // 3) Shared cumulative mask
    GrayMap *gm = GrayMapCreate(indexed->width, indexed->height);
    for (int y = 0; y < gm->height; ++y) {
        for (int x = 0; x < gm->width; ++x) {
            gm->setPixel(gm, x, y, GRAYMAP_WHITE);
        }
    }

    // 4) Iterate classes in index order and append in same order
    for (int colorIndex = 0; colorIndex < indexed->nrColors; ++colorIndex) {
        for (int y = 0; y < indexed->height; ++y) {
            for (int x = 0; x < indexed->width; ++x) {
                int idx = (int)indexed->getPixel(indexed, x, y);
                if (idx == colorIndex) {
                    gm->setPixel(gm, x, y, GRAYMAP_BLACK);
                }
                // STACK ON: no white reset for non-match.
            }
        }

        std::string d = grayMapToPath(gm, params);
        if (d.empty()) continue;

        RGB rgbc = indexed->clut[colorIndex];
        char style_buf[32];
        snprintf(style_buf, sizeof(style_buf), "fill:#%02x%02x%02x", rgbc.r, rgbc.g, rgbc.b);

        out.push_back({style_buf, d});
    }

    // 5) Optional behavior
    if (cfg.remove_background && out.size() > 1) {
        out.erase(out.end() - 1);
    }

    gm->destroy(gm);
    indexed->destroy(indexed);
    potrace_param_free(params);
    return out;
}
```

---

## 9) Instrumentation examples (recommended during recreation)

### 9.1 Log class histogram before tracing

```cpp
std::vector<int> class_counts(indexed->nrColors, 0);
for (int y = 0; y < indexed->height; ++y) {
    for (int x = 0; x < indexed->width; ++x) {
        class_counts[indexed->getPixel(indexed, x, y)]++;
    }
}
for (int i = 0; i < indexed->nrColors; ++i) {
    printf("class %d pixels=%d\n", i, class_counts[i]);
}
```

### 9.2 Log cumulative black pixel count per pass (Stack ON)

```cpp
auto count_black = [&](GrayMap *gm) {
    long n = 0;
    for (int y = 0; y < gm->height; ++y) {
        for (int x = 0; x < gm->width; ++x) {
            if (gm->getPixel(gm, x, y) == GRAYMAP_BLACK) n++;
        }
    }
    return n;
};

for (int colorIndex = 0; colorIndex < indexed->nrColors; ++colorIndex) {
    ... update gm for colorIndex ...
    printf("pass=%d black_pixels=%ld\n", colorIndex, count_black(gm));
}
```

Expected: black pixel count should be non-decreasing for Stack ON.

### 9.3 Log append order and style

```cpp
printf("append layer idx=%d style=%s path_len=%zu\n",
       colorIndex, style_buf, d.size());
```

---

## 10) Common divergence points (and fixes)

1. **Quantizer implementation differs** -> palette/index order changes.
2. **Dithering enabled** -> noisy masks and extra jaggies.
3. **Non-match white reset accidentally left on** -> behaves like Stack OFF.
4. **Layer reorder in renderer** -> visual mismatch despite same paths.
5. **Preview judged instead of final rendered vector** -> false "pixelated" diagnosis.

---

## 11) Parity checklist (Stack ON edition)

- [ ] Use `TRACE_QUANT_COLOR` mode.
- [ ] Quantize once to `scans` colors.
- [ ] Optional Gaussian blur is applied pre-quantization only.
- [ ] `GrayMap` starts all-white once and is reused across passes.
- [ ] For each pass, only matching index is set black.
- [ ] Non-matching pixels are **not** reset (Stack ON contract).
- [ ] Potrace params fixed: `turdsize=2`, `alphamax=1.0`, `opticurve=true`, `opttolerance=0.2`.
- [ ] Layers appended in ascending `colorIndex` order.
- [ ] Renderer does not reorder layers during parity tests.

---

## 12) Non-negotiable invariants (treat as hard requirements)

If any item below is violated, output can diverge significantly from Inkscape behavior even when the same
numeric parameters are used.

1. **Quantize exactly once per trace call** (never per-layer quantization).
2. **Do not mutate indexed class IDs between passes**.
3. **Use one shared `GrayMap` across all passes** for Stack ON cumulative semantics.
4. **Never reset non-matching pixels to white** when Stack ON is intended.
5. **Append layers in strict ascending `colorIndex` order**.
6. **Use fixed Potrace params consistently for all passes** (`turdsize=2`, `alphamax=1.0`, `opticurve=true`, `opttolerance=0.2`).
7. **Do not reorder layers in rendering/composition during parity runs**.

Implementation note: the Stack ON behavior and append order are directly reflected in the Inkscape loop structure,
including the condition that only resets non-matching pixels when stack is OFF.

---

## 13) Stack ON truth table (pixel update contract)

For each pass and each pixel:

| Condition | Stack flag | Action on `gm(x,y)` |
|---|---|---|
| `idx == colorIndex` | ON/OFF | Set `GRAYMAP_BLACK` |
| `idx != colorIndex` | ON | **No-op** (leave prior value) |
| `idx != colorIndex` | OFF | Set `GRAYMAP_WHITE` |

For this reference profile, Stack is ON, so only the first two rows apply.

Reference implementation expression:

```cpp
if (idx == colorIndex) {
    gm->setPixel(gm, x, y, GRAYMAP_BLACK);
} else if (!multiScanStack) {
    gm->setPixel(gm, x, y, GRAYMAP_WHITE);
}
```

---

## 14) Order-of-operations checksum (pipeline guardrail)

Use this canonical execution order and fail fast if your call graph deviates:

```text
load bitmap
-> to RGB map
-> optional gaussian pre-blur (multiscans smooth)
-> quantize to IndexedMap once
-> init one white GrayMap
-> for colorIndex in ascending order:
     update mask (Stack ON no-op for non-match)
     trace with Potrace
     append layer with clut[colorIndex]
-> optional remove-background
```

Recommended runtime assertions:

```cpp
assert(indexed != nullptr);
assert(indexed->nrColors > 0);
assert(gm->width == indexed->width && gm->height == indexed->height);
assert(params->opticurve == true);
assert(params->opttolerance == 0.2);
assert(params->alphamax == 1.0);
assert(params->turdsize == 2);
```

---

## 15) Regression metrics for deterministic parity

In addition to visual checks, use numeric parity tests per sample image.

### Required metrics

1. Quantized class histogram (`class_counts[i]`).
2. Monotonic cumulative black-pixel count per pass (Stack ON).
3. Layer append sequence (`colorIndex` order).
4. Layer style sequence (`clut[colorIndex]` mapping).
5. Node count per layer and total node count.

### Example test expectations

```cpp
// monotonic mask growth
for (int i = 1; i < black_counts.size(); ++i) {
    assert(black_counts[i] >= black_counts[i - 1]);
}

// append order integrity
for (int i = 0; i < appended_indices.size(); ++i) {
    assert(appended_indices[i] == i);
}
```

If these assertions pass but visuals still diverge, the likely culprit is renderer-side layer ordering/compositing.

---

## 16) Renderer append/paint contract (often-missed detail)

Generation order and visual order are related but not identical unless your renderer is deterministic.

- Generation: `out.push_back(layer)` in ascending `colorIndex`.
- Typical SVG-like paint: first element painted first (bottom), last painted last (top).

If your engine performs batching/sorting by color/material, parity can break.
Disable those optimizations during calibration.

Example deterministic emission:

```cpp
for (int i = 0; i < out.size(); ++i) {
    emit_path(out[i].d, out[i].style); // preserve array order exactly
}
```

---

## 17) Common migration pitfalls when adapting existing Potrace pipelines

1. **Per-layer re-quantization** (wrong): causes index drift and unstable layering.
2. **Dithering left enabled in quantizer**: introduces micro-islands and jagged outlines.
3. **Stack ON implemented as overwrite instead of cumulative update**.
4. **Potrace parameters set once but overridden in helper functions later**.
5. **Preview pipeline mistaken for final render pipeline**.
6. **Color index mapped to sorted palette instead of original quantizer `clut` order**.

Mitigation: log class histogram, `clut` ordering, append indices, and per-pass black pixel counts in one record.

---

## 18) Optional: strict parity harness outline

For teams integrating into another stack, add a tiny parity harness around this procedure:

```cpp
struct ParityRecord {
    std::vector<int> class_counts;
    std::vector<long> black_counts;
    std::vector<int> append_indices;
    std::vector<std::string> styles;
    std::vector<long> node_counts;
};

ParityRecord run_trace_parity_capture(GdkPixbuf *pixbuf, TraceBitmapConfig const &cfg);
```

Persist one golden record per test image and compare within tolerances for node counts/path lengths where needed.

---

## 19) Source-aligned constants and UI defaults (quick reference)

- Multiscan count UI range: `2..256`; default `8`.
- Multiscan smooth tooltip clarifies pre-trace Gaussian blur.
- Stack tooltip clarifies no-gaps cumulative behavior.

Keep these defaults aligned in external recreations to reduce accidental drift during user testing.

---

## 20) Validation against an external implementation summary

If your implementation follows the process below, it is aligned with this reference:

1. Load source image and construct analysis buffer once.
2. Resolve one trace settings object and keep it fixed for the call.
3. Apply optional pre-quantization blur only if multiscan smooth is enabled.
4. Quantize once to `(labels, palette, percentages)`.
5. Do **not** post-merge, re-sort, or relabel classes after quantization.
6. Iterate `colorIndex` in ascending order, using one shared cumulative mask for Stack ON.
7. Trace each pass and append path output in the same index order.
8. Style each layer with quantizer palette entry `palette[colorIndex]` (CLUT-equivalent).
9. Return final SVG; keep any non-tracing fallback path separate from parity evaluation.

### Important clarifications/caveats

- **Your summary is correct overall.**
- The largest remaining parity risks are usually:
  1. renderer-side reordering after append,
  2. non-deterministic k-means initialization,
  3. hidden label remapping during downstream optimization,
  4. evaluating fallback/preview output instead of traced output.

### Determinism requirements to add explicitly

For repeatable parity from run to run, also lock:

- k-means seed / initialization strategy,
- color space used for distance computation,
- blur kernel/radius for pre-quantization smoothing,
- image resampling policy before quantization (if any).

Even with the same Stack ON tracing logic, differences in those pre-quantization details can change label maps and therefore final path geometry.

### Quick “correctness assertion pack”

Add these checks around your current process:

```cpp
// labels must remain stable across pipeline stages
assert(no_post_quantization_relabel == true);

// append order must match index order
for (int i = 0; i < appended_indices.size(); ++i) {
    assert(appended_indices[i] == i);
}

// style must match quantizer palette index
for (int i = 0; i < layers.size(); ++i) {
    assert(layers[i].fill == palette[i]);
}

// Stack ON cumulative mask growth
for (int i = 1; i < black_counts.size(); ++i) {
    assert(black_counts[i] >= black_counts[i - 1]);
}
```

If these pass and output still differs visually, compare quantizer internals first (seed/space/blur), then renderer paint/compositing order.
