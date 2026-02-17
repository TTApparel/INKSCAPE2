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

#ifndef INKSCAPE_DISPLAY_RENDERER_SURFACE_H
#define INKSCAPE_DISPLAY_RENDERER_SURFACE_H

#include <any>
#include <memory>
#include <cairomm/surface.h>
#include <2geom/point.h>

#include "colors/forward.h"
#include "pixel-access.h"

namespace Inkscape::Renderer {

class Surface
{
public:
    /**
     * Create a new Surface with the given domentions and device scale.
     *
     * @arg dimensions   - The width and height in pixels of the surface memory.
     * @arg device_scale - The scale of the pixels as used by Cairo::ImageSurface, default 1.
     * @arg color_space  - A color space which allows this surface to be translated to
     *                     other color spaces. If one is not provided everything is
     *                     assumed to be sRGB and floating point precion is turned off.
     */
    explicit Surface(Geom::IntPoint const &dimensions, int device_scale = 1, std::shared_ptr<Colors::Space::AnySpace> const &color_space = {});

    /**
     * Returns true if the memory has been allocated for this surface.
     */
    bool ready() const { return !_surfaces.empty(); }

    /**
     * Returns the dimensional size of the surface in pixels.
     */
    Geom::IntPoint dimensions() const { return _dimensions; }

    /**
     * Get the device scale for this surface.
     */
    double getDeviceScale() const { return _device_scale; }

    /**
     * Returns the color space being used for this surface.
     */
    std::shared_ptr<Colors::Space::AnySpace> getColorSpace() const { return _color_space; }

    /**
     * Returns the number of components in this surface.
     */
    int components() const;

    /**
     * Returns the cairo image format type for this surface.
     */
    cairo_format_t format() const { return cairo_image_surface_get_format(getCairoSurfaces()[0]->cobj()); }

    /**
     * Create an image surface formatted the same as this one.
     *
     * @arg dimensions - Optional, if provided overrides the dimensions of the new surface.
     *
     * @returns A new unintialised Surface. If no optional args were present, this is an effective copy with a fresh surface.
     */
    std::shared_ptr<Surface> similar(std::optional<Geom::IntPoint> dimensions = {}) const;

    /**
     * Create an image surface formatted the same, but with a different color space.
     *
     * @arg color_space - Overrides the color space and thus the final memory format of the new surface.
     */
    std::shared_ptr<Surface> similar(std::optional<Geom::IntPoint> dimensions, std::shared_ptr<Colors::Space::AnySpace> const &color_space) const;

   /**
     * Returns the underlying Cairo surfaces. Only used by Context and PixelAccess to
     * initalise their access of the pixel data in this Surface.
     */
    std::vector<Cairo::RefPtr<Cairo::ImageSurface>> const &getCairoSurfaces() const;

    /**
     * Transform the surface memory into a specific inkscape color space. This will change any
     * future use of this image surface so care should be taken when using different spaces together.
     */
    void convertToColorSpace(std::shared_ptr<Colors::Space::AnySpace> const &color_space);

