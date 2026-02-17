// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * SVG drawing for display.
 *//*
 * Authors:
 *   Krzysztof Kosiński <tweenk.pl@gmail.com>
 *
 * Copyright (C) 2011 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_RENDERER_DRAWING_H
#define INKSCAPE_RENDERER_DRAWING_H

#include <optional>
#include <set>
#include <cstdint>
#include <vector>
#include <boost/operators.hpp>
#include <2geom/rect.h>
#include <2geom/pathvector.h>
#include <sigc++/sigc++.h>

#include "colors/color.h"
#include "display/rendermode.h"
#include "drawing-item.h"
#include "preferences.h"
#include "util/funclog.h"

namespace Inkscape::Renderer {

class DrawingItem;
class Context;

class Drawing
{
public:
    Drawing(Drawing const &) = delete;
    Drawing &operator=(Drawing const &) = delete;
    ~Drawing();

    void setRoot(DrawingItem *root);
    DrawingItem *root() { return _root; }

    void setRenderMode(RenderMode);
    void setColorMode(ColorMode);
    void setOutlineOverlay(bool);
    void setGrayscaleMatrix(double[20]);
    void setClipOutlineColor(Colors::Color);
    void setMaskOutlineColor(Colors::Color);
    void setImageOutlineColor(Colors::Color);
    void setImageOutlineMode(bool);
    void setFilterQuality(int);
    void setBlurQuality(int);
    void setDithering(bool);
    void setCursorTolerance(double tol) { _cursor_tolerance = tol; }
    void setSelectZeroOpacity(bool select_zero_opacity) { _select_zero_opacity = select_zero_opacity; }
    void setCacheBudget(size_t bytes);
    void setCacheLimit(Geom::OptIntRect const &rect);
    void setClip(std::optional<Geom::PathVector> &&clip);
    void setAntialiasingOverride(std::optional<Antialiasing> antialiasing_override);

    RenderMode renderMode() const { return _rendermode; }
    ColorMode colorMode() const { return _colormode; }
    bool outlineOverlay() const { return _outlineoverlay; }
    Colors::Color const &clipOutlineColor() const { return _clip_outline_color; }
    Colors::Color const &maskOutlineColor() const { return _mask_outline_color; }
    Colors::Color const &imageOutlineColor() const { return _image_outline_color; }
    bool imageOutlineMode() const { return _image_outline_mode; }
    int filterQuality() const { return _filter_quality; }
    int blurQuality() const { return _blur_quality; }
    bool useDithering() const { return _use_dithering; }
    double cursorTolerance() const { return _cursor_tolerance; }
    bool selectZeroOpacity() const { return _select_zero_opacity; }
    Geom::OptIntRect const &cacheLimit() const { return _cache_limit; }

    void update(Geom::IntRect const &area = Geom::IntRect::infinite(), Geom::Affine const &affine = Geom::identity(),
                unsigned flags = DrawingItem::STATE_ALL, unsigned reset = 0);
    void render(Context &dc, Geom::IntRect const &area, unsigned flags = 0) const;
    DrawingItem *pick(Geom::Point const &p, double delta, unsigned flags);

    void snapshot();
    void unsnapshot();
    bool snapshotted() const { return _snapshotted; }

    // Convenience
    Colors::Color averageColor(Geom::IntRect const &area) const;
    Colors::Color averageColor(Geom::PathVector const &path, bool evenodd) const;
    void setExact();
    void setOpacity(double opacity = 1.0);

private:
    void _pickItemsForCaching();
    void _clearCache();
    void _loadPrefs();

    DrawingItem *_root = nullptr;
    std::unique_ptr<Preferences::PreferencesObserver> _pref_tracker;

    RenderMode _rendermode = RenderMode::NORMAL;
    ColorMode _colormode = ColorMode::NORMAL;
    bool _outlineoverlay = false;
    Colors::Color _clip_outline_color;
    Colors::Color _mask_outline_color;
    Colors::Color _image_outline_color;
    bool _image_outline_mode; ///< Always draw images as images, even in outline mode.
    int _filter_quality;
    int _blur_quality;
    bool _use_dithering;
    double _cursor_tolerance;
    size_t _cache_budget; ///< Maximum allowed size of cache.
    Geom::OptIntRect _cache_limit;
    std::optional<Geom::PathVector> _clip;
    bool _select_zero_opacity;
    std::optional<Antialiasing> _antialiasing_override;

    std::set<DrawingItem*> _cached_items; // modified by DrawingItem::_setCached()
    CacheList _candidate_items;           // keep this list always sorted with std::greater

    /*
     * Simple cacheline separator compatible with x86 (64 bytes) and M* (128 bytes).
     * Ideally alignas(std::hardware_destructive_interference_size) could be used instead,
     * but this is extremely painful to make work across all supported platforms/compilers.
     */
    char cacheline_separator[127];

    bool _snapshotted = false;
    Util::FuncLog _funclog;

    template<typename F>
    void defer(F &&f) { _snapshotted ? _funclog.emplace(std::forward<F>(f)) : f(); }

    friend class DrawingItem;
};

} // namespace Inkscape::Renderer

#endif // INKSCAPE_RENDERER_DRAWING_H

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :
