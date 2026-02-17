// SPDX-License-Identifier: GPL-2.0-or-later

#include <memory>

#include "colors/manager.h"
#include "colors/spaces/cms.h"
#include "renderer/pixel-filters/color-space.h"
#include "pixel-access-testbase.h"

using namespace Inkscape::Colors;
using namespace Inkscape::Renderer::PixelFilter;

static std::string cmyk_filename = INKSCAPE_TESTS_DIR "/data/colors/default_cmyk.icc";

class PixelColorSpaceTest : public ::testing::Test
{
public:
    void SetUp() override
    {
        alpha = Manager::get().find(Space::Type::Alpha);
        rgb = Manager::get().find(Space::Type::RGB);
        lrgb = Manager::get().find(Space::Type::linearRGB);
        hsl = Manager::get().find(Space::Type::HSL);
        cmyk_cpp = Manager::get().find(Space::Type::CMYK);
        cmyk_profile = CMS::Profile::create_from_uri(cmyk_filename);
        cmyk_icc = std::make_shared<Space::CMS>(cmyk_profile, "cmyk");
    }

    std::shared_ptr<Space::AnySpace> alpha;
    std::shared_ptr<Space::AnySpace> rgb;
    std::shared_ptr<Space::AnySpace> lrgb;
    std::shared_ptr<Space::AnySpace> hsl;
    std::shared_ptr<Space::AnySpace> cmyk_cpp;
    std::shared_ptr<Space::AnySpace> cmyk_icc;
    std::shared_ptr<CMS::Profile> cmyk_profile;
    TestCairoSurface<3> s1{60,60};
    TestCairoSurface<3> s2{60,60};
    TestCairoSurface<4> s3{60,60};
    TestCairoSurface<4> s4{60,60};
    TestCairoSurface<3, PixelAccessEdgeMode::NO_CHECK, CAIRO_FORMAT_ARGB32> i1{60, 60};

    int d1 = 6;
    int d2 = 52;
    int sam = 10;
};

TEST_F(PixelColorSpaceTest, Int32toFloat128)
{
    i1.rect(d1, d1, d2, d2, {1.0, 0.0, 1.0, 0.5});
    ColorSpaceTransform({}, lrgb).filter(*s1._d, *i1._d);
    ASSERT_TRUE(ColorIs(*s1._d, sam, sam, {1.0, 0.0, 1.0, 0.5}, true));
}

TEST_F(PixelColorSpaceTest, Float128ToInt32)
{
    s1.rect(d1, d1, d2, d2, {1.0, 0.0, 1.0, 0.5});
    ColorSpaceTransform(lrgb, {}).filter(*i1._d, *s1._d);
    ASSERT_TRUE(ColorIs(*i1._d, sam, sam, {1.0, 0.0, 1.0, 0.5}, true));
}

TEST_F(PixelColorSpaceTest, Lcms4x3Conversion)
{
    s3.rect(d1, d1, d2, d2, {1.0, 0.0, 1.0, 0.5, 0.5});
    ColorSpaceTransform(cmyk_icc, rgb).filter(*s1._d, *s3._d);
    ASSERT_TRUE(ColorIs(*s1._d, sam, sam, {-0.4505, 0.407, 0.207, 0.5}, true));
}

TEST_F(PixelColorSpaceTest, Lcms3x4Conversion)
{
    s1.rect(d1, d1, d2, d2, {1.0, 0.0, 0.0, 0.5});
    ColorSpaceTransform(rgb, cmyk_icc).filter(*s3._d, *s1._d);
    ASSERT_TRUE(ColorIs(*s3._d, sam, sam, {0.0, 1.0, 1.0, 0.0, 0.5}, true));
}

TEST_F(PixelColorSpaceTest, Lcms3x3Conversion)
{
    s1.rect(d1, d1, d2, d2, {0.435, 0.017, 0.055, 0.5});
    ColorSpaceTransform(lrgb, rgb).filter(*s2._d, *s1._d);
    ASSERT_TRUE(ColorIs(*s2._d, sam, sam, {0.691, 0.139, 0.259, 0.5}, true));
}

/* Needs another icc profile in test suite, skipping for now, covered by Lcms4x4NoOp
TEST_F(PixelColorSpaceTest, Lcms4x4Conversion)
{}
*/

TEST_F(PixelColorSpaceTest, Internal3x3Conversion)
{
    s1.rect(d1, d1, d2, d2, {1.0, 0.0, 0.0, 0.5});
    ColorSpaceTransform(rgb, hsl).filter(*s2._d, *s1._d);
    ASSERT_TRUE(ColorIs(*s2._d, sam, sam, {0.0, 1.0, 0.5, 0.5}, true));
}

