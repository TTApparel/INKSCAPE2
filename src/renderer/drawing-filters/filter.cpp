// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * SVG filters rendering
 *
 * Author:
 *   Niko Kiirala <niko@kiirala.com>
 *
 * Copyright (C) 2006-2008 Niko Kiirala
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "filter.h"

#include <cmath>
#include <cstring>
#include <string>
#include <2geom/affine.h>
#include <2geom/rect.h>

#include "primitive.h"
#include "slot.h"
#include "renderer/drawing/drawing-options.h"
#include "renderer/context.h"

namespace Inkscape::Renderer::DrawingFilter {

using Geom::X;
using Geom::Y;

Filter::Filter()
{
    _common_init();
}

Filter::Filter(int n)
{
    if (n > 0) primitives.reserve(n);
    _common_init();
}

void Filter::_common_init()
{
    _slot_count = 1;
    // Having "not set" here as value means the output of last filter
    // primitive will be used as output of this filter
    _output_slot = SLOT_NOT_SET;

    // These are the default values for filter region,
    // as specified in SVG standard
    // NB: SVGLength.set takes prescaled percent values: -.10 means -10%
    _region_x.set(SVGLength::PERCENT, -.10, 0);
    _region_y.set(SVGLength::PERCENT, -.10, 0);
    _region_width.set(SVGLength::PERCENT, 1.20, 0);
    _region_height.set(SVGLength::PERCENT, 1.20, 0);

    // Filter resolution, negative value here stands for "automatic"
    _x_pixels = -1.0;
    _y_pixels = -1.0;

    _filter_units = SP_FILTER_UNITS_OBJECTBOUNDINGBOX;
    _primitive_units = SP_FILTER_UNITS_USERSPACEONUSE;
}

void Filter::update()
{
    for (auto &p : primitives) {
        p->update();
    }
}

int Filter::render(Geom::Rect const &carea, Geom::Affine const &trans, Geom::OptRect const &item_bbox, std::shared_ptr<Surface> image, std::shared_ptr<Surface> const background, DrawingOptions const &draw_opt) const
{
    if (primitives.empty()) {
        // when no primitives are defined, clear source graphic
        auto graphic = Context(*image);
        graphic.resetSource(0.0);
        graphic.setOperator(Cairo::Context::Operator::SOURCE);
        graphic.paint();
        graphic.setOperator(Cairo::Context::Operator::OVER);
        return 1;
    }

    Geom::OptRect filter_area = filter_effect_area(item_bbox);
    if (!filter_area) return 1;

    Units item_opt(_filter_units, _primitive_units);
    item_opt.set_ctm(trans);
    item_opt.set_item_bbox(item_bbox);
    item_opt.set_filter_area(*filter_area);
    item_opt.set_render_area(carea);

    auto resolution = filter_resolution(*filter_area, trans, draw_opt.filterquality);
    if (!(resolution.first > 0 && resolution.second > 0)) {
        // zero resolution - clear source graphic and return
        auto graphic = Context(*image);
        graphic.resetSource(0.0);
        graphic.setOperator(Cairo::Context::Operator::SOURCE);
        graphic.paint();
        graphic.setOperator(Cairo::Context::Operator::OVER);
        return 1;
    }

    item_opt.set_resolution(resolution.first, resolution.second);
    item_opt.set_automatic_resolution(_x_pixels <= 0);

    item_opt.set_paraller(false);
    Geom::Affine pbtrans = item_opt.get_matrix_display2pb();
    for (auto &i : primitives) {
        if (!i->can_handle_affine(pbtrans)) {
            item_opt.set_paraller(true);
            break;
        }
    }

    auto slot = Slot(draw_opt, item_opt);

    // We could check if source is needed, but it's used too pervasively for dimensions
    slot.set(SLOT_SOURCE_IMAGE, image);
    if (uses_input(SLOT_SOURCE_ALPHA)) {
        slot.set_alpha(SLOT_SOURCE_IMAGE, SLOT_SOURCE_ALPHA);
    }
    // Add external sources to the filter slots
    if (background) {
        slot.set(SLOT_BACKGROUND_IMAGE, background);
        if (uses_input(SLOT_BACKGROUND_ALPHA)) {
            slot.set_alpha(SLOT_BACKGROUND_IMAGE, SLOT_BACKGROUND_ALPHA);
        }
    }

    for (auto &primitive : primitives) {
        primitive->render(slot);
    }

    auto result = slot.get_result(_output_slot);

    // Sometimes the filter stack will just reuse the image on return, consider moving this to slot
    if (result && image && result != image) {
        auto graphic = Context(*image);
        graphic.setSource(*result);
        graphic.setOperator(Cairo::Context::Operator::SOURCE);
        graphic.paint();
        graphic.setOperator(Cairo::Context::Operator::OVER);
    }

    return 0;
}

void Filter::add_primitive(std::unique_ptr<Primitive> primitive)
{
    primitives.emplace_back(std::move(primitive));
}

void Filter::set_filter_units(SPFilterUnits unit)
{
    _filter_units = unit;
}

void Filter::set_primitive_units(SPFilterUnits unit)
{
    _primitive_units = unit;
}

void Filter::area_enlarge(Geom::IntRect &bbox, Geom::Affine const &item_ctm) const
{
    for (auto const &i : primitives) {
        if (i) i->area_enlarge(bbox, item_ctm);
    }

/*
  TODO: something. See images at the bottom of filters.svg with medium-low
  filtering quality.

    FilterQuality const filterquality = ...

    if (_x_pixels <= 0 && (filterquality == Quality::BEST ||
                           filterquality == Quality::BETTER)) {
        return;
    }

    Geom::Rect item_bbox;
    Geom::OptRect maybe_bbox = item->itemBounds();
    if (maybe_bbox.empty()) {
        // Code below needs a bounding box
        return;
    }
    item_bbox = *maybe_bbox;

    std::pair<double,double> res_low
        = filter_resolution(item_bbox, item->ctm(), filterquality);
    //std::pair<double,double> res_full
    //    = filter_resolution(item_bbox, item->ctm(), Quality::BEST);
    double pixels_per_block = fmax(item_bbox.width() / res_low.first,
                                   item_bbox.height() / res_low.second);
    bbox.x0 -= (int)pixels_per_block;
    bbox.x1 += (int)pixels_per_block;
    bbox.y0 -= (int)pixels_per_block;
    bbox.y1 += (int)pixels_per_block;
*/
}

Geom::OptRect Filter::filter_effect_area(Geom::OptRect const &bbox) const
{
    Geom::Point minp, maxp;

    if (_filter_units == SP_FILTER_UNITS_OBJECTBOUNDINGBOX) {
        double len_x = bbox ? bbox->width() : 0;
        double len_y = bbox ? bbox->height() : 0;
        /* TODO: fetch somehow the object ex and em lengths */

        // Update for em, ex, and % values
        auto compute = [] (SVGLength length, double scale) {
            length.update(12, 6, scale);
            return length.computed;
        };
        auto const region_x_computed = compute(_region_x, len_x);
        auto const region_y_computed = compute(_region_y, len_y);
        auto const region_w_computed = compute(_region_width, len_x);
        auto const region_h_computed = compute(_region_height, len_y);;

        if (!bbox) return Geom::OptRect();

        if (_region_x.unit == SVGLength::PERCENT) {
            minp[X] = bbox->left() + region_x_computed;
        } else {
            minp[X] = bbox->left() + region_x_computed * len_x;
        }
        if (_region_width.unit == SVGLength::PERCENT) {
            maxp[X] = minp[X] + region_w_computed;
        } else {
            maxp[X] = minp[X] + region_w_computed * len_x;
        }

        if (_region_y.unit == SVGLength::PERCENT) {
            minp[Y] = bbox->top() + region_y_computed;
        } else {
            minp[Y] = bbox->top() + region_y_computed * len_y;
        }
        if (_region_height.unit == SVGLength::PERCENT) {
            maxp[Y] = minp[Y] + region_h_computed;
        } else {
            maxp[Y] = minp[Y] + region_h_computed * len_y;
        }
    } else if (_filter_units == SP_FILTER_UNITS_USERSPACEONUSE) {
        // Region already set in sp-filter.cpp
        minp[X] = _region_x.computed;
        maxp[X] = minp[X] + _region_width.computed;
        minp[Y] = _region_y.computed;
        maxp[Y] = minp[Y] + _region_height.computed;
    } else {
        g_warning("Error in Inkscape::Filters::Filter::filter_effect_area: unrecognized value of _filter_units");
    }

    Geom::OptRect area(minp, maxp);
    // std::cout << "Filter::filter_effect_area: area: " << *area << std::endl;
    return area;
}

double Filter::complexity(Geom::Affine const &ctm) const
{
    double factor = 1.0;
    for (auto &i : primitives) {
        if (i) {
            double f = i->complexity(ctm);
            factor += f - 1.0;
        }
    }
    return factor;
}

bool Filter::uses_input(int slot) const
{
    for (auto &i : primitives) {
        if (i && i->uses_input(slot)) {
            return true;
        }
    }
    return false;
}

void Filter::clear_primitives()
{
    primitives.clear();
}

void Filter::set_x(SVGLength const &length)
{
  if (length._set)
      _region_x = length;
}

void Filter::set_y(SVGLength const &length)
{
  if (length._set)
      _region_y = length;
}

void Filter::set_width(SVGLength const &length)
{
  if (length._set)
      _region_width = length;
}

void Filter::set_height(SVGLength const &length)
{
  if (length._set)
      _region_height = length;
}

void Filter::set_resolution(double pixels)
{
    if (pixels > 0) {
        _x_pixels = pixels;
        _y_pixels = pixels;
    }
}

void Filter::set_resolution(double x_pixels, double y_pixels)
{
    if (x_pixels >= 0 && y_pixels >= 0) {
        _x_pixels = x_pixels;
        _y_pixels = y_pixels;
    }
}

void Filter::reset_resolution()
{
    _x_pixels = -1;
    _y_pixels = -1;
}

int Filter::_resolution_limit(Quality quality)
{
    switch (quality) {
        case Quality::WORST:
            return 32;
        case Quality::WORSE:
            return 64;
        case Quality::NORMAL:
            return 256;
        case Quality::BETTER:
            return 1024;
        case Quality::BEST:
        default:
            return -1;
    }
}

std::pair<double, double> Filter::filter_resolution(Geom::Rect const &area, Geom::Affine const &trans, Quality filterquality) const
{
    std::pair<double, double> resolution;
    if (_x_pixels > 0) {
        double y_len;
        if (_y_pixels > 0) {
            y_len = _y_pixels;
        } else {
            y_len = (_x_pixels * (area.max()[Y] - area.min()[Y])) / (area.max()[X] - area.min()[X]);
        }
        resolution.first = _x_pixels;
        resolution.second = y_len;
    } else {
        auto origo = area.min() * trans;
        auto max_i = Geom::Point(area.max()[X], area.min()[Y]) * trans;
        auto max_j = Geom::Point(area.min()[X], area.max()[Y]) * trans;
        double i_len = (origo - max_i).length();
        double j_len = (origo - max_j).length();
        int limit = _resolution_limit(filterquality);
        if (limit > 0 && (i_len > limit || j_len > limit)) {
            double aspect_ratio = i_len / j_len;
            if (i_len > j_len) {
                i_len = limit;
                j_len = i_len / aspect_ratio;
            }
            else {
                j_len = limit;
                i_len = j_len * aspect_ratio;
            }
        }
        resolution.first = i_len;
        resolution.second = j_len;
    }
    return resolution;
}

} // namespace Inkscape::Renderer::DrawingFilter

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
