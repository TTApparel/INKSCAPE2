// SPDX-License-Identifier: GPL-2.0-or-later

#include <gtest/gtest.h>
#include <2geom/svg-path-parser.h>

#include "colors/color.h"
#include "colors/manager.h"
#include "renderer/context.h"
#include "renderer/surface.h"

#include "surface-testbase.h"

using namespace Inkscape::Renderer;
using namespace Inkscape::Colors;

class RenderContextTest : public ::testing::Test
{
public:
    void SetUp() override
    {
        size = {21, 21};
        bounds = {0, 0, 21, 21};
        scale = {1, 1};
        cmyk = Manager::get().find(Space::Type::CMYK);

        surface = std::make_unique<Surface>(size, 1, cmyk);
        context = std::make_unique<Context>(*surface, bounds, scale);
    }

    void clear()
    {
        surface->run_pixel_filter(ClearPixels());
    }

    Geom::IntPoint size = {21, 21};
    Geom::IntRect bounds;
    Geom::Scale scale;
    std::shared_ptr<Space::AnySpace> cmyk;
    std::unique_ptr<Surface> surface;
    std::unique_ptr<Context> context;
};

TEST_F(RenderContextTest, Construction)
{
    ASSERT_EQ(context->logicalBounds(), Geom::Rect(0, 0, 21, 21));
}

TEST_F(RenderContextTest, CopyContext)
{
    {
        auto copy = *context;
    }
    auto _f = [](Context &dc) {
        auto copy = dc;
    };
    _f(*context);
}

TEST_F(RenderContextTest, RectangleAndFill)
{
    context->setSource(Color(cmyk, {0, 0, 0, 1.0, 1.0}));
    context->rectangle(Geom::Rect(3, 3, 18, 18));
    context->fill();
    EXPECT_IMAGE_IS(*surface,
                    "       "
                    " fffff "
                    " fffff "
                    " fffff "
                    " fffff "
                    " fffff "
                    "       ");
    context->setSource(Color(cmyk, {0.5, 0.5, 0.5, 0.0, 1.0}));
    context->rectangle(Geom::Rect(12, 12, 21, 21));
    context->fillPreserve();
    EXPECT_IMAGE_IS(*surface,
                    "       "
                    " fffff "
                    " fffff "
                    " fffff "
                    " fffZZZ"
                    " fffZZZ"
                    "    ZZZ");
    context->setSource(Color(cmyk, {0.0, 0.0, 0.0, 1.0, 1.0}));
    context->fillPreserve();
    EXPECT_IMAGE_IS(*surface,
                    "       "
                    " fffff "
                    " fffff "
                    " fffff "
                    " ffffff"
                    " ffffff"
                    "    fff");
}

TEST_F(RenderContextTest, LineAndStroke)
{
    context->setLineWidth(1);
    context->moveTo({0, 0});
    context->lineTo({21, 21});
    context->setSource(Color(cmyk, {0, 0, 0, 1.0, 1.0}));
    context->stroke();

    EXPECT_IMAGE_IS(*surface,
                    "f      "
                    " f     "
                    "  f    "
                    "   f   "
                    "    f  "
                    "     f "
                    "      f");

    context->setSource(Color(cmyk, {0.5, 0.5, 0.5, 0.0, 1.0}));
    context->moveTo({0, 21});
    context->lineTo({21, 0});
    context->stroke();
    EXPECT_IMAGE_IS(*surface,
                    "f     Z"
                    " f   Z "
                    "  f Z  "
                    "   .   "
                    "  Z f  "
                    " Z   f "
                    "Z     f");
}

TEST_F(RenderContextTest, LineCap)
{
    context->setSource(Color(cmyk, {0, 0, 0, 1.0, 1.0}));
    context->setLineWidth(12);
    context->moveTo({10.5, 10.5});
    context->lineTo({10.5, 30});
    context->setLineCap(Cairo::Context::LineCap::ROUND);
    context->strokePreserve();

    EXPECT_IMAGE_IS(*surface,
                    "       "
                    "  pfp  "
                    " pfffp "
                    " fffff "
                    " fffff "
                    " fffff "
                    " fffff ");

    clear();
    context->setLineCap(Cairo::Context::LineCap::SQUARE);
    context->strokePreserve();
    EXPECT_IMAGE_IS(*surface,
                    "       "
                    " pfffp "
                    " fffff "
                    " fffff "
                    " fffff "
                    " fffff "
                    " fffff ");
    clear();
    context->setLineCap(Cairo::Context::LineCap::BUTT);
    context->strokePreserve();
    EXPECT_IMAGE_IS(*surface,
                    "       "
                    "       "
                    "       "
                    " pfffp "
                    " fffff "
                    " fffff "
                    " fffff ");
}

TEST_F(RenderContextTest, LineJoinAndMiter)
{
    context->setSource(Color(cmyk, {0, 0, 0, 1.0, 1.0}));
    context->setLineWidth(12);
    context->moveTo({2, 23});
    context->lineTo({10.5, 10.5});
    context->lineTo({19, 23});
    context->setLineJoin(Cairo::Context::LineJoin::ROUND);
    context->strokePreserve();

    EXPECT_IMAGE_IS(*surface,
                    "       "
                    "  pfp  "
                    " pfffp "
                    " fffff "
                    "fffffff"
                    "fffffff"
                    "fffffff");
    clear();
    context->setLineJoin(Cairo::Context::LineJoin::MITER);
    context->strokePreserve();
    EXPECT_IMAGE_IS(*surface,
                    "   f   "
                    "  fff  "
                    " pfffp "
                    " fffff "
                    "fffffff"
                    "fffffff"
                    "fffffff");
    clear();
    context->setMiterLimit(0);
    context->strokePreserve();
    EXPECT_IMAGE_IS(*surface,
                    "       "
                    "       "
                    " pfffp "
                    " fffff "
                    "fffffff"
                    "fffffff"
                    "fffffff");
}

TEST_F(RenderContextTest, PathVector)
{
    static auto path = Geom::parse_svg_path("M 16.231027,8.0417894e-6 C 15.209899,-0.00199196 14.198607,0.36905404 13.465826,1.119077 L 1.2750816,13.593839 c -4.119,5.100881 2.80296,4.507445 5.770382,5.976636 1.064461,1.088055 -4.079954,1.891134 -3.015494,2.980097 1.064461,1.088054 6.4366434,2.096344 7.5029194,3.184398 1.06446,1.088055 -2.1787914,2.242396 -1.114331,3.330451 1.064461,1.088054 3.526403,0.05711 3.987397,2.56898 0.328504,1.794973 4.436591,0.771343 6.445726,-0.698756 1.064461,-1.088962 -2.036365,-0.986474 -0.971904,-2.074528 2.647085,-2.706979 5.111955,-0.98371 6.017609,-3.696133 0.447382,-1.340331 -3.896697,-2.066242 -2.830422,-3.154297 3.062706,-1.788621 13.648169,-2.952793 8.625331,-7.975631 L 19.055875,1.119077 C 18.283166,0.37722204 17.252156,0.00205004 16.231027,8.0417894e-6 Z M 16.340565,1.187364 c 0.729038,0.0039 1.455213,0.276381 1.978369,0.806343 l 4.823285,4.89854 c 0.457364,0.467346 0.451011,1.373146 0.195105,1.63359 L 20.942544,6.61018 20.471504,9.446734 18.470559,8.390656 15.26637,10.415014 14.205554,6.147223 12.484167,9.125646 H 9.8524726 c -1.072628,0 -1.198992,-1.361348 -0.224371,-2.335969 1.7024114,-1.837624 3.6564694,-3.710638 4.7182074,-4.79597 0.533591,-0.545389 1.265219,-0.8102 1.994256,-0.806343 z m 13.366951,22.487293 c -0.991409,0.03414 -1.968899,0.540661 -2.238417,1.490327 0,0.618893 4.559892,1.024584 4.559892,-0.14605 -0.324873,-0.940137 -1.330065,-1.37842 -2.321475,-1.344277 z M 8.6129956,26.359308 c -1.381862,0.07341 -2.864573,1.073019 -1.685431,2.094317 1.079887,0.933785 2.747932,-0.232076 3.2479474,-1.535201 -0.3269724,-0.43445 -0.9343964,-0.592485 -1.5625164,-0.559116 z m 18.2844444,0.07721 c -1.392056,1.248677 0.156141,2.515305 1.528234,1.708566 0.305816,-0.310355 -0.0082,-1.398212 -1.528234,-1.708566 z");
    context->transform(Geom::Scale(0.6, 0.6));
    context->setSource(Color(cmyk, {0.2, 0.2, 0, 1.0, 1.0}));
    context->path(path);
    context->fill();
    // This is a cute little inkscape logo in rich black cmyk,
    // run surface->write_to_png(..) to see it
    EXPECT_IMAGE_IS(*surface,
                    "  ffp  "
                    " ffpfp "
                    "ffffffp"
                    "fffffff"
                    " ffffp "
                    " fffff "
                    "  pf   ");
}

