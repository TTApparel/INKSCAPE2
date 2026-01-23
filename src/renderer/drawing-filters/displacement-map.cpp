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

#include "displacement-map.h"
#include "slot.h"

#include "renderer/context.h"
#include "renderer/pixel-filters/displacement-map.h"
#include "renderer/surface.h"

namespace Inkscape::Renderer::DrawingFilter {

void DisplacementMap::render(Slot &slot) const
{
    auto texture = slot.get(_input, _color_space);
    auto map = slot.get(_input2, _color_space);
    if (!map || !texture) {
        g_warning("Displacement Map requires two inputs, ignoring.");
        return;
    }

    auto dest = texture->similar();
    Geom::Affine trans = slot.get_item_options().get_matrix_primitiveunits2pb();
    int device_scale = slot.get_drawing_options().device_scale;
    double scalex = scale * trans.expansionX() * device_scale;
    double scaley = scale * trans.expansionY() * device_scale;

    dest->run_pixel_filter<PixelAccessEdgeMode::ZERO>(PixelFilter::DisplacementMap(Xchannel, Ychannel, scalex, scaley), *texture, *map);
    slot.set(_output, dest);
}

void DisplacementMap::set_input(int slot)
{
    _input = slot;
}

void DisplacementMap::set_scale(double s)
{
    scale = s;
}

void DisplacementMap::set_input(int input, int slot)
{
    if (input == 0) _input = slot;
    if (input == 1) _input2 = slot;
}

void DisplacementMap::set_channels(int channelX, int channelY)
{
    Xchannel = channelX;
    Ychannel = channelY;
}

void DisplacementMap::area_enlarge(Geom::IntRect &area, Geom::Affine const &trans) const
{
    //I assume scale is in user coordinates (?!?)
    //FIXME: trans should be multiplied by some primitiveunits2user, shouldn't it?
    
    double scalex = scale / 2. * (std::fabs(trans[0]) + std::fabs(trans[1]));
    double scaley = scale / 2. * (std::fabs(trans[2]) + std::fabs(trans[3]));

    //FIXME: no +2 should be there!... (noticeable only for big scales at big zoom factor)
    area.expandBy(scalex + 2, scaley + 2);
}

double DisplacementMap::complexity(Geom::Affine const &) const
{
    return 3.0;
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
