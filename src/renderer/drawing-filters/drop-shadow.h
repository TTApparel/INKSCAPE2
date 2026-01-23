// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * SVG DropShadow primitive filter
 *
 * Authors:
 *   Martin Owens <doctormo@geek-2.com>
 *
 * Copyright (C) 2026 AUTHORS
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_INKSCAPE_RENDERER_DRAWING_FILTER_DROPSHADOW_H
#define SEEN_INKSCAPE_RENDERER_DRAWING_FILTER_DROPSHADOW_H

#include <2geom/point.h>

#include "colors/color.h"
#include "gaussian-blur.h"

namespace Inkscape::Renderer::DrawingFilter {

class DropShadow : public GaussianBlur
{
public:
    DropShadow() = default;
    ~DropShadow() override = default;

    void render(Slot &slot) const override;
    void area_enlarge(Geom::IntRect &area, Geom::Affine const &trans) const override;
    bool can_handle_affine(Geom::Affine const &) const override;

    void set_offset(Geom::Point const &offset) { _offset = offset; }
    void set_color(Colors::Color color);

    Glib::ustring name() const override { return Glib::ustring("DropShadow"); }

private:
    Geom::Point _offset = {2.0, 2.0};
    std::optional<Colors::Color> _color;
};

} // namespace Inkscape::Renderer::DrawingFilter

#endif // SEEN_INKSCAPE_RENDERER_DRAWING_FILTER_DROPSHADOW_H
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
