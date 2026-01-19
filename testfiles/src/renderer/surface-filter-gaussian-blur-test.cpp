// SPDX-License-Identifier: GPL-2.0-or-later

#include "colors/manager.h"
#include "renderer/pixel-filters/gaussian-blur.h"
#include "surface-testbase.h"

using namespace Inkscape::Renderer;

TEST(SurfaceGaussianBlurTest, GaussianBlurCMYK)
{
    auto cmyk = Colors::Manager::get().find(Colors::Space::Type::CMYK);
    auto surface = TestSurface(Surface({21, 21}, 1, cmyk));
    surface.rect(3, 3, 15, 15, {0.5, 0.3, 0.0, 0.2, 1.0});

    surface.run_pixel_filter(PixelFilter::GaussianBlur({4, 4}));

    EXPECT_TRUE(VectorIsNear(surface.get_pixel(5, 5), {0.5, 0.3, 0, 0.2, 0.542}, 0.01));

    EXPECT_IMAGE_IS<PixelPatch::Method::ALPHA>(surface, 
                    " ..... "
                    ".:+=+:."
                    ".+O*O+."
                    ".=*X*=."
                    ".+O*O+."
                    ".:+=+:."
                    " ..... ");
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
