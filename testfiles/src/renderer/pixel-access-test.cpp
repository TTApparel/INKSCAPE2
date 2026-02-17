// SPDX-License-Identifier: GPL-2.0-or-later

#include "pixel-access-testbase.h"

using namespace Inkscape;

/***** TEST DATA *******/
constexpr inline double dbl(bool a)
{
    return a ? 1.0 : 0.0;
}

struct TestFilter
{
    template <class AccessDst, class AccessSrc>
    void filter(AccessDst &dst, AccessSrc const &src)
    {
        for (int y = 0; y < dst.height(); y++) {
            for (int x = 0; x < dst.width(); x++) {
                if (x / 3 == y / 3) {
                    auto c = src.colorAt(x, y);
                    dst.colorTo(x, y, c);
                }
            }
        }
    }
};

/**** TESTS *****/

TEST(PixelAccessTest, ColorIs)
{
    for (auto format : {CAIRO_FORMAT_ARGB32, CAIRO_FORMAT_RGBA128F}) {
        auto cobj = cairo_image_surface_create(format, 21, 21);
        auto s = Cairo::RefPtr<Cairo::ImageSurface>(new Cairo::ImageSurface(cobj, true));

        // Draw something here
        {
            auto c = cairo_create(cobj);
            for (auto channel = 0; channel < (format == CAIRO_FORMAT_A8 ? 1 : 3); channel++) {
                cairo_rectangle(c, 3 + channel * 6, 3 + channel * 6, 6, 6);
                cairo_set_source_rgba(c, channel == 0, channel == 1, channel == 2, 0.6);
                cairo_fill(c);
            }
            cairo_destroy(c);
        }
        s->flush();

        ASSERT_TRUE(ImageIs(s, "       "
                               " 22    "
                               " 22    "
                               "   88  "
                               "   88  "
                               "     PP"
                               "     PP"))
        << "Format: " << get_format_name(format) << "\n"
        << "Method: INTEGER COORDS\n";

        // Bilinear should be the same as Int (just slower)
        ASSERT_TRUE(ImageIs(s,
                            "       "
                            " 22    "
                            " 22    "
                            "   88  "
                            "   88  "
                            "     PP"
                            "     PP",
                            PixelPatch::Method::COLORS, true, true))
            << "Format: " << get_format_name(format) << "\n"
            << "Method: BILINEAR DECIMAL COORDS (Premult)\n";

        ASSERT_TRUE(ImageIs(s,
                            "       "
                            " 22    "
                            " 22    "
                            "   88  "
                            "   88  "
                            "     PP"
                            "     PP",
                            PixelPatch::Method::COLORS, false, true))
            << "Format: " << get_format_name(format) << "\n"
            << "Method: BILINEAR DECIMAL COORDS (Unpremult)\n";
    }
}

TEST(PixelAccessTest, AlphaIs)
{
    for (auto format : {CAIRO_FORMAT_A8, CAIRO_FORMAT_ARGB32, CAIRO_FORMAT_RGBA128F}) {
        auto cobj = cairo_image_surface_create(format, 21, 21);
        auto s = Cairo::RefPtr<Cairo::ImageSurface>(new Cairo::ImageSurface(cobj, true));

        // Draw something here
        {
            auto c = cairo_create(cobj);
            cairo_rectangle(c, 3, 3, 15, 15);
            cairo_set_source_rgba(c, 0.0, 0.0, 0.0, 1.0);
            cairo_fill(c);
            cairo_destroy(c);
        }
        s->flush();

        ASSERT_TRUE(ImageIs(s,
                            "       "
                            " &&&&& "
                            " &&&&& "
                            " &&&&& "
                            " &&&&& "
                            " &&&&& "
                            "       ",
                            PixelPatch::Method::ALPHA))
            << "Format: " << get_format_name(format) << "\n"
            << "Method: INTEGER COORDS\n";

        ASSERT_TRUE(ImageIs(s,
                            "       "
                            " &&&&& "
                            " &&&&& "
                            " &&&&& "
                            " &&&&& "
                            " &&&&& "
                            "       ",
                            PixelPatch::Method::ALPHA, false, true))
            << "Format: " << get_format_name(format) << "\n"
            << "Method: BILINEAR FLOAT COORDS\n";

        if (format == CAIRO_FORMAT_A8) {
            ASSERT_TRUE(ImageIs(s, "       "
                                   " ..... "
                                   " ..... "
                                   " ..... "
                                   " ..... "
                                   " ..... "
                                   "       "))
                << "Format: " << get_format_name(format) << "\n"
                << "Method: Color for Alpha\n";
        }
    }
}

TEST(PixelAccessTest, BilinearInterpolation)
{
    auto src = TestCairoSurface<3>(4, 4);
    src.rect(1, 1, 2, 2, {1.0, 0.0, 1.0, 1.0});

    EXPECT_NEAR(src._d->alphaAt(0.5, 0.5), 0.25, 0.001);
    EXPECT_NEAR(src._d->alphaAt(2.5, 2.5), 0.25, 0.001);
    EXPECT_NEAR(src._d->alphaAt(0.5, 2.5), 0.25, 0.001);
    EXPECT_NEAR(src._d->alphaAt(2.5, 0.5), 0.25, 0.001);
    EXPECT_NEAR(src._d->alphaAt(1.5, 0.5), 0.50, 0.001);
    EXPECT_NEAR(src._d->alphaAt(0.5, 1.5), 0.50, 0.001);
    EXPECT_NEAR(src._d->alphaAt(0.3, 1.3), 0.6999, 0.001);

    EXPECT_TRUE(ColorIs<double>(*src._d, 0.5, 0.5, {1.0, 0.0, 1.0, 0.25}, true));
    EXPECT_TRUE(ColorIs<double>(*src._d, 0.5, 1.5, {1.0, 0.0, 1.0, 0.50}, true));
    EXPECT_TRUE(ColorIs<double>(*src._d, 0.3, 1.3, {1.0, 0.0, 1.0, 0.7}, true));
    EXPECT_TRUE(ColorIs<double>(*src._d, 0.5, 0.5, {0.25, 0.0, 0.25, 0.25}, false));
    EXPECT_TRUE(ColorIs<double>(*src._d, 0.5, 1.5, {0.5, 0.0, 0.5, 0.50}, false));
    EXPECT_TRUE(ColorIs<double>(*src._d, 0.3, 1.3, {0.7, 0.0, 0.7, 0.7}, false));
}

TEST(PixelAccessTest, UnmultiplyColor)
{
    auto src = TestCairoSurface<3>(4, 4);
    src.rect(1, 1, 2, 2, {1.0, 0.0, 1.0, 0.5});

    ASSERT_TRUE(ColorIs(*src._d, 1, 1, {0.5, 0.0, 0.5, 0.5}, false));
    ASSERT_TRUE(ColorWillBe(*src._d, 2, 2, {0.5, 0.5, 0.5, 0.5}, false));

    ASSERT_TRUE(ColorIs(*src._d, 1, 1, {1.0, 0.0, 1.0, 0.5}, true));
    ASSERT_TRUE(ColorWillBe(*src._d, 3, 3, {0.5, 0.5, 0.5, 0.5}, true));

    auto src2 = TestCairoSurface<4>(4, 4);
    src2.rect(1, 1, 2, 2, {1.0, 0.0, 1.0, 0.4, 0.5});

    ASSERT_TRUE(ColorIs(*src2._d, 1, 1, {0.5, 0.0, 0.5, 0.2, 0.5}, false));
    ASSERT_TRUE(ColorWillBe(*src2._d, 2, 2, {0.5, 0.5, 0.5, 0.5, 0.5}, false));

    ASSERT_TRUE(ColorIs(*src2._d, 1, 1, {1.0, 0.0, 1.0, 0.4, 0.5}, true));
    ASSERT_TRUE(ColorWillBe(*src2._d, 3, 3, {0.5, 0.5, 0.5, 0.5, 0.5}, true));
}

template <PixelAccessEdgeMode edge_mode>
PixelAccess<CAIRO_FORMAT_RGBA128F, 4, edge_mode> getEdgeModeSurface(int width, int height)
{
    auto src = TestCairoSurface<4, edge_mode>(width, height);

    // Draw a box of 4 channels, one edge per channel, overlaps at the corners
    for (int x = 0; x < (int)width; x++) {
        for (int y = 0; y < (int)height; y++) {
            src._d->colorTo(x, y, {dbl(y == 0), dbl(y == height - 1), dbl(x == 0), dbl(x == width - 1), 1.0});
        }
    }
    return *src._d;
}

