// SPDX-License-Identifier: GPL-2.0-or-later

#include <2geom/transforms.h>

#include "renderer/drawing-filters/slot.h"
#include "drawing-testbase.h"
#include "surface-testbase.h"

using namespace Inkscape::Renderer;

TEST(DrawingFilterSlotTest, Construction)
{
    auto slot = DrawingFilter::Slot();
}

TEST(DrawingFilterSlotTest, setGetSlot)
{
    auto slot = DrawingFilter::Slot();
    auto surface = std::make_shared<Surface>(Geom::IntPoint(4, 4));
    EXPECT_EQ(slot.get_slot_count(), 0);
    slot.set(DrawingFilter::SLOT_SOURCE_IMAGE, surface);
    EXPECT_EQ(slot.get_slot_count(), 1);
    EXPECT_EQ(slot.get(DrawingFilter::SLOT_SOURCE_IMAGE)->dimensions(), Geom::IntPoint(4, 4));

    // Last out was always SOURCE_IMAGE by defauly anyway
    ASSERT_EQ(slot.get(), surface);
}

TEST(DrawingFilterSlotTest, getInColorSpace)
{
    auto rgb = Colors::Manager::get().find(Colors::Space::Type::RGB);
    auto cmyk = Colors::Manager::get().find(Colors::Space::Type::CMYK);

    auto slot = DrawingFilter::Slot();
    auto surface = std::make_shared<Surface>(Geom::IntPoint(4, 4), 1.0, rgb);
    slot.set(DrawingFilter::SLOT_SOURCE_IMAGE, surface);

    ASSERT_EQ(surface, slot.get(DrawingFilter::SLOT_SOURCE_IMAGE));
    ASSERT_EQ(surface, slot.get(DrawingFilter::SLOT_SOURCE_IMAGE, rgb));

    auto copy = slot.get(DrawingFilter::SLOT_SOURCE_IMAGE, cmyk);
    ASSERT_NE(surface, copy);
    ASSERT_EQ(copy->getColorSpace(), cmyk);
}

TEST(DrawingFilterSlotTest, getCopyInt)
{
    auto rgb = Colors::Manager::get().find(Colors::Space::Type::RGB);

    auto slot = DrawingFilter::Slot();
    auto rgbint = std::make_shared<TestSurface>(Inkscape::Renderer::Surface({21, 21}, 1, {}));
    rgbint->rect(3,  3,  15, 15,  {0.0, 0.9, 0.0, 0.5});

    slot.set(DrawingFilter::SLOT_SOURCE_IMAGE, rgbint);

    // All tests should result in a copy, so the same alpha result
    auto result = "       "
                  " ----- "
                  " ----- "
                  " ----- "
                  " ----- "
                  " ----- "
                  "       ";

    auto copy = slot.get_copy(DrawingFilter::SLOT_SOURCE_IMAGE);
    ASSERT_NE(rgbint, copy);
    ASSERT_FALSE(copy->getColorSpace());
    ASSERT_EQ(copy->format(), 0);
    EXPECT_IMAGE_IS<PixelPatch::Method::ALPHA>(*copy, result);

    auto rgbfloat = slot.get_copy(DrawingFilter::SLOT_SOURCE_IMAGE, rgb);
    ASSERT_NE(rgbint, rgbfloat);
    ASSERT_EQ(rgbfloat->getColorSpace(), rgb);
    ASSERT_EQ(rgbfloat->format(), 7);
    EXPECT_IMAGE_IS<PixelPatch::Method::ALPHA>(*rgbfloat, result);

    slot.set(DrawingFilter::SLOT_SOURCE_IMAGE, rgbfloat);
    auto copy2 = slot.get_copy(DrawingFilter::SLOT_SOURCE_IMAGE, {});
    ASSERT_NE(rgbint, copy2);
    ASSERT_NE(rgbfloat, copy2);
    ASSERT_EQ(copy2->format(), 0);
    ASSERT_FALSE(copy2->getColorSpace());
    EXPECT_IMAGE_IS<PixelPatch::Method::ALPHA>(*copy2, result);
}

TEST(DrawingFilterSlotTest, getCopyFloat)
{
    auto rgb = Colors::Manager::get().find(Colors::Space::Type::RGB);
    auto cmyk = Colors::Manager::get().find(Colors::Space::Type::CMYK);

    auto slot = DrawingFilter::Slot();
    auto surface = std::make_shared<TestSurface>(Inkscape::Renderer::Surface({21, 21}, 1, rgb));
    surface->rect(3,  3,  15, 15,  {0.0, 0.9, 0.0, 0.5});

    slot.set(DrawingFilter::SLOT_SOURCE_IMAGE, surface);

    // All tests should result in a copy, so the same alpha result
    auto result = "       "
                  " ----- "
                  " ----- "
                  " ----- "
                  " ----- "
                  " ----- "
                  "       ";

    auto copy = slot.get_copy(DrawingFilter::SLOT_SOURCE_IMAGE);
    ASSERT_NE(surface, copy);
    ASSERT_EQ(copy->getColorSpace(), surface->getColorSpace());
    EXPECT_IMAGE_IS<PixelPatch::Method::ALPHA>(*copy, result);

    auto copy_rgb = slot.get_copy(DrawingFilter::SLOT_SOURCE_IMAGE, rgb);
    ASSERT_NE(surface, copy_rgb);
    ASSERT_EQ(copy_rgb->getColorSpace(), rgb);
    EXPECT_IMAGE_IS<PixelPatch::Method::ALPHA>(*copy_rgb, result);

    auto copy_cmyk = slot.get_copy(DrawingFilter::SLOT_SOURCE_IMAGE, cmyk);
    ASSERT_NE(surface, copy_cmyk);
    ASSERT_EQ(copy_cmyk->getColorSpace(), cmyk);
    EXPECT_IMAGE_IS<PixelPatch::Method::ALPHA>(*copy_cmyk, result);
}

TEST(DrawingFilterSlotTest, setSetLast)
{
    auto slot = DrawingFilter::Slot();
    auto surface1 = std::make_shared<Surface>(Geom::IntPoint(4, 4));
    auto surface2 = std::make_shared<Surface>(Geom::IntPoint(4, 4));

    slot.set(DrawingFilter::SLOT_SOURCE_IMAGE, surface1);
    EXPECT_EQ(slot.get(), surface1);
    slot.set(DrawingFilter::SLOT_BACKGROUND_IMAGE, surface2);
    EXPECT_EQ(slot.get(), surface1);
    EXPECT_EQ(slot.get(DrawingFilter::SLOT_BACKGROUND_IMAGE), surface2);
    slot.set(2, surface2);
    EXPECT_EQ(slot.get(), surface2);
    EXPECT_EQ(slot.get(2), surface2);

    slot.set(2, surface1);
    EXPECT_EQ(slot.get(2), surface1);
}

TEST(DrawingFilterSlotTest, setAlphaSlot)
{
    auto slot = DrawingFilter::Slot();

    auto rgb = Colors::Manager::get().find(Colors::Space::Type::RGB);
    auto src = std::make_shared<TestSurface>(Inkscape::Renderer::Surface({21, 21}, 1, rgb));
    src->rect(0,  3,  21, 3,  {0.0, 0.9, 0.0, 0.5});
    src->rect(15, 0,  3,  21, {0.5, 0.5, 0.5, 0.5});
    src->rect(0,  15, 21, 3,  {0.9, 0.0, 0.0, 0.5});
    src->rect(3,  0,  3,  21, {0.0, 0.0, 0.9, 0.5});
    slot.set(DrawingFilter::SLOT_SOURCE_IMAGE, src);

    EXPECT_EQ(slot.get_slot_count(), 1);
    slot.set_alpha(DrawingFilter::SLOT_SOURCE_IMAGE, DrawingFilter::SLOT_SOURCE_ALPHA);
    EXPECT_EQ(slot.get_slot_count(), 2);

    auto dest = slot.get(DrawingFilter::SLOT_SOURCE_ALPHA);

    ASSERT_TRUE(dest);
    EXPECT_IMAGE_IS<PixelPatch::Method::ALPHA>(*dest,
                    " -   - "
                    "-O---O-"
                    " -   - "
                    " -   - "
                    " -   - "
                    "-O---O-"
                    " -   - ");

    // set_alpha doesn't change the last_out from src
    ASSERT_EQ(slot.get(), src);
}

TEST(DrawingFilterSlotTest, slotOptions)
{
    auto dopt = DrawingOptions(2.0);
    auto iopt = DrawingFilter::Units(SP_FILTER_UNITS_OBJECTBOUNDINGBOX, SP_FILTER_UNITS_USERSPACEONUSE);
    auto slot = DrawingFilter::Slot(dopt, iopt);
    ASSERT_EQ(slot.get_drawing_options().device_scale, 2.0);
    ASSERT_EQ(slot.get_item_options().get_filter_units(), SP_FILTER_UNITS_OBJECTBOUNDINGBOX);
}

TEST(DrawingFilterSlotTest, renderTransform)
{
    auto slot = get_transformed_slot();
    slot.set(DrawingFilter::SLOT_SOURCE_IMAGE, get_transformed_input());
    auto img1 = slot.get(DrawingFilter::SLOT_SOURCE_IMAGE);
    EXPECT_IMAGE_IS<PixelPatch::Method::ALPHA>(*img1,
                    "        "
                    "  ::::  "
                    " :$$$$: "
                    " :$$$$: "
                    " :$$$$: "
                    " :$$$$: "
                    "  ::::  "
                    "        "
                  , 50, {100, 100, 540, 540});

    auto img2 = slot.get_result();
    EXPECT_IMAGE_IS<PixelPatch::Method::ALPHA>(*img2,
                    "        "
                    "   :*   "
                    "  :$$*  "
                    " :$$$$* "
                    " *$$$$$."
                    "  *$$$. "
                    "   *$.  "
                    "    .   "
                  , 50);
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
