// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_INKSCAPE_RENDERER_DRAWING_FILTER_COMPONENT_TRANSFER_H
#define SEEN_INKSCAPE_RENDERER_DRAWING_FILTER_COMPONENT_TRANSFER_H

/*
 * feComponentTransfer filter primitive renderer
 *
 * Authors:
 *   Felipe Corrêa da Silva Sanches <juca@members.fsf.org>
 *
 * Copyright (C) 2007 authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <vector>

#include "primitive.h"

namespace Inkscape::Renderer::DrawingFilter {

enum ComponentTransferType
{
    IDENTITY,
    TABLE,
    DISCRETE,
    LINEAR,
    GAMMA,
    ERROR
};

class ComponentTransfer : public Primitive
{
public:
    ComponentTransfer() = default;
    ~ComponentTransfer() override = default;

    void render(Slot &slot) const override;
    bool can_handle_affine(Geom::Affine const &) const override;
    double complexity(Geom::Affine const &ctm) const override;

    ComponentTransferType type[4];
    std::vector<double> tableValues[4];
    double slope[4];
    double intercept[4];
    double amplitude[4];
    double exponent[4];
    double offset[4];

    Glib::ustring name() const override { return Glib::ustring("Component Transfer"); }
};

} // namespace Inkscape::Renderer::DrawingFilter

#endif // SEEN_INKSCAPE_RENDERER_DRAWING_FILTER_COMPONENT_TRANSFER_H
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
