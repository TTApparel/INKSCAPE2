// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * feFlood filter primitive renderer
 *
 * Authors:
 *   Felipe Corrêa da Silva Sanches <juca@members.fsf.org>
 *
 * Copyright (C) 2007 authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_INKSCAPE_RENDERER_DRAWING_FILTER_FLOOD_H
#define SEEN_INKSCAPE_RENDERER_DRAWING_FILTER_FLOOD_H

#include <optional>
#include "colors/color.h"

#include "primitive.h"

namespace Inkscape::Renderer::DrawingFilter {

class Slot;

class Flood : public Primitive
{
public:
    Flood();
    ~Flood() override = default;

    void render(Slot &slot) const override;
    bool can_handle_affine(Geom::Affine const &) const override;
    double complexity(Geom::Affine const &ctm) const override;
    
    void set_color(Colors::Color const &c);

    Glib::ustring name() const override { return Glib::ustring("Flood"); }

    bool uses_input(int slot)  const override { return false; }

private:
    Colors::Color color;
};

} // namespace Inkscape::Renderer::DrawingFilter

#endif // SEEN_INKSCAPE_RENDERER_DRAWING_FILTER_FLOOD_H
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
