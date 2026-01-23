// SPDX-License-Identifier: GPL-2.0-or-later
/***** SHARED TEST TOOLS FOR SURFACE TESTS *******/

#ifndef INKSCAPE_TEST_RENDERER_DRAWING_TESTBASE_H
#define INKSCAPE_TEST_RENDERER_DRAWING_TESTBASE_H

#include <memory>

#include "surface-testbase.h"
#include "renderer/drawing-filters/filter.h"
#include "renderer/drawing-filters/primitive.h"
#include "renderer/drawing-filters/slot.h"

using namespace Inkscape::Renderer;

/**
 * Return slot recorded from a live SVG file producing transform-source.png
 */
DrawingFilter::Slot get_transformed_slot()
{
    auto dopt = DrawingOptions();
    auto iopt = DrawingFilter::Units(SP_FILTER_UNITS_OBJECTBOUNDINGBOX, SP_FILTER_UNITS_USERSPACEONUSE);
    iopt.set_ctm(Geom::Affine(841.81, 841.81, -841.81, 841.81, 0, 0));
    iopt.set_item_bbox(Geom::Rect(0, 0, 0.2, 0.2));
    iopt.set_filter_area(Geom::Rect(-0.048, -0.0024, 0.248, 0.2024));
    iopt.set_render_area(Geom::Rect(-211, -43, 211, 380)); // carea
    iopt.set_resolution(352.388, 243.814);
    iopt.set_automatic_resolution(false);
    iopt.set_paraller(false);
    return DrawingFilter::Slot(dopt, iopt);
}

std::shared_ptr<TestSurface> get_transformed_input()
{
    // Normally these images are drawn in-test, but this one is proving consistancy
    // between the old code and the new code, so it's a recording of the inputs of
    // this section of code to make sure the same inputs produce the same outputs.
    return std::make_shared<TestSurface>(Renderer::Surface(INKSCAPE_TESTS_DIR "/data/renderer/transform-source-16.png"));
}

DrawingFilter::Filter get_transformed_filter()
{
    auto filter = DrawingFilter::Filter();

    SVGLength x;
    SVGLength y;
    SVGLength width;
    SVGLength height;

    x.fromString("-0.23999999", "");
    y.fromString("-0.012000001", "");
    width.fromString("1.48", "");
    height.fromString("1.024", "");

    filter.set_x(x);
    filter.set_y(y);
    filter.set_width(width);
    filter.set_height(height);
    return filter;
}

template <PixelPatch::Method method = PixelPatch::Method::COLORS>
void EXPECT_PRIMITIVE_IS(std::unique_ptr<Renderer::DrawingFilter::Primitive> primitive, std::string result, Geom::OptRect clip = {})
{
    auto image = get_transformed_input();
    auto filter = get_transformed_filter();

    if (primitive) {
        filter.add_primitive(std::move(primitive));
    }

    std::shared_ptr<Surface> background;
    if ( filter.uses_input(DrawingFilter::SLOT_BACKGROUND_IMAGE)
      || filter.uses_input(DrawingFilter::SLOT_BACKGROUND_ALPHA)) {
        background = std::make_shared<TestSurface>(Renderer::Surface(INKSCAPE_TESTS_DIR "/data/renderer/rainbow-source-16.png"));
    }

    auto dopt = Renderer::DrawingOptions();
    dopt.blurquality = PixelFilter::BlurQuality::NORMAL;
    filter.render(
        Geom::Rect(-211, -43, 211, 380),
        Geom::Affine(841.81, 841.81, -841.81, 841.81, 0, 0),
        Geom::Rect(0, 0, 0.2, 0.2),
        image, background, dopt);

    if (image && getenv("DEBUG_PNG")) {
        auto name = primitive ? primitive->name() : "{no-primitive}";
        image->write_to_png("/tmp/pngs/filter-primitive-" + name);
    }

    int scale = 40;
    if (clip) {
        scale *= clip->width() / image->dimensions()[Geom::X];
    }

    EXPECT_IMAGE_IS<method>(*image, result, scale, clip);
}


#endif // INKSCAPE_TEST_RENDERER_DRAWING_TESTBASE_H

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
