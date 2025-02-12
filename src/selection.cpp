// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Per-desktop selection container
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   MenTaLguY <mental@rydia.net>
 *   bulia byak <buliabyak@users.sf.net>
 *   Andrius R. <knutux@gmail.com>
 *   Abhishek Sharma
 *   Adrian Boguszewski
 *
 * Copyright (C) 2016 Adrian Boguszewski
 * Copyright (C) 2006 Andrius R.
 * Copyright (C) 2004-2005 MenTaLguY
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "selection.h"

#include <cmath>
#include <glibmm/i18n.h>

#include "desktop.h"
#include "document-undo.h"
#include "document.h"
#include "inkscape.h"
#include "layer-manager.h"
#include "page-manager.h"

#include "object/sp-defs.h"
#include "object/sp-page.h"
#include "object/sp-shape.h"
#include "ui/icon-names.h"
#include "ui/tool/control-point-selection.h"
#include "ui/tool/path-manipulator.h"
#include "ui/tools/node-tool.h"

static constexpr auto SP_SELECTION_UPDATE_PRIORITY = G_PRIORITY_HIGH_IDLE + 1;

namespace Inkscape {

Selection::Selection(SPDesktop *desktop)
    : ObjectSet(desktop)
{}

Selection::Selection(SPDocument *document)
    : ObjectSet(document)
{}

Selection::~Selection() {
    if (_idle) {
        g_source_remove(_idle);
        _idle = 0;
    }
}

/* Handler for selected objects "modified" signal */

void Selection::_schedule_modified(SPObject */*obj*/, guint flags) {
    if (!this->_idle) {
        /* Request handling to be run in _idle loop */
        this->_idle = g_idle_add_full(SP_SELECTION_UPDATE_PRIORITY, GSourceFunc(&Selection::_emit_modified), this, nullptr);
    }

    /* Collect all flags */
    this->_flags |= flags;
}

gboolean Selection::_emit_modified(Selection *selection)
{
    /* force new handler to be created if requested before we return */
    selection->_idle = 0;
    guint flags = selection->_flags;
    selection->_flags = 0;

    selection->_emitModified(flags);

    /* drop this handler */
    return FALSE;
}

void Selection::_emitModified(guint flags)
{
    _modified_signal.emit(this, flags);

    if (!_desktop || isEmpty()) {
        return;
    }

    auto &pm = _document->getPageManager();

    // If the selected items have been moved to a new page...
    if (auto item = singleItem()) {
        pm.selectPage(item, false);
    } else {
        SPPage *page = pm.getPageFor(firstItem(), true);
        for (auto this_item : this->items()) {
            if (page != pm.getPageFor(this_item, true)) {
                return;
            }
        }
        pm.selectPage(page);
    }
}

void Selection::_emitChanged(bool persist_selection_context)
{
    ObjectSet::_emitChanged();
    if (persist_selection_context) {
        if (nullptr == _selection_context) {
            _selection_context = _desktop->layerManager().currentLayer();
            sp_object_ref(_selection_context, nullptr);
            _context_release_connection = _selection_context->connectRelease(sigc::mem_fun(*this, &Selection::_releaseContext));
        }
    } else {
        _releaseContext(_selection_context);
    }

    /** Change the layer selection to the item selection
      * TODO: Should it only change if there's a single object?
      */
    if (_document && _desktop) {
        if (auto item = singleItem()) {
            if (_change_layer) {
                auto layer = _desktop->layerManager().layerForObject(item);
                if (layer && layer != _selection_context) {
                    _desktop->layerManager().setCurrentLayer(layer);
                }
            }
            if (_change_page) {
                // This could be more complex if we want to be smarter.
                _document->getPageManager().selectPage(item, false);
            }
        }
        DocumentUndo::resetKey(_document);
    }

    _changed_signal.emit(this);
}

void Selection::_releaseContext(SPObject *obj)
{
    if (!_selection_context || _selection_context != obj) {
        return;
    }

    _context_release_connection.disconnect();

    sp_object_unref(_selection_context, nullptr);
    _selection_context = nullptr;
}

SPObject *Selection::activeContext()
{
    if (_selection_context) {
        return _selection_context;
    }
    return _desktop->layerManager().currentLayer();
}

std::vector<Inkscape::SnapCandidatePoint> Selection::getSnapPoints(SnapPreferences const *snapprefs) const {
    std::vector<Inkscape::SnapCandidatePoint> p;

    if (snapprefs != nullptr){
        SnapPreferences snapprefs_dummy = *snapprefs; // create a local copy of the snapping prefs
        snapprefs_dummy.setTargetSnappable(Inkscape::SNAPTARGET_ROTATION_CENTER, false); // locally disable snapping to the item center
        auto items = const_cast<Selection *>(this)->items();
        for (auto iter = items.begin(); iter != items.end(); ++iter) {
            SPItem *this_item = *iter;
            this_item->getSnappoints(p, &snapprefs_dummy);

            //Include the transformation origin for snapping
            //For a selection or group only the overall center is considered, not for each item individually
            if (snapprefs->isTargetSnappable(Inkscape::SNAPTARGET_ROTATION_CENTER)) {
                p.emplace_back(this_item->getCenter(), SNAPSOURCE_ROTATION_CENTER);
            }
        }
    }

    return p;
}

void Selection::setAnchor(double x, double y, bool set)
{
    constexpr double epsilon = 1e-12;
    auto const pt = Geom::Point{x, y};
    if (Geom::LInfty(anchor - pt) > epsilon || set != has_anchor) {
        anchor = pt;
        has_anchor = set;
        _emitModified(SP_OBJECT_MODIFIED_FLAG);
    }
}

void Selection::scaleAnchored(double amount, bool fixed)
{
    if (Geom::OptRect bbox = visualBounds()) {
        // Scale the amount by the size to get the final scale amount
        if (fixed) {
            double const max_len = bbox->maxExtent();
            if (max_len + amount <= 1e-3) {
                return;
            }
            amount = 1.0 + amount / max_len;
        }

        auto center = has_anchor ? bbox->min() + bbox->dimensions() * Geom::Scale(anchor) : bbox->midpoint();
        scaleRelative(center, Geom::Scale(amount, amount));

        DocumentUndo::maybeDone(document(),
                                ((amount > 0) ? "selector:grow:larger" : "selector:grow:smaller" ),
                                ((amount > 0) ? _("Grow") : _("Shrink")), INKSCAPE_ICON("tool-pointer"));
    }
}

void Selection::rotateAnchored(double angle_degrees, double zoom)
{
    if (Geom::OptRect bbox = visualBounds()) {
        auto mid = center() ? *center() : bbox->midpoint();
        auto center = has_anchor ? bbox->min() + bbox->dimensions() * Geom::Scale(anchor) : mid;

        if (auto d = desktop()) {
            angle_degrees = d->yaxisdir() ? angle_degrees : -angle_degrees;
        }

        if (zoom != 1.0) {
            Geom::Point m = bbox->midpoint();
            unsigned i = 0;
            if (center[Geom::X] < m[Geom::X]) { 
                i = 1;
            }
            if (center[Geom::Y] < m[Geom::Y]) {
                i = 3 - i;
            }

            double const r = Geom::L2(bbox->corner(i) - center);
            angle_degrees = 180 * atan2(angle_degrees / zoom, r) / M_PI;
        }

        rotateRelative(center, angle_degrees);

        if (angle_degrees == 90.0) {
            DocumentUndo::done(document(), _("Rotate 90\xc2\xb0 CW"), INKSCAPE_ICON("object-rotate-right"));
        } else if (angle_degrees == -90.0) {
            DocumentUndo::done(document(), _("Rotate 90\xc2\xb0 CCW"), INKSCAPE_ICON("object-rotate-left"));
        } else {
            DocumentUndo::maybeDone(document(),
                                ( ( angle_degrees > 0 )? "selector:rotate:ccw": "selector:rotate:cw" ),
                                _("Rotate"), INKSCAPE_ICON("tool-pointer"));
        }
    }
}


SPObject *Selection::_objectForXMLNode(Inkscape::XML::Node *repr) const {
    g_return_val_if_fail(repr != nullptr, NULL);
    auto object = _document->getObjectByRepr(repr);
    assert(object == _document->getObjectById(repr->attribute("id")));
    return object;
}

size_t Selection::numberOfLayers() {
    auto items = this->items();
    std::set<SPObject*> layers;
    for (auto iter = items.begin(); iter != items.end(); ++iter) {
        SPObject *layer = _desktop->layerManager().layerForObject(*iter);
        layers.insert(layer);
    }

    return layers.size();
}

size_t Selection::numberOfParents() {
    auto items = this->items();
    std::set<SPObject*> parents;
    for (auto iter = items.begin(); iter != items.end(); ++iter) {
        SPObject *parent = (*iter)->parent;
        parents.insert(parent);
    }
    return parents.size();
}

void Selection::_connectSignals(SPObject *object) {
    _modified_connections[object] = object->connectModified(sigc::mem_fun(*this, &Selection::_schedule_modified));
}

void Selection::_releaseSignals(SPObject *object) {
    _modified_connections.erase(object);
}

void
Selection::emptyBackup(){
    _selected_ids.clear();
    _seldata.clear();
    params.clear();
}

void
Selection::setBackup ()
{
    SPDesktop *desktop = this->desktop();
    Inkscape::UI::Tools::NodeTool *tool = nullptr;
    if (desktop) {
        if (auto nt = dynamic_cast<Inkscape::UI::Tools::NodeTool*>(desktop->getTool())) {
            tool = nt;
        }
    }

    emptyBackup();

    for (auto const * const item : items()) {
        auto id = item->getId();
        if (!id) continue;

        std::string selected_id;
        selected_id += "--id=";
        selected_id += id;
        params.push_back(std::move(selected_id));

        _selected_ids.emplace_back(std::move(id));
    }

    if (!tool) return;

    for (auto const point : tool->_selected_nodes->_points_list) {
        auto const node = dynamic_cast<Inkscape::UI::Node const *>(point);
        if (!node) continue;

        auto const &nodeList = node->nodeList();
        auto const &subpathList = nodeList.subpathList();

        int sp = 0;
        bool found_sp = false;
        for (auto i = subpathList.begin(), e = subpathList.end(); i != e; ++i, ++sp) {
            if (&**i == &nodeList) {
                found_sp = true;
                break;
            }
        }

        int nl = 0;
        bool found_nl = false;
        for (auto j = nodeList.begin(), e = nodeList.end(); j != e; ++j, ++nl) {
            if (&*j == node){
                found_nl = true;
                break;
            }
        }

        if (!(found_nl && found_sp)) {
            g_warning("Something went wrong while trying to pass selected nodes to extension. Please report a bug.");
            return;
        }

        auto id = subpathList.pm().item()->getId();
        params.push_back(Glib::ustring::compose("--selected-nodes=%1:%2:%3", id, sp, nl));
        _seldata.emplace_back(std::move(id), std::make_pair(sp, nl));
    }
}

void
Selection::restoreBackup()
{
    SPDesktop *desktop = this->desktop();
    SPDocument *document = SP_ACTIVE_DOCUMENT;
    SPDefs * defs = document->getDefs();
    Inkscape::UI::Tools::NodeTool *tool = nullptr;
    if (desktop) {
        if (auto nt = dynamic_cast<Inkscape::UI::Tools::NodeTool*>(desktop->getTool())) {
            tool = nt;
        }
    }

    // update selection
    std::vector<SPItem *> new_selection;
    for (auto const &selected_id : _selected_ids) {
        auto const item = cast<SPItem>(document->getObjectById(selected_id.c_str()));
        if (item && !defs->isAncestorOf(item)) {
            new_selection.push_back(item);
        }
    }
    clear();
    add(new_selection.begin(), new_selection.end());
    new_selection.clear();

    if (!tool) return;

    auto const cps = tool->_selected_nodes;
    cps->selectAll();
    auto const point = !cps->_points_list.empty() ? cps->_points_list.front() : nullptr;
    cps->clear();
    if (!point) return;

    auto const node = dynamic_cast<Inkscape::UI::Node const *>(point);
    if (!node) return;

    auto const &sp = node->nodeList().subpathList();
    for (auto & l : _seldata) {
        int sp_count = 0;
        for (auto j = sp.begin(); j != sp.end(); ++j, ++sp_count) {
            if (sp_count != l.second.first) continue;

            int nt_count = 0;
            for (auto k = (*j)->begin(); k != (*j)->end(); ++k, ++nt_count) {
                if (nt_count == l.second.second) {
                    cps->insert(k.ptr());
                    break;
                }
            }
            break;
        }
    }
}

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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :
