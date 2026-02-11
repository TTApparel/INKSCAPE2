// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_SP_POLYGON_H
#define SEEN_SP_POLYGON_H

/*
 * SVG <polygon> implementation
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "sp-shape.h"

class SPPolygon : public SPShape
{
public:
    SPPolygon();
    ~SPPolygon() override;

    int tag() const override { return tag_of<decltype(*this)>; }
    void tag_name_changed(gchar const* oldname, gchar const* newname) override;

    void build(SPDocument *document, Inkscape::XML::Node *repr) override;
    void update(SPCtx* ctx, unsigned int flags);
    void modified(unsigned int flags) override;
    Inkscape::XML::Node *write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, unsigned int flags) override;
    void set(SPAttr key, char const *value) override;
    void update_patheffect(bool write) override;

    char const *typeName() const override;
    char const *displayName() const override;
    char *description() const override;

private:
    void set_shape() override;

    enum GenericPolygonType {
        SP_GENERIC_POLYGON, // Default
        SP_GENERIC_PATH     // LPE
    };

    GenericPolygonType type = SP_GENERIC_POLYGON;

    std::vector<Geom::Point> points;
};

// Functionality shared with SPPolyline
enum SPPolyParseError : uint8_t
{
    POLY_OK = 0,
    POLY_END_OF_STRING,
    POLY_INVALID_NUMBER,
    POLY_INFINITE_VALUE,
    POLY_NOT_A_NUMBER
};

std::optional<Geom::Path> sp_poly_parse_curve(char const *points);
std::vector<Geom::Point> sp_poly_parse_points(char const *points);

#endif

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