TEST(PixelAccessTest, EdgeModeError)
{
    std::array<double, 5> c;
    auto d = getEdgeModeSurface<PixelAccessEdgeMode::ERROR>(4, 4);
    for (int x = -1; x < 5; x++) {
        for (int y = -1; y < 5; y++) {
            if (x < 0 || x > 3 || y < 0 || y > 3) {
                EXPECT_THROW(d.colorAt(x, y), std::exception);
                EXPECT_THROW(d.colorTo(x, y, c), std::exception);
            } else {
                // Checking for no error, and confirming the test-suite
                ASSERT_TRUE(ColorIs(d, x, y, {dbl(y == 0), dbl(y == 3), dbl(x == 0), dbl(x == 3), 1.0}, false));
                ASSERT_TRUE(ColorIs(d, x, y, {dbl(y == 0), dbl(y == 3), dbl(x == 0), dbl(x == 3), 1.0}, true));
                ASSERT_TRUE(ColorWillBe(d, x, y, {0.5, 0.5, 0.5, 0.5, 0.5}));
            }
        }
    }
}

TEST(PixelAccessTest, EdgeModeExtend)
{
    auto d = getEdgeModeSurface<PixelAccessEdgeMode::EXTEND>(4, 4);

    for (int x = -1; x < 5; x++) {
        for (int y = -1; y < 5; y++) {
            ASSERT_TRUE(ColorIs(d, x, y, {dbl(y <= 0), dbl(y >= 3), dbl(x <= 0), dbl(x >= 3), 1.0}, false));
            ASSERT_TRUE(ColorIs(d, x, y, {dbl(y <= 0), dbl(y >= 3), dbl(x <= 0), dbl(x >= 3), 1.0}, true));
            ASSERT_TRUE(
                ColorWillBe(d, x, y, {0.5, 0.5, 0.5, 0.5, 0.5}, true, std::clamp(x, 0, 3), std::clamp(y, 0, 3)));
        }
    }
}

TEST(PixelAccessTest, EdgeModeWrap)
{
    auto d = getEdgeModeSurface<PixelAccessEdgeMode::WRAP>(4, 4);

    for (int x = -1; x < 5; x++) {
        for (int y = -1; y < 5; y++) {
            ASSERT_TRUE(ColorIs(
                d, x, y,
                {dbl(y == 0 || y == 4), dbl(y == -1 || y == 3), dbl(x == 0 || x == 4), dbl(x == -1 || x == 3), 1.0},
                false));
            ASSERT_TRUE(ColorIs(
                d, x, y,
                {dbl(y == 0 || y == 4), dbl(y == -1 || y == 3), dbl(x == 0 || x == 4), dbl(x == -1 || x == 3), 1.0},
                true));
            ASSERT_TRUE(ColorWillBe(d, x, y, {0.5, 0.5, 0.5, 0.5, 0.5}, true, x % 4, y % 4));
        }
    }
}

TEST(PixelAccessTest, EdgeModeNone)
{
    auto d = getEdgeModeSurface<PixelAccessEdgeMode::ZERO>(4, 4);

    for (int x = -1; x < 5; x++) {
        for (int y = -1; y < 5; y++) {
            if (x < 0 || x > 3 || y < 0 || y > 3) {
                ASSERT_TRUE(ColorIs(d, x, y, {0.0, 0.0, 0.0, 0.0, 0.0}, false));
                ASSERT_TRUE(ColorIs(d, x, y, {0.0, 0.0, 0.0, 0.0, 0.0}, true));
                ASSERT_FALSE(ColorWillBe(d, x, y, {0.5, 0.5, 0.5, 0.5, 0.5}));
            }
        }
    }
}

