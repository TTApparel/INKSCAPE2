// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Our nice measuring tool
 *
 * Authors:
 *   Felipe Correa da Silva Sanches <juca@members.fsf.org>
 *   Jon A. Cruz <jon@joncruz.org>
 *   Jabiertxo Arraiza <jabier.arraiza@marker.es>
 *
 * Copyright (C) 2011 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "measure-tool.h"

#include <iomanip>

#include <gtkmm.h>
#include <glibmm/i18n.h>

#include <2geom/line.h>
#include <2geom/path-intersection.h>

#include "colors/utils.h"
#include "desktop-style.h"
#include "desktop.h"
#include "document-undo.h"
#include "layer-manager.h"
#include "page-manager.h"
#include "path-chemistry.h"
#include "selection.h"
#include "text-editing.h"

#include "display/curve.h"
#include "display/control/canvas-item-curve.h"
#include "display/control/canvas-item-ctrl.h"
#include "display/control/canvas-item-group.h"
#include "display/control/canvas-item-text.h"

#include "helper/geom.h"

#include "object/sp-defs.h"
#include "object/sp-ellipse.h"
#include "object/sp-flowtext.h"
#include "object/sp-namedview.h"
#include "object/sp-root.h"
#include "object/sp-shape.h"
#include "object/sp-text.h"

#include "svg/svg.h"

#include "ui/clipboard.h"
#include "ui/dialog/knot-properties.h"
#include "ui/icon-names.h"
#include "ui/knot/knot.h"
#include "ui/tools/freehand-base.h"
#include "ui/widget/canvas.h" // Canvas area
#include "ui/widget/events/canvas-event.h"

#include "util/units.h"
#include "util-string/ustring-format.h"

using Inkscape::DocumentUndo;

