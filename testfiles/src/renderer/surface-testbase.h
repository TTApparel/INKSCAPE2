// SPDX-License-Identifier: GPL-2.0-or-later
/***** SHARED TEST TOOLS FOR SURFACE TESTS *******/

#ifndef INKSCAPE_TEST_RENDERER_SURFACE_TESTBASE_H
#define INKSCAPE_TEST_RENDERER_SURFACE_TESTBASE_H

#include <memory>
#include <cairomm/context.h>
#include <2geom/rect.h>

#include "../test-utils.h"

#include "colors/manager.h"
#include "colors/color.h"
#include "renderer/surface.h"
#include "pixel-filter-testfilters.h"

using namespace Inkscape;


class TestSurface : public Renderer::Surface
{
public:
    auto testCairoSurfaces() const { return getCairoSurfaces(); }

    void rect(int x, int y, int w, int h, std::vector<double> const &c) 
    {   
        unsigned channel_count = components();
        auto surfaces = testCairoSurfaces();
        for (unsigned i = 0; i < surfaces.size(); i++) {
            unsigned off = i * 3;
            auto cr = Cairo::Context::create(surfaces[i]);
            cr->rectangle(x, y, w, h);
            cr->set_source_rgba(c[off + 0], c.size() > off + 1 && off + 1 < channel_count ? c[off + 1] : 0.0,
                                c.size() > off + 2 && off + 2 < channel_count ? c[off + 2] : 0.0, c.back());
            cr->fill();
        }   
    }

    auto get_pixel(int x, int y)
    {
        return run_pixel_filter(SampleColor(x, y));
    }
};

template <PixelPatch::Method method = PixelPatch::Method::COLORS>
void EXPECT_IMAGE_IS(Renderer::Surface &surface, std::string result, unsigned scale = 3, Geom::OptRect clip = {})
{
    auto patch = surface.run_pixel_filter(PixelPatch{
        ._method = method,
        ._patch_x = scale,
        ._patch_y = scale,
        ._clip = clip,
    });
    EXPECT_EQ(patch, PatchResult(result, patch._stride));
}

#endif // INKSCAPE_TEST_RENDERER_SURFACE_TESTBASE_H

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
