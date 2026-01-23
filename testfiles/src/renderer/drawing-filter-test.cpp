// SPDX-License-Identifier: GPL-2.0-or-later

#include <2geom/transforms.h>

#include "drawing-testbase.h"
#include "surface-testbase.h"

#include "renderer/drawing-filters/enums.h"
#include "renderer/drawing-filters/filter.h"
#include "renderer/drawing-filters/gaussian-blur.h"

using namespace Inkscape::Renderer;

// Move to 2geom

bool area_same(Geom::Rect const &a, Geom::Rect const &b, double eps = Geom::EPSILON)
{
    return Geom::are_near(a.min(), b.min(), eps) && Geom::are_near(a.max(), b.max(), eps);
}

TEST(DrawingFilterTest, EffectArea)
{
    auto filter = get_transformed_filter();
    ASSERT_TRUE(area_same(*filter.filter_effect_area(Geom::Rect(0, 0, 0.2, 0.2)),
                          Geom::Rect(-0.048, -0.0024, 0.248, 0.2024)));
}

TEST(DrawingFilterTest, Resolution)
{
    auto filter = get_transformed_filter();
    auto res = filter.filter_resolution(
        Geom::Rect(-0.048, -0.0024, 0.248, 0.2024),
        Geom::Affine(841.81, 841.81, -841.81, 841.81, 0, 0), DrawingFilter::Quality::BETTER);
    ASSERT_NEAR(res.first, 352.388, 0.01);
    ASSERT_NEAR(res.second, 243.814, 0.01);
}

TEST(DrawingFilterTest, NoPrimitive)
{
    // Empty filter returns an empty image (not an unmodified one)
    EXPECT_PRIMITIVE_IS({},
                        "          "
                        "          "
                        "          "
                        "          "
                        "          "
                        "          "
                        "          "
                        "          "
                        "          "
                        "          ");
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
