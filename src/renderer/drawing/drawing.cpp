// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * SVG drawing for display.
 *//*
 * Authors:
 *   Krzysztof Kosiński <tweenk.pl@gmail.com>
 *   Johan Engelen <j.b.c.engelen@alumnus.utwente.nl>
 *
 * Copyright (C) 2011-2012 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "drawing.h"

#include <array>
#include <thread>

#include "renderer/context.h"
#include "threading.h"

namespace Inkscape::Renderer {

static auto rendermode_to_renderflags(RenderMode mode)
{
    switch (mode) {
        case RenderMode::OUTLINE:           return DrawingItem::RENDER_OUTLINE;
        case RenderMode::NO_FILTERS:        return DrawingItem::RENDER_NO_FILTERS;
        case RenderMode::VISIBLE_HAIRLINES: return DrawingItem::RENDER_VISIBLE_HAIRLINES;
        default:                            return DrawingItem::RenderFlags::RENDER_DEFAULT;
    }
}

static auto default_numthreads()
{
    auto ret = std::thread::hardware_concurrency();
    return ret == 0 ? 4 : ret; // Sensible fallback if not reported.
}

Drawing::~Drawing()
{
    delete _root;
}

void Drawing::setRoot(DrawingItem *root)
{
    delete _root;
    _root = root;
    if (_root) {
        assert(_root->_child_type == DrawingItem::ChildType::ORPHAN);
        _root->_child_type = DrawingItem::ChildType::ROOT;
    }
}

void Drawing::setRenderMode(RenderMode mode)
{
    assert(mode != RenderMode::OUTLINE_OVERLAY && "Drawing::setRenderMode: OUTLINE_OVERLAY is not a true render mode");

    defer([=, this] {
        if (mode == _rendermode) return;
        _root->_markForRendering();
        _rendermode = mode;
        _root->_markForUpdate(DrawingItem::STATE_ALL, true);
        _clearCache();
    });
}

void Drawing::setColorMode(ColorMode mode)
{
    defer([=, this] {
        if (mode == _colormode) return;
        _colormode = mode;
        if (_rendermode != RenderMode::OUTLINE || _image_outline_mode) {
            _root->_markForRendering();
        }
    });
}

void Drawing::setOutlineOverlay(bool outlineoverlay)
{
    defer([=, this] {
        if (outlineoverlay == _outlineoverlay) return;
        _outlineoverlay = outlineoverlay;
        _root->_markForUpdate(DrawingItem::STATE_ALL, true);
    });
}

void Drawing::setClipOutlineColor(Colors::Color col)
{
    defer([=, this] {
        _clip_outline_color = std::move(col);
        if (_rendermode == RenderMode::OUTLINE || _outlineoverlay) {
            _root->_markForRendering();
        }
    });
}

void Drawing::setMaskOutlineColor(Colors::Color col)
{
    defer([=, this] {
        _mask_outline_color = std::move(col);
        if (_rendermode == RenderMode::OUTLINE || _outlineoverlay) {
            _root->_markForRendering();
        }
    });
}

void Drawing::setImageOutlineColor(Colors::Color col)
{
    defer([=, this] {
        _image_outline_color = std::move(col);
        if ((_rendermode == RenderMode::OUTLINE || _outlineoverlay) && !_image_outline_mode) {
            _root->_markForRendering();
        }
    });
}

void Drawing::setImageOutlineMode(bool enabled)
{
    defer([=, this] {
        _image_outline_mode = enabled;
        if (_rendermode == RenderMode::OUTLINE || _outlineoverlay) {
            _root->_markForRendering();
        }
    });
}

void Drawing::setFilterQuality(int quality)
{
    defer([=, this] {
        _filter_quality = quality;
        if (!(_rendermode == RenderMode::OUTLINE || _rendermode == RenderMode::NO_FILTERS)) {
            _root->_markForUpdate(DrawingItem::STATE_ALL, true);
            _clearCache();
        }
    });
}

void Drawing::setBlurQuality(int quality)
{
    defer([=, this] {
        _blur_quality = quality;
        if (!(_rendermode == RenderMode::OUTLINE || _rendermode == RenderMode::NO_FILTERS)) {
            _root->_markForUpdate(DrawingItem::STATE_ALL, true);
            _clearCache();
        }
    });
}

void Drawing::setDithering(bool use_dithering)
{
    defer([=, this] {
        _use_dithering = use_dithering;
        #if CAIRO_VERSION >= CAIRO_VERSION_ENCODE(1, 18, 0)
        if (_rendermode != RenderMode::OUTLINE) {
            _root->_markForUpdate(DrawingItem::STATE_ALL, true);
            _clearCache();
        }
        #endif
    });
}

void Drawing::setCacheBudget(size_t bytes)
{
    defer([=, this] {
        _cache_budget = bytes;
        _pickItemsForCaching();
    });
}

void Drawing::setCacheLimit(Geom::OptIntRect const &rect)
{
    defer([=, this] {
        _cache_limit = rect;
        for (auto item : _cached_items) {
            item->_markForUpdate(DrawingItem::STATE_CACHE, false);
        }
    });
}

void Drawing::setClip(std::optional<Geom::PathVector> &&clip)
{
    defer([=, this] {
        if (clip == _clip) return;
        _clip = std::move(clip);
        _root->_markForRendering();
    });
}

void Drawing::setAntialiasingOverride(std::optional<Antialiasing> antialiasing_override)
{
    defer([=, this] {
        _antialiasing_override = antialiasing_override;
        _root->_markForUpdate(DrawingItem::STATE_ALL, true);
        _clearCache();
    });
}

void Drawing::update(Geom::IntRect const &area, Geom::Affine const &affine, unsigned flags, unsigned reset)
{
    if (_root) {
        _root->update(area, { affine }, flags, reset);
    }
    if (flags & DrawingItem::STATE_CACHE) {
        // Process the updated cache scores.
        _pickItemsForCaching();
    }
}

void Drawing::render(Context &dc, Geom::IntRect const &area, unsigned flags) const
{
    auto opt = Options{
        .outline_color = Colors::Color(0xff),
        .antialiasing_override = _antialiasing_override,
        .dithering = _use_dithering
        // TODO: Add blurquality and filterquality
    };
    flags |= rendermode_to_renderflags(_rendermode);

    dc.setAntialias(_antialiasing_override.value_or(Antialiasing(_root->_antialias)));
    if (_clip) {
        dc.save();
        dc.path(*_clip * _root->_ctm);
        dc.clip();
    }
    _root->render(dc, opt, area, flags);
    if (_clip) {
        dc.restore();
    }
}

DrawingItem *Drawing::pick(Geom::Point const &p, double delta, unsigned flags)
{
    return _root->pick(p, delta, flags);
}

void Drawing::snapshot()
{
    assert(!_snapshotted);
    _snapshotted = true;
}

void Drawing::unsnapshot()
{
    assert(_snapshotted);
    _snapshotted = false; // Unsnapshot before replaying log so further work is not deferred.
    _funclog();
}

void Drawing::_pickItemsForCaching()
{
    // Build sorted list of items that should be cached.
    std::vector<DrawingItem*> to_cache;
    size_t used = 0;
    for (auto &rec : _candidate_items) {
        if (used + rec.cache_size > _cache_budget) break;
        to_cache.emplace_back(rec.item);
        used += rec.cache_size;
    }
    std::sort(to_cache.begin(), to_cache.end());

    // Uncache the items that are cached but should not be cached.
    // Note: setCached() modifies _cached_items, so the temporary container is necessary.
    std::vector<DrawingItem*> to_uncache;
    std::set_difference(_cached_items.begin(), _cached_items.end(),
                        to_cache.begin(), to_cache.end(),
                        std::back_inserter(to_uncache));
    for (auto item : to_uncache) {
        item->_setCached(false);
    }

    // Cache all items that should be cached (no-op if already cached).
    for (auto item : to_cache) {
        item->_setCached(true);
    }
}

void Drawing::_clearCache()
{
    // Note: setCached() modifies _cached_items, so the temporary container is necessary.
    std::vector<DrawingItem*> to_uncache;
    std::copy(_cached_items.begin(), _cached_items.end(), std::back_inserter(to_uncache));
    for (auto item : to_uncache) {
        item->_setCached(false, true);
    }
}

void Drawing::_loadPrefs()
{
    auto prefs = Inkscape::Preferences::get();

    // Set the initial values of preferences.
    _clip_outline_color  = prefs->getColor     ("/options/wireframecolors/clips",        "#00ff00"); // Green clip outlines by default.
    _mask_outline_color  = prefs->getColor     ("/options/wireframecolors/masks",        "#0000ff"); // Blue mask outlines by default.
    _image_outline_color = prefs->getColor     ("/options/wireframecolors/images",       "#ff0000"); // Red image outlines by default.
    _image_outline_mode  = prefs->getBool      ("/options/rendering/imageinoutlinemode", false);
    _filter_quality      = prefs->getIntLimited("/options/filterquality/value",          0, Filters::FILTER_QUALITY_WORST, Filters::FILTER_QUALITY_BEST);
    _blur_quality        = prefs->getInt       ("/options/blurquality/value",            0);
    _use_dithering       = prefs->getBool      ("/options/dithering/value",              true);
    _cursor_tolerance    = prefs->getDouble    ("/options/cursortolerance/value",        1.0);
    _select_zero_opacity = prefs->getBool      ("/options/selection/zeroopacity",        false);

    // Enable caching only for the Canvas's drawing, since only it is persistent.
    if (_canvas_item_drawing) {
        // Preference is stored in MiB; convert to bytes, taking care not to overflow.
        _cache_budget = (size_t{1} << 20) * prefs->getIntLimited("/options/renderingcache/size", 64, 0, 4096);
    } else {
        _cache_budget = 0;
    }

    // Set the global variable governing the number of threads, and track it too. (This is ugly, but hopefully
    // transitional.)
    set_num_dispatch_threads(prefs->getIntLimited("/options/threading/numthreads", default_numthreads(), 1, 256));

    // Similarly, enable preference tracking only for the Canvas's drawing.
    if (_canvas_item_drawing) {
        std::unordered_map<std::string, std::function<void (Preferences::Entry const &)>> actions;

        // Todo: (C++20) Eliminate this repetition by baking the preference metadata into the variables themselves using structural templates.
        actions.emplace("/options/wireframecolors/clips",        [this] (auto &entry) { setClipOutlineColor (entry.getColor("#00ff00")); });
        actions.emplace("/options/wireframecolors/masks",        [this] (auto &entry) { setMaskOutlineColor (entry.getColor("#0000ff")); });
        actions.emplace("/options/wireframecolors/images",       [this] (auto &entry) { setImageOutlineColor(entry.getColor("#ff0000")); });
        actions.emplace("/options/rendering/imageinoutlinemode", [this] (auto &entry) { setImageOutlineMode(entry.getBool(false)); });
        actions.emplace("/options/filterquality/value",          [this] (auto &entry) { setFilterQuality(entry.getIntLimited(0, Filters::FILTER_QUALITY_WORST, Filters::FILTER_QUALITY_BEST)); });
        actions.emplace("/options/blurquality/value",            [this] (auto &entry) { setBlurQuality(entry.getInt(0)); });
        actions.emplace("/options/dithering/value",              [this] (auto &entry) { setDithering(entry.getBool(true)); });
        actions.emplace("/options/cursortolerance/value",        [this] (auto &entry) { setCursorTolerance(entry.getDouble(1.0)); });
        actions.emplace("/options/selection/zeroopacity",        [this] (auto &entry) { setSelectZeroOpacity(entry.getBool(false)); });
        actions.emplace("/options/renderingcache/size",          [this] (auto &entry) { setCacheBudget((1 << 20) * entry.getIntLimited(64, 0, 4096)); });
        actions.emplace("/options/threading/numthreads", [this](auto &entry) {
            set_num_dispatch_threads(entry.getIntLimited(default_numthreads(), 1, 256));
        });

        _pref_tracker = Inkscape::Preferences::PreferencesObserver::create("/options", [actions = std::move(actions)] (auto &entry) {
            auto it = actions.find(entry.getPath());
            if (it == actions.end()) return;
            it->second(entry);
        });
    }
}

/*
 * Return average color over area. Used by Calligraphic, Dropper, and Spray tools.
 */
Colors::Color Drawing::averageColor(Geom::IntRect const &area) const
{
    auto surface = Cairo::ImageSurface::create(Cairo::Surface::Format::ARGB32, area.width(), area.height());
    auto dc = Inkscape::Context(surface->cobj(), area.min());
    render(dc, area);
    return ink_cairo_surface_average_color(surface->cobj());
}

/*
 * Return the average color inside the given path.
 */
Colors::Color Drawing::averageColor(Geom::PathVector const &path, bool evenodd) const
{
    auto area = path.boundsExact();
    if (!area || area->hasZeroArea()) {
        return Colors::Color(0x0); // Transparent black sRGB
    }

    // Scale the graphic so there's a predictable number of pixels to choose from
    static constexpr auto width = 200.0;
    static constexpr auto height = 200.0;

    auto affine = Geom::Scale(width / area->width(), height / area->height());
    auto offset = area->min() * affine;

    // Build a mask of pixels to ignore
    auto mask = Cairo::ImageSurface::create(Cairo::Surface::Format::A8, width, height);
    auto dc_mask = Context(mask->cobj(), offset);
    dc_mask.scale(affine);

    dc_mask.setFillRule(evenodd ? CAIRO_FILL_RULE_EVEN_ODD : CAIRO_FILL_RULE_WINDING);
    dc_mask.path(path);
    dc_mask.clip();
    dc_mask.resetSource(1.0);
    dc_mask.setOperator(CAIRO_OPERATOR_SOURCE);
    dc_mask.paint();

    // Render the output, no need to clip as the mask will say what values to use
    auto image = Cairo::ImageSurface::create(Cairo::Surface::Format::ARGB32, width, height);
    auto dc = Context(image->cobj(), offset);
    dc.scale(affine);
    render(dc, area->roundOutwards());

    return ink_cairo_surface_average_color(image->cobj(), mask->cobj());
}

/*
 * Convenience function to set high quality options for export.
 */
void Drawing::setExact()
{
    setFilterQuality(Filters::FILTER_QUALITY_BEST);
    setBlurQuality(BLUR_QUALITY_BEST);
}

/*
 * Set the opacity of the drawing root drawing-item
 */
void Drawing::setOpacity(double opacity)
{
    _root->setOpacity(opacity);
}

} // namespace Inkscape

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
