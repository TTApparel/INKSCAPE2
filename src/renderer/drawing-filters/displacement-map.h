// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * feDisplacementMap filter primitive renderer
 *
 * Authors:
 *   Felipe Corrêa da Silva Sanches <juca@members.fsf.org>
 *
 * Copyright (C) 2007 authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_INKSCAPE_RENDERER_DRAWING_FILTER_DISPLACEMENT_MAP_H
#define SEEN_INKSCAPE_RENDERER_DRAWING_FILTER_DISPLACEMENT_MAP_H

#include "primitive.h"

namespace Inkscape::Renderer::DrawingFilter {

class DisplacementMap : public Primitive
{
public:
    void render(Slot &slot) const override;
    void area_enlarge(Geom::IntRect &area, Geom::Affine const &trans) const override;
    double complexity(Geom::Affine const &ctm) const override;

    void set_input(int slot) override;
    void set_input(int input, int slot) override;
    void set_scale(double s);
    void set_channels(int channelX, int channelY);

    Glib::ustring name() const override { return Glib::ustring("Displacement Map"); }

    bool uses_input(int slot)  const override { return _input2 == slot || Primitive::uses_input(slot); }

private:
    double scale;
    int _input2;
    unsigned Xchannel, Ychannel;
};

} // namespace Inkscape::Renderer::DrawingFilter

#endif // SEEN_INKSCAPE_RENDERER_DRAWING_FILTER_DISPLACEMENT_MAP_H
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