TEST_F(PixelColorSpaceTest, Internal4x4Conversion)
{
    s3.rect(d1, d1, d2, d2, {1.0, 0.0, 1.0, 0.5, 0.5});
    ColorSpaceTransform(cmyk_icc, cmyk_cpp).filter(*s4._d, *s3._d);
    ASSERT_TRUE(ColorIs(*s4._d, sam, sam, {2.11, 0, 0.49, 0.593, 0.5}, true));
}

// Missing test, CmykIcc to Different CmykIcc

TEST_F(PixelColorSpaceTest, Internal4x4NoOpp)
{
    s3.rect(d1, d1, d2, d2, {1.0, 0.0, 1.0, 0.5, 0.5});
    ColorSpaceTransform(cmyk_icc, cmyk_icc).filter(*s3._d, *s3._d);
    ASSERT_TRUE(ColorIs(*s3._d, sam, sam, {1.0, 0.0, 1.0, 0.5, 0.5}, true));
}

TEST_F(PixelColorSpaceTest, Lcms4x4NoOpp)
{
    s3.rect(d1, d1, d2, d2, {1.0, 0.0, 1.0, 0.5, 0.5});
    ColorSpaceTransform(cmyk_icc, cmyk_icc).filter(*s4._d, *s3._d);
    // This change happens inside lcms2 and I haven't decided if it's correct yet
    ASSERT_TRUE(ColorIs(*s4._d, sam, sam, {1, 0.32, 1, 0.268, 0.5}, true));
}

TEST_F(PixelColorSpaceTest, LuminosityToAlpha)
{
    TestCairoSurface<0> a1{21, 21};
    TestCairoSurface<3> c1{21, 21};

    c1.rect(0,  3,  21, 3,  {0.0, 0.9, 0.0, 0.5});
    c1.rect(15, 0,  3,  21, {0.5, 0.5, 0.5, 0.5});
    c1.rect(0,  15, 21, 3,  {0.9, 0.0, 0.0, 0.5});
    c1.rect(3,  0,  3,  21, {0.0, 0.0, 0.9, 0.5});

    ColorSpaceTransform(rgb, alpha).filter(*a1._d, *c1._d);

    EXPECT_TRUE(ImageIs(*a1._d,
                " .   - "
                "*.***+*"
                " .   - "
                " .   - "
                " .   - "
                ":.:::::"
                " .   - ",
                PixelPatch::Method::ALPHA, true));

}

TEST_F(PixelColorSpaceTest, RGBAToAlpha)
{
    TestCairoSurface<0> a1{60,60};

    s1.rect(d1, d1, d2, d2, {1.0, 0.0, 0.75, 0.5});
    AlphaSpaceExtraction().filter(*a1._d, *s1._d);
    ASSERT_TRUE(ColorIs(*a1._d, sam, sam, {0.5}));
}

TEST_F(PixelColorSpaceTest, CMYKAToAlpha)
{
    TestCairoSurface<0> a1{60,60};

    s3.rect(d1, d1, d2, d2, {1.0, 0.0, 0.4, 0.75, 0.5});
    AlphaSpaceExtraction().filter(*a1._d, *s3._d);
    ASSERT_TRUE(ColorIs(*a1._d, sam, sam, {0.5}));
}

TEST_F(PixelColorSpaceTest, AlphaToLuminosity)
{
    TestCairoSurface<0> a1{21, 21};
    TestCairoSurface<3> c1{21, 21};

    c1.rect(0,  3,  21, 3,  {0.0, 0.9, 0.0, 0.5});
    c1.rect(15, 0,  3,  21, {0.5, 0.5, 0.5, 0.5});
    c1.rect(0,  15, 21, 3,  {0.9, 0.0, 0.0, 0.5});
    c1.rect(3,  0,  3,  21, {0.0, 0.0, 0.9, 0.5});

    AlphaSpaceExtraction().filter(*a1._d, *c1._d);

    // Notice that this result is different from LuminosityToAlpha
    // despite the same c1 source. That extracts luminosity of the
    // color channels while this discards color and extracts alpha.
    EXPECT_TRUE(ImageIs(*a1._d,
                " -   - "
                "-O---O-"
                " -   - "
                " -   - "
                " -   - "
                "-O---O-"
                " -   - ",
                PixelPatch::Method::ALPHA, true));
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
