// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * feMerge filter effect renderer
 *
 * Copyright (C) 2007-2026 authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "renderer/context.h"
#include "renderer/surface.h"

#include "merge.h"
#include "slot.h"
#include "units.h"

namespace Inkscape::Renderer::DrawingFilter {

Merge::Merge()
    : _input_image({SLOT_NOT_SET}) {}

void Merge::render(Slot &slot) const
{
    if (_input_image.empty()) return;

    Geom::Rect vp = filter_primitive_area(slot.get_item_options());
    slot.set_primitive_area(_output, vp); // Needed for tiling

    if (auto out = slot.get(_input_image[0], _color_space)) {
        auto ct = Context(*out);
        for (auto &slot_n : _input_image) {
            if (auto in = slot.get(slot_n, _color_space)) {
                ct.setSource(*in);
                ct.paint();
            }
        }
        slot.set(_output, out);
    }
}

bool Merge::can_handle_affine(Geom::Affine const &) const
{
    // Merge is a per-pixel primitive and is immutable under transformations
    return true;
}

double Merge::complexity(Geom::Affine const &) const
{
    return 1.02;
}

void Merge::set_input(int slot)
{
    _input_image[0] = slot;
}

void Merge::set_input(int input, int slot)
{
    if (input < 0) return;

    if (_input_image.size() > input) {
        _input_image[input] = slot;
    } else {
        for (int i = _input_image.size(); i < input ; i++) {
            _input_image.emplace_back(SLOT_NOT_SET);
        }
        _input_image.push_back(slot);
    }
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