template <cairo_format_t F>
void TestColorTo()
{
    auto cobj = cairo_image_surface_create(F, 21, 21);
    auto s = Cairo::RefPtr<Cairo::ImageSurface>(new Cairo::ImageSurface(cobj, true));
    auto d = PixelAccess<F, 3>(s);

    for (unsigned x = 0; x < 21; x++) {
        for (unsigned y = 0; y < 21; y++) {
            // Build a red X in the image surface
            if (x == y || 20 - x == y) {
                d.colorTo(x, y, {1.0, 0.0, 0.0, 1.0}, true);
                // Build blue square outline
            } else if (x == 0 || x == 20 || y == 0 || y == 20) {
                d.colorTo(x, y, {0.0, 0.0, 0.5, 0.5}, false);
                // Build a green cross (unpremultiplied)
            } else if (x == 10 || y == 10) {
                d.colorTo(x, y, {0.0, 0.7, 0.0, 0.7}, true);
            }
        }
    }

    // Premultiplied test ignores semi-transparent:
    // blue - because it's value of 1.0 is READ as 0.5 premultiplied
    // green - because it's value of 0.7 is WRITTEN as 0.49 premultiplied
    ASSERT_TRUE(ImageIs(s,
                             "1  .  1"
                             " 1   1 "
                             "  1 1  "
                             ".  1  ."
                             "  1 1  "
                             " 1   1 "
                             "1  .  1",
                             PixelPatch::Method::COLORS, false));

    // Unpremultiplied includes semi-transparent blue and green.
    ASSERT_TRUE(ImageIs(s,
                             "A@@@@@A"
                             "@1 4 1@"
                             "@ 141 @"
                             "@44544@"
                             "@ 141 @"
                             "@1 4 1@"
                             "A@@@@@A",
                             PixelPatch::Method::COLORS, true));
}

TEST(PixelAccessTest, colorTo)
{
    {
        auto cobj = cairo_image_surface_create(CAIRO_FORMAT_A8, 21, 21);
        auto s = Cairo::RefPtr<Cairo::ImageSurface>(new Cairo::ImageSurface(cobj, true));
        auto d = PixelAccess<CAIRO_FORMAT_A8, 0>(s);

        for (unsigned x = 0; x < 21; x++) {
            for (unsigned y = 0; y < 21; y++) {
                // Build a cross in the image surface
                if (x == y || 20 - x == y) {
                    d.colorTo(x, y, {1.0});
                }
            }
        }

        ASSERT_TRUE(ImageIs(s,
                                 ".     ."
                                 " .   . "
                                 "  . .  "
                                 "   +   "
                                 "  . .  "
                                 " .   . "
                                 ".     .",
                                 PixelPatch::Method::ALPHA));
    }

    TestColorTo<CAIRO_FORMAT_RGBA128F>();
    TestColorTo<CAIRO_FORMAT_ARGB32>();
}

TEST(PixelAccessTest, multiSpanChannels)
{
    // We only test RGBA128F, since this is what is going to be used
    auto src = TestCairoSurface<4>(21, 21);
    {
        auto c1 = cairo_create(src._cobj[0]);
        auto c2 = cairo_create(src._cobj[1]);
        for (auto channel = 0; channel < 4; channel++) {
            cairo_rectangle(c1, channel * 5, channel * 5, 6, 6);
            cairo_set_source_rgba(c1, channel == 0, channel == 1, channel == 2, 1.0);
            cairo_fill(c1);

            cairo_rectangle(c2, channel * 5, channel * 5, 6, 6);
            cairo_set_source_rgba(c2, channel == 3, 0.0, 0.0, 1.0);
            cairo_fill(c2);
        }
        cairo_destroy(c1);
        cairo_destroy(c2);
    }

    EXPECT_TRUE(ImageIs(*src._d,
                        "&&     "
                        "&&..   "
                        " .&o   "
                        " .o*o. "
                        "   o&. "
                        "   ..&&"
                        "     &&",
                        PixelPatch::Method::ALPHA));

    ASSERT_TRUE(ColorIs(*src._d, 0, 0, {1.0, 0.0, 0.0, 0.0, 1.0}, true));
    ASSERT_TRUE(ColorIs(*src._d, 20, 20, {0.0, 0.0, 0.0, 1.0, 1.0}, true));

    ASSERT_TRUE(ImageIs(*src._d, "22     "
                                 "224    "
                                 " 488   "
                                 "  8DP  "
                                 "   PP@ "
                                 "    @ff"
                                 "     ff", PixelPatch::Method::COLORS, false, false));
    // Request by float coordinates instead
    ASSERT_TRUE(ImageIs(*src._d, "22     "
                                 "224    "
                                 " 488   "
                                 "  8DP  "
                                 "   PP@ "
                                 "    @ff"
                                 "     ff", PixelPatch::Method::COLORS, false, true));

for (auto x = 0; x < 21; x++) {
        for (auto y = 0; y < 21; y++) {
            src._d->colorTo(x, y, {1.0, 0.0, 0.0, 1.0, 1.0});
        }
    }

    ASSERT_TRUE(ImageIs(*src._d, "hhhhhhh"
                                      "hhhhhhh"
                                      "hhhhhhh"
                                      "hhhhhhh"
                                      "hhhhhhh"
                                      "hhhhhhh"
                                      "hhhhhhh"));
}

