// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * SVG filters rendering
 *
 * Author:
 *   Niko Kiirala <niko@kiirala.com>
 *   Tavmjong Bah <tavmjong@free.fr> (primitive subregion)
 *
 * Copyright (C) 2006 Niko Kiirala
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "primitive.h"

//#include "style.h"
#include "slot.h"
#include "enums.h"
#include "units.h"
#include "object/sp-filter-units.h"
#include "svg/svg-length.h"

#include "colors/manager.h"

namespace Inkscape::Renderer::DrawingFilter {

Primitive::Primitive()
{
    _input = SLOT_NOT_SET;
    _output = SLOT_NOT_SET;

    // Primitive subregion, should default to the union of all subregions of referenced nodes
    // (i.e. other filter primitives except feTile).  If no referenced nodes, defaults to filter
    // region expressed in percent. At the moment, we do not check referenced nodes.

    // We must keep track if a value is set or not, if not set then the region defaults to 0%, 0%,
    // 100%, 100% ("x", "y", "width", "height") of the -> filter <- region. If set, then
    // percentages are in terms of bounding box or viewbox, depending on value of "primitiveUnits".

    // NB: SVGLength.set takes prescaled percent values: 1 means 100%
    _subregion_x.unset(SVGLength::PERCENT, 0, 0);
    _subregion_y.unset(SVGLength::PERCENT, 0, 0);
    _subregion_width.unset(SVGLength::PERCENT, 1, 0);
    _subregion_height.unset(SVGLength::PERCENT, 1, 0);

    // Default interpolation linearRGB.
    setInterpolationSpace({});
}

Primitive::~Primitive() = default;

void Primitive::render(Slot &slot) const
{
    if (_output != _input) {
        // passthrough
        slot.set(_output, slot.get(_input));
    }
}

void Primitive::set_input(int slot)
{
    set_input(0, slot);
}

void Primitive::set_input(int input, int slot)
{
    if (input == 0) _input = slot;
}

void Primitive::set_output(int slot)
{
    if (slot >= 0) _output = slot;
}

// We need to copy reference even if unset as we need to know if
// someone has unset a value.
void Primitive::set_x(SVGLength const &length)
{
    _subregion_x = length;
}

void Primitive::set_y(SVGLength const &length)
{
    _subregion_y = length;
}

void Primitive::set_width(SVGLength const &length)
{
    _subregion_width = length;
}

void Primitive::set_height(SVGLength const &length)
{
    _subregion_height = length;
}

void Primitive::set_subregion(SVGLength const &x, SVGLength const &y,
                              SVGLength const &width, SVGLength const &height)
{
    _subregion_x = x;
    _subregion_y = y;
    _subregion_width = width;
    _subregion_height = height;
}

Geom::Rect Primitive::filter_primitive_area(Units const &units) const
{
    Geom::OptRect const fa_opt = units.get_filter_area();
    if (!fa_opt) {
        std::cerr << "Primitive::filter_primitive_area: filter area undefined." << std::endl;
        return Geom::Rect::from_xywh(0, 0, 0, 0);
    }
    Geom::Rect fa = *fa_opt;

    // x, y, width, and height are independently defined (i.e. one can be defined, by default, to
    // the filter area (via default value ) while another is defined relative to the bounding
    // box). It is better to keep track of them separately and then compose the Rect at the end.
    double x = 0;
    double y = 0;
    double width = 0;
    double height = 0;

    // If subregion not set, by special case use filter region.
    if (!_subregion_x._set)      x = fa.left();
    if (!_subregion_y._set)      y = fa.top();
    if (!_subregion_width._set)  width  = fa.width();
    if (!_subregion_height._set) height = fa.height();

    if (units.get_primitive_units() == SP_FILTER_UNITS_OBJECTBOUNDINGBOX) {

        Geom::OptRect const bb_opt = units.get_item_bbox();
        if (!bb_opt) {
            std::cerr << "Primitive::filter_primitive_area: bounding box undefined and 'primitiveUnits' is 'objectBoundingBox'." << std::endl;
            return Geom::Rect();
        }
        Geom::Rect bb = *bb_opt;

        // Update computed values for ex, em, %.
        // For %, assumes primitive unit is objectBoundingBox.
        // TODO: fetch somehow the object ex and em lengths; 12, 6 are just dummy values.
        auto const len_x = bb.width();
        auto const len_y = bb.height();

        auto compute = [] (SVGLength length, double scale) {
            length.update(12, 6, scale);
            return length.computed;
        };

        auto const subregion_x_computed = compute(_subregion_x, len_x);
        auto const subregion_y_computed = compute(_subregion_y, len_y);
        auto const subregion_w_computed = compute(_subregion_width, len_x);
        auto const subregion_h_computed = compute(_subregion_height, len_y);

        // Values are in terms of fraction of bounding box.
        if (_subregion_x._set      && _subregion_x.unit      != SVGLength::PERCENT)      x = bb.left() + bb.width() * _subregion_x.value;
        if (_subregion_y._set      && _subregion_y.unit      != SVGLength::PERCENT)      y = bb.top() + bb.height() * _subregion_y.value;
        if (_subregion_width._set  && _subregion_width.unit  != SVGLength::PERCENT)  width = bb.width()  * _subregion_width.value;
        if (_subregion_height._set && _subregion_height.unit != SVGLength::PERCENT) height = bb.height() * _subregion_height.value;
        // Values are in terms of percent                                                         
        if (_subregion_x._set      && _subregion_x.unit      == SVGLength::PERCENT)      x = bb.left() + subregion_x_computed;
        if (_subregion_y._set      && _subregion_y.unit      == SVGLength::PERCENT)      y = bb.top() + subregion_y_computed;
        if (_subregion_width._set  && _subregion_width.unit  == SVGLength::PERCENT)  width = subregion_w_computed;
        if (_subregion_height._set && _subregion_height.unit == SVGLength::PERCENT) height = subregion_h_computed;
    } else {
        // Values are in terms of user space coordinates or percent of viewport (already calculated in sp-filter-primitive.cpp).
        if (_subregion_x._set     )      x = _subregion_x.computed;
        if (_subregion_y._set     )      y = _subregion_y.computed;
        if (_subregion_width._set )  width = _subregion_width.computed;
        if (_subregion_height._set) height = _subregion_height.computed;
    }

    return Geom::Rect::from_xywh(x, y, width, height);
}

void Primitive::setInterpolationSpace(std::shared_ptr<Colors::Space::AnySpace> const &space)
{
    // Automatic becomes "sRGB INTEGER" in the surface drawing API, we don't want this
    // So we assign linearRGB directly which will also put the surface into float mode.
    _color_space = space ? space : Colors::Manager::get().find(Colors::Space::Type::linearRGB);
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
