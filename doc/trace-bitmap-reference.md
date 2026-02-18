# Trace Bitmap (Multicolor / Colors) Reference

This document consolidates the key logic for Inkscape's **Trace Bitmap** flow for:

- Multiple scans
- Detection mode: **Colors** (`MS_C`)
- Stack scans: enabled
- Requested Potrace shaping parameters:
  - Speckles (`turdsize`) = `2`
  - Smooth corners (`alphamax`) = `1.0`
  - Optimize (`opticurve`) = `true`, (`opttolerance`) = `0.2`

The goal is to provide a single-file implementation reference for recreating behavior in another codebase.

---

## 1) Parameter mapping (UI -> tracing engine)

For this mode, values are mapped into `PotraceTracingEngine` and Potrace params as follows:

```cpp
// mode selection
traceType = TRACE_QUANT_COLOR; // "MS_C"

// multiscans
multiScanNrColors = scans;      // e.g. 4, 8, etc.
multiScanStack = true;          // stack scans
multiScanSmooth = true|false;   // gaussian pre-blur toggle
multiScanRemoveBackground = false;

// potrace shaping
potraceParams->opticurve    = true;
potraceParams->opttolerance = 0.2; // Optimize
potraceParams->alphamax     = 1.0; // Smooth corners
potraceParams->turdsize     = 2;   // Speckles
```

---

## 2) Consolidated reference procedure

The behavior can be recreated by following this sequence exactly.

```cpp
struct TraceBitmapConfig {
    int scans = 8;                  // 2..256 in UI
    bool stack = true;              // "Stack scans"
    bool smooth_preblur = true;     // multiscans "Smooth"
    bool remove_background = false; // "Remove background"

    // Potrace parameters
    bool optimize = true;           // opticurve
    double optimize_tolerance = 0.2;// opttolerance
    double smooth_corners = 1.0;    // alphamax
    int speckles = 2;               // turdsize
};

struct Layer {
    std::string style; // "fill:#rrggbb"
    std::string path_d;
};

std::vector<Layer> trace_bitmap_multicolor_colors(
    GdkPixbuf *pixbuf,
    TraceBitmapConfig const &cfg)
{
    std::vector<Layer> out;
    if (!pixbuf) return out;

    // -------------------------------------------------------------
    // A) Quantize source into indexed colors (optionally pre-blurred)
    // -------------------------------------------------------------
    RgbMap *rgb = gdkPixbufToRgbMap(pixbuf);
    if (!rgb) return out;

    IndexedMap *indexed = nullptr;
    if (cfg.smooth_preblur) {
        RgbMap *gauss = rgbMapGaussian(rgb);              // pre-quantization blur
        indexed = rgbMapQuantize(gauss, cfg.scans);       // N-color quantization
        gauss->destroy(gauss);
    } else {
        indexed = rgbMapQuantize(rgb, cfg.scans);
    }
    rgb->destroy(rgb);
    if (!indexed) return out;

    // -------------------------------------------------------------
    // B) Configure Potrace
    // -------------------------------------------------------------
    potrace_param_t *params = potrace_param_default();
    params->opticurve = cfg.optimize;
    params->opttolerance = cfg.optimize_tolerance;
    params->alphamax = cfg.smooth_corners;
    params->turdsize = cfg.speckles;

    // Shared grayscale mask reused for all color passes.
    GrayMap *gm = GrayMapCreate(indexed->width, indexed->height);
    for (int y = 0; y < gm->height; ++y) {
        for (int x = 0; x < gm->width; ++x) {
            gm->setPixel(gm, x, y, GRAYMAP_WHITE);
        }
    }

    // -------------------------------------------------------------
    // C) Iterate each quantized color class and trace
    // -------------------------------------------------------------
    for (int colorIndex = 0; colorIndex < indexed->nrColors; ++colorIndex) {

        // Build pass mask for this color index.
        for (int y = 0; y < indexed->height; ++y) {
            for (int x = 0; x < indexed->width; ++x) {
                int idx = (int)indexed->getPixel(indexed, x, y);
                if (idx == colorIndex) {
                    gm->setPixel(gm, x, y, GRAYMAP_BLACK);
                } else if (!cfg.stack) {
                    // If NOT stacking, erase non-matching pixels each pass.
                    gm->setPixel(gm, x, y, GRAYMAP_WHITE);
                }
                // If stacking, leave previous content in gm as-is.
            }
        }

        // Convert GrayMap -> potrace bitmap -> vector path d.
        std::string d = grayMapToPath(gm, params);
        if (d.empty()) continue;

        RGB rgb = indexed->clut[colorIndex];
        char style_buf[32];
        snprintf(style_buf, sizeof(style_buf), "fill:#%02x%02x%02x", rgb.r, rgb.g, rgb.b);

        out.push_back({style_buf, d});
    }

    // Optional: remove bottom-most layer when requested.
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

## 3) Layer ordering notes

- Layers are generated in `colorIndex` order from the quantized palette.
- The final visual stack depends on how your renderer paints array order.
- If your renderer paints in insertion order, earlier layers are "under" later ones.

For parity checks, keep this deterministic and do not reorder by area unless you intentionally want different behavior.

---

## 4) Why results look pixelated in recreations

If output looks too blocky compared to Inkscape-like output, the most common causes are:

1. Pre-quantization Gaussian blur not enabled or too weak.
2. Different quantizer behavior/dithering.
3. Potrace params not applied consistently per pass.
4. Comparing against nearest-neighbor preview instead of rendered final SVG.

---

## 5) Minimal parity checklist

- [ ] `TRACE_QUANT_COLOR` path selected.
- [ ] Same `scans` value.
- [ ] Same pre-blur toggle (`smooth_preblur`).
- [ ] Same `stack` behavior for non-matching pixels.
- [ ] `opticurve=true`, `opttolerance=0.2`, `alphamax=1.0`, `turdsize=2`.
- [ ] Same layer paint order.
- [ ] Optional `remove_background` behavior matched.

