// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Group belonging to an SVG drawing element.
 *//*
 * Authors:
 *   Krzysztof Kosiński <tweenk.pl@gmail.com>
 *
 * Copyright (C) 2011 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_RENDERER_DRAWING_TEXT_H
#define INKSCAPE_RENDERER_DRAWING_TEXT_H

#include <memory>
#include "drawing-group.h"
#include "drawing-style.h"

class SPStyle;
class FontInstance;

namespace Inkscape::Renderer {

class DrawingGlyphs
    : public DrawingItem
{
public:
    DrawingGlyphs(Drawing &drawing);
    int tag() const override { return tag_of<decltype(*this)>; }

    void setGlyph(std::shared_ptr<FontInstance> font, unsigned int glyph, Geom::Affine const &trans);
    void setStyle(SPStyle const *style, SPStyle const *context_style = nullptr) override; // Not to be used
    Geom::IntRect getPickBox() const { return bbox_pick_scaled; };

protected:
    ~DrawingGlyphs() override = default;

    unsigned _updateItem(Geom::IntRect const &area, UpdateContext const &ctx, unsigned flags, unsigned reset) override;
    DrawingItem *_pickItem(Geom::Point const &p, double delta, unsigned flags) override;

    std::shared_ptr<void const> _font_data; // keeps alive pathvec, pathvec_ref, and pixbuf
    unsigned int   _glyph;
    float          _width;          // These three are used to set up bounding box
    float          _asc;            //
    float          _dsc;            //
    float          _pl;             // phase length

    double design_units;
    Geom::PathVector const *pathvec = nullptr; // pathvector of glyph.
    //Pixbuf const *pixbuf = nullptr;  // pixbuf, if SVG font
    Geom::Rect              bbox_exact;        // Exact bounding box of glyph.
    Geom::Rect              bbox_pick;         // Pick bounding box of glyph.
    Geom::Rect              bbox_draw;         // Draw bounding box of glyph (adds space for text decorations)
    // Geom::IntRect        bbox_exact_scaled;
    Geom::IntRect           bbox_pick_scaled;
    Geom::IntRect           bbox_draw_scaled;

    char*                   font_descr; // For debugging

    friend class DrawingText;
};

class DrawingText
    : public DrawingGroup
{
public:
    DrawingText(Drawing &drawing);
    int tag() const override { return tag_of<decltype(*this)>; }

    bool addComponent(std::shared_ptr<FontInstance> const &font, int unsigned glyph,
                      Geom::Affine const &trans, float width, float ascent, float descent, float phase_length);
    void setStyle(SPStyle const *style, SPStyle const *context_style = nullptr) override;
    void setChildrenStyle(SPStyle const *context_style) override;

protected:
    ~DrawingText() override = default;

    unsigned _updateItem(Geom::IntRect const &area, UpdateContext const &ctx, unsigned flags, unsigned reset) override;
    unsigned _renderItem(Context &dc, Options &rc, Geom::IntRect const &area, unsigned flags, DrawingItem const *stop_at) const override;
    void _clipItem(Context &dc, Options &rc, Geom::IntRect const &area) const override;
    DrawingItem *_pickItem(Geom::Point const &p, double delta, unsigned flags) override;
    bool _canClip() const override { return true; }

    void decorateItem(Context &dc, double phase_length, bool under) const;
    void decorateStyle(Context &dc, double vextent, double xphase, Geom::Point const &p1, Geom::Point const &p2, double thickness) const;
    NRStyle _nrstyle;

    bool style_vector_effect_stroke : 1;
    bool style_stroke_extensions_hairline : 1;
    SPWindRule style_clip_rule;

    friend class DrawingGlyphs;
};

} // namespace Inkscape::Renderer

#endif // INKSCAPE_RENDERER_DRAWING_TEXT_H

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
