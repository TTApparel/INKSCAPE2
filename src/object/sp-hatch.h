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

#ifndef SEEN_SP_HATCH_H
#define SEEN_SP_HATCH_H

#include <vector>
#include <cstddef>
#include <glibmm/ustring.h>
#include <sigc++/connection.h>

#include "object-view.h"
#include "svg/svg-length.h"
#include "svg/svg-angle.h"
#include "sp-paint-server.h"
#include "uri-references.h"

class SPHatchReference;
class SPHatchPath;
class SPItem;

namespace Inkscape {
class Drawing;
class DrawingPattern;
namespace XML { class Node; }
} // namespace Inkscape

class SPHatch final : public SPPaintServer
{
public:
    enum HatchUnits
    {
        UNITS_USERSPACEONUSE,
        UNITS_OBJECTBOUNDINGBOX
    };

    struct RenderInfo
    {
        Geom::Affine child_transform;
        Geom::Affine pattern_to_user_transform;
        Geom::Rect tile_rect;

        int overflow_steps = 0;
        Geom::Affine overflow_step_transform;
        Geom::Affine overflow_initial_transform;
    };

    SPHatch();
    ~SPHatch() override;
    int tag() const override { return tag_of<decltype(*this)>; }

    // Reference (href)
    Glib::ustring href;
    SPHatchReference *ref;

    gdouble x() const;
    gdouble y() const;
    gdouble pitch() const;
    gdouble rotate() const;
    HatchUnits hatchUnits() const;
    HatchUnits hatchContentUnits() const;
    Geom::Affine const &hatchTransform() const;
    SPHatch *rootHatch(); //TODO: const

    std::vector<SPHatchPath *> hatchPaths();
    std::vector<SPHatchPath const *> hatchPaths() const;

    SPHatch *clone_if_necessary(SPItem *item, const gchar *property);
    void transform_multiply(Geom::Affine postmul, bool set);

    bool isValid() const override;

    Inkscape::DrawingPattern *show(Inkscape::Drawing &drawing, unsigned key, Geom::OptRect const &bbox) override;
    void hide(unsigned key) override;

    RenderInfo calculateRenderInfo(unsigned key) const;
    Geom::Interval bounds() const;
    void setBBox(unsigned int key, Geom::OptRect const &bbox) override;

protected:
    void build(SPDocument* doc, Inkscape::XML::Node* repr) override;
    void release() override;
    void child_added(Inkscape::XML::Node* child, Inkscape::XML::Node* ref) override;
    void set(SPAttr key, const gchar* value) override;
    void update(SPCtx* ctx, unsigned int flags) override;
    void modified(void* sender, unsigned int flags) override;

private:
    using View = ObjectView<Inkscape::DrawingPattern>;
    std::vector<View> views;

    static bool _hasHatchPatchChildren(SPHatch const *hatch);

    void _updateView(View &view);
    RenderInfo _calculateRenderInfo(View const &view) const;
    Geom::OptInterval _calculateStripExtents(Geom::OptRect const &bbox) const;

    /**
     * Count how many times hatch is used by the styles of o and its descendants
    */
    guint _countHrefs(SPObject *o) const;

    /**
     * Gets called when the hatch is reattached to another <hatch>
     */
    void _onRefChanged(SPObject *old_ref, SPObject *ref);

    /**
     * Gets called when the referenced <hatch> is changed
     */
    void _onRefModified(void* sender, SPObject *ref, guint flags);

    // patternUnits and patternContentUnits attribute
    HatchUnits _hatchUnits : 1;
    bool _hatchUnits_set : 1;
    HatchUnits _hatchContentUnits : 1;
    bool _hatchContentUnits_set : 1;

    // hatchTransform attribute
    Geom::Affine _hatchTransform;
    bool _hatchTransform_set : 1;

    // Strip
    SVGLength _x;
    SVGLength _y;
    SVGLength _pitch;
    SVGAngle _rotate;

    sigc::connection _modified_connection;
};

class SPHatchReference : public Inkscape::URIReference
{
public:
    SPHatchReference(SPHatch *obj)
        : URIReference(obj)
    {}

    SPHatch *getObject() const
    {
        return static_cast<SPHatch*>(URIReference::getObject());
    }

protected:
    bool _acceptObject(SPObject *obj) const override
    {
        return is<SPHatch>(obj) && URIReference::_acceptObject(obj);
    }
};

#endif // SEEN_SP_HATCH_H

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