TEST(PixelAccessTest, Filter)
{
    auto src1 = TestCairoSurface<4>(21, 21);
    src1.rect(3, 3, 15, 15, {1.0, 0.0, 1.0, 0.0, 0.5});

    ASSERT_TRUE(ImageIs(*src1._d, "       "
                                       " RRRRR "
                                       " RRRRR "
                                       " RRRRR "
                                       " RRRRR "
                                       " RRRRR "
                                       "       "));

    auto src2 = TestCairoSurface<4>(21, 21);
    src2.rect(12, 12, 9, 9, {1.0, 0.5, 1.0, 0.5, 1.0});

    ASSERT_TRUE(ImageIs(*src2._d, "       "
                                       "       "
                                       "       "
                                       "       "
                                       "    FFF"
                                       "    FFF"
                                       "    FFF"));

    TestFilter().filter(*src1._d, *src2._d);

    ASSERT_TRUE(ImageIs(*src1._d, "       "
                                       "  RRRR "
                                       " R RRR "
                                       " RR RR "
                                       " RRRFR "
                                       " RRRRF "
                                       "      F"));
}

TEST(PixelAccessTest, nonCairoMemoryAccess)
{
    auto src = TestCustomSurface<4>(21, 21);
    src.rect(6, 6, 9, 9, {1.0, 0.0, 1.0, 0.5, 1.0});

    EXPECT_TRUE(ImageIs(*src._d,
                             "       "
                             "       "
                             "  &&&  "
                             "  &&&  "
                             "  &&&  "
                             "       "
                             "       ",
                             PixelPatch::Method::ALPHA));
}

template <typename T0, cairo_format_t F>
void TestContiguousMemory(std::array<double, 5> in, std::array<T0, 5> cmp, bool unpre = false)
{
    int w = 21;
    int h = 21;
    auto src = TestCairoSurface<4, PixelAccessEdgeMode::NO_CHECK, F>(w, h);
    src.rect(6, 6, 9, 9, in);

    auto copy = src._d->template contiguousMemory<T0>(true, unpre);
    ASSERT_EQ(copy.size(), w * h * 5);

    std::array<T0, 5> zero;
    std::array<T0, 5> first;
    std::array<T0, 5> mid;
    int x = 10, y = 10;
    for (auto i = 0; i < 5; i++) {
        first[i] = copy[(0 * w + 0) * 5 + i];
        mid[i] = copy[(y * w + x) * 5 + i];
    }
    if constexpr (std::is_integral_v<T0>) {
        EXPECT_EQ(first, zero);
        EXPECT_EQ(mid, cmp);
    } else {
        EXPECT_TRUE(VectorIsNear(first, zero, 0.005));
        EXPECT_TRUE(VectorIsNear(mid, cmp, 0.005));
    }
}