    /**
     * Same as above but does not replace the drawing surface internally, it returns a new copy.
     */
    std::shared_ptr<Surface> convertedToColorSpace(std::shared_ptr<Colors::Space::AnySpace> const &color_space) const;

// TODO: Recode using constexpr inline functions if possible
#define A8(s, e)  PixelAccess<CAIRO_FORMAT_A8,       0, e>(s._surfaces[0])
#define RGB(s, e) PixelAccess<CAIRO_FORMAT_ARGB32,   3, e>(s._surfaces[0])
#define C3(s, e)  PixelAccess<CAIRO_FORMAT_RGBA128F, 3, e>(s._surfaces[0])
#define C4(s, e)  PixelAccess<CAIRO_FORMAT_RGBA128F, 4, e>(s._surfaces[0], s._surfaces[1])

#define INNER(...)\
        auto d_comp = components();\
        auto d_format = format();\
        if (d_format == CAIRO_FORMAT_A8) {\
            auto s1 = A8((*this), DstEdgeMode);\
            return filter.filter(s1 __VA_OPT__(,) __VA_ARGS__);\
        } else if (d_format == CAIRO_FORMAT_ARGB32) {\
            auto s1 = RGB((*this), DstEdgeMode);\
            return filter.filter(s1 __VA_OPT__(,) __VA_ARGS__);\
        } else if (d_format == CAIRO_FORMAT_RGBA128F && d_comp == 3) {\
            auto s1 = C3((*this), DstEdgeMode);\
            return filter.filter(s1 __VA_OPT__(,) __VA_ARGS__);\
        } else if (d_format == CAIRO_FORMAT_RGBA128F && d_comp == 4) {\
            auto s1 = C4((*this), DstEdgeMode);\
            return filter.filter(s1 __VA_OPT__(,) __VA_ARGS__);\
        }\
        __builtin_unreachable(); // C++23 std::unreachable();

#define OUTER(...)\
        auto s_comp = src.components();\
        auto s_format = src.format();\
        if (s_format == CAIRO_FORMAT_A8) {\
            auto s2 = A8(src, SrcEdgeMode);\
            INNER(s2 __VA_OPT__(,) __VA_ARGS__)\
        } else if (s_format == CAIRO_FORMAT_ARGB32) {\
            auto s2 = RGB(src, SrcEdgeMode);\
            INNER(s2 __VA_OPT__(,) __VA_ARGS__)\
        } else if (s_format == CAIRO_FORMAT_RGBA128F && s_comp == 3) {\
            auto s2 = C3(src, SrcEdgeMode);\
            INNER(s2 __VA_OPT__(,) __VA_ARGS__)\
        } else if (s_format == CAIRO_FORMAT_RGBA128F && s_comp == 4) {\
            auto s2 = C4(src, SrcEdgeMode);\
            INNER(s2 __VA_OPT__(,) __VA_ARGS__)\
        }\
        __builtin_unreachable(); // C++23 std::unreachable();

    /**
     * Filters the contents of this surface according to the filter.
     *
     * @arg filter - The filter to run on this surface
     */
    template <PixelAccessEdgeMode DstEdgeMode = PixelAccessEdgeMode::NO_CHECK, typename Filter>
    auto run_pixel_filter(Filter const &filter)
    {
        INNER()
    }

    /**
     * Filters the contents of this surface according to the filter.
     *
     * @arg filter - The filter to run on this surface
     * @arg src    - Source image to feed to the filter function.
     */
    template <PixelAccessEdgeMode DstEdgeMode = PixelAccessEdgeMode::NO_CHECK,
              PixelAccessEdgeMode SrcEdgeMode = DstEdgeMode, typename Filter>
    auto run_pixel_filter(Filter const &filter, Surface const &src)
    {
        OUTER()
    }

    /**
     * Filters the contents of this surface according to the filter.
     *
     * @arg filter - The filter to run on this surface
     * @arg src    - Source image to feed to the filter function.
     * @arg mask   - A second source image, ususally a mask of some kind.
     */
    template <PixelAccessEdgeMode DstEdgeMode = PixelAccessEdgeMode::NO_CHECK,
              PixelAccessEdgeMode SrcEdgeMode = DstEdgeMode,
              PixelAccessEdgeMode MaskEdgeMode = SrcEdgeMode, typename Filter>
    auto run_pixel_filter(Filter const &filter, Surface const &src, Surface const &mask)
    {
        auto m_comp = mask.components();
        auto m_format = mask.format();

        if (m_format == CAIRO_FORMAT_A8) {
            OUTER(A8(mask, MaskEdgeMode))
        } else if (m_format == CAIRO_FORMAT_ARGB32) {
            OUTER(RGB(mask, MaskEdgeMode))
        } else if (m_format == CAIRO_FORMAT_RGBA128F && m_comp == 3) {
            OUTER(C3(mask, MaskEdgeMode))
        } else if (m_format == CAIRO_FORMAT_RGBA128F && m_comp == 4) {
            OUTER(C4(mask, MaskEdgeMode))
        }
        __builtin_unreachable(); // C++23 std::unreachable();
    }

private:

    mutable std::vector<Cairo::RefPtr<Cairo::ImageSurface>> _surfaces;
    const Geom::IntPoint _dimensions;
    const double _device_scale;
    std::shared_ptr<Colors::Space::AnySpace> _color_space;

#ifdef UNIT_TEST

public:
    /**
     * Construct a surface from a png file for testing
     */
    explicit Surface(std::string const &filename);

    /**
     * The debug function does not respect color spaces and is raw data output only.
     */
    void write_to_png(std::string const &filename) {
        for (unsigned i = 0; i < _surfaces.size(); i++) {
            _surfaces[i]->write_to_png(filename + std::to_string(i) + ".png");
        }
    }

#endif
};

} // namespace Inkscape::Renderer

#endif // INKSCAPE_DISPLAY_RENDERER_SURFACE_H

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
