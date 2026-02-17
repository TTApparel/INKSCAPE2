// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * SVG <hatch> implementation
 */
/*
 * Authors:
 *   Tomasz Boczkowski <penginsbacon@gmail.com>
 *   Jon A. Cruz <jon@joncruz.org>
 *
 * Copyright (C) 2014 Tomasz Boczkowski
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "sp-hatch.h"

#include <cstring>

#include <2geom/transforms.h>
#include <sigc++/functors/mem_fun.h>

#include "style.h"
#include "attributes.h"
#include "bad-uri-exception.h"
#include "document.h"

#include "display/curve.h"       // pathvector_append
#include "display/drawing.h"
#include "display/drawing-pattern.h"

#include "sp-defs.h"
#include "sp-hatch-path.h"
#include "sp-item.h"

#include "livarot/LivarotDefs.h" // FillRule, BooleanOp
#include "path/path-boolop.h"
#include "svg/svg.h"
#include "xml/href-attribute-helper.h"

SPHatchReference::SPHatchReference(SPHatch *obj)
    : URIReference(obj)
{}

SPHatch *SPHatchReference::getObject() const
{
    return cast_unsafe<SPHatch>(URIReference::getObject());
}

bool SPHatchReference::_acceptObject(SPObject *obj) const
{
    return is<SPHatch>(obj) && URIReference::_acceptObject(obj);
}

SPHatch::SPHatch()
    : ref{this}
{
    ref.changedSignal().connect(sigc::mem_fun(*this, &SPHatch::_onRefChanged));
}

SPHatch::~SPHatch() = default;

void SPHatch::build(SPDocument *doc, Inkscape::XML::Node *repr)
{
    SPPaintServer::build(doc, repr);

    readAttr(SPAttr::HATCHUNITS);
    readAttr(SPAttr::HATCHCONTENTUNITS);
    readAttr(SPAttr::TRANSFORM);
    readAttr(SPAttr::X);
    readAttr(SPAttr::Y);
    readAttr(SPAttr::PITCH);
    readAttr(SPAttr::ROTATE);
    readAttr(SPAttr::XLINK_HREF);
    readAttr(SPAttr::STYLE);

    // Register ourselves
    doc->addResource("hatch", this);
}

void SPHatch::release()
{
    if (document) {
        // Unregister ourselves
        document->removeResource("hatch", this);
    }

    auto children = hatchPaths();
    for (auto &v : views) {
        for (auto child : children) {
            child->hide(v.key);
        }
        v.drawingitem.reset();
    }
    views.clear();

    _modified_connection.disconnect();
    ref.detach();

    SPPaintServer::release();
}

void SPHatch::child_added(Inkscape::XML::Node* child, Inkscape::XML::Node* ref)
{
    SPObject::child_added(child, ref);

    auto path_child = cast<SPHatchPath>(document->getObjectByRepr(child));

    if (path_child) {
        for (auto &v : views) {
            Geom::OptInterval extents = _calculateStripExtents(v.bbox);
            auto ac = path_child->show(v.drawingitem->drawing(), v.key, extents);

            path_child->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
            if (ac) {
                v.drawingitem->prependChild(ac);
            }
        }
    }
    //FIXME: notify all hatches that refer to this child set
}

void SPHatch::set(SPAttr key, char const *value)
{
    switch (key) {
    case SPAttr::HATCHUNITS:
        if (value) {
            if (!std::strcmp(value, "userSpaceOnUse")) {
                _hatch_units = HatchUnits::UserSpaceOnUse;
            } else {
                _hatch_units = HatchUnits::ObjectBoundingBox;
            }
        } else {
            _hatch_units = {};
        }

        requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
        break;

    case SPAttr::HATCHCONTENTUNITS:
        if (value) {
            if (!std::strcmp(value, "userSpaceOnUse")) {
                _hatch_content_units = HatchUnits::UserSpaceOnUse;
            } else {
                _hatch_content_units = HatchUnits::ObjectBoundingBox;
            }
        } else {
            _hatch_content_units = {};
        }

        requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
        break;

    case SPAttr::TRANSFORM: {
        Geom::Affine t;
        if (value && sp_svg_transform_read(value, &t)) {
            _hatch_transform = t;
        } else {
            _hatch_transform = {};
        }

        requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
        break;
    }
    case SPAttr::X:
        _x.readOrUnset(value);
        requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
        break;

    case SPAttr::Y:
        _y.readOrUnset(value);
        requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
        break;

    case SPAttr::PITCH:
        _pitch.readOrUnset(value);
        requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
        break;

    case SPAttr::ROTATE:
        _rotate.readOrUnset(value);
        requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
        break;

    case SPAttr::XLINK_HREF:
        if (value && href == value) {
            // Href unchanged, do nothing.
        } else {
            href.clear();

            if (value) {
                // First, set the href field; it's only used in the "unchanged" check above.
                href = value;
                // Now do the attaching, which emits the changed signal.
                if (value) {
                    try {
                        ref.attach(Inkscape::URI(value));
                    } catch (Inkscape::BadURIException const &e) {
                        g_warning("%s", e.what());
                        ref.detach();
                    }
                } else {
                    ref.detach();
                }
            }
        }
        requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
        break;

    default:
        if (SP_ATTRIBUTE_IS_CSS(key)) {
            style->clear(key);
            requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_STYLE_MODIFIED_FLAG);
        } else {
            SPPaintServer::set(key, value);
        }
        break;
    }
}

bool SPHatch::_hasHatchPatchChildren(SPHatch const *hatch)
{
    return std::any_of(hatch->children.begin(),
                       hatch->children.end(),
                       [] (auto &c) { return is<SPHatchPath>(&c); });
}

Geom::Affine SPHatch::get_this_transform() const {
    return _hatch_transform.value_or(Geom::identity());
}

std::vector<SPHatchPath *> SPHatch::hatchPaths()
{
    auto const src = rootHatch();
    if (!src) {
        return {};
    }

    std::vector<SPHatchPath *> list;

    for (auto &child : src->children) {
        if (auto hatch_path = cast<SPHatchPath>(&child)) {
            list.push_back(hatch_path);
        }
    }

    return list;
}

std::vector<SPHatchPath const *> SPHatch::hatchPaths() const
{
    auto const src = rootHatch();
    if (!src) {
        return {};
    }

    std::vector<SPHatchPath const *> list;

    for (auto &child : src->children) {
        if (auto hatch_path = cast<SPHatchPath>(&child)) {
            list.push_back(hatch_path);
        }
    }

    return list;
}

// TODO: ::remove_child and ::order_changed handles - see SPPattern

void SPHatch::update(SPCtx* ctx, unsigned flags)
{
    if (flags & SP_OBJECT_MODIFIED_FLAG) {
        flags |= SP_OBJECT_PARENT_MODIFIED_FLAG;
    }

    flags &= SP_OBJECT_MODIFIED_CASCADE;

    std::vector<SPHatchPath *> children(hatchPaths());

    // TO DO: Swap inner/outer loops (to avoid recacluating strip extents repeatedly).
    for (auto child : children) {
        sp_object_ref(child, nullptr);

        for (auto &v : views) {
            Geom::OptInterval strip_extents = _calculateStripExtents(v.bbox);
            child->setStripExtents(v.key, strip_extents);
        }

        if ((flags & SP_OBJECT_FLAGS_ALL) || (child->mflags & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_CHILD_MODIFIED_FLAG))) {
            child->updateDisplay(ctx, flags);
        }

        sp_object_unref(child, nullptr);
    }

    for (auto &v : views) {
        _updateView(v);
    }
}

void SPHatch::modified(unsigned flags)
{
    flags = cascade_flags(flags);

    std::vector<SPHatchPath *> children(hatchPaths());

    for (auto child : children) {
        sp_object_ref(child, nullptr);

        if ((flags & SP_OBJECT_FLAGS_ALL) || (child->mflags & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_CHILD_MODIFIED_FLAG))) {
            child->emitModified(flags);
        }

        sp_object_unref(child, nullptr);
    }
}

void SPHatch::_onRefChanged(SPObject *old_ref, SPObject *ref)
{
    if (old_ref) {
        _modified_connection.disconnect();
    }

    auto hatch = cast<SPHatch>(ref);
    if (hatch) {
        _modified_connection = ref->connectModified(sigc::mem_fun(*this, &SPHatch::_onRefModified));
    }

    if (!_hasHatchPatchChildren(this)) {
        SPHatch *old_shown = nullptr;
        SPHatch *new_shown = nullptr;
        std::vector<SPHatchPath *> oldhatchPaths;
        std::vector<SPHatchPath *> newhatchPaths;

        auto old_hatch = cast<SPHatch>(old_ref);
        if (old_hatch) {
            old_shown = old_hatch->rootHatch();
            oldhatchPaths = old_shown->hatchPaths();
        }
        if (hatch) {
            new_shown = hatch->rootHatch();
            newhatchPaths = new_shown->hatchPaths();
        }
        if (old_shown != new_shown) {

            for (auto &v : views) {
                Geom::OptInterval extents = _calculateStripExtents(v.bbox);

                for (auto child : oldhatchPaths) {
                    child->hide(v.key);
                }
                for (auto child : newhatchPaths) {
                    auto cai = child->show(v.drawingitem->drawing(), v.key, extents);
                    child->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
                    if (cai) {
                        v.drawingitem->appendChild(cai);
                    }

                }
            }
        }
    }

    _onRefModified(ref, 0);
}

void SPHatch::_onRefModified(SPObject *, unsigned)
{
    requestModified(SP_OBJECT_MODIFIED_FLAG);
}

SPHatch const *SPHatch::rootHatch() const
{
    for (auto p = this; p; p = p->ref.getObject()) {
        if (_hasHatchPatchChildren(p)) { // find the first one with hatch patch children
            return p;
        }
    }
    return this; // document is broken, we can't get to root; but at least we can return ourself which is supposedly a valid hatch
}

// Access functions that look up fields up the chain of referenced hatchs and return the first one which is set

SPHatch::HatchUnits SPHatch::hatchUnits() const
{
    for (auto hatch = this; hatch; hatch = hatch->ref.getObject()) {
        if (hatch->_hatch_units) {
            return *hatch->_hatch_units;
        }
    }
    return HatchUnits::ObjectBoundingBox;
}

SPHatch::HatchUnits SPHatch::hatchContentUnits() const
{
    for (auto hatch = this; hatch; hatch = hatch->ref.getObject()) {
        if (hatch->_hatch_content_units) {
            return *hatch->_hatch_content_units;
        }
    }
    return HatchUnits::UserSpaceOnUse;
}

Geom::Affine SPHatch::hatchTransform() const
{
    for (auto hatch = this; hatch; hatch = hatch->ref.getObject()) {
        if (hatch->_hatch_transform) {
            return *hatch->_hatch_transform;
        }
    }
    return Geom::identity();
}

double SPHatch::x() const
{
    for (auto hatch = this; hatch; hatch = hatch->ref.getObject()) {
        if (hatch->_x._set) {
            return hatch->_x.computed;
        }
    }
    return 0;
}

double SPHatch::y() const
{
    for (auto hatch = this; hatch; hatch = hatch->ref.getObject()) {
        if (hatch->_y._set) {
            return hatch->_y.computed;
        }
    }
    return 0;
}

double SPHatch::pitch() const
{
    for (auto hatch = this; hatch; hatch = hatch->ref.getObject()) {
        if (hatch->_pitch._set) {
            return hatch->_pitch.computed;
        }
    }
    return 0;
}

double SPHatch::rotate() const
{
    for (auto hatch = this; hatch; hatch = hatch->ref.getObject()) {
        if (hatch->_rotate._set) {
            return hatch->_rotate.computed;
        }
    }
    return 0;
}

int SPHatch::_countHrefs(SPObject *obj) const
{
    if (!obj) {
        return 1;
    }

    int i = 0;

    SPStyle *style = obj->style;
    if (style && style->fill.isPaintserver() && style->getFillPaintServer() == this) {
        i++;
    }
    if (style && style->stroke.isPaintserver() && style->getStrokePaintServer() == this) {
        i++;
    }

    for (auto &child : obj->children) {
        i += _countHrefs(&child);
    }

    return i;
}

SPHatch *SPHatch::clone_if_necessary(SPItem *item, char const *property)
{
    SPHatch *hatch = this;
    if (hatch->href.empty() || hatch->hrefcount > _countHrefs(item)) {
        Inkscape::XML::Document *xml_doc = document->getReprDoc();
        Inkscape::XML::Node *defsrepr = document->getDefs()->getRepr();

        Inkscape::XML::Node *repr = xml_doc->createElement("svg:hatch");
        repr->setAttribute("inkscape:collect", "always");
        Glib::ustring parent_ref = Glib::ustring::compose("#%1", getRepr()->attribute("id"));
        Inkscape::setHrefAttribute(*repr, parent_ref);

        defsrepr->addChild(repr, nullptr);
        char const *child_id = repr->attribute("id");
        SPObject *child = document->getObjectById(child_id);
        g_assert(is<SPHatch>(child));

        hatch = cast<SPHatch>(child);

        Glib::ustring href = Glib::ustring::compose("url(#%1)", hatch->getRepr()->attribute("id"));

        SPCSSAttr *css = sp_repr_css_attr_new();
        sp_repr_css_set_property(css, property, href.c_str());
        sp_repr_css_change_recursive(item->getRepr(), css, "style");
        sp_repr_css_attr_unref(css);
    }

    return hatch;
}

void SPHatch::transform_multiply(Geom::Affine const &postmul, bool set)
{
    _hatch_transform = set ? postmul : hatchTransform() * postmul;
    setAttributeOrRemoveIfEmpty("transform", sp_svg_transform_write(*_hatch_transform));
}

bool SPHatch::isValid() const
{
    if (pitch() <= 0) {
        return false;
    }

    auto const children = hatchPaths();
    if (children.empty()) {
        return false;
    }

    return std::all_of(children.begin(),
                       children.end(),
                       [] (auto c) { return c->isValid(); });
}

Inkscape::DrawingPattern *SPHatch::show(Inkscape::Drawing &drawing, unsigned key, Geom::OptRect const &bbox)
{
    views.emplace_back(make_drawingitem<Inkscape::DrawingPattern>(drawing), bbox, key);
    auto &v = views.back();
    auto ai = v.drawingitem.get();

    auto children = hatchPaths();

    Geom::OptInterval extents = _calculateStripExtents(bbox);
    for (auto child : children) {
        Inkscape::DrawingItem *cai = child->show(drawing, key, extents);
        if (cai) {
            ai->appendChild(cai);
        }
    }

    _updateView(v);

    return ai;
}

void SPHatch::hide(unsigned key)
{
    auto const children = hatchPaths();

    for (auto child : children) {
        child->hide(key);
    }

    auto it = std::find_if(views.begin(), views.end(), [=] (auto &v) {
        return v.key == key;
    });

    if (it != views.end()) {
        views.erase(it);
        return;
    }

    g_assert_not_reached();
}

Geom::Interval SPHatch::bounds() const
{
    Geom::Interval result;
    auto children = hatchPaths();

    for (auto child : children) {
        if (result.extent() == 0) {
            result = child->bounds();
        } else {
            result |= child->bounds();
        }
    }
    return result;
}

// Render info based on object bounding box.
SPHatch::RenderInfo SPHatch::calculateRenderInfo(Geom::OptRect const &bbox) const
{
    return _calculateRenderInfo(bbox);
}

// Render info based on viewable area.
SPHatch::RenderInfo SPHatch::calculateRenderInfo(unsigned key) const
{
    for (auto const &v : views) {
        if (v.key == key) {
            return _calculateRenderInfo(v.bbox);
        }
    }
    g_assert_not_reached();
    return {};
}

void SPHatch::_updateView(View &view)
{
    RenderInfo info = _calculateRenderInfo(view.bbox);
    //The rendering of hatch overflow is implemented by repeated drawing
    //of hatch paths over one strip. Within each iteration paths are moved by pitch value.
    //The movement progresses from right to left. This gives the same result
    //as drawing whole strips in left-to-right order.

    view.drawingitem->setChildTransform(info.content_to_hatch);
    view.drawingitem->setPatternToUserTransform(info.hatch_to_user);
    view.drawingitem->setTileRect(info.hatch_tile);
    view.drawingitem->setStyle(style);
    view.drawingitem->setOverflow(info.overflow_initial_transform, info.overflow_steps, info.overflow_step_transform);
}

// Calculate render info based on x, y, rotate, pitch, and bounding box of child hatch paths.
// Tile height is not calculated here as different hatch paths in the same hatch can have
// different heights.
SPHatch::RenderInfo SPHatch::_calculateRenderInfo(Geom::OptRect const &bbox) const
{
    if (!bbox) {
        std::cerr << "SPHatch::RenderInfo: no bounding box!" << std::endl;
        return {};
    }

    // Calculate hatch transformation to user space.
    double hatch_x = x(); // tile refers to "base" tile.
    double hatch_y = y();
    double hatch_rotate = rotate();

    // Find size of strip:
    double strip_width = pitch();

    // Correct for units:
    if (hatchUnits() == HatchUnits::ObjectBoundingBox) {
        hatch_x = hatch_x * bbox->width()  + bbox->min()[Geom::X];
        hatch_y = hatch_y * bbox->height() + bbox->min()[Geom::Y];
        strip_width *= bbox->width();
    }

    Geom::Affine hatch2user = Geom::Rotate::from_degrees(hatch_rotate) * Geom::Translate(hatch_x, hatch_y) * hatchTransform();
    Geom::Affine user2hatch = hatch2user.inverse();

    Geom::Affine content2hatch;
    if (hatchContentUnits() == HatchUnits::ObjectBoundingBox) {
        content2hatch = Geom::Scale(bbox->width(), bbox->height());
    }
    Geom::Affine hatch2content = content2hatch.inverse();

    // Rotate/translate object bounding box to hatch space, find axis-aligned bounding box, this
    // ensures the hatch will cover the object. Hatch origin is now at (0, 0).
    RenderInfo info;
    info.hatch_bbox = *bbox * user2hatch;
    info.hatch_tile = Geom::Rect(Geom::Interval(0, strip_width), info.hatch_bbox[Geom::Y]);
    info.hatch_origin = Geom::Point(hatch_x, hatch_y);
    info.strip_width = strip_width;
    info.hatch_to_user = hatch2user;
    info.content_to_hatch = content2hatch;

    // Overflow (uses union of all hatch-path base tiles).
    if (style->overflow.computed == SP_CSS_OVERFLOW_VISIBLE) {
        Geom::Interval bounds = this->bounds();
        if (hatchContentUnits() == HatchUnits::ObjectBoundingBox) {
            bounds *= bbox->width();
        }
        info.overflow_right = floor(bounds.min() / strip_width); // Number of extra strips on right (negative)
        info.overflow_left  = ceil (bounds.max() / strip_width); // Number of extra strips on left + 1

        info.overflow_steps = info.overflow_left - info.overflow_right; // Includes base strip.
        info.overflow_step_transform = Geom::Translate(strip_width, 0.0);
        info.overflow_initial_transform = Geom::Translate((-info.overflow_left + 1) * strip_width, 0.0);

    }

    std::cout << "SPHatch::RenderInfo:" << std::endl;
    std::cout << "  hatch_bbox: " << info.hatch_bbox << std::endl;
    std::cout << "  hatch_origin: " << info.hatch_origin << std::endl;
    std::cout << "  hatch_to_user: " << info.hatch_to_user << std::endl;
    std::cout << "  content_to_hatch: " << info.content_to_hatch << std::endl;
    std::cout << "  overflow_right: " << info.overflow_right << std::endl;
    std::cout << "  overflow_left: " << info.overflow_left << std::endl;
    std::cout << "  overflow_steps: " << info.overflow_steps << std::endl;
    std::cout << "  overflow_steps: " << info.overflow_step_transform << std::endl;
    std::cout << "  overflow_init: " << info.overflow_initial_transform << std::endl;
    return info;
}

bool is_inside(int winding, SPWindRule wind_rule) {
    switch (wind_rule) {
        case SP_WIND_RULE_EVENODD:
            return winding % 2 !=0;
        case SP_WIND_RULE_POSITIVE:
            return winding > 0;
        case SP_WIND_RULE_NONZERO:
        default:
            return winding != 0;
    }
}

// Convert a hatch to pathvectors (one for each hatch-path).
// This is particularily useful for creating SVG's for plotters and cutters.
// Note: This does not handle CSS visible="none" (which isn't that useful).
bool SPHatch::toPaths(SPShape &shape)
{
    auto shape_bbox = shape.geometricBounds();
    auto render_info = calculateRenderInfo(shape_bbox);
    auto parent = shape.getRepr()->parent();
    auto xml_doc = shape.getRepr()->document();

    for (auto hatch_path : hatchPaths()) {
        auto curve = hatch_path->calculateRenderCurve(
            render_info.hatch_bbox   * render_info.content_to_hatch.inverse(),
            render_info.hatch_origin * render_info.content_to_hatch.inverse());
        curve *= render_info.content_to_hatch;

        // Calculate maximum, minimum strip. Overflow is handled by adding strips to the left and right.
        auto x_interval   = render_info.hatch_bbox[Geom::X];
        auto hatch_origin = render_info.hatch_origin;
        auto strip_width  = render_info.strip_width;
        int strip_min = floor(x_interval.min()/strip_width);
        int strip_max = ceil( x_interval.max()/strip_width);
        strip_min -= render_info.overflow_left;
        strip_max -= render_info.overflow_right;
        strip_min++; // overflow_left includes base strip

        Geom::PathVector new_curve;
        for (int i = strip_min; i < strip_max; ++i) {
            auto temp_curve = curve;
            temp_curve *= Geom::Translate(i * strip_width, 0);
            pathvector_append(new_curve, temp_curve);
        }

        // Curve is in hatch space, transform it to object space.
        new_curve *= render_info.hatch_to_user;

        auto shape_curve = shape.curve();
        auto shape_fill_rule = shape.style->fill_rule.computed;
        auto shape_fill_rule_livarot = to_livarot(shape_fill_rule);

        // Cut hatch by shape.
        auto cut_vector = sp_pathvector_boolop(*shape_curve, new_curve, bool_op_slice,
                                               shape_fill_rule_livarot, fill_oddEven);

        // Find part of hatch inside shape.
        Geom::PathVector inside_vector;
        for (auto path : cut_vector) {
            if (is_inside(shape_curve->winding(path.pointAt(0.5)), shape_fill_rule)) {
                inside_vector.push_back(path);
            }
        }

        // Create new path.
        auto new_path = xml_doc->createElement("svg:path");
        new_path->setAttribute("d", sp_svg_write_path(inside_vector));
        new_path->setAttribute("transform", shape.getRepr()->attribute("transform"));

        // Find style
        auto style = hatch_path->style;
        Glib::ustring style_string = "fill:none;stroke:purple;stroke-width:3";// + style->write();
        new_path->setAttribute("style", style_string);

        parent->addChildAtPos(new_path, shape.getRepr()->position());
        Inkscape::GC::release(new_path);
    }
    return true;
}

//calculates strip extents in content space
Geom::OptInterval SPHatch::_calculateStripExtents(Geom::OptRect const &bbox) const
{
    if (bbox.hasZeroArea()) {
        return {};
    }

    double tile_x = x();
    double tile_y = y();
    double tile_rotate = rotate();

    // Correct for units:
    if (hatchUnits() == HatchUnits::ObjectBoundingBox) {
        tile_x = tile_x * bbox->width()  + bbox->min()[Geom::X];
        tile_y = tile_y * bbox->height() + bbox->min()[Geom::Y];
    }

    Geom::Affine ps2user = Geom::Rotate::from_degrees(tile_rotate) * Geom::Translate(tile_x, tile_y) * hatchTransform();
    Geom::Affine user2ps = ps2user.inverse();

    Geom::Interval extents;
    for (int i = 0; i < 4; ++i) {
        Geom::Point corner = bbox->corner(i);
        Geom::Point corner_ps  =  corner * user2ps;
        if (i == 0 || corner_ps.y() < extents.min()) {
            extents.setMin(corner_ps.y());
        }
        if (i == 0 || corner_ps.y() > extents.max()) {
            extents.setMax(corner_ps.y());
        }
    }

    if (hatchContentUnits() == HatchUnits::ObjectBoundingBox) {
        extents /= bbox->height();
    }

    return extents;
}

void SPHatch::setBBox(unsigned key, Geom::OptRect const &bbox)
{
    for (auto &v : views) {
        if (v.key == key) {
            v.bbox = bbox;
            break;
        }
    }
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :
