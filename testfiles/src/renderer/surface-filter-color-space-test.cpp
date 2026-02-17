// SPDX-License-Identifier: GPL-2.0-or-later

#include <memory>

#include "colors/manager.h"
#include "colors/spaces/cms.h"
#include "renderer/pixel-filters/color-space.h"
#include "surface-testbase.h"

using namespace Inkscape::Renderer;

static std::string cmyk_icc = INKSCAPE_TESTS_DIR "/data/colors/default_cmyk.icc";

TEST(SurfaceColorMatrixTest, RgbToCmyk)
{
    auto rgb = Colors::Manager::get().find(Colors::Space::Type::RGB);
    auto cmyk_profile = Colors::CMS::Profile::create_from_uri(cmyk_icc);
    auto cmyk = std::make_shared<Colors::Space::CMS>(cmyk_profile, "cmyk");

    auto surface_cmyk = TestSurface(Inkscape::Renderer::Surface({4, 4}, 1, cmyk));
    auto surface_rgb = TestSurface(Inkscape::Renderer::Surface({4, 4}, 1, rgb));

    surface_cmyk.rect(0, 0, 4, 4, {1, 0, 1, 0.5, 0.5});

    auto filter = PixelFilter::ColorSpaceTransform(cmyk, rgb);
    surface_rgb.run_pixel_filter(filter, surface_cmyk);
    auto color = surface_rgb.get_pixel(0, 0);
    EXPECT_TRUE(VectorIsNear(color, {-0.4505, 0.407, 0.207, 0.5}, 0.01));
}

TEST(SurfaceColorMatrixTest, AlphaExtraction)
{
    auto rgb = Colors::Manager::get().find(Colors::Space::Type::RGB);
    auto src = TestSurface(Inkscape::Renderer::Surface({21, 21}, 1, rgb));
    auto dest = TestSurface(Inkscape::Renderer::Surface({21, 21}, 1, rgb));

    src.rect(0,  3,  21, 3,  {0.0, 0.9, 0.0, 0.5});
    src.rect(15, 0,  3,  21, {0.5, 0.5, 0.5, 0.5});
    src.rect(0,  15, 21, 3,  {0.9, 0.0, 0.0, 0.5});
    src.rect(3,  0,  3,  21, {0.0, 0.0, 0.9, 0.5});

    dest.run_pixel_filter(PixelFilter::AlphaSpaceExtraction(), src);

    EXPECT_IMAGE_IS<PixelPatch::Method::ALPHA>(dest,
                    " -   - "
                    "-O---O-"
                    " -   - "
                    " -   - "
                    " -   - "
                    "-O---O-"
                    " -   - ");
}

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
