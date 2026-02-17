// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * feTile filter primitive renderer
 *
 * Authors:
 *   Felipe Corrêa da Silva Sanches <juca@members.fsf.org>
 *
 * Copyright (C) 2007 authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "slot.h"
#include "tile.h"

#include "renderer/context.h"
#include "renderer/surface.h"

namespace Inkscape::Renderer::DrawingFilter {

void Tile::render(Slot &slot) const
{
    // This input source contains only the "rendering" tile.
    auto in = slot.get(_input, _color_space);

    // This is the feTile source area as determined by the input primitive area (see SVG spec).
    Geom::Rect tile_area = slot.get_primitive_area(_input);

    if (tile_area.width() == 0.0 || tile_area.height() == 0.0) {
        std::cerr << "FileTile::render: tile has zero width or height" << std::endl;
        slot.set(_output, in);
        return;

    }

    auto out = in->similar();

    // The rectangle of the "rendering" tile.
    Geom::Rect sa = *slot.get_item_options().get_slot_box();
    Geom::Affine trans = slot.get_item_options().get_matrix_user2pb();

    // Create feTile tile ----------------

    // Get tile area in pixbuf units (tile transformed).
    Geom::Rect tt = tile_area * trans;
        
    // Shift between "rendering" tile and feTile tile
    Geom::Point shift = sa.min() - tt.min(); 

    // Create feTile tile surface
    auto tile = in->similar(Geom::IntPoint(tt.width(), tt.height()));

    {
        auto ct_tile = Context(*tile);
        ct_tile.setSource(*in, shift[Geom::X], shift[Geom::Y]);
        ct_tile.paint();
    }

        // Paint tiles ------------------
        
    // Determine number of feTile rows and columns
    Geom::Rect pr = filter_primitive_area(slot.get_item_options());
    int tile_cols = std::ceil(pr.width()  / tile_area.width());
    int tile_rows = std::ceil(pr.height() / tile_area.height());

    auto ct = Context(*out);
    // Do tiling (TO DO: restrict to slot area.)
    for (int col = 0; col < tile_cols; ++col) {
        for (int row = 0; row < tile_rows; ++row) {
            Geom::Point offset(col * tile_area.width(), row * tile_area.height());
            offset *= trans;
            offset[Geom::X] -= trans[4];
            offset[Geom::Y] -= trans[5];
    
            ct.setSource(*tile, offset[Geom::X], offset[Geom::Y]);
            ct.paint();
        }
    }
    slot.set(_output, out);
}

void Tile::area_enlarge(Geom::IntRect &area, Geom::Affine const &trans) const
{
    // Set to very large rectangle so we get tile source. It will be clipped later.

    // Note, setting to infinite using Geom::IntRect::infinite() causes overflow/underflow problems.
    Geom::IntCoord max = std::numeric_limits<Geom::IntCoord>::max() / 4;
    area = Geom::IntRect(-max, -max, max, max);
}

double Tile::complexity(Geom::Affine const &) const
{
    return 1.0;
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
