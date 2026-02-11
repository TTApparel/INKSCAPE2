// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * SVG <polygon> implementation
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Abhishek Sharma
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "sp-polygon.h"

#include <glibmm/i18n.h>

#include <2geom/curve.h>         // for Curve
#include <2geom/path.h>          // for Path, BaseIterator
#include <2geom/pathvector.h>    // for PathVector
#include <2geom/point.h>         // for Point

#include "attributes.h"          // for SPAttr
#include "document.h"            // for SPDocument
#include "helper/geom-curves.h"  // for is_straight_curve
#include "object/sp-object.h"    // for SP_OBJECT_WRITE_BUILD
#include "object/sp-shape.h"     // for SPShape
#include "svg/stringstream.h"    // for SVGOStringStream
#include "svg/svg.h"             // for sp_svg_write_path
#include "xml/document.h"        // for Document
#include "xml/node.h"            // for Node

class SPDocument;

SPPolygon::SPPolygon() : SPShape() {
}

SPPolygon::~SPPolygon() = default;

/*
 * Can change type when LPE applied.
 */
void SPPolygon::tag_name_changed(gchar const* oldname, gchar const* newname)
{
    const std::string typeString = newname;
    if (typeString == "svg:polygon") {
        type = SP_GENERIC_POLYGON;
    } else if (typeString == "svg:path") {
        type = SP_GENERIC_PATH;
    }
}

void SPPolygon::build(SPDocument *document, Inkscape::XML::Node *repr) {
	SPPolygon* object = this;

    SPShape::build(document, repr);

    object->readAttr(SPAttr::POINTS);
}

void SPPolygon::update(SPCtx* ctx, unsigned int flags) {
    if (flags & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_STYLE_MODIFIED_FLAG | SP_OBJECT_VIEWPORT_MODIFIED_FLAG)) {
        set_shape();
    }

    SPShape::update(ctx, flags);
}

Inkscape::XML::Node* SPPolygon::write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags) {
    // Tolerable workaround: we need to update the object's curve before we set points=
    // because it's out of sync when e.g. some extension attrs of the polygon or star are changed in XML editor
    set_shape();

#ifdef OBJECT_TRACE
    objectTrace( "SPPolygon::write", true, flags );
#endif
    GenericPolygonType new_type = SP_GENERIC_POLYGON;
    if (hasPathEffectOnClipOrMaskRecursive(this)) {
        new_type = SP_GENERIC_PATH;
    }
    if ((flags & SP_OBJECT_WRITE_BUILD) && !repr) {
        switch (new_type) {
            case SP_GENERIC_POLYGON:
                repr = xml_doc->createElement("svg:polygon");
                break;
            case SP_GENERIC_PATH:
                repr = xml_doc->createElement("svg:path");
                break;
            default:
                std::cerr << "SPGenericPolygon::write(): unknown type." << std::endl;
        }
    }
    if (type != new_type) {
        switch (new_type) {
            case SP_GENERIC_POLYGON:
                repr->setCodeUnsafe(g_quark_from_string("svg:polygon"));
                break;
            case SP_GENERIC_PATH:
                repr->setCodeUnsafe(g_quark_from_string("svg:path"));
                repr->setAttribute("sodipodi:type", "polygon");
                break;
            default:
                std::cerr << "SPGenericPolygon::write(): unknown type." << std::endl;
        }
        type = new_type;
    }

    if (type == SP_GENERIC_PATH) {
        // write d=
        if (_curve) {
            repr->setAttribute("d", sp_svg_write_path(*_curve));
        } else {
            repr->removeAttribute("d");
        }
    } else {
        Inkscape::SVGOStringStream os;
        for (auto point : points) {
            os << point.x() << "," << point.y() << " ";
        }
        repr->setAttribute("points", os.str().c_str());
    }

    SPShape::write(xml_doc, repr, flags);

    return repr;
}

void SPPolygon::set_shape()
{
    if (checkBrokenPathEffect()) {
        return;
    }

    Geom::Path c;
    auto first = true;
    for (auto point : points) {
        if (first) {
            c.start(point);
            first = false;
        } else {
            c.appendNew<Geom::LineSegment>(point);
        }
    }
    c.close();

    prepareShapeForLPE(std::move(c));
}

void SPPolygon::modified(unsigned int flags)
{
    if (flags & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_STYLE_MODIFIED_FLAG | SP_OBJECT_VIEWPORT_MODIFIED_FLAG)) {
        set_shape();
    }

    SPShape::modified(flags);
}

/**
 * @brief Parse a double from the string passed by pointer and advance the string start.
 *
 * @param[in,out] p A pointer to a string (representing a piece of the `points` attribute).
 * @param[out] v The parsed value.
 * @return Parse status.
 */
SPPolyParseError sp_poly_get_value(char const **p, double *v)
{
    while (**p != '\0' && (**p == ',' || **p == '\x20' || **p == '\x9' || **p == '\xD' || **p == '\xA')) {
        (*p)++;
    }

    if (**p == '\0') {
        return POLY_END_OF_STRING;
    }

    gchar *e = nullptr;
    double value = g_ascii_strtod(*p, &e);
    if (e == *p) {
        return POLY_INVALID_NUMBER;
    }
    if (std::isnan(value)) {
        return POLY_NOT_A_NUMBER;
    }
    if (std::isinf(value)) {
        return POLY_INFINITE_VALUE;
    }

    *p = e;
    *v = value;
    return POLY_OK;
}

/**
 * @brief Print a warning message related to the parsing of a 'points' attribute.
 */
static void sp_poly_print_warning(char const *points, char const *error_location, SPPolyParseError error)
{
    switch (error) {
        case POLY_END_OF_STRING: // Unexpected end of string!
            {
                size_t constexpr MAX_DISPLAY_SIZE = 64;
                Glib::ustring s{points};
                if (s.size() > MAX_DISPLAY_SIZE) {
                    s = "... " + s.substr(s.size() - MAX_DISPLAY_SIZE);
                }
                g_warning("Error parsing a 'points' attribute: string ended unexpectedly!\n\t\"%s\"", s.c_str());
                break;
            }
        case POLY_INVALID_NUMBER:
            g_warning("Invalid number in the 'points' attribute:\n\t\"(...) %s\"", error_location);
            break;

        case POLY_INFINITE_VALUE:
            g_warning("Infinity is not allowed in the 'points' attribute:\n\t\"(...) %s\"", error_location);
            break;

        case POLY_NOT_A_NUMBER:
            g_warning("NaN-value is not allowed in the 'points' attribute:\n\t\"(...) %s\"", error_location);
            break;

        case POLY_OK:
        default:
            break;
    }
}

/**
 * @brief Parse a 'points' attribute, printing a warning when an error occurs.
 *
 * @param points The points attribute.
 * @return The corresponding polyline curve (open).
 * To do: move to sp-polyline. (No longer used here.)
 */
std::optional<Geom::Path> sp_poly_parse_curve(char const *points)
{
    std::optional<Geom::Path> result;
    char const *cptr = points;

    while (true) {
        double x, y;

        if (auto error = sp_poly_get_value(&cptr, &x)) {
            // If the error is something other than end of input, we must report it.
            // End of input is allowed when scanning for the next x coordinate: it
            // simply means that we have reached the end of the coordinate list.
            if (error != POLY_END_OF_STRING) {
                sp_poly_print_warning(points, cptr, error);
            }
            break;
        }
        if (auto error = sp_poly_get_value(&cptr, &y)) {
            // End of input is not allowed when scanning for y.
            sp_poly_print_warning(points, cptr, error);
            break;
        }

        if (result) {
            result->appendNew<Geom::LineSegment>(Geom::Point{x, y});
        } else {
            result.emplace(Geom::Point{x, y});
        }
    }

    return result;
}

/**
 * @brief Parse a 'points' attribute, printing a warning when an error occurs.
 *
 * @param points The points attribute.
 * @return A vector of points parsed.
 */
std::vector<Geom::Point> sp_poly_parse_points(char const *points)
{
    std::vector<Geom::Point> result;
    char const *cptr = points;

    while (true) {
        double x, y;

        if (auto error = sp_poly_get_value(&cptr, &x)) {
            // If the error is something other than end of input, we must report it.
            // End of input is allowed when scanning for the next x coordinate: it
            // simply means that we have reached the end of the coordinate list.
            if (error != POLY_END_OF_STRING) {
                sp_poly_print_warning(points, cptr, error);
            }
            break;
        }
        if (auto error = sp_poly_get_value(&cptr, &y)) {
            // End of input is not allowed when scanning for y.
            sp_poly_print_warning(points, cptr, error);
            break;
        }

        result.emplace_back(Geom::Point{x, y});
    }

    return result;
}

void SPPolygon::set(SPAttr key, const gchar* value) {
    switch (key) {
        case SPAttr::POINTS: {
            if (!value) {
                /* fixme: The points attribute is required.  We should handle its absence as per
                 * http://www.w3.org/TR/SVG11/implnote.html#ErrorProcessing. */
                break;
            }

            points = sp_poly_parse_points(value);
            requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
            break;
        }
        default:
            SPShape::set(key, value);
            break;
    }
}

void SPPolygon::update_patheffect(bool write) {
    if (type != SP_GENERIC_PATH && !cloned && hasPathEffectOnClipOrMaskRecursive(this)) {
        SPPolygon::write(document->getReprDoc(), getRepr(), SP_OBJECT_MODIFIED_FLAG);
    }
    SPShape::update_patheffect(write);
}

const char* SPPolygon::typeName() const {
    return "polygon";
}

const char* SPPolygon::displayName() const {
    return _("Polygon");
}

gchar* SPPolygon::description() const {
    return g_strdup(_("<b>Polygon</b>"));
}

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
