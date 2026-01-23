// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * A container class for filter slots. Allows for simple getting and
 * setting images in filter slots without having to bother with
 * table indexes and such.
 *
 * Author:
 *   Niko Kiirala <niko@kiirala.com>
 *
 * Copyright (C) 2006,2007 Niko Kiirala
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <cassert>
#include <cstring>

#include "slot.h"

#include <2geom/transforms.h>
#include "renderer/surface.h"
#include "renderer/context.h"
#include "colors/manager.h"
#include "colors/spaces/base.h"

namespace Inkscape::Renderer::DrawingFilter {

Slot::Slot(DrawingOptions const &draw_opt, Units const &item_opt)
    : _last_out(SLOT_SOURCE_IMAGE)
    , _draw_opt(draw_opt)
    , _item_opt(item_opt)
{}

Slot::Slot()
    : _last_out(SLOT_SOURCE_IMAGE)
{}

std::shared_ptr<Surface> Slot::get(int slot) const
{
    if (slot == SLOT_NOT_SET) {
        slot = _last_out;
    }

    auto found = _slots.find(slot);
    if (found == _slots.end()) {
        return {};
    }
    return found->second;
}

std::shared_ptr<Surface> Slot::get(int slot, std::shared_ptr<Colors::Space::AnySpace> const &space) const
{
    auto surface = get(slot);

    if (!surface || space == surface->getColorSpace()) {
        return surface;
    }

    // Return a version of the surface in the new color space instead
    return surface->convertedToColorSpace(space);
}

std::shared_ptr<Surface> Slot::get_copy(int slot) const
{
    if (auto surface = get(slot)) {
        auto copy = surface->similar();
        auto context = Context(*copy);
        context.setSource(*surface);
        context.setOperator(Cairo::Context::Operator::SOURCE);
        context.paint();
        return copy;
    }
    return {};
}

std::shared_ptr<Surface> Slot::get_copy(int slot, std::shared_ptr<Colors::Space::AnySpace> const &space) const
{
    auto surface = get(slot);
    if (!surface) {
        return {};
    }
    if (surface->getColorSpace() != space) {
        // This process does a copy anyway
        return get(slot, space);
    }
    return get_copy(slot);
}

void Slot::set_alpha(int slot_from, int slot_to)
{
    static auto alpha = Colors::Manager::get().find(Colors::Space::Type::Alpha);
    if (auto color_surface = get(slot_from)) {
        // Make an alpha surface and copy the alpha into it from the source slot
        set(slot_to, color_surface->convertedToColorSpace(alpha));
    } else {
        g_error("Couldn't convert a filter image source(%d) into an alpha(%d), source image missing.", slot_from, slot_to);
    }
}

void Slot::set(int slot, std::shared_ptr<Surface> surface)
{
    if (slot == SLOT_NOT_SET)
        slot = SLOT_UNNAMED;

    // This crufty bit of code *untransforms* the rendered source or background
    // so the filter can be applied to the original orientation before being
    // re-transformed when painted back.
    auto trans = _item_opt.get_matrix_item2filter();
    if ((slot == SLOT_SOURCE_IMAGE || slot == SLOT_BACKGROUND_IMAGE) && trans) {
        auto sbox = _item_opt.get_slot_box();
        auto tsg = surface->similar(sbox->dimensions().ceil());
        auto context = Context(*tsg);
        context.transform(*trans);
        context.setSource(*surface);
        context.setOperator(Cairo::Context::Operator::SOURCE);
        context.paint();

        // Save the input surface for later as it's the right size and format for the result
        if (slot == SLOT_SOURCE_IMAGE) {
            _slots[SLOT_RESULT] = std::move(surface);
        }
        surface = tsg;
    }

    auto found = _slots.find(slot);
    if (found == _slots.end() || found->second != surface) {
        _slots[slot] = std::move(surface);
    }

    // Setting internal slots doesn't change the last_out
    if (slot > SLOT_NOT_SET) {
        _last_out = slot;
    }
}

std::shared_ptr<Surface> Slot::get_result(int slot) const
{
    auto result = get(slot, get(SLOT_SOURCE_IMAGE)->getColorSpace());

    // This is the mirror of "crufty bit of code" above to undo the
    // transformation which was added to the source graphic.
    if (auto trans = _item_opt.get_matrix_item2filter()) {
        auto output = get(SLOT_RESULT); // Reuse from set(...)
        auto context = Context(*output);
        context.transform(trans->inverse());
        context.setSource(*result);
        context.setOperator(Cairo::Context::Operator::SOURCE);
        context.paint();
        return output;
    }

    return result;
}

void Slot::set_primitive_area(int slot, Geom::Rect &area)
{
    if (slot == SLOT_NOT_SET)
        slot = SLOT_UNNAMED;

    _primitiveAreas[slot] = area;
}

Geom::Rect Slot::get_primitive_area(int slot) const
{
    if (slot == SLOT_NOT_SET)
        slot = _last_out;

    auto s = _primitiveAreas.find(slot);

    if (s == _primitiveAreas.end()) {
        //return *_units.get_filter_area();
    }
    return s->second;
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
