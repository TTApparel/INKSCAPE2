// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Contain multiple Cairo surfaces for rendering
 *//*
 * Authors:
 *  Martin Owens <doctormo@geek-2.com>
 *
 * Copyright (C) 2026 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <cairomm/surface.h>

#include "colors/spaces/components.h"
#include "pixel-filters/color-space.h"
#include "renderer/surface.h"


namespace Inkscape::Renderer {

/**
 * Create a rendering surface
 */
Surface::Surface(Geom::IntPoint const &dimensions, int device_scale, std::shared_ptr<Colors::Space::AnySpace> const &color_space)
    : _dimensions(dimensions)
    , _device_scale(device_scale)
    , _color_space(color_space)
{}

#ifdef UNIT_TEST
Surface::Surface(std::string const &filename)
        : _surfaces{Cairo::ImageSurface::create_from_png(filename)}
        , _dimensions{_surfaces.back()->get_width(), _surfaces.back()->get_height()}
        , _device_scale{1.0}
        , _color_space(cairo_image_surface_get_format(_surfaces[0]->cobj()) == CAIRO_FORMAT_ARGB32
            ? nullptr // 8bit PNGs are INTs, 16bit are FLOATs
            : Colors::Manager::get().find(Colors::Space::Type::RGB))
{}
#endif

/**
 * Create or return existing cairo surfaces.
 */
std::vector<Cairo::RefPtr<Cairo::ImageSurface>> const &Surface::getCairoSurfaces() const
{
    // deferred allocation
    if (_surfaces.empty()) {
        // Backwards compatability for smaller memory footprint.
        auto format = CAIRO_FORMAT_ARGB32;
        unsigned count = 1;
   
        // Get enough surfaces to store all the channels in the format
        if (_color_space) {
            auto size = _color_space->getComponentCount();
            if (size == 0) { // Alpha channel, opacity only
                format = CAIRO_FORMAT_A8;
            } else {
                format = CAIRO_FORMAT_RGBA128F;
                count = std::ceil((double)size / 3);
            }
        }
        for (unsigned i = 0; i < count; i++) {
            // Must be created in C as CairoMM doesn't support all the needed formats yet.
            auto cobj = cairo_image_surface_create(format,
                                                   _dimensions.x() * _device_scale,
                                                   _dimensions.y() * _device_scale);
            cairo_surface_set_device_scale(cobj, _device_scale, _device_scale);
            _surfaces.push_back(Cairo::RefPtr<Cairo::ImageSurface>(new Cairo::ImageSurface(cobj, true)));
        }
    }
    return _surfaces;
}

int Surface::components() const
{
    return _color_space ? _color_space->getComponentCount() : 3;
}

std::shared_ptr<Surface> Surface::similar(std::optional<Geom::IntPoint> dimensions) const
{
    return std::make_shared<Surface>(dimensions ? *dimensions : _dimensions, _device_scale, _color_space);
}

std::shared_ptr<Surface> Surface::similar(std::optional<Geom::IntPoint> dimensions, std::shared_ptr<Colors::Space::AnySpace> const &color_space) const
{
    return std::make_shared<Surface>(dimensions ? *dimensions : _dimensions, _device_scale, color_space);
}

void Surface::convertToColorSpace(std::shared_ptr<Colors::Space::AnySpace> const &color_space)
{
    if (color_space->getType() == Colors::Space::Type::Alpha) {
        std::cerr << "Refusing to convert to alpha in-place, make a copy instead\n";
        return;
    }
    if (!color_space || !_color_space) {
        std::cerr << "Refusing to convert to or from a legacy color space sRGB:RGBA32\n";
        return;
    }
    if (ready() && color_space != _color_space) {
        run_pixel_filter(PixelFilter::ColorSpaceTransform(_color_space, color_space), *this);
    }
    _color_space = color_space;
}

std::shared_ptr<Surface> Surface::convertedToColorSpace(std::shared_ptr<Colors::Space::AnySpace> const &color_space) const
{
    auto dest = similar(_dimensions, color_space);
    if (ready()) {
        if (color_space && color_space->getType() == Colors::Space::Type::Alpha) {
            dest->run_pixel_filter(PixelFilter::AlphaSpaceExtraction(), *this);
        } else {
            dest->run_pixel_filter(PixelFilter::ColorSpaceTransform(_color_space, color_space), *this);
        }
    }
    return dest;
}

} // namespace Inkscape::Renderer

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