TEST_F(RenderContextTest, SetSourcePaint)
{
    auto rainbow = std::make_shared<TestSurface>(Renderer::Surface(INKSCAPE_TESTS_DIR "/data/renderer/rainbow-source-16.png"));
    auto output = rainbow->similar(Geom::IntPoint(21, 21));
    EXPECT_IMAGE_IS(*rainbow,
                    "RRRRRRR"
                    "PPPPPPP"
                    "XXXXXXX"
                    "8888888"
                    ":::::::"
                    ":::::::"
                    "2222222", 60);
    auto ctx = Context(*output);
    ctx.transform(Geom::Scale(0.05));
    ctx.setSource(*rainbow);
    ctx.setOperator(Cairo::Context::Operator::SOURCE);
    ctx.paint();
    EXPECT_IMAGE_IS(*output,
                    "RRRRRRR"
                    "PPPPPPP"
                    "XXXXXXX"
                    "8888888"
                    ":::::::"
                    ":::::::"
                    "2222222", 3);
}

TEST_F(RenderContextTest, SetMaskPaint)
{
    auto rainbow = std::make_shared<TestSurface>(Renderer::Surface(INKSCAPE_TESTS_DIR "/data/renderer/rainbow-source-16.png"));
    auto mask = std::make_shared<TestSurface>(Renderer::Surface(INKSCAPE_TESTS_DIR "/data/renderer/transform-source-16.png"));
    
    auto output = rainbow->similar(Geom::IntPoint(21, 21));
    auto ctx = Context(*output);
    ctx.transform(Geom::Scale(0.05));
    ctx.setSource(*rainbow);
    ctx.mask(*mask);

    output->write_to_png("/tmp/pngs/output");
    EXPECT_IMAGE_IS(*output,
                    "   A   "
                    "  PPP  "
                    " XXXXX "
                    "4888884"
                    " 9:::9 "
                    "  :::  "
                    "   1   ");
}

TEST_F(RenderContextTest, SetSourceSurface)
{
    context->setSource(Color(cmyk, {0.0, 1.0, 0.0, 0.6, 1.0}));
    context->rectangle(Geom::Rect(3, 3, 9, 9));
    context->fill();

    auto cross_s = std::make_unique<Surface>(Geom::IntPoint(9, 9), 1, cmyk);
    auto cross_ct = std::make_unique<Context>(*cross_s, bounds, scale);

    cross_ct->setSource(Color(cmyk, {0.7, 0, 0.7, 0.2, 0.7}));
    cross_ct->setLineWidth(1);
    // Draw an 'X' into our pattern
    cross_ct->moveTo({0, 0});
    cross_ct->lineTo({9, 9});
    cross_ct->moveTo({0, 9});
    cross_ct->lineTo({9, 0});
    cross_ct->stroke();

    context->setSource(*cross_s, 3, 3, Cairo::SurfacePattern::Filter::FAST, Cairo::Pattern::Extend::REPEAT);
    context->rectangle(Geom::Rect(0, 0, 21, 21));
    context->fill();
    EXPECT_IMAGE_IS(*surface,
                    "RR RR R"
                    "RnnRR R"
                    " n4  R "
                    "RR RR R"
                    "RR RR R"
                    "  R  R "
                    "RR RR R"
    );
}

TEST_F(RenderContextTest, SetPatternSolidColor)
{

    auto pattern = std::make_unique<Pattern>(Color(cmyk, {0.0, 1.0, 0.0, 0.6, 1.0}));

    context->setSource(pattern);
    context->rectangle(Geom::Rect(3, 3, 18, 18));
    context->fill();

    EXPECT_IMAGE_IS(*surface,
                    "       "
                    " nnnnn "
                    " nnnnn "
                    " nnnnn "
                    " nnnnn "
                    " nnnnn "
                    "       ");
}

TEST_F(RenderContextTest, SetPatternSurface)
{
    auto image_s = std::make_unique<Surface>(Geom::IntPoint(9, 9), 1, cmyk);
    auto image_ct = std::make_unique<Context>(*image_s, bounds, scale);

    image_ct->setSource(Color(cmyk, {0.7, 0, 0.7, 0.2, 0.7}));
    image_ct->setLineWidth(1);
    image_ct->moveTo({0, 0});
    image_ct->lineTo({9, 9});
    image_ct->stroke();

    auto pattern = std::make_unique<Pattern>(*image_s);
    pattern->setExtend(Cairo::Pattern::Extend::REFLECT);
    context->setSource(pattern);
    context->rectangle(Geom::Rect(3, 3, 18, 18));
    context->fill();

    EXPECT_IMAGE_IS(*surface,
                    "       "
                    " R  R  "
                    "  RR   "
                    "  RR   "
                    " R  R  "
                    "     R "
                    "       ");
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