TEST(PixelAccessTest, contiguousMemory)
{
    // Direct copy of original data
    TestContiguousMemory<unsigned char, CAIRO_FORMAT_ARGB32>({0.2, 0.4, 0.6, 0.8, 0.5}, {25, 51, 76, 102, 128}, false);
    TestContiguousMemory<float, CAIRO_FORMAT_RGBA128F>({1.0, 0.0, 1.0, 0.5, 0.5}, {0.5, 0.0, 0.5, 0.25, 0.5}, false);

    // Data upscaling shows small numbers just get rounded down to zero
    TestContiguousMemory<uint16_t, CAIRO_FORMAT_ARGB32>({0.002, 0.004, 0.006, 0.008, 0.5}, {0, 0, 0, 257, 32896}, false);
    TestContiguousMemory<uint32_t, CAIRO_FORMAT_ARGB32>({0.002, 0.004, 0.006, 0.008, 0.5},
                                                        {0, 0, 0, 16843009, 2155905152}, false);
    TestContiguousMemory<float, CAIRO_FORMAT_ARGB32>({1.0, 0.0, 1.0, 0.5, 0.5}, {0.5, 0.0, 0.5, 0.25, 0.5}, false);
    TestContiguousMemory<double, CAIRO_FORMAT_ARGB32>({1.0, 0.0, 1.0, 0.5, 0.5}, {0.5, 0.0, 0.5, 0.25, 0.5}, false);

    // Data isn't rescaled so we see actual values not rounded
    TestContiguousMemory<unsigned char, CAIRO_FORMAT_RGBA128F>({0.2, 0.4, 0.6, 0.8, 0.5}, {25, 51, 76, 102, 127}, false);
    TestContiguousMemory<uint16_t, CAIRO_FORMAT_RGBA128F>({0.002, 0.004, 0.006, 0.008, 0.5}, {65, 130, 196, 261, 32767}, false);
    // TODO: DOESN'T COMPILE TestContiguousMemory<uint32_t, CAIRO_FORMAT_RGBA128F>();
    TestContiguousMemory<double, CAIRO_FORMAT_RGBA128F>({1.0, 0.0, 1.0, 0.5, 0.5}, {0.5, 0.0, 0.5, 0.25, 0.5}, false);

    // Unpremultiply alpha in returned values
    TestContiguousMemory<unsigned char, CAIRO_FORMAT_ARGB32>({0.1, 0.2, 0.3, 0.4, 0.5}, {23, 49, 75, 101, 128}, true);
    TestContiguousMemory<float, CAIRO_FORMAT_RGBA128F>({1.0, 0.0, 1.0, 0.5, 0.5}, {1.0, 0.0, 1.0, 0.5, 0.5}, true);
    TestContiguousMemory<uint16_t, CAIRO_FORMAT_ARGB32>({0.001, 0.002, 0.003, 0.004, 0.5}, {0, 0, 0, 0, 32896}, true);
    TestContiguousMemory<uint32_t, CAIRO_FORMAT_ARGB32>({0.001, 0.002, 0.003, 0.004, 0.5}, {0, 0, 0, 0, 2155905152}, true);
    TestContiguousMemory<float, CAIRO_FORMAT_ARGB32>({1.0, 0.0, 1.0, 0.5, 0.5}, {1.0, 0.0, 1.0, 0.5, 0.5}, true);
    TestContiguousMemory<double, CAIRO_FORMAT_ARGB32>({1.0, 0.0, 1.0, 0.5, 0.5}, {1.0, 0.0, 1.0, 0.5, 0.5}, true);
    TestContiguousMemory<unsigned char, CAIRO_FORMAT_RGBA128F>({0.1, 0.2, 0.3, 0.4, 0.5}, {25, 51, 76, 101, 127}, true);
    TestContiguousMemory<uint16_t, CAIRO_FORMAT_RGBA128F>({0.001, 0.002, 0.003, 0.004, 0.5}, {65, 131, 195, 261, 32767}, true);
}

template <int write = 3, int read = 1, bool is_column = false, typename Access>
void testPixelAccess(Access &access, std::string result)
{
    {
        auto line_alpha = access.template getLineAccess<is_column, write>();
        auto line_color = std::as_const(access).template getLineAccess<is_column, read>();
        for (auto y = 0; y < 7; y+=2) {
            line_alpha.gotoLine(y);
            line_color.gotoLine(y);
            for (auto x = 0; x < 7; x+=2) {
                *(line_alpha.pixels + x * line_alpha.next) = (float)1.0;
                if (x > 0 && x < 6 && y > 0 && y < 6) {
                    EXPECT_EQ(*(line_color.pixels + x * line_color.next), 1.0);
                } else {
                    EXPECT_EQ(*(line_color.pixels + x * line_color.next), 0.0);
                }
            }
            *(line_alpha.pixels + line_alpha.next) = (float)1.0;
        }
    }
    EXPECT_TRUE(ImageIs(access, result, PixelPatch::Method::COLORS, false, false, 1));
}

