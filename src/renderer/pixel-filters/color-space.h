// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Convert between color spaces
 *//*
 * Authors:
 *   Martin Owens
 *
 * Copyright (C) 2025 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_RENDERER_PIXEL_FILTER_COLOR_SPACE_H
#define INKSCAPE_RENDERER_PIXEL_FILTER_COLOR_SPACE_H

#include <iostream>
#include <optional>
#include <vector>

#include "colors/cms/transform-surface.h"
#include "colors/spaces/base.h"
#include "colors/manager.h"

namespace Inkscape::Renderer::PixelFilter {

struct ColorSpaceTransform
{
    // We expect to get transfer functions in the correct order for the input color space
    ColorSpaceTransform(std::shared_ptr<Colors::Space::AnySpace> from, std::shared_ptr<Colors::Space::AnySpace> to)
        : _from(from)
        , _to(to)
    {}

    template <class AccessDst, class AccessSrc>
    void transform_lcms(AccessDst &dst, AccessSrc const &src) const
    {
        // This caching key should be used to cache transforms, if possible.
        auto from = _from ? _from : Colors::Manager::get().find(Colors::Space::Type::RGB);
        auto to = _to ? _to : Colors::Manager::get().find(Colors::Space::Type::RGB);

        Colors::CMS::TransformSurface::Format in = {
            from->getProfile(),
            AccessSrc::primary_size,
            AccessSrc::is_integer,
            true,
            // When the primary count is zero, this means one channel, alpha only and
            // since alpha is the primary, we don't need an extra alpha channel in transform.
            AccessSrc::primary_count > 0
        };
        Colors::CMS::TransformSurface::Format out = {
            to->getProfile(),
            AccessDst::primary_size,
            AccessDst::is_integer,
            false, // lcms2 can not output premultiplied alpha
            AccessDst::primary_count > 0
        };

        // Error is printed by TransformContext error_handler
        if (auto transform = Colors::CMS::TransformSurface(in, out, from->getBestIntent(to))) {
            transform.do_transform(dst.width(), dst.height(), src.memory(), dst.memory(),
                // Access strides are by type size stride, but lcms2 is by byte stride.
                src.stride() * AccessSrc::primary_size,
                dst.stride() * AccessDst::primary_size);
        }
    }

    /**
     * Convert from source to dest, converting the unpremultiplied colors to premultiplied
     * this can be used to copy the data, but also can be used in-place.
     *
     * This is needed because lcms2 always returns unpremultiplied color channels.
     */
    template <class AccessDst, class AccessSrc>
    void transform_mult(AccessDst &dst, AccessSrc const &src) const
        requires (AccessDst::channel_total == AccessSrc::channel_total)
    {
        dst.forEachPixel([&](int x, int y) {
            // Src is already unnmultiplied, true would double unmultiply here.
            auto color = src.colorAt(x, y, false);
            // Dst might be any format, which means this does more than just multiply alpha.
            dst.colorTo(x, y, color, true);
        });
    }

    template <class AccessDst, class AccessSrc>
    void filter(AccessDst &dst, AccessSrc const &src) const
    {
        if constexpr (std::is_same<AccessSrc, AccessDst>::value) {
            if (_to == _from && &dst == &src) {
                return; // Nothing to do.
            }
        }

        auto from = _from ? _from : Colors::Manager::get().find(Colors::Space::Type::RGB);
        auto to = _to ? _to : Colors::Manager::get().find(Colors::Space::Type::RGB);

        // lcms2 doesn't like converting from INT8 to FLOAT, INT16 works but that's not what
        // cairo is using in its integer surfaces.
        constexpr static bool is_same_format = AccessDst::is_integer == AccessSrc::is_integer;

        if (is_same_format && from->isDirect() && to->isDirect()) {
            if constexpr (!AccessDst::has_more_channels && !AccessSrc::has_more_channels) {
                if ( AccessDst::primary_count == to->getComponentCount()
                  && AccessSrc::primary_count == from->getComponentCount()) {
                    // This means the src and dst are both contiguous surfaces and are an icc profile transform.
                    transform_lcms(dst, src);
                    transform_mult(dst, dst);
                } else {
                    std::cerr << "Output surface format doesn't match color space!  ("
                        << AccessDst::primary_count << " != " << to->getComponentCount()
                        << " || "
                        << AccessSrc::primary_count << " != " << from->getComponentCount()
                        << ")\n";
                    return;
                }
            } else if (AccessSrc::has_more_channels && AccessDst::has_more_channels) {
                // ASSUMPTION: has_more_channels means channel_total is always the same
                if constexpr (AccessDst::channel_total == AccessSrc::channel_total) {
                    auto contiguous = src.createContiguous(true); // Copy
                    transform_lcms(contiguous, contiguous);
                    transform_mult(dst, contiguous);
                } else {
                    __builtin_unreachable(); // C++23 std::unreachable();
                }
            } else if constexpr (!AccessDst::has_more_channels && AccessSrc::has_more_channels) {
                auto contiguous = src.createContiguous(true); // Copy
                transform_lcms(dst, contiguous);
                transform_mult(dst, dst);
            } else if constexpr (AccessDst::has_more_channels && !AccessSrc::has_more_channels) {
                auto contiguous = dst.createContiguous(false); // No copy
                transform_lcms(contiguous, src);
                transform_mult(dst, contiguous);
            } else {
                __builtin_unreachable(); // C++23 std::unreachable();
            }
            return;
        }

        // Manual conversion is very slow as it has to convert one pixel at a time
        // using the entire Color::Space calling stack instead of lcms2 directly.
        typename AccessDst::Color c1;
        //dst.forEachPixel([&](int x, int y) {
        for (int y = 0; y < src.height(); y++) {
            for (int x = 0; x < src.width(); x++) {
            // The same color spaces, so just copy without converting
            if constexpr (std::is_same<AccessSrc, AccessDst>::value) {
                if (to == from) {
                    // Copy and fall through
                    auto c0 = src.colorAt(x, y, false);
                    dst.colorTo(x, y, c0, false);
                    return;
                }
            }
            // Conversions in inkscape are always alpha unmultiplied
            auto c0 = src.colorAt(x, y, true);
            // Expensive conversion, array to vector and back

            // These can't be constructed outside because it needs a fresh cin per thread on larger surfaces.
            std::vector<double> cin(c1.size(), 0.0);

            cin.assign(c0.begin(), c0.end());
            from->convert(cin, to);
            for (int i = 0; i < c1.size(); i++) {
                c1[i] = cin[i];
            }
            dst.colorTo(x, y, c1, true);
            }
        }//);
    }

    std::shared_ptr<Colors::Space::AnySpace> _from;
    std::shared_ptr<Colors::Space::AnySpace> _to;
};

struct AlphaSpaceExtraction
{
    template <class AccessDst, class AccessSrc>
    void filter(AccessDst &dst, AccessSrc const &src) const
        // requires (AccessDst::channel_total == 1)
    {
        dst.forEachPixel([&](int x, int y) {
            dst.alphaTo(x, y, src.alphaAt(x, y));
        });
    }
};

} // namespace Inkscape::Renderer::PixelFilter

#endif // INKSCAPE_RENDERER_PIXEL_FILTER_COLOR_SPACE_H

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
