// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#include "lpe-powermask.h"

#include <glibmm/i18n.h>
#include <2geom/intersection-graph.h>
#include <2geom/path-intersection.h>

#include "inkscape.h"
#include "live_effects/lpeobject-reference.h"
#include "live_effects/lpeobject.h"
#include "object/sp-defs.h"
#include "object/sp-item-group.h"
#include "object/sp-mask.h"
#include "preferences.h"
#include "selection.h"
#include "svg/svg.h"
#include "util/safe-printf.h"
#include "util/uri.h"

namespace Inkscape {
namespace LivePathEffect {

LPEPowerMask::LPEPowerMask(LivePathEffectObject *lpeobject)
    : Effect(lpeobject)
    , uri("Store the uri of mask", "", "uri", &wr, this, "false", false)
    , invert(_("Invert mask"), _("Invert mask"), "invert", &wr, this, false)
    ,
    // wrap(_("Wrap mask data"), _("Wrap mask data allowing previous filters"), "wrap", &wr, this, false),
    hide_mask(_("Hide mask"), _("Hide mask"), "hide_mask", &wr, this, false)
    , background(_("Add background to mask"), _("Add background to mask"), "background", &wr, this, false)
    , background_color(_("Background color and opacity"), _("Set color and opacity of the background"),
                       "background_color", &wr, this, Colors::Color(0xffffffff))
{
    registerParameter(&uri);
    registerParameter(&invert);
    registerParameter(&hide_mask);
    registerParameter(&background);
    registerParameter(&background_color);
    previous_color = background_color.get_value();
}

LPEPowerMask::~LPEPowerMask() = default;

Glib::ustring LPEPowerMask::getId()
{
    return Glib::ustring("mask-powermask-") + getLPEObj()->getId();
}

void LPEPowerMask::doOnApply(SPLPEItem const *lpeitem)
{
    auto const item = const_cast<SPLPEItem *>(lpeitem);
    SPObject *mask = item->getMaskObject();
    bool hasit = false;
    if (lpeitem->hasPathEffect() && lpeitem->pathEffectsEnabled()) {
        PathEffectList path_effect_list(*lpeitem->path_effect_list);
        for (auto const &lperef : path_effect_list) {
            auto const lpeobj = lperef->lpeobject;
            if (!lpeobj) {
                /** \todo Investigate the cause of this.
                 * For example, this happens when copy pasting an object with LPE applied. Probably because the object
                 * is pasted while the effect is not yet pasted to defs, and cannot be found.
                 */
                g_warning("SPLPEItem::performPathEffect - NULL lpeobj in list!");
                return;
            }
            if (LPETypeConverter.get_key(lpeobj->effecttype) == "powermask") {
                hasit = true;
                break;
            }
        }
    }
    if (!mask || hasit) {
        item->removeCurrentPathEffect(false);
    } else {
        auto const newmask = getId();
        auto const new_uri = "url(#" + newmask + ")";
        mask->setAttribute("id", newmask);
        item->setAttribute("mask", new_uri);
    }
}

void LPEPowerMask::tryForkMask()
{
    auto const document = getSPDoc();
    if (!document || !sp_lpe_item) {
        return;
    }
    SPObject *mask = sp_lpe_item->getMaskObject();
    auto const elem_ref = document->getObjectById(getId().c_str());
    if (!elem_ref && sp_lpe_item && mask) {
        auto const newmask = getId();
        auto const new_uri = "url(#" + newmask + ")";
        auto const xml_doc = document->getReprDoc();
        XML::Node *fork = mask->getRepr()->duplicate(xml_doc);
        document->getDefs()->appendChildRepr(fork);
        fork->setAttribute("id", newmask);
        Inkscape::GC::release(fork);
        sp_lpe_item->setAttribute("mask", new_uri);
    }
}

void LPEPowerMask::doBeforeEffect(SPLPEItem const *lpeitem)
{
    // To avoid close of color dialog and better performance on change color
    tryForkMask();
    auto mask = sp_lpe_item->getMaskObject();
    auto uri_str = uri.param_getSVGValue();
    if (hide_mask && mask) {
        sp_lpe_item->getMaskRef().detach();
    } else if (!hide_mask && !mask && !uri_str.empty()) {
        sp_lpe_item->getMaskRef().try_attach(uri_str.c_str());
    }
    mask = sp_lpe_item->getMaskObject();
    if (mask) {
        if (previous_color != *background_color.get_value()) {
            previous_color = *background_color.get_value();
            setMask();
        } else {
            uri.param_setValue(extract_uri(sp_lpe_item->getAttribute("mask")), true);
            sp_lpe_item->getMaskRef().detach();
            Geom::OptRect const bbox = lpeitem->visualBounds();
            if (!bbox) {
                return;
            }
            uri_str = uri.param_getSVGValue();
            sp_lpe_item->getMaskRef().try_attach(uri_str.c_str());

            Geom::Rect bboxrect = (*bbox);
            bboxrect.expandBy(1);
            mask_box.clear();
            mask_box = Geom::Path(bboxrect);
            auto const document = getSPDoc();
            if (!document) {
                return;
            }
            DocumentUndo::ScopedInsensitive tmp(document);
            setMask();
        }
    } else if (!hide_mask) {
        auto const item = const_cast<SPLPEItem *>(lpeitem);
        item->removeCurrentPathEffect(false);
    }
}

void LPEPowerMask::setMask()
{
    auto const mask = sp_lpe_item->getMaskObject();
    auto const document = getSPDoc();
    if (!document || !mask) {
        return;
    }
    XML::Document *xml_doc = document->getReprDoc();
    SPDefs *defs = document->getDefs();
    auto const mask_id = getId();
    auto const box_id = mask_id + "_box";
    auto const filter_id = mask_id + "_inverse";
    auto const filter_label = "filter" + mask_id;
    auto const filter_uri = "url(#" + filter_id + ")";
    if (!document->getObjectById(filter_id)) {
        auto filter = xml_doc->createElement("svg:filter");
        filter->setAttribute("id", filter_id);
        filter->setAttribute("inkscape:label", filter_label);
        SPCSSAttr *css = sp_repr_css_attr_new();
        sp_repr_css_set_property(css, "color-interpolation-filters", "sRGB");
        sp_repr_css_change(filter, css, "style");
        sp_repr_css_attr_unref(css);
        filter->setAttribute("height", "100");
        filter->setAttribute("width", "100");
        filter->setAttribute("x", "-50");
        filter->setAttribute("y", "-50");
        Inkscape::XML::Node *primitive1 = xml_doc->createElement("svg:feColorMatrix");
        auto const primitive1_id = mask_id + "_primitive1";
        primitive1->setAttribute("id", primitive1_id);
        primitive1->setAttribute("values", "1");
        primitive1->setAttribute("type", "saturate");
        primitive1->setAttribute("result", "fbSourceGraphic");
        Inkscape::XML::Node *primitive2 = xml_doc->createElement("svg:feColorMatrix");
        auto const primitive2_id = mask_id + "_primitive2";
        primitive2->setAttribute("id", primitive2_id);
        primitive2->setAttribute("values", "-1 0 0 0 1 0 -1 0 0 1 0 0 -1 0 1 0 0 0 1 0 ");
        primitive2->setAttribute("in", "fbSourceGraphic");
        defs->appendChildRepr(filter);
        Inkscape::GC::release(filter);
        filter->appendChild(primitive1);
        Inkscape::GC::release(primitive1);
        filter->appendChild(primitive2);
        Inkscape::GC::release(primitive2);
    }

    SPObject *elemref = nullptr;
    auto const g_data_id = mask_id + "_container";
    if ((elemref = document->getObjectById(g_data_id))) {
        auto const &item_list = cast<SPGroup>(elemref)->item_list();
        for (auto const iter : item_list) {
            Inkscape::XML::Node *mask_node = iter->getRepr();
            elemref->getRepr()->removeChild(mask_node);
            mask->getRepr()->appendChild(mask_node);
            Inkscape::GC::release(mask_node);
        }
        elemref->deleteObject(true);
    }

    auto const &mask_list = mask->childList(true);
    for (auto const iter : mask_list) {
        auto mask_data = cast<SPItem>(iter);
        auto mask_node = mask_data->getRepr();
        if (mask_data->getId() == box_id.c_str()) {
            continue;
        }
        auto const css = sp_repr_css_attr_new();
        if (mask_node->attribute("style")) {
            sp_repr_css_attr_add_from_string(css, mask_node->attribute("style"));
        }
        auto const css_filter = sp_repr_css_property(css, "filter", nullptr);
        if (!css_filter || filter_uri == Glib::ustring(css_filter)) {
            if (invert && is_visible) {
                sp_repr_css_set_property(css, "filter", filter_uri.c_str());
            } else {
                sp_repr_css_set_property(css, "filter", nullptr);
            }
            Glib::ustring css_str;
            sp_repr_css_write_string(css, css_str);
            mask_node->setAttribute("style", css_str);
        }
    }

    if (background && is_visible) {
        XML::Node *box = nullptr;
        bool exist = true;

        if (auto box_elem_ref = document->getObjectById(box_id)) {
            box = box_elem_ref->getRepr();
        } else {
            box = xml_doc->createElement("svg:path");
            box->setAttribute("id", box_id);
            exist = false;
        }

        auto const css = sp_repr_css_attr_new();
        sp_repr_css_set_property_string(css, "fill", background_color.get_value()->toString(false));
        sp_repr_css_set_property_double(css, "fill-opacity", background_color.get_value()->getOpacity());
        sp_repr_css_set_property_string(css, "stroke", "none");

        auto const css_filter = sp_repr_css_property(css, "filter", nullptr);
        if (!css_filter || filter_uri == Glib::ustring(css_filter)) {
            if (invert && is_visible) {
                sp_repr_css_set_property(css, "filter", filter_uri.c_str());
            } else {
                sp_repr_css_set_property(css, "filter", nullptr);
            }
        }
        sp_repr_css_change(box, css, "style");
        sp_repr_css_attr_unref(css);
        box->setAttribute("d", sp_svg_write_path(mask_box));
        if (!exist) {
            mask->appendChildRepr(box);
            box->setPosition(0);
            Inkscape::GC::release(box);
        }
    } else if (!background && ((elemref = document->getObjectById(box_id)))) {
        elemref->deleteObject(true);
    }
    mask->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
}

void LPEPowerMask::doOnVisibilityToggled(SPLPEItem const *lpeitem)
{
    doBeforeEffect(lpeitem);
}

void LPEPowerMask::doEffect(Geom::PathVector &curve) {}

void LPEPowerMask::doOnRemove(SPLPEItem const *lpeitem)
{
    if (!lpeitem->getMaskObject()) {
        return;
    }

    if (keep_paths || Inkscape::Preferences::get()->getBool("/options/onungroup", false)) {
        return;
    }

    invert.param_setValue(false);
    auto const document = getSPDoc();
    auto const mask_id = getId();
    auto const filter_id = mask_id + "_inverse";
    auto const box_id = mask_id + "_box";

    if (auto const elem_ref = document->getObjectById(filter_id)) {
        elem_ref->deleteObject(true);
    }

    if (auto const elem_ref = document->getObjectById(box_id)) {
        elem_ref->deleteObject(true);
    }
}

void sp_inverse_powermask(Inkscape::Selection *sel)
{
    if (sel->isEmpty() || !SP_ACTIVE_DOCUMENT) {
        return;
    }

    for (auto lpeitem : sel->objects_of_type<SPLPEItem>() | std::views::reverse) {
        if (lpeitem->getMaskObject()) {
            Effect::createAndApply(POWERMASK, SP_ACTIVE_DOCUMENT, lpeitem);
            if (auto lpe = lpeitem->getCurrentLPE()) {
                lpe->getRepr()->setAttribute("invert", "false");
                lpe->getRepr()->setAttribute("is_visible", "true");
                lpe->getRepr()->setAttribute("hide_mask", "false");
                lpe->getRepr()->setAttribute("background", "true");
                lpe->getRepr()->setAttribute("background_color", "#ffffffff");
            }
        }
    }
}

void sp_remove_powermask(Inkscape::Selection *sel)
{
    if (sel->isEmpty()) {
        return;
    }

    for (auto lpeitem : sel->objects_of_type<SPLPEItem>() | std::views::reverse) {
        if (lpeitem->hasPathEffect() && lpeitem->pathEffectsEnabled()) {
            PathEffectList path_effect_list(*lpeitem->path_effect_list);
            for (auto const &lperef : path_effect_list) {
                auto const lpeobj = lperef->lpeobject;
                if (!lpeobj) {
                    /** \todo Investigate the cause of this.
                     * For example, this happens when copy pasting an object with LPE applied. Probably because
                     * the object is pasted while the effect is not yet pasted to defs, and cannot be found.
                     */
                    g_warning("SPLPEItem::performPathEffect - NULL lpeobj in list!");
                    return;
                }
                if (LPETypeConverter.get_key(lpeobj->effecttype) == "powermask") {
                    lpeitem->setCurrentPathEffect(lperef);
                    lpeitem->removeCurrentPathEffect(false);
                    break;
                }
            }
        }
    }
}

} // namespace LivePathEffect
} // namespace Inkscape

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
