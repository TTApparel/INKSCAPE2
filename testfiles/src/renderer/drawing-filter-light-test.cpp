// SPDX-License-Identifier: GPL-2.0-or-later

#include "renderer/drawing-filters/light.h"
#include "drawing-testbase.h"
#include "surface-testbase.h"

using namespace Inkscape::Renderer;

TEST(DrawingLightTest, LightDiffuse)
{
    auto dl = std::make_unique<DrawingFilter::DiffuseLighting>();
    dl->set_output(1);

    dl->light_type = DrawingFilter::POINT_LIGHT;
    dl->light.point.x = 9;
    dl->light.point.y = 40;
    dl->light.point.z = 33;

    // This is much slower in linearRGB which is the default
    auto rgb = Colors::Manager::get().find(Colors::Space::Type::RGB);
    dl->setInterpolationSpace(rgb);

    dl->lighting_color = Colors::Color(rgb, {1.0, 1.0, 1.0, 1.0});
    dl->diffuseConstant = 1.0;
    dl->surfaceScale = 1.0;

    EXPECT_PRIMITIVE_IS<PixelPatch::Method::LIGHT>(std::move(dl),
                        "          "
                        "          "
                        "          "
                        "          "
                        "          "
                        " .        "
                        " ..       "
                        "  ..      "
                        "   ..     "
                        "    ..    "
    );
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