TEST(PixelAccessLineTest, FloatSingleSurfaceHorz)
{
    auto src = TestCustomSurface<3>(7, 7);
    src.rect(2, 2, 3, 3, {0.0, 1.0, 0.0, 1.0});
    testPixelAccess(*src._d,
                    "... . ."
                    "       "
                    "..888 ."
                    "  888  "
                    "..888 ."
                    "       "
                    "... . .");
}

TEST(PixelAccessLineTest, FloatSingleSurfaceVert)
{
    auto src = TestCustomSurface<3>(7, 7);
    src.rect(2, 2, 3, 3, {0.0, 1.0, 0.0, 1.0});
    testPixelAccess<3, 1, true>(*src._d,
                    ". . . ."
                    ". . . ."
                    ". 888 ."
                    "  888  "
                    ". 888 ."
                    "       "
                    ". . . .");
}

TEST(PixelAccessLineTest, IntSingleSurfaceHorz)
{
    auto src = TestCairoSurface<3, PixelAccessEdgeMode::NO_CHECK, CAIRO_FORMAT_ARGB32>(7, 7);
    src.rect(2, 2, 3, 3, {0.0, 1.0, 0.0, 1.0});
    testPixelAccess(*src._d,
                    "... . ."
                    "       "
                    "..888 ."
                    "  888  "
                    "..888 ."
                    "       "
                    "... . .");
}

TEST(PixelAccessLineTest, IntSingleSurfaceVert)
{
    auto src = TestCairoSurface<3, PixelAccessEdgeMode::NO_CHECK, CAIRO_FORMAT_ARGB32>(7, 7);
    src.rect(2, 2, 3, 3, {0.0, 1.0, 0.0, 1.0});
    testPixelAccess<3, 1, true>(*src._d,
                    ". . . ."
                    ". . . ."
                    ". 888 ."
                    "  888  "
                    ". 888 ."
                    "       "
                    ". . . .");
}

TEST(PixelAccessLineTest, A8SingleSurfaceHorz)
{
    auto src = TestCairoSurface<0>(7, 7);
    src.rect(2, 2, 3, 3, {1.0});
    testPixelAccess<0, 0>(*src._d,
                          "... . ."
                          "       "
                          "..... ."
                          "  ...  "
                          "..... ."
                          "       "
                          "... . .");
}

TEST(PixelAccessLineTest, A8SingleSurfaceVert)
{
    auto src = TestCairoSurface<0>(7, 7);
    src.rect(2, 2, 3, 3, {1.0});
    testPixelAccess<0, 0, true>(*src._d,
                          ". . . ."
                          ". . . ."
                          ". ... ."
                          "  ...  "
                          ". ... ."
                          "       "
                          ". . . .");
}
TEST(PixelAccessLineTest, FloatDoubleSurfaceHorz)
{
    auto src = TestCairoSurface<4>(7, 7);
    src.rect(2, 2, 3, 3, {0.0, 1.0, 0.0, 1.0, 1.0});
    testPixelAccess<4, 1>(*src._d,
                          "... . ."
                          "       "
                          "..nnn ."
                          "  nnn  "
                          "..nnn ."
                          "       "
                          "... . .");
    testPixelAccess<3, 1>(*src._d,
                          "fff f f"
                          "       "
                          "ffnnn f"
                          "  nnn  "
                          "ffnnn f"
                          "       "
                          "fff f f");
}

TEST(PixelAccessLineTest, FloatDoubleSurfaceVert)
{
    auto src = TestCairoSurface<4>(7, 7);
    src.rect(2, 2, 3, 3, {0.0, 1.0, 0.0, 1.0, 1.0});
    testPixelAccess<4, 1, true>(*src._d,
                          ". . . ."
                          ". . . ."
                          ". nnn ."
                          "  nnn  "
                          ". nnn ."
                          "       "
                          ". . . .");
    testPixelAccess<3, 1, true>(*src._d,
                          "f f f f"
                          "f f f f"
                          "f nnn f"
                          "  nnn  "
                          "f nnn f"
                          "       "
                          "f f f f");
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