namespace Inkscape::UI::Tools {
namespace {

/**
 * Simple class to use for removing label overlap.
 */
struct LabelPlacement
{
    Glib::ustring label;
    double lengthVal;
    double offset;
    Geom::Point start;
    Geom::Point end;
};

bool SortLabelPlacement(LabelPlacement const &first, LabelPlacement const &second)
{
    if (first.end.y() == second.end.y()) {
        return first.end.x() < second.end.x();
    } else {
        return first.end.y() < second.end.y();
    }
}

//precision is for give the number of decimal positions
//of the label to calculate label width
void repositionOverlappingLabels(std::vector<LabelPlacement> &placements, SPDesktop *desktop, Geom::Point const &normal, double fontsize, int precision)
{
    std::sort(placements.begin(), placements.end(), SortLabelPlacement);

    double border = 3;
    Geom::Rect box;
    {
        Geom::Point tmp(fontsize * (6 + precision) + (border * 2), fontsize + (border * 2));
        tmp = desktop->w2d(tmp);
        box = Geom::Rect(-tmp[Geom::X] / 2, -tmp[Geom::Y] / 2, tmp[Geom::X] / 2, tmp[Geom::Y] / 2);
    }

    // Using index since vector may be re-ordered as we go.
    // Starting at one, since the first item can't overlap itself
    for (size_t i = 1; i < placements.size(); i++) {
        LabelPlacement &place = placements[i];

        bool changed = false;
        do {
            Geom::Rect current(box + place.end);

            changed = false;
            bool overlaps = false;
            for (size_t j = i; (j > 0) && !overlaps; --j) {
                LabelPlacement &otherPlace = placements[j - 1];
                Geom::Rect target(box + otherPlace.end);
                if (current.intersects(target)) {
                    overlaps = true;
                }
            }
            if (overlaps) {
                place.offset += (fontsize + border);
                place.end = place.start - desktop->w2d(normal * place.offset);
                changed = true;
            }
        } while (changed);

        std::sort(placements.begin(), placements.begin() + i + 1, SortLabelPlacement);
    }
}

/**
 * Calculates where to place the anchor for the display text and arc.
 * 
 * @param desktop the desktop that is being used.
 * @param angle the angle to be displaying.
 * @param baseAngle the angle of the initial baseline.
 * @param startPoint the point that is the vertex of the selected angle.
 * @param endPoint the point that is the end the user is manipulating for measurement.
 * @param fontsize the size to display the text label at.
 */
Geom::Point calcAngleDisplayAnchor(SPDesktop *desktop, double angle, double baseAngle,
                                   Geom::Point const &startPoint, Geom::Point const &endPoint,
                                   double fontsize)
{
    // Time for the trick work of figuring out where things should go, and how.
    double lengthVal = (endPoint - startPoint).length();
    double effective = baseAngle + angle / 2;
    auto where = Geom::Point(lengthVal, 0) * Geom::Rotate(effective) * Geom::Translate(startPoint);

    // When the angle is tight, the label would end up under the cursor and/or lines. Bump it
    double scaledFontsize = std::abs(fontsize * desktop->w2d(Geom::Point(0, 1)).y());
    if (std::abs((where - endPoint).length()) < scaledFontsize) {
        where.y() += scaledFontsize * 2;
    }

    // We now have the ideal position, but need to see if it will fit/work.

    Geom::Rect screen_world = desktop->getCanvas()->get_area_world();
    if (screen_world.interiorContains(desktop->d2w(startPoint)) ||
        screen_world.interiorContains(desktop->d2w(endPoint))) {
        screen_world.expandBy(fontsize * -3, fontsize / -2);
        where = desktop->w2d(screen_world.clamp(desktop->d2w(where)));
    } // else likely initialized the measurement tool, keep display near the measurement.

    return where;
}


/**
 * @brief Calculates the point where to position the delta text label
 * 
 *  returns the point to use for the text anchor
 * 
 * @param placements: the positions of the labels to try to avoid overlapping
 * @param basePoint: the base point along the delta line from where the perpendicular line starts
 * @param maxStrLength: the maximum length of the text labels shown (may be approx., but better than hard coding it)
 * @param normal: the normal of the delta line
 * @param is_dX: if true calculates for dX; if false calculates for dY
 * 
 */
Geom::Point calcDeltaLabelTextPos(std::vector<LabelPlacement> placements, SPDesktop *desktop, Geom::Point basePoint,
                            double fontsize, Glib::ustring unit_name, int maxStrLength, Geom::Point normal, bool is_dX = true)
{
    double border = 3;
    Geom::Rect box;
    {
        Geom::Point tmp((fontsize * maxStrLength * 0.66) + (border * 2), fontsize + (border * 2));
        tmp = desktop->w2d(tmp);
        box = Geom::Rect(-tmp[Geom::X] / 2, -tmp[Geom::Y] / 2, tmp[Geom::X] / 2, tmp[Geom::Y] / 2);
    }
    Geom::Point textPos = basePoint;
    double step;
    if (is_dX) {
        step =  normal[Geom::Y] * fontsize * 2; // the label box is bigger than the font...
        textPos[Geom::Y] += step * 1.5; // bringing it slightly higher at the initial position
    } else {
        step =  normal[Geom::X] * fontsize * 2;
        textPos[Geom::X] += step; 
    }    
    
    bool changed = false;
    do {
        changed = false;
        for (auto item : placements) { // placements are not ordered so checking all of them 
            Geom::Rect itemBox(box + item.end);
            Geom::Rect boxDelta(box + textPos);
            if (boxDelta.intersects(itemBox)) {
                if (is_dX) {
                    textPos[Geom::Y] += step;  // the normals to dX and dY are always horizontal/vertical
                } else {
                    textPos[Geom::X] += step; 
                }
                changed = true;
            }
        }
    } while (changed);
    return textPos;
}

} // namespace

/**
 * Given an angle, the arc center and edge point, draw an arc segment centered around that edge point.
 *
 * @param desktop the desktop that is being used.
 * @param center the center point for the arc.
 * @param end the point that ends at the edge of the arc segment.
 * @param anchor the anchor point for displaying the text label.
 * @param angle the angle of the arc segment to draw.
 * @param measure_rpr the container of the curve if converted to items.
 *
 */
void MeasureTool::createAngleDisplayCurve(Geom::Point const &center, Geom::Point const &end, Geom::Point const &anchor,
                                          double angle, bool to_phantom,
                                          Inkscape::XML::Node *measure_repr)
{
    // Given that we have a point on the arc's edge and the angle of the arc, we need to get the two endpoints.

    double textLen = std::abs((anchor - center).length());
    double sideLen = std::abs((end - center).length());
    if (sideLen > 0.0) {
        double factor = std::min(1.0, textLen / sideLen);

        // arc start
        Geom::Point p1 = end * Geom::Translate(-center)
                                * Geom::Scale(factor)
                                * Geom::Translate(center);

        // arc end
        Geom::Point p4 = p1 * Geom::Translate(-center)
                               * Geom::Rotate(-angle)
                               * Geom::Translate(center);

        // from Riskus
        double xc = center[Geom::X];
        double yc = center[Geom::Y];
        double ax = p1[Geom::X] - xc;
        double ay = p1[Geom::Y] - yc;
        double bx = p4[Geom::X] - xc;
        double by = p4[Geom::Y] - yc;
        double q1 = (ax * ax) + (ay * ay);
        double q2 = q1 + (ax * bx) + (ay * by);

        double k2;

        /*
         * The denominator of the expression for k2 can become 0, so this should be handled.
         * The function for k2 tends to a limit for very small values of (ax * by) - (ay * bx), so theoretically
         * it should be correct for values close to 0, however due to floating point inaccuracies this
         * is not the case, and instabilities still exist. Therefore do a range check on the denominator.
         * (This also solves some instances where again due to floating point inaccuracies, the square root term
         * becomes slightly negative in case of very small values for ax * by - ay * bx).
         * The values of this range have been generated by trying to make this term as small as possible,
         * by zooming in as much as possible in the GUI, using the measurement tool and
         * trying to get as close to 180 or 0 degrees as possible.
         * Smallest value I was able to get was around 1e-5, and then I added some zeroes for good measure.
         */
        if (!((ax * by - ay * bx < 0.00000000001) && (ax * by - ay * bx > -0.00000000001))) {
            k2 = (4.0 / 3.0) * (std::sqrt(2 * q1 * q2) - q2) / ((ax * by) - (ay * bx));
        } else {
            // If the denominator is 0, there are 2 cases:
            // Either the angle is (almost) +-180 degrees, in which case the limit of k2 tends to -+4.0/3.0.
            if (angle > 3.14 || angle < -3.14) { // The angle is in radians
                // Now there are also 2 cases, where inkscape thinks it is 180 degrees, or -180 degrees.
                // Adjust the value of k2 accordingly
                if (angle > 0) {
                    k2 = -4.0 / 3.0;
                } else {
                    k2 = 4.0 / 3.0;
                }
            } else {
                // if the angle is (almost) 0, k2 is equal to 0
                k2 = 0.0;
            }
        }

        Geom::Point p2(xc + ax - (k2 * ay),
                       yc + ay  + (k2 * ax));
        Geom::Point p3(xc + bx + (k2 * by),
                       yc + by - (k2 * bx));

        auto *curve = new Inkscape::CanvasItemCurve(_desktop->getCanvasTemp(), p1, p2, p3, p4);
        curve->set_name("CanvasItemCurve:MeasureToolCurve");
        curve->set_stroke(Inkscape::CANVAS_ITEM_SECONDARY);
        curve->lower_to_bottom();
        curve->set_visible(true);
        if (to_phantom){
            curve->set_stroke(0x8888887f);
            measure_phantom_items.emplace_back(curve);
        } else {
            measure_tmp_items.emplace_back(curve);
        }

        if (measure_repr) {
            Geom::PathVector pathv;
            Geom::Path path;
            path.start(_desktop->doc2dt(p1));
            path.appendNew<Geom::CubicBezier>(_desktop->doc2dt(p2), _desktop->doc2dt(p3), _desktop->doc2dt(p4));
            pathv.push_back(path);
            auto layer = _desktop->layerManager().currentLayer();
            pathv *= layer->i2doc_affine().inverse();
            if (!pathv.empty()) {
                setMeasureItem(pathv, true, false, 0xff00007f, measure_repr);
            }
        }
    }
}

static std::optional<Geom::Point> explicit_base_tmp;

MeasureTool::MeasureTool(SPDesktop *desktop)
    : ToolBase(desktop, "/tools/measure", "measure.svg")
{
    start_p = readMeasurePoint(true);
    end_p = readMeasurePoint(false);

    // create the knots
    this->knot_start = new SPKnot(desktop, _("Measure start, <b>Shift+Click</b> for position dialog"),
                                  Inkscape::CANVAS_ITEM_CTRL_TYPE_POINT, "CanvasItemCtrl:MeasureTool");
    this->knot_start->updateCtrl();
    this->knot_start->moveto(start_p);
    this->knot_start->show();

    this->knot_end = new SPKnot(desktop, _("Measure end, <b>Shift+Click</b> for position dialog"),
                                Inkscape::CANVAS_ITEM_CTRL_TYPE_POINT, "CanvasItemCtrl:MeasureTool");
    this->knot_end->updateCtrl();
    this->knot_end->moveto(end_p);
    this->knot_end->show();

    showCanvasItems();

    this->_knot_start_moved_connection = this->knot_start->moved_signal.connect(sigc::mem_fun(*this, &MeasureTool::knotStartMovedHandler));
    this->_knot_start_click_connection = this->knot_start->click_signal.connect(sigc::mem_fun(*this, &MeasureTool::knotClickHandler));
    this->_knot_start_ungrabbed_connection = this->knot_start->ungrabbed_signal.connect(sigc::mem_fun(*this, &MeasureTool::knotUngrabbedHandler));
    this->_knot_end_moved_connection = this->knot_end->moved_signal.connect(sigc::mem_fun(*this, &MeasureTool::knotEndMovedHandler));
    this->_knot_end_click_connection = this->knot_end->click_signal.connect(sigc::mem_fun(*this, &MeasureTool::knotClickHandler));
    this->_knot_end_ungrabbed_connection = this->knot_end->ungrabbed_signal.connect(sigc::mem_fun(*this, &MeasureTool::knotUngrabbedHandler));
}

MeasureTool::~MeasureTool()
{
    enableGrDrag(false);
    ungrabCanvasEvents();

    // unref should call destroy
    SPKnot::unref(knot_start);
    SPKnot::unref(knot_end);
}

static char const *endpoint_to_pref(bool is_start)
{
    return is_start ? "/tools/measure/measure-start" : "/tools/measure/measure-end";
}

Geom::Point MeasureTool::readMeasurePoint(bool is_start) const
{
    return Preferences::get()->getPoint(endpoint_to_pref(is_start), Geom::Point(Geom::infinity(), Geom::infinity()));
}

void MeasureTool::writeMeasurePoint(Geom::Point point, bool is_start) const
{
    Preferences::get()->setPoint(endpoint_to_pref(is_start), point);
}

//This function is used to reverse the Measure, I do it in two steps because when
//we move the knot the start_ or the end_p are overwritten so I need the original values.
void MeasureTool::reverseKnots()
{
    auto const start = start_p;
    auto const end = end_p;
    knot_start->moveto(end);
    knot_start->show();
    knot_end->moveto(start);
    knot_end->show();
    start_p = end;
    end_p = start;
    showCanvasItems();
}

void MeasureTool::knotClickHandler(SPKnot *knot, guint state)
{
    if (state & GDK_SHIFT_MASK) {
        auto prefs = Preferences::get();
        auto const unit_name =  prefs->getString("/tools/measure/unit", "px");
        explicit_base = explicit_base_tmp;
        Dialog::KnotPropertiesDialog::showDialog(_desktop, knot, unit_name);
    }
}

void MeasureTool::knotStartMovedHandler(SPKnot */*knot*/, Geom::Point const &ppointer, guint state)
{
    Geom::Point point = this->knot_start->position();
    if (state & GDK_CONTROL_MASK) {
        spdc_endpoint_snap_rotation(this, point, end_p, state);
    } else if (!(state & GDK_SHIFT_MASK)) {
        SnapManager &snap_manager = _desktop->getNamedView()->snap_manager;
        snap_manager.setup(_desktop);
        Inkscape::SnapCandidatePoint scp(point, Inkscape::SNAPSOURCE_OTHER_HANDLE);
        scp.addOrigin(this->knot_end->position());
        Inkscape::SnappedPoint sp = snap_manager.freeSnap(scp);
        point = sp.getPoint();
        snap_manager.unSetup();
    }
    if(start_p != point) {
        start_p = point;
        this->knot_start->moveto(start_p);
    }
    showCanvasItems();
}

void MeasureTool::knotEndMovedHandler(SPKnot */*knot*/, Geom::Point const &ppointer, guint state)
{
    Geom::Point point = this->knot_end->position();
    if (state & GDK_CONTROL_MASK) {
        spdc_endpoint_snap_rotation(this, point, start_p, state);
    } else if (!(state & GDK_SHIFT_MASK)) {
        SnapManager &snap_manager = _desktop->getNamedView()->snap_manager;
        snap_manager.setup(_desktop);
        Inkscape::SnapCandidatePoint scp(point, Inkscape::SNAPSOURCE_OTHER_HANDLE);
        scp.addOrigin(this->knot_start->position());
        Inkscape::SnappedPoint sp = snap_manager.freeSnap(scp);
        point = sp.getPoint();
        snap_manager.unSetup();
    }
    if(end_p != point) {
        end_p = point;
        this->knot_end->moveto(end_p);
    }
    showCanvasItems();
}

void MeasureTool::knotUngrabbedHandler(SPKnot */*knot*/, unsigned state)
{
    knot_start->moveto(start_p);
    knot_end->moveto(end_p);
    showCanvasItems();
}

static void calculate_intersections(SPDesktop *desktop, SPItem *item, Geom::PathVector const &lineseg,
                                    SPCurve curve, std::vector<double> &intersections)
{
    curve.transform(item->i2doc_affine());
    // Find all intersections of the control-line with this shape
    Geom::CrossingSet cs = Geom::crossings(lineseg, curve.get_pathvector());
    Geom::delete_duplicates(cs[0]);

    // Reconstruct and store the points of intersection
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    bool show_hidden = prefs->getBool("/tools/measure/show_hidden", true);
    for (const auto & m : cs[0]) {
        if (!show_hidden) {
            double eps = 0.0001;
            if ((m.ta > eps &&
             item == desktop->getItemAtPoint(desktop->d2w(desktop->dt2doc(lineseg[0].pointAt(m.ta - eps))), true, nullptr)) ||
            (m.ta + eps < 1 &&
             item == desktop->getItemAtPoint(desktop->d2w(desktop->dt2doc(lineseg[0].pointAt(m.ta + eps))), true, nullptr))) {
                intersections.push_back(m.ta);
            }
        } else {
            intersections.push_back(m.ta);
        }
    }
}

bool MeasureTool::root_handler(CanvasEvent const &event)
{
    bool ret = false;

    inspect_event(event,
    [&] (ButtonPressEvent const &event) {
        if (event.num_press != 1 || event.button != 1) {
            return;
        }
        knot_start->hide();
        knot_end->hide();
        explicit_base = {};
        explicit_base_tmp = {};
        last_end = {};

        saveDragOrigin(event.pos);
        start_p = _desktop->w2d(event.pos);

        auto &snap_manager = _desktop->getNamedView()->snap_manager;
        snap_manager.setup(_desktop);
        snap_manager.freeSnapReturnByRef(start_p, SNAPSOURCE_OTHER_HANDLE);
        snap_manager.unSetup();

        grabCanvasEvents(EventType::KEY_PRESS      |
                         EventType::KEY_RELEASE    |
                         EventType::BUTTON_PRESS   |
                         EventType::BUTTON_RELEASE |
                         EventType::MOTION);
        ret = true;
    },
    [&] (KeyPressEvent const &event) {
        if (event.keyval == GDK_KEY_Control_L || event.keyval == GDK_KEY_Control_R) {
            explicit_base_tmp = explicit_base;
            explicit_base = end_p;
            showInfoBox(last_pos, true);
        }
        if ((event.modifiers & GDK_ALT_MASK ) && ((event.keyval == GDK_KEY_c) || (event.keyval == GDK_KEY_C))) {
            copyToClipboard();
            ret = true;
        }
    },
    [&] (KeyReleaseEvent const &event) {
        if (event.keyval == GDK_KEY_Control_L || event.keyval == GDK_KEY_Control_R) {
            showInfoBox(last_pos, false);
        }
    },
    [&] (MotionEvent const &event) {
        if (!(event.modifiers & GDK_BUTTON1_MASK)) {
            if (!(event.modifiers & GDK_SHIFT_MASK)) {
                auto const motion_dt = _desktop->w2d(event.pos);

                auto &snap_manager = _desktop->getNamedView()->snap_manager;
                snap_manager.setup(_desktop);

                auto scp = SnapCandidatePoint(motion_dt, SNAPSOURCE_OTHER_HANDLE);
                scp.addOrigin(start_p);

                snap_manager.preSnap(scp);
                snap_manager.unSetup();
            }
            last_pos = event.pos;
            showInfoBox(last_pos, event.modifiers & GDK_CONTROL_MASK);
        } else {
            if (!checkDragMoved(event.pos)) {
                return;
            }

            auto prefs = Preferences::get();
            tolerance = prefs->getIntLimited("/options/dragtolerance/value", 0, 0, 100);

            measure_item.clear();

            if (!last_end || Geom::LInfty(event.pos - *last_end) > tolerance / 4.0) {
                auto const motion_dt = _desktop->w2d(event.pos);
                end_p = motion_dt;

                if (event.modifiers & GDK_CONTROL_MASK) {
                    spdc_endpoint_snap_rotation(this, end_p, start_p, event.modifiers);
                } else if (!(event.modifiers & GDK_SHIFT_MASK)) {
                    auto &snap_manager = _desktop->getNamedView()->snap_manager;
                    snap_manager.setup(_desktop);
                    auto scp = SnapCandidatePoint(end_p, SNAPSOURCE_OTHER_HANDLE);
                    scp.addOrigin(start_p);
                    auto const sp = snap_manager.freeSnap(scp);
                    end_p = sp.getPoint();
                    snap_manager.unSetup();
                }
                showCanvasItems();
                last_end = event.pos;
            }
            gobble_motion_events(GDK_BUTTON1_MASK);

            ret = true;
        }
    },
    [&] (ButtonReleaseEvent const &event) {
        if (event.button != 1) {
            return;
        }
        knot_start->moveto(start_p);
        knot_start->show();
        if (last_end) {
            end_p = _desktop->w2d(*last_end);
            if (event.modifiers & GDK_CONTROL_MASK) {
                spdc_endpoint_snap_rotation(this, end_p, start_p, event.modifiers);
            } else if (!(event.modifiers & GDK_SHIFT_MASK)) {
                auto &snap_manager = _desktop->getNamedView()->snap_manager;
                snap_manager.setup(_desktop);
                auto scp = SnapCandidatePoint(end_p, SNAPSOURCE_OTHER_HANDLE);
                scp.addOrigin(start_p);
                auto const sp = snap_manager.freeSnap(scp);
                end_p = sp.getPoint();
                snap_manager.unSetup();
            }
        }
        knot_end->moveto(end_p);
        knot_end->show();
        showCanvasItems();

        ungrabCanvasEvents();
    },
    [&] (CanvasEvent const &event) {}
    );

    return ret || ToolBase::root_handler(event);
}

void MeasureTool::setMarkers()
{
    SPDocument *doc = _desktop->getDocument();
    SPObject *arrowStart = doc->getObjectById("Arrow2Sstart");
    SPObject *arrowEnd = doc->getObjectById("Arrow2Send");
    if (!arrowStart) {
        setMarker(true);
    }
    if (!arrowEnd) {
        setMarker(false);
    }
}

void MeasureTool::setMarker(bool isStart)
{
    SPDocument *doc = _desktop->getDocument();
    SPDefs *defs = doc->getDefs();
    Inkscape::XML::Node *rmarker;
    Inkscape::XML::Document *xml_doc = doc->getReprDoc();
    rmarker = xml_doc->createElement("svg:marker");
    rmarker->setAttribute("id", isStart ? "Arrow2Sstart" : "Arrow2Send");
    rmarker->setAttribute("inkscape:isstock", "true");
    rmarker->setAttribute("inkscape:stockid", isStart ? "Arrow2Sstart" : "Arrow2Send");
    rmarker->setAttribute("orient", "auto");
    rmarker->setAttribute("refX", "0.0");
    rmarker->setAttribute("refY", "0.0");
    rmarker->setAttribute("style", "overflow:visible;");
    auto marker = cast<SPItem>(defs->appendChildRepr(rmarker));
    Inkscape::GC::release(rmarker);
    marker->updateRepr();
    Inkscape::XML::Node *rpath;
    rpath = xml_doc->createElement("svg:path");
    rpath->setAttribute("d", "M 8.72,4.03 L -2.21,0.02 L 8.72,-4.00 C 6.97,-1.63 6.98,1.62 8.72,4.03 z");
    rpath->setAttribute("id", isStart ? "Arrow2SstartPath" : "Arrow2SendPath");
    SPCSSAttr *css = sp_repr_css_attr_new();
    sp_repr_css_set_property (css, "stroke", "none");
    sp_repr_css_set_property (css, "fill", "#000000");
    sp_repr_css_set_property (css, "fill-opacity", "1");
    Glib::ustring css_str;
    sp_repr_css_write_string(css,css_str);
    rpath->setAttribute("style", css_str);
    sp_repr_css_attr_unref (css);
    rpath->setAttribute("transform", isStart ? "scale(0.3) translate(-2.3,0)" : "scale(0.3) rotate(180) translate(-2.3,0)");
    auto path = cast<SPItem>(marker->appendChildRepr(rpath));
    Inkscape::GC::release(rpath);
    path->updateRepr();
}

void MeasureTool::toGuides()
{
    if (!_desktop || !start_p.isFinite() || !end_p.isFinite() || start_p == end_p) {
        return;
    }
    SPDocument *doc = _desktop->getDocument();
    Geom::Point start = _desktop->doc2dt(start_p) * _desktop->doc2dt();
    Geom::Point end = _desktop->doc2dt(end_p) * _desktop->doc2dt();
    Geom::Ray ray(start,end);
    SPNamedView *namedview = _desktop->getNamedView();
    if(!namedview) {
        return;
    }
    setGuide(start,ray.angle(), _("Measure"));
    if(explicit_base) {
        auto layer = _desktop->layerManager().currentLayer();
        explicit_base = *explicit_base * layer->i2doc_affine().inverse();
        ray.setPoints(start, *explicit_base);
        if(ray.angle() != 0) {
            setGuide(start,ray.angle(), _("Base"));
        }
    }
    setGuide(start,0,"");
    setGuide(start,Geom::rad_from_deg(90),_("Start"));
    setGuide(end,0,_("End"));
    setGuide(end,Geom::rad_from_deg(90),"");
    showCanvasItems(true);
    doc->ensureUpToDate();
    DocumentUndo::done(_desktop->getDocument(), _("Add guides from measure tool"), INKSCAPE_ICON("tool-measure"));
}

void MeasureTool::toPhantom()
{
    if (!_desktop || !start_p.isFinite() || !end_p.isFinite() || start_p == end_p) {
        return;
    }
    SPDocument *doc = _desktop->getDocument();

    measure_phantom_items.clear();
    measure_tmp_items.clear();

    showCanvasItems(false, false, true);
    doc->ensureUpToDate();
    DocumentUndo::done(_desktop->getDocument(), _("Keep last measure on the canvas, for reference"), INKSCAPE_ICON("tool-measure"));
}

void MeasureTool::toItem()
{
    if (!_desktop || !start_p.isFinite() || !end_p.isFinite() || start_p == end_p) {
        return;
    }
    SPDocument *doc = _desktop->getDocument();
    Geom::Ray ray(start_p,end_p);
    guint32 line_color_primary = 0x0000ff7f;
    Inkscape::XML::Document *xml_doc = _desktop->doc()->getReprDoc();
    Inkscape::XML::Node *rgroup = xml_doc->createElement("svg:g");
    showCanvasItems(false, true, false, rgroup);
    setLine(start_p,end_p, false, line_color_primary, rgroup);
    auto measure_item = cast<SPItem>(_desktop->layerManager().currentLayer()->appendChildRepr(rgroup));
    Inkscape::GC::release(rgroup);
    measure_item->updateRepr();
    doc->ensureUpToDate();
    DocumentUndo::done(_desktop->getDocument(), _("Convert measure to items"), INKSCAPE_ICON("tool-measure"));
    reset();
}

void MeasureTool::toMarkDimension()
{
    if (!_desktop || !start_p.isFinite() || !end_p.isFinite() || start_p == end_p) {
        return;
    }
    SPDocument *doc = _desktop->getDocument();
    setMarkers();
    Geom::Ray ray(start_p,end_p);
    Geom::Point start = start_p + Geom::Point::polar(ray.angle(), 5);
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    dimension_offset = prefs->getDouble("/tools/measure/offset", 5.0);
    start = start + Geom::Point::polar(ray.angle() + Geom::rad_from_deg(90), -dimension_offset);
    Geom::Point end = end_p + Geom::Point::polar(ray.angle(), -5);
    end = end+ Geom::Point::polar(ray.angle() + Geom::rad_from_deg(90), -dimension_offset);
    guint32 color = 0x000000ff;
    setLine(start, end, true, color);
    Glib::ustring unit_name = prefs->getString("/tools/measure/unit");
    if (!unit_name.compare("")) {
        unit_name = DEFAULT_UNIT_NAME;
    }
    double fontsize = prefs->getDouble("/tools/measure/fontsize", 10.0);

    Geom::Point middle = Geom::middle_point(start, end);
    double totallengthval = (end_p - start_p).length();
    totallengthval = Inkscape::Util::Quantity::convert(totallengthval, "px", unit_name);
    double scale = prefs->getDouble("/tools/measure/scale", 100.0) / 100.0;

    int precision = prefs->getInt("/tools/measure/precision", 2);
    Glib::ustring total = Inkscape::ustring::format_classic(std::fixed, std::setprecision(precision), totallengthval * scale);
    total += unit_name;

    double textangle = Geom::rad_from_deg(180) - ray.angle();
    if (_desktop->is_yaxisdown()) {
        textangle = ray.angle() - Geom::rad_from_deg(180);
    }

    setLabelText(total, middle, fontsize, textangle, color);

    doc->ensureUpToDate();
    DocumentUndo::done(_desktop->getDocument(), _("Add global measure line"), INKSCAPE_ICON("tool-measure"));
}

void MeasureTool::setGuide(Geom::Point origin, double angle, const char *label)
{
    SPDocument *doc = _desktop->getDocument();
    Inkscape::XML::Document *xml_doc = doc->getReprDoc();
    SPRoot const *root = doc->getRoot();
    Geom::Affine affine(Geom::identity());
    if(root) {
        affine *= root->c2p.inverse();
    }
    SPNamedView *namedview = _desktop->getNamedView();
    if(!namedview) {
        return;
    }

    // <sodipodi:guide> stores inverted y-axis coordinates
    if (_desktop->is_yaxisdown()) {
        origin[Geom::Y] = doc->getHeight().value("px") - origin[Geom::Y];
        angle *= -1.0;
    }

    origin *= affine;
    //measure angle
    Inkscape::XML::Node *guide;
    guide = xml_doc->createElement("sodipodi:guide");
    std::stringstream position;
    position.imbue(std::locale::classic());
    position <<  origin[Geom::X] << "," << origin[Geom::Y];
    guide->setAttribute("position", position.str() );
    guide->setAttribute("inkscape:color", "rgb(167,0,255)");
    guide->setAttribute("inkscape:label", label);
    Geom::Point unit_vector = Geom::rot90(origin.polar(angle));
    std::stringstream angle_str;
    angle_str.imbue(std::locale::classic());
    angle_str << unit_vector[Geom::X] << "," << unit_vector[Geom::Y];
    guide->setAttribute("orientation", angle_str.str());
    namedview->appendChild(guide);
    Inkscape::GC::release(guide);
}

void MeasureTool::setLine(Geom::Point start_point,Geom::Point end_point, bool markers, guint32 color, Inkscape::XML::Node *measure_repr)
{
    if (!_desktop || !start_p.isFinite() || !end_p.isFinite()) {
        return;
    }
    Geom::PathVector pathv;
    Geom::Path path;
    path.start(_desktop->doc2dt(start_point));
    path.appendNew<Geom::LineSegment>(_desktop->doc2dt(end_point));
    pathv.push_back(path);
    pathv *= _desktop->layerManager().currentLayer()->i2doc_affine().inverse();
    if(!pathv.empty()) {
        setMeasureItem(pathv, false, markers, color, measure_repr);
    }
}

void MeasureTool::setPoint(Geom::Point origin, Inkscape::XML::Node *measure_repr)
{
    if (!_desktop || !origin.isFinite()) {
        return;
    }
    char const * svgd;
    svgd = "m 0.707,0.707 6.586,6.586 m 0,-6.586 -6.586,6.586";
    Geom::PathVector pathv = sp_svg_read_pathv(svgd);
    Geom::Scale scale = Geom::Scale(_desktop->current_zoom()).inverse();
    pathv *= Geom::Translate(Geom::Point(-3.5,-3.5));
    pathv *= scale;
    pathv *= Geom::Translate(Geom::Point() - (scale.vector() * 0.5));
    pathv *= Geom::Translate(_desktop->doc2dt(origin));
    pathv *= _desktop->layerManager().currentLayer()->i2doc_affine().inverse();
    if (!pathv.empty()) {
        guint32 line_color_secondary = 0xff0000ff;
        setMeasureItem(pathv, false, false, line_color_secondary, measure_repr);
    }
}

void MeasureTool::setLabelText(Glib::ustring const &value, Geom::Point pos, double fontsize, Geom::Coord angle,
                               guint32 background, Inkscape::XML::Node *measure_repr)
{
    Inkscape::XML::Document *xml_doc = _desktop->doc()->getReprDoc();
    /* Create <text> */
    pos = _desktop->doc2dt(pos);
    Inkscape::XML::Node *rtext = xml_doc->createElement("svg:text");
    rtext->setAttribute("xml:space", "preserve");


    /* Set style */
    sp_desktop_apply_style_tool(_desktop, rtext, "/tools/text", true);
    if(measure_repr) {
        rtext->setAttributeSvgDouble("x", 2);
        rtext->setAttributeSvgDouble("y", 2);
    } else {
        rtext->setAttributeSvgDouble("x", 0);
        rtext->setAttributeSvgDouble("y", 0);
    }

    /* Create <tspan> */
    Inkscape::XML::Node *rtspan = xml_doc->createElement("svg:tspan");
    rtspan->setAttribute("sodipodi:role", "line");
    SPCSSAttr *css = sp_repr_css_attr_new();
    std::stringstream font_size;
    font_size.imbue(std::locale::classic());
    if(measure_repr) {
        font_size <<  fontsize;
    } else {
        font_size <<  fontsize << "pt";
    }
    sp_repr_css_set_property (css, "font-size", font_size.str().c_str());
    sp_repr_css_set_property (css, "font-style", "normal");
    sp_repr_css_set_property (css, "font-weight", "normal");
    sp_repr_css_set_property (css, "line-height", "125%");
    sp_repr_css_set_property (css, "letter-spacing", "0");
    sp_repr_css_set_property (css, "word-spacing", "0");
    sp_repr_css_set_property (css, "text-align", "center");
    sp_repr_css_set_property (css, "text-anchor", "middle");
    if(measure_repr) {
        sp_repr_css_set_property (css, "fill", "#FFFFFF");
    } else {
        sp_repr_css_set_property (css, "fill", "#000000");
    }
    sp_repr_css_set_property (css, "fill-opacity", "1");
    sp_repr_css_set_property (css, "stroke", "none");
    Glib::ustring css_str;
    sp_repr_css_write_string(css,css_str);
    rtspan->setAttribute("style", css_str);
    sp_repr_css_attr_unref (css);
    rtext->addChild(rtspan, nullptr);
    Inkscape::GC::release(rtspan);
    /* Create TEXT */
    Inkscape::XML::Node *rstring = xml_doc->createTextNode(value.c_str());
    rtspan->addChild(rstring, nullptr);
    Inkscape::GC::release(rstring);
    auto layer = _desktop->layerManager().currentLayer();
    auto text_item = cast<SPText>(layer->appendChildRepr(rtext));
    Inkscape::GC::release(rtext);
    text_item->rebuildLayout();
    text_item->updateRepr();
    Geom::OptRect bbox = text_item->geometricBounds();
    if (!measure_repr && bbox) {
        Geom::Point center = bbox->midpoint();
        text_item->transform *= Geom::Translate(center).inverse();
        pos += Geom::Point::polar(angle+ Geom::rad_from_deg(90), -bbox->height());
    }
    if (measure_repr) {
        /* Create <group> */
        Inkscape::XML::Node *rgroup = xml_doc->createElement("svg:g");
        /* Create <rect> */
        Inkscape::XML::Node *rrect = xml_doc->createElement("svg:rect");
        SPCSSAttr *css = sp_repr_css_attr_new ();
        sp_repr_css_set_property_string(css, "fill", Inkscape::Colors::rgba_to_hex(background));
        sp_repr_css_set_property_double(css, "fill-opacity", 0.5);
        sp_repr_css_set_property (css, "stroke-width", "0");
        Glib::ustring css_str;
        sp_repr_css_write_string(css,css_str);
        rrect->setAttribute("style", css_str);
        sp_repr_css_attr_unref (css);
        rgroup->setAttributeSvgDouble("x", 0);
        rgroup->setAttributeSvgDouble("y", 0);
        rrect->setAttributeSvgDouble("x", -bbox->width()/2.0);
        rrect->setAttributeSvgDouble("y", -bbox->height());
        rrect->setAttributeSvgDouble("width", bbox->width() + 6);
        rrect->setAttributeSvgDouble("height", bbox->height() + 6);
        Inkscape::XML::Node *rtextitem = text_item->getRepr();
        text_item->deleteObject();
        rgroup->addChild(rtextitem, nullptr);
        Inkscape::GC::release(rtextitem);
        rgroup->addChild(rrect, nullptr);
        Inkscape::GC::release(rrect);
        auto text_item_box = cast<SPItem>(layer->appendChildRepr(rgroup));
        Geom::Scale scale = Geom::Scale(_desktop->current_zoom()).inverse();
        if(bbox) {
            text_item_box->transform *= Geom::Translate(bbox->midpoint() - Geom::Point(1.0,1.0)).inverse();
        }
        text_item_box->transform *= scale;
        text_item_box->transform *= Geom::Translate(Geom::Point() - (scale.vector() * 0.5));
        text_item_box->transform *= Geom::Translate(pos);
        text_item_box->transform *= layer->i2doc_affine().inverse();
        text_item_box->updateRepr();
        text_item_box->doWriteTransform(text_item_box->transform, nullptr, true);
        Inkscape::XML::Node *rlabel = text_item_box->getRepr();
        text_item_box->deleteObject();
        measure_repr->addChild(rlabel, nullptr);
        Inkscape::GC::release(rlabel);
    } else {
        text_item->transform *= Geom::Rotate(angle);
        text_item->transform *= Geom::Translate(pos);
        text_item->transform *= layer->i2doc_affine().inverse();
        text_item->doWriteTransform(text_item->transform, nullptr, true);
    }
}

void MeasureTool::reset()
{
    knot_start->hide();
    knot_end->hide();

    measure_tmp_items.clear();
}

void MeasureTool::setMeasureCanvasText(bool is_angle, double precision, double amount, double fontsize,
                                       Glib::ustring unit_name, Geom::Point position, guint32 background,
                                       bool to_left, bool to_item,
                                       bool to_phantom, Inkscape::XML::Node *measure_repr, Glib::ustring label)
{
    Glib::ustring measure = Inkscape::ustring::format_classic(std::setprecision(precision), std::fixed, amount);
    measure += " ";
    measure += (is_angle ? "°" : unit_name);
    if (!label.empty()) { measure = label + ": " + measure; }
    auto canvas_tooltip = new Inkscape::CanvasItemText(_desktop->getCanvasTemp(), position, measure);
    canvas_tooltip->set_fontsize(fontsize);
    canvas_tooltip->set_fill(0xffffffff);
    canvas_tooltip->set_background(background);
    if (to_left) {
        canvas_tooltip->set_anchor(Geom::Point(0, 0.5));
    } else {
        canvas_tooltip->set_anchor(Geom::Point(0.5, 0.5));
    }

    if (to_phantom){
        canvas_tooltip->set_background(0x4444447f);
        measure_phantom_items.emplace_back(canvas_tooltip);
    } else {
        measure_tmp_items.emplace_back(canvas_tooltip);
    }

    if (to_item) {
        setLabelText(measure, position, fontsize, 0, background, measure_repr);
    }

    canvas_tooltip->set_visible(true);
}

void MeasureTool::setMeasureCanvasItem(Geom::Point position, bool to_item, bool to_phantom, XML::Node *measure_repr)
{
    uint32_t color = 0xff0000ff;
    if (to_phantom) {
        color = 0x888888ff;
    }

    auto canvas_item = new Inkscape::CanvasItemCtrl(_desktop->getCanvasTemp(), Inkscape::CANVAS_ITEM_CTRL_TYPE_MARKER, position);
    canvas_item->lower_to_bottom();
    canvas_item->set_pickable(false);
    canvas_item->set_visible(true);

    (to_phantom ? measure_phantom_items : measure_tmp_items).emplace_back(std::move(canvas_item));

    if (to_item) {
        setPoint(position, measure_repr);
    }
}

void MeasureTool::setMeasureCanvasControlLine(Geom::Point start, Geom::Point end, bool to_item, bool to_phantom,
                                              CanvasItemColor ctrl_line_type,
                                              XML::Node *measure_repr)
{
    uint32_t color = ctrl_line_type == CANVAS_ITEM_PRIMARY ? 0x0000ff7f : 0xff00007f;
    if (to_phantom) {
        color = ctrl_line_type == CANVAS_ITEM_PRIMARY ? 0x4444447f : 0x8888887f;
    }

    auto control_line = make_canvasitem<CanvasItemCurve>(_desktop->getCanvasTemp(), start, end);
    control_line->set_stroke(color);
    control_line->lower_to_bottom();
    control_line->set_visible(true);

    (to_phantom ? measure_phantom_items : measure_tmp_items).emplace_back(std::move(control_line));

    if (to_item) {
        setLine(start, end, false, color, measure_repr);
    }
}

// This is the text that follows the cursor around.
void MeasureTool::showItemInfoText(Geom::Point pos, Glib::ustring const &measure_str, double fontsize)
{
    auto canvas_tooltip = make_canvasitem<CanvasItemText>(_desktop->getCanvasTemp(), pos, measure_str);
    canvas_tooltip->set_fontsize(fontsize);
    canvas_tooltip->set_fill(0xffffffff);
    canvas_tooltip->set_background(0x00000099);
    canvas_tooltip->set_anchor(Geom::Point());
    canvas_tooltip->set_fixed_line(true);
    canvas_tooltip->set_visible(true);
    measure_item.emplace_back(std::move(canvas_tooltip));
}

void MeasureTool::showInfoBox(Geom::Point cursor, bool into_groups)
{
    using Inkscape::Util::Quantity;

    measure_item.clear();

    auto newover = _desktop->getItemAtPoint(cursor, into_groups);
    if (!newover) {
        // Clear over when the cursor isn't over anything.
        over = nullptr;
        clipBMeas.unsetShapeMeasures(); // shape measurements are not set and will not be copied to the clipboard
        return;
    }
    auto unit = _desktop->getNamedView()->getDisplayUnit();

    // Load preferences for measuring the new object.
    auto prefs = Preferences::get();
    int precision = prefs->getInt("/tools/measure/precision", 2);
    bool selected = prefs->getBool("/tools/measure/only_selected", false);
    auto box_type = prefs->getBool("/tools/bounding_box", false) ? SPItem::GEOMETRIC_BBOX : SPItem::VISUAL_BBOX;
    double fontsize = prefs->getDouble("/tools/measure/fontsize", 10.0);
    double scale    = prefs->getDouble("/tools/measure/scale", 100.0) / 100.0;
    Glib::ustring unit_name = prefs->getString("/tools/measure/unit", unit->abbr);

    auto const zoom = Geom::Scale(Quantity::convert(_desktop->current_zoom(), "px", unit->abbr)).inverse();

    if (newover != over) {
        // Get information for the item, and cache it to save time.
        over = newover;
        auto affine = over->i2dt_affine() * Geom::Scale(scale);
        // Correct for the current page's position.
        if (_desktop->getDocument()->get_origin_follows_page()) {
            affine *= _desktop->getDocument()->getPageManager().getSelectedPageAffine().inverse();
        }
        if (auto bbox = over->bounds(box_type, affine)) {
            item_width  = Quantity::convert(bbox->width(), "px", unit_name);
            item_height = Quantity::convert(bbox->height(), "px", unit_name);
            item_x      = Quantity::convert(bbox->left(), "px", unit_name);
            item_y      = Quantity::convert(bbox->top(), "px", unit_name);

            if (auto shape = cast<SPShape>(over)) {
                auto pw = paths_to_pw(shape->curve()->get_pathvector());
                item_length = Quantity::convert(Geom::length(pw * affine), "px", unit_name);
            }
        }
    }

    gchar *measure_str = nullptr;
    std::stringstream precision_str;
    precision_str.imbue(std::locale::classic());
    double origin = Quantity::convert(14, "px", unit->abbr);
    double yaxis_shift = Quantity::convert(fontsize, "px", unit->abbr);
    Geom::Point rel_position = Geom::Point(origin, origin + yaxis_shift);
    /* Keeps infobox just above the cursor */
    Geom::Point pos = _desktop->w2d(cursor);
    double gap = Quantity::convert(7 + fontsize, "px", unit->abbr);
    double yaxisdir = _desktop->yaxisdir();

    if (selected) {
        showItemInfoText(pos - (yaxisdir * Geom::Point(0, rel_position[Geom::Y]) * zoom), _desktop->getSelection()->includes(over) ? _("Selected") : _("Not selected"), fontsize);
        rel_position = Geom::Point(rel_position[Geom::X], rel_position[Geom::Y] + gap);
    }

    if (is<SPShape>(over)) {

        precision_str << _("Length") <<  ": %." << precision << "f %s";
        measure_str = g_strdup_printf(precision_str.str().c_str(), item_length, unit_name.c_str());
        precision_str.str("");
        showItemInfoText(pos - (yaxisdir * Geom::Point(0, rel_position[Geom::Y]) * zoom), measure_str, fontsize);
        rel_position = Geom::Point(rel_position[Geom::X], rel_position[Geom::Y] + gap);

    } else if (is<SPGroup>(over)) {

        measure_str = _("Press 'CTRL' to measure into group");
        showItemInfoText(pos - (yaxisdir * Geom::Point(0, rel_position[Geom::Y]) * zoom), measure_str, fontsize);
        rel_position = Geom::Point(rel_position[Geom::X], rel_position[Geom::Y] + gap);

    }

    precision_str <<  "Y: %." << precision << "f %s";
    measure_str = g_strdup_printf(precision_str.str().c_str(), item_y, unit_name.c_str());
    precision_str.str("");
    showItemInfoText(pos - (yaxisdir * Geom::Point(0, rel_position[Geom::Y]) * zoom), measure_str, fontsize);
    rel_position = Geom::Point(rel_position[Geom::X], rel_position[Geom::Y] + gap);

    precision_str <<  "X: %." << precision << "f %s";
    measure_str = g_strdup_printf(precision_str.str().c_str(), item_x, unit_name.c_str());
    precision_str.str("");
    showItemInfoText(pos - (yaxisdir * Geom::Point(0, rel_position[Geom::Y]) * zoom), measure_str, fontsize);
    rel_position = Geom::Point(rel_position[Geom::X], rel_position[Geom::Y] + gap);

    precision_str << _("Height") << ": %." << precision << "f %s";
    measure_str = g_strdup_printf(precision_str.str().c_str(), item_height, unit_name.c_str());
    precision_str.str("");
    showItemInfoText(pos - (yaxisdir * Geom::Point(0, rel_position[Geom::Y]) * zoom), measure_str, fontsize);
    rel_position = Geom::Point(rel_position[Geom::X], rel_position[Geom::Y] + gap);

    precision_str << _("Width") << ": %." << precision << "f %s";
    measure_str = g_strdup_printf(precision_str.str().c_str(), item_width, unit_name.c_str());
    precision_str.str("");
    showItemInfoText(pos - (yaxisdir * Geom::Point(0, rel_position[Geom::Y]) * zoom), measure_str, fontsize);
    g_free(measure_str);

    clipBMeas.lengths[MT::LengthIDs::SHAPE_LENGTH] = item_length; // will be copied to the clipboard 
    clipBMeas.lengths[MT::LengthIDs::SHAPE_WIDTH] = item_width;
    clipBMeas.lengths[MT::LengthIDs::SHAPE_HEIGHT] = item_height;
    clipBMeas.lengths[MT::LengthIDs::SHAPE_X] = item_x;
    clipBMeas.lengths[MT::LengthIDs::SHAPE_Y] = item_y;
    clipBMeas.measureIsSet[MT::LengthIDs::SHAPE_LENGTH] = true;
    clipBMeas.measureIsSet[MT::LengthIDs::SHAPE_WIDTH] = true;
    clipBMeas.measureIsSet[MT::LengthIDs::SHAPE_HEIGHT] = true;
    clipBMeas.measureIsSet[MT::LengthIDs::SHAPE_X] = true;
    clipBMeas.measureIsSet[MT::LengthIDs::SHAPE_Y] = true;

}

void MeasureTool::showCanvasItems(bool to_guides, bool to_item, bool to_phantom, Inkscape::XML::Node *measure_repr)
{
    if (!_desktop || !start_p.isFinite() || !end_p.isFinite() || start_p == end_p) {
        return;
    }
    writeMeasurePoint(start_p, true);
    writeMeasurePoint(end_p, false);

    //clear previous canvas items, we'll draw new ones
    measure_tmp_items.clear();

    //TODO:Calculate the measure area for current length and origin
    // and use canvas->redraw_all(). In the calculation need a gap for outside text
    // maybe this remove the trash lines on measure use
    auto prefs = Preferences::get();
    bool show_in_between = prefs->getBool("/tools/measure/show_in_between", true);
    bool all_layers = prefs->getBool("/tools/measure/all_layers", true);
    dimension_offset = 70;
    Geom::PathVector lineseg;
    Geom::Path p;
    Geom::Point start_p_doc = start_p * _desktop->dt2doc();
    Geom::Point end_p_doc = end_p * _desktop->dt2doc();
    p.start(start_p_doc);
    p.appendNew<Geom::LineSegment>(end_p_doc);
    lineseg.push_back(p);

    double angle = atan2(end_p - start_p);
    double baseAngle = 0;

    if (explicit_base) {
        baseAngle = atan2(*explicit_base - start_p);
        angle -= baseAngle;

        // make sure that the angle is between -pi and pi.
        if (angle > M_PI) {
            angle -= 2 * M_PI;
        }
        if (angle < -M_PI) {
            angle += 2 * M_PI;
        }
    }

    std::vector<SPItem*> items;
    SPDocument *doc = _desktop->getDocument();
    Geom::Rect rect(start_p_doc, end_p_doc);
    items = doc->getItemsPartiallyInBox(_desktop->dkey, rect, false, true, false, true);
    SPGroup *current_layer = _desktop->layerManager().currentLayer();

    std::vector<double> intersection_times;
    bool only_selected = prefs->getBool("/tools/measure/only_selected", false);
    for (auto i : items) {
        SPItem *item = i;
        if (!_desktop->getSelection()->includes(i) && only_selected) {
            continue;
        }
        if (all_layers || _desktop->layerManager().layerForObject(item) == current_layer) {
            if (auto e = cast<SPGenericEllipse>(item)) { // this fixes a bug with the calculation of the intersection on
                e->set_shape();                          // ellipses and circles. If the calculate_intersections(...) is fixed
                                                         // then this if() can be removed
                Geom::PathVector new_pv = pathv_to_linear_and_cubic_beziers(e->curve()->get_pathvector());
                calculate_intersections(_desktop, item, lineseg, SPCurve(new_pv), intersection_times);
            } else if (auto shape = cast<SPShape>(item)) {
                calculate_intersections(_desktop, item, lineseg, *shape->curve(), intersection_times);
            } else {
                if (is<SPText>(item) || is<SPFlowtext>(item)) {
                    Inkscape::Text::Layout::iterator iter = te_get_layout(item)->begin();
                    do {
                        Inkscape::Text::Layout::iterator iter_next = iter;
                        iter_next.nextGlyph(); // iter_next is one glyph ahead from iter
                        if (iter == iter_next) {
                            break;
                        }

                        // get path from iter to iter_next:
                        auto curve = te_get_layout(item)->convertToCurves(iter, iter_next);
                        iter = iter_next; // shift to next glyph
                        if (curve.is_empty()) { // whitespace glyph?
                            continue;
                        }

                        calculate_intersections(_desktop, item, lineseg, std::move(curve), intersection_times);
                        if (iter == te_get_layout(item)->end()) {
                            break;
                        }
                    } while (true);
                }
            }
        }
    }
    Glib::ustring unit_name = prefs->getString("/tools/measure/unit");
    if (!unit_name.compare("")) {
        unit_name = DEFAULT_UNIT_NAME;
    }
    double scale = prefs->getDouble("/tools/measure/scale", 100.0) / 100.0;
    double fontsize = prefs->getDouble("/tools/measure/fontsize", 10.0);
    // Normal will be used for lines and text
    Geom::Point windowNormal = Geom::unit_vector(Geom::rot90(_desktop->d2w(end_p - start_p)));
    Geom::Point normal = _desktop->w2d(windowNormal);

    std::vector<Geom::Point> intersections;
    std::sort(intersection_times.begin(), intersection_times.end());
    for (double & intersection_time : intersection_times) {
        intersections.push_back(lineseg[0].pointAt(intersection_time));
    }

    if(!show_in_between && intersection_times.size() > 1) {
        Geom::Point start = lineseg[0].pointAt(intersection_times[0]);
        Geom::Point end = lineseg[0].pointAt(intersection_times[intersection_times.size()-1]);
        intersections.clear();
        intersections.push_back(start);
        intersections.push_back(end);
    }
    if (!prefs->getBool("/tools/measure/ignore_1st_and_last", true)) {
        intersections.insert(intersections.begin(),lineseg[0].pointAt(0));
        intersections.push_back(lineseg[0].pointAt(1));
    }
    int precision = prefs->getInt("/tools/measure/precision", 2);
    Glib::ustring MTSpath = prefs->getString("/tools/measure/MTSpath","");// path to the settings of the dialog
    bool showDeltas = false;                                 
    bool show_deltas_label = false;
    bool show_segments_label = false;
    double seg_min_len = 0.1;
    bool showAngle = true;
    if (!MTSpath.empty()){
        Glib::ustring pathStr = MTSpath;
        pathStr.append("/segments_min_length");
        seg_min_len = prefs->getDouble(pathStr.c_str(), 0.1);
        pathStr = MTSpath;
        pathStr.append("/show_segments_label");
        show_segments_label = prefs->getBool(pathStr.c_str(), false);
        pathStr = MTSpath;
        pathStr.append("/show_deltas_label");
        show_deltas_label = prefs->getBool(pathStr.c_str(), false);
        pathStr = MTSpath;
        pathStr.append("/show_deltas");
        showDeltas = prefs->getBool(pathStr.c_str(), false);
        pathStr = MTSpath;
        pathStr.append("/show_angle");
        showAngle = prefs->getBool(pathStr.c_str(), true);
    }
    int segIndex = 1;
    clipBMeas.segLengths.clear();
    std::vector<LabelPlacement> placements;
    for (size_t idx = 1; idx < intersections.size(); ++idx) {
        LabelPlacement placement;
        placement.lengthVal = (intersections[idx] - intersections[idx - 1]).length();
        placement.lengthVal = Inkscape::Util::Quantity::convert(placement.lengthVal, "px", unit_name);
        placement.offset = dimension_offset / 2;
        placement.start = _desktop->doc2dt((intersections[idx - 1] + intersections[idx]) / 2);
        placement.end = placement.start - (normal * placement.offset);
        if (placement.lengthVal > seg_min_len) { // trying to avoid 0length segments 
            placement.label = clipBMeas.symbols[MT::LengthIDs::SEGMENT] + std::to_string(segIndex);
            clipBMeas.segLengths[placement.label] = placement.lengthVal * scale; // will be copied to the clipboard
            clipBMeas.measureIsSet[MT::LengthIDs::SEGMENT] = true;
            placements.push_back(placement);
            segIndex++;
        }
    }

    // Adjust positions
    repositionOverlappingLabels(placements, _desktop, windowNormal, fontsize, precision);

    Geom::Point deltasBasePoint;                            // will use these to show lines later
    Geom::Point dXmidpos, dYmidpos, dXTextPos, dYTextPos;   //
    bool dX_is0, dY_is0;                                    //
    if (showDeltas) {
        Geom::Point dPoint = end_p - start_p;
        double dX = dPoint[Geom::X];
        double dY = dPoint[Geom::Y];
        dX_is0 = equalWithinRange(dX, 0, precision);
        dY_is0 = equalWithinRange(dY, 0, precision);
        if (!dX_is0 && !dY_is0) { // not showing deltas if either of them is 0 ...
            std::vector<Geom::Point> basePointinfo = calcDeltaBasePoint(dX, dY);  
            deltasBasePoint = basePointinfo[0];
            dXmidpos = basePointinfo[3];
            dYmidpos = basePointinfo[4];
            std::vector<LabelPlacement> allPlacements = placements;  // placements only has the segments
            if (placements.size() > 1) {  // between length
                LabelPlacement placement;
                placement.lengthVal = ((intersections[0] + normal * dimension_offset) -
                                        (intersections[intersections.size() - 1] + normal * dimension_offset)).length();
                placement.lengthVal = Inkscape::Util::Quantity::convert(placement.lengthVal, "px", unit_name);
                placement.offset = dimension_offset / 2;
                placement.start = _desktop->doc2dt(((intersections[0] + normal * dimension_offset) +
                                                    (intersections[intersections.size() - 1] + normal * dimension_offset)) / 2);
                placement.end = placement.start;  // this label is not displaced
                allPlacements.push_back(placement);
            }
            const int intdXdY = static_cast<int>(std::ceil(dX * dY / 2)); // averaging the number of chars from dX and dY
            int maxStrLength = (show_segments_label ? 3 : 0) + std::to_string(intdXdY).length() + precision + unit_name.length();
            dXTextPos = calcDeltaLabelTextPos(allPlacements, _desktop, dXmidpos, fontsize, unit_name, maxStrLength, basePointinfo[1], true);
            dYTextPos = calcDeltaLabelTextPos(allPlacements, _desktop, dYmidpos, fontsize, unit_name, maxStrLength, basePointinfo[2], false);
            dX = Inkscape::Util::Quantity::convert(dX, "px", unit_name);
            dY = Inkscape::Util::Quantity::convert(dY, "px", unit_name);
            double dYscaled = dY * scale;
            int dYstrLen = std::to_string(dYscaled).length();
            if (show_deltas_label) { dYstrLen += 3; }
            setMeasureCanvasText(false, precision, dX * scale, fontsize, unit_name, dXTextPos, 0x3333337f,
                false, to_item, to_phantom, measure_repr, (show_deltas_label ? clipBMeas.symbols[MT::LengthIDs::DX] : ""));
            setMeasureCanvasText(false, precision, dYscaled, fontsize, unit_name, dYTextPos - Geom::Point((dYstrLen * fontsize / 2),0),
                0x3333337f, false, to_item, to_phantom, measure_repr, (show_deltas_label ? clipBMeas.symbols[MT::LengthIDs::DY] : ""));
            clipBMeas.lengths[MT::LengthIDs::DX] = dX * scale; // will be copied to the clipboard
            clipBMeas.lengths[MT::LengthIDs::DY] = dYscaled;
            clipBMeas.measureIsSet[MT::LengthIDs::DX] = true;
            clipBMeas.measureIsSet[MT::LengthIDs::DY] = true;
        }
    } else { // measures are unset and will not be copied to the clipboard
        clipBMeas.measureIsSet[MT::LengthIDs::DX] = false;
        clipBMeas.measureIsSet[MT::LengthIDs::DY] = false;
    }

    for (auto & place : placements) {
        setMeasureCanvasText(false, precision, place.lengthVal * scale, fontsize, unit_name, place.end, 0x0000007f,
                             false, to_item, to_phantom, measure_repr, (show_segments_label ? place.label : ""));
    }
    Geom::Point angleDisplayPt = calcAngleDisplayAnchor(_desktop, angle, baseAngle, start_p, end_p, fontsize);
    if (showAngle) { // angleDisplayPt needs to be outside to be used below for the lines
        setMeasureCanvasText(true, precision, Geom::deg_from_rad(angle), fontsize, unit_name, angleDisplayPt, 0x337f337f,
                            false, to_item, to_phantom, measure_repr);
        clipBMeas.lengths[MT::LengthIDs::ANGLE] = Geom::deg_from_rad(angle); // will be copied to the clipboard
        clipBMeas.measureIsSet[MT::LengthIDs::ANGLE] = true;
    } else { // measure is unset and will not be copied to the clipboard
        clipBMeas.measureIsSet[MT::LengthIDs::ANGLE] = false;
    }

    {
        double totallengthval = (end_p - start_p).length();
        totallengthval = Inkscape::Util::Quantity::convert(totallengthval, "px", unit_name);
        Geom::Point origin = end_p + _desktop->w2d(Geom::Point(3 * fontsize, -fontsize));
        setMeasureCanvasText(false, precision, totallengthval * scale, fontsize, unit_name, origin, 0x3333337f,
                             true, to_item, to_phantom, measure_repr);
        clipBMeas.lengths[MT::LengthIDs::LENGTH] = totallengthval  * scale; // will be copied to the clipboard
        clipBMeas.measureIsSet[MT::LengthIDs::LENGTH] = true;
    }

    if (placements.size() > 1) {
        double totallengthval = (intersections[intersections.size()-1] - intersections[0]).length();
        totallengthval = Inkscape::Util::Quantity::convert(totallengthval, "px", unit_name);
        Geom::Point origin = _desktop->doc2dt((intersections[0] + intersections[intersections.size()-1])/2) + normal * dimension_offset;
        setMeasureCanvasText(false, precision, totallengthval * scale, fontsize, unit_name, origin, 0x33337f7f,
                             false, to_item, to_phantom, measure_repr);
        clipBMeas.lengths[MT::LengthIDs::LENGTH_BETWEEN] = totallengthval  * scale; // will be copied to the clipboard
        clipBMeas.measureIsSet[MT::LengthIDs::LENGTH_BETWEEN] = true;
    } else { // measure is unset and will not be copied to the clipboard
        clipBMeas.measureIsSet[MT::LengthIDs::LENGTH_BETWEEN] = false;
    }

    // Initial point
    setMeasureCanvasItem(start_p, false, to_phantom, measure_repr);

    // Now that text has been added, we can add lines and controls so that they go underneath
    for (size_t idx = 0; idx < intersections.size(); ++idx) {
        setMeasureCanvasItem(_desktop->doc2dt(intersections[idx]), to_item, to_phantom, measure_repr);
        if(to_guides) {
            gchar *cross_number;
            if (!prefs->getBool("/tools/measure/ignore_1st_and_last", true)) {
                cross_number= g_strdup_printf(_("Crossing %lu"), static_cast<unsigned long>(idx));
            } else {
                cross_number= g_strdup_printf(_("Crossing %lu"), static_cast<unsigned long>(idx + 1));
            }
            if (!prefs->getBool("/tools/measure/ignore_1st_and_last", true) && idx == 0) {
                setGuide(_desktop->doc2dt(intersections[idx]), angle + Geom::rad_from_deg(90), "");
            } else {
                setGuide(_desktop->doc2dt(intersections[idx]), angle + Geom::rad_from_deg(90), cross_number);
            }
            g_free(cross_number);
        }
    }
    // Since adding goes to the bottom, do all lines last.

    // draw main control line
    {
        setMeasureCanvasControlLine(start_p, end_p, false, to_phantom, Inkscape::CANVAS_ITEM_PRIMARY, measure_repr);
        if (showAngle) {
            double length = std::abs((end_p - start_p).length());
            Geom::Point anchorEnd = start_p;
            anchorEnd[Geom::X] += length;
            if (explicit_base) {
                anchorEnd *= (Geom::Affine(Geom::Translate(-start_p))
                            * Geom::Affine(Geom::Rotate(baseAngle))
                            * Geom::Affine(Geom::Translate(start_p)));
            }
            setMeasureCanvasControlLine(start_p, anchorEnd, to_item, to_phantom, Inkscape::CANVAS_ITEM_SECONDARY, measure_repr);
            createAngleDisplayCurve(start_p, end_p, angleDisplayPt, angle, to_phantom, measure_repr);
        }
    }

     if ((showDeltas) && (!dX_is0) && (!dY_is0)) {  // adding delta lines 
        setMeasureCanvasControlLine(start_p, deltasBasePoint, to_item, to_phantom, Inkscape::CANVAS_ITEM_SECONDARY, measure_repr);
        setMeasureCanvasControlLine(end_p, deltasBasePoint, to_item, to_phantom, Inkscape::CANVAS_ITEM_SECONDARY, measure_repr);
        setMeasureCanvasControlLine(dXmidpos, dXTextPos, to_item, to_phantom, Inkscape::CANVAS_ITEM_SECONDARY, measure_repr);
        setMeasureCanvasControlLine(dYmidpos, dYTextPos - Geom::Point((5 * fontsize),0), to_item, to_phantom, Inkscape::CANVAS_ITEM_SECONDARY, measure_repr);
    }

    if (placements.size() > 1) {
        setMeasureCanvasControlLine(_desktop->doc2dt(intersections[0]) + normal * dimension_offset, _desktop->doc2dt(intersections[intersections.size() - 1]) + normal * dimension_offset, to_item, to_phantom, Inkscape::CANVAS_ITEM_PRIMARY , measure_repr);

        setMeasureCanvasControlLine(_desktop->doc2dt(intersections[0]), _desktop->doc2dt(intersections[0]) + normal * dimension_offset, to_item, to_phantom, Inkscape::CANVAS_ITEM_PRIMARY , measure_repr);

        setMeasureCanvasControlLine(_desktop->doc2dt(intersections[intersections.size() - 1]), _desktop->doc2dt(intersections[intersections.size() - 1]) + normal * dimension_offset, to_item, to_phantom, Inkscape::CANVAS_ITEM_PRIMARY , measure_repr);
    }

    // call-out lines
    for (auto & place : placements) {
        setMeasureCanvasControlLine(place.start, place.end, to_item, to_phantom, Inkscape::CANVAS_ITEM_SECONDARY, measure_repr);       
    }
/* this is not needed, it does the same thing as the for (auto & place : placements) above ... but now the shortest segments will
   not be shown, so this will show extra lines.
   This whole comment block should be deleted. But I didn't want to delete it without giving an explanation.
    for (size_t idx = 1; idx < intersections.size(); ++idx) {
        Geom::Point measure_text_pos = (intersections[idx - 1] + intersections[idx]) / 2;
        setMeasureCanvasControlLine(_desktop->doc2dt(measure_text_pos), _desktop->doc2dt(measure_text_pos) - (normal * dimension_offset / 2), to_item, to_phantom, Inkscape::CANVAS_ITEM_SECONDARY, measure_repr);
    }
     */
}

/**
 * Create a measure item in current document.
 *
 * @param pathv the path to create.
 * @param markers if the path results get markers.
 * @param color of the stroke.
 * @param measure_repr container element.
 */
void MeasureTool::setMeasureItem(Geom::PathVector pathv, bool is_curve, bool markers, guint32 color, Inkscape::XML::Node *measure_repr)
{
    if(!_desktop) {
        return;
    }
    SPDocument *doc = _desktop->getDocument();
    Inkscape::XML::Document *xml_doc = doc->getReprDoc();
    Inkscape::XML::Node *repr;
    repr = xml_doc->createElement("svg:path");
    auto str = sp_svg_write_path(pathv);
    SPCSSAttr *css = sp_repr_css_attr_new();
    auto layer = _desktop->layerManager().currentLayer();
    Geom::Coord strokewidth = layer->i2doc_affine().inverse().expansionX();
    std::stringstream stroke_width;
    stroke_width.imbue(std::locale::classic());
    if(measure_repr) {
        stroke_width <<  strokewidth / _desktop->current_zoom();
    } else {
        stroke_width <<  strokewidth;
    }
    sp_repr_css_set_property (css, "stroke-width", stroke_width.str().c_str());
    sp_repr_css_set_property (css, "fill", "none");
    sp_repr_css_set_property_string(css, "stroke", color ? Inkscape::Colors::rgba_to_hex(color) : "#ff0000");
    char const * stroke_linecap = is_curve ? "butt" : "square";
    sp_repr_css_set_property (css, "stroke-linecap", stroke_linecap);
    sp_repr_css_set_property (css, "stroke-linejoin", "miter");
    sp_repr_css_set_property (css, "stroke-miterlimit", "4");
    sp_repr_css_set_property (css, "stroke-dasharray", "none");
    if(measure_repr) {
        sp_repr_css_set_property (css, "stroke-opacity", "0.5");
    } else {
        sp_repr_css_set_property (css, "stroke-opacity", "1");
    }
    if(markers) {
        sp_repr_css_set_property (css, "marker-start", "url(#Arrow2Sstart)");
        sp_repr_css_set_property (css, "marker-end", "url(#Arrow2Send)");
    }
    Glib::ustring css_str;
    sp_repr_css_write_string(css,css_str);
    repr->setAttribute("style", css_str);
    sp_repr_css_attr_unref (css);
    repr->setAttribute("d", str);
    if(measure_repr) {
        measure_repr->addChild(repr, nullptr);
        Inkscape::GC::release(repr);
    } else {
        auto item = cast<SPItem>(layer->appendChildRepr(repr));
        Inkscape::GC::release(repr);
        item->updateRepr();
        _desktop->getSelection()->clear();
        _desktop->getSelection()->add(item);
    }
}

/**
 * @brief Copies some measurements to the clipboard
 * 
 * It deals with Alt + C event
 *
 * It copies the measurements to the clipboard.
 * The settings for what should be copied are in the MeasureToolSettingsDialog.
 * The path to the settings of the MeasureToolSettingsDialog is saved in the preferences,
 * so if for any reason the path of the MeasureToolSettingsDialog is changed, no change is needed here.
 * Probably all settings can go in /tools/measure/, but the other dialogs have their own path starting 
 * with /dialog/, so I have done it the same.
 * 
 * The measurements are unset only when they are not visible, visible measurements are always accurate.
 * Measurements that are not visible have not been (re)calculated, so the stored value may be inaccurate.
 */
void MeasureTool::copyToClipboard() {
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    int precision = prefs->getInt("/tools/measure/precision", 2);
    Glib::ustring unit_name = prefs->getString("/tools/measure/unit");
    Glib::ustring MTSpath = prefs->getString("/tools/measure/MTSpath",""); // path to the settings
    Glib::ustring pathStr = MTSpath;
    pathStr.append("/show_angle");
    bool showAngleOpt = prefs->getBool(pathStr.c_str(), true);
    pathStr = MTSpath;
    pathStr.append("/show_deltas");
    bool deltasOpt = prefs->getBool(pathStr.c_str(), true);
    pathStr = MTSpath;
    pathStr.append("/labels");
    bool labelsOpt = prefs->getBool(pathStr.c_str(), true);
    pathStr = MTSpath;
    pathStr.append("/units");
    bool unitsOpt = prefs->getBool(pathStr.c_str(), true);
    pathStr = MTSpath;
    pathStr.append("/tabs");
    bool tabsOpt = prefs->getBool(pathStr.c_str(), true);
    pathStr = MTSpath;
    pathStr.append("/length");
    bool lengthOpt = prefs->getBool(pathStr.c_str(), true);
    pathStr = MTSpath;
    pathStr.append("/between");
    bool betweenOpt = prefs->getBool(pathStr.c_str(), true);
    pathStr = MTSpath;
    pathStr.append("/angle");
    bool angleOpt = prefs->getBool(pathStr.c_str(), true);
    pathStr = MTSpath;
    pathStr.append("/dX");
    bool dXOpt = prefs->getBool(pathStr.c_str(), true);
    pathStr = MTSpath;
    pathStr.append("/dY");
    bool dYOpt = prefs->getBool(pathStr.c_str(), true);
    pathStr = MTSpath;
    pathStr.append("/segments");
    bool segmentsOpt = prefs->getBool(pathStr.c_str(), true);
    pathStr = MTSpath;
    pathStr.append("/shape_width");
    bool shape_widthOpt = prefs->getBool(pathStr.c_str(), true);
    pathStr = MTSpath;
    pathStr.append("/shape_height");
    bool shape_heightOpt = prefs->getBool(pathStr.c_str(), true);
    pathStr = MTSpath;
    pathStr.append("/shape_X");
    bool shape_XOpt = prefs->getBool(pathStr.c_str(), true);
    pathStr = MTSpath;
    pathStr.append("/shape_Y");
    bool shape_YOpt = prefs->getBool(pathStr.c_str(), true);
    pathStr = MTSpath;
    pathStr.append("/shape_length");
    bool shape_lengthOpt = prefs->getBool(pathStr.c_str(), true);

    Glib::ustring stringToCopy = "";
    if ((lengthOpt) && (clipBMeas.measureIsSet[MT::LengthIDs::LENGTH])) { 
        stringToCopy += clipBMeas.composeMeaStr(MT::LengthIDs::LENGTH, precision, unit_name, labelsOpt, unitsOpt, tabsOpt) + "\n";
    }
    if ((betweenOpt) && (clipBMeas.measureIsSet[MT::LengthIDs::LENGTH_BETWEEN])) { // not copying it if it is the same as the length
        if (clipBMeas.lengths[MT::LengthIDs::LENGTH] != clipBMeas.lengths[MT::LengthIDs::LENGTH_BETWEEN]) {
            stringToCopy += clipBMeas.composeMeaStr(MT::LengthIDs::LENGTH_BETWEEN, precision, unit_name, labelsOpt, unitsOpt, tabsOpt) + "\n";
        } else {
            if (!lengthOpt) { // if the length is not being copied, then will copy this
                stringToCopy += clipBMeas.composeMeaStr(MT::LengthIDs::LENGTH_BETWEEN, precision, unit_name, labelsOpt, unitsOpt, tabsOpt) + "\n";
            }
        }
    }
    if ((deltasOpt) && (dXOpt) && (clipBMeas.measureIsSet[MT::LengthIDs::DX])) { 
        stringToCopy += clipBMeas.composeMeaStr(MT::LengthIDs::DX, precision, unit_name, labelsOpt, unitsOpt, tabsOpt) + "\n";
    }
    if ((deltasOpt) && (dYOpt) && (clipBMeas.measureIsSet[MT::LengthIDs::DY])) { 
        stringToCopy += clipBMeas.composeMeaStr(MT::LengthIDs::DY, precision, unit_name, labelsOpt, unitsOpt, tabsOpt) + "\n";
    }
    if ((showAngleOpt) && (angleOpt) && (clipBMeas.measureIsSet[MT::LengthIDs::ANGLE])) { 
        stringToCopy += clipBMeas.composeMeaStr(MT::LengthIDs::ANGLE, precision, "°", labelsOpt, unitsOpt, tabsOpt) + "\n";
    }
    if ((clipBMeas.segLengths.size() > 0) && (segmentsOpt) && (clipBMeas.measureIsSet[MT::LengthIDs::SEGMENT])) {
        Glib::ustring firstSeg = clipBMeas.symbols[MT::LengthIDs::SEGMENT] + "1";
        if ((clipBMeas.segLengths.size() == 1) && (clipBMeas.segLengths["S1"] == clipBMeas.lengths[MT::LengthIDs::LENGTH])) {
            // do nothing the segment is the same as the total length - no point in showing it
        } else {
            stringToCopy += _("\nIntersection segments lengths:\n");
            Glib::ustring sep = tabsOpt ? "\t" : " ";
            for (const auto& [key,value] : clipBMeas.segLengths) {
                if (labelsOpt) { stringToCopy += key +":" + sep; }
                stringToCopy += Glib::ustring::format(std::setprecision(precision), std::fixed, value);
                if (unitsOpt) { stringToCopy += sep + unit_name; }
                stringToCopy += "\n";
            }
        }
    }

        bool showTitle = true;
        const char* title = _("\nInfo about the shape under the pointer:\n");
        if ((shape_widthOpt) && (clipBMeas.measureIsSet[MT::LengthIDs::SHAPE_WIDTH])) { 
            if (showTitle) {
                stringToCopy += title;
                showTitle = false;
            }
            stringToCopy += clipBMeas.composeMeaStr(MT::LengthIDs::SHAPE_WIDTH, precision, unit_name, labelsOpt, unitsOpt, tabsOpt) + "\n";
        }
        if ((shape_heightOpt) && (clipBMeas.measureIsSet[MT::LengthIDs::SHAPE_HEIGHT])) {
            if (showTitle) {
                stringToCopy += title;
                showTitle = false;
            }
            stringToCopy += clipBMeas.composeMeaStr(MT::LengthIDs::SHAPE_HEIGHT, precision, unit_name, labelsOpt, unitsOpt, tabsOpt) + "\n";
        }
        if ((shape_XOpt) && (clipBMeas.measureIsSet[MT::LengthIDs::SHAPE_X])) {
            if (showTitle) {
                stringToCopy += title;
                showTitle = false;
            }
            stringToCopy += clipBMeas.composeMeaStr(MT::LengthIDs::SHAPE_X, precision, unit_name, labelsOpt, unitsOpt, tabsOpt) + "\n";
        }
        if ((shape_YOpt) && (clipBMeas.measureIsSet[MT::LengthIDs::SHAPE_Y])) {
            if (showTitle) {
                stringToCopy += title;
                showTitle = false;
            }
            stringToCopy += clipBMeas.composeMeaStr(MT::LengthIDs::SHAPE_Y, precision, unit_name, labelsOpt, unitsOpt, tabsOpt) + "\n";
        }
        if ((shape_lengthOpt) && (clipBMeas.measureIsSet[MT::LengthIDs::SHAPE_LENGTH])) {
            if (showTitle) {
                stringToCopy += title;
                showTitle = false;
            }
            stringToCopy += clipBMeas.composeMeaStr(MT::LengthIDs::SHAPE_LENGTH, precision, unit_name, labelsOpt, unitsOpt, tabsOpt) + "\n";
        }
    

    Inkscape::UI::ClipboardManager *cm = Inkscape::UI::ClipboardManager::get();
    if (cm->copyString(stringToCopy)) {
        _desktop->messageStack()->flash(Inkscape::MessageType::INFORMATION_MESSAGE,_("The measurements have been copied to the clipboard"));
    } 

}

/**
 * @brief Calculates the base point from which to draw the dX and dY lines
 * 
 * It puts the point on the opposite side from where the angle is drawn
 * The returned array contains the following points:
 * 
 * ___point0 is the base point; ___point1 is the dX normal; ___point2 is the dY normal;  
 * ___point3 is the dXbase (mid point along the dX line);  
 * ___point4 is the dYbase (mid point along the dY line)
 */
std::vector<Geom::Point> MeasureTool::calcDeltaBasePoint(double dX, double dY)
{
    Geom::Point deltasBasePoint;
    Geom::Point dXnormal;
    Geom::Point dYnormal;
    Geom::Point dXbase;
    Geom::Point dYbase;
    double midX = (std::abs(dX) / 2);
    double midY = (std::abs(dY) / 2);
    if ((dX > 0) && (dY > 0)) {  // positioning the measures on the outside to avoid the clutter
        deltasBasePoint = Geom::Point(start_p[Geom::X], end_p[Geom::Y]);
        dXnormal = Geom::Point(0, 1);
        dYnormal = Geom::Point(-1, 0);
        dXbase = Geom::Point(start_p[Geom::X] + midX / 2, end_p[Geom::Y]); // putting closer to the base point to avoid other labels
        dYbase = Geom::Point(start_p[Geom::X], start_p[Geom::Y] + midY);
    }
    if ((dX > 0) && (dY < 0)) {
        deltasBasePoint = Geom::Point(start_p[Geom::X], end_p[Geom::Y]);
        dXnormal = Geom::Point(0, -1);
        dYnormal = Geom::Point(-1, 0);
        dXbase = Geom::Point(start_p[Geom::X] + midX / 2, end_p[Geom::Y]);
        dYbase = Geom::Point(start_p[Geom::X], end_p[Geom::Y] + midY);
    }
    if ((dX < 0) && (dY > 0)) {
        deltasBasePoint = Geom::Point(end_p[Geom::X],start_p[Geom::Y]);
        dXnormal = Geom::Point(0, -1);
        dYnormal = Geom::Point(-1, 0);
        dXbase = Geom::Point(end_p[Geom::X] + midX / 2, start_p[Geom::Y]);
        dYbase = Geom::Point(end_p[Geom::X], start_p[Geom::Y] + midY);
    }
    if ((dX < 0) && (dY < 0)) {
        deltasBasePoint = Geom::Point(end_p[Geom::X],start_p[Geom::Y]);
        dXnormal = Geom::Point(0, 1);
        dYnormal = Geom::Point(-1, 0);
        dXbase = Geom::Point(end_p[Geom::X] + midX / 2, start_p[Geom::Y]);
        dYbase = Geom::Point(end_p[Geom::X], end_p[Geom::Y] + midY);
    }
    std::vector<Geom::Point> result;
    result.push_back(deltasBasePoint);
    result.push_back(dXnormal);
    result.push_back(dYnormal);
    result.push_back(dXbase);
    result.push_back(dYbase);
    return result;
}

/**
 * @brief Checks if a value is very close to a reference value and can be considered equal to it
 * 
 * @param epsilon: the value of the acceptable discrepancy from true equality
 * @param positiveAllowed: if true (default) it checks for reference_value + epsilon range
 * @param negativeAllowed: if true (default) it checks for reference_value - epsilon range
 *    
 *    If value is not allowed to cross a limit, then the range can be limited to either side of the limit
 *    by setting the appropriate flag to false                  
 */
bool MeasureTool::equalWithinRange(double value, double reference_value, double epsilon, bool positiveAllowed, bool negativeAllowed) {
    if (positiveAllowed) {
        if ((value <= reference_value + epsilon) && (value >= reference_value)) { return true; }
    }
    if (negativeAllowed) {
        if ((value >= reference_value - epsilon) && (value <= reference_value)) { return true; }
    }
    return false;
}

MT::ClipboardMeaClass::ClipboardMeaClass() {}
MT::ClipboardMeaClass::~ClipboardMeaClass() {}

/**
 * @brief Composes the string for a easurement to be copied to the clipboards
 * 
 * @param id: one of the values of the enum MT::LengthIDs that identifies the measurement
 * @param unit: the unit of measurement
 * @param :  the other parameters are formatting options (if true are included; if false they are omitted) 
 *                     
 */
Glib::ustring MT::ClipboardMeaClass::composeMeaStr(int id, int precision, Glib::ustring unit, bool withLabel, bool withUnit, bool tabSeparated) {
    Glib::ustring value = Glib::ustring::format(std::setprecision(precision), std::fixed, lengths[id]);
    Glib::ustring sep = tabSeparated ? "\t" : " ";
    Glib::ustring result = withLabel ? labels[id] + ":" + sep : "";
    result += value;
    if (withUnit) {
        result += sep + unit;
    }
    return result;
}

void MT::ClipboardMeaClass::unsetShapeMeasures() {
    measureIsSet[LengthIDs::SHAPE_LENGTH] = false;
    measureIsSet[LengthIDs::SHAPE_WIDTH] = false;
    measureIsSet[LengthIDs::SHAPE_HEIGHT] = false;
    measureIsSet[LengthIDs::SHAPE_X] = false;
    measureIsSet[LengthIDs::SHAPE_Y] = false;
}

} // namespace Inkscape::UI::Tools

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
