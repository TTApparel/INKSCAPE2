// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Author(s):
 *   Jabiertxo Arraiza Cenoz <jabier.arraiza@marker.es>
 *
 * Copyright (C) 2014 Author(s)
 *
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "lpe-slice-markers.h"
#include "helper/geom.h"
#include "object/sp-shape.h"
#include "object/sp-marker.h"

namespace Inkscape::LivePathEffect {

LPESliceMarkers::LPESliceMarkers(LivePathEffectObject *lpeobject)
    : Effect(lpeobject)
{
    _provides_knotholder_entities = false;
    _provides_path_adjustment = true;
}

void LPESliceMarkers::doOnApply(SPLPEItem const *lpeItem)
{
    SPLPEItem *splpeitem = const_cast<SPLPEItem *>(lpeItem);
    auto shape = cast<SPShape>(splpeitem);
    if (!shape) {
        g_warning("LPE Slice Nodes can only be applied to shapes (not groups).");
        SPLPEItem *item = const_cast<SPLPEItem *>(lpeItem);
        item->removeCurrentPathEffect(false);
    }
}

Gtk::Widget *LPESliceMarkers::newWidget()
{
    return Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL);
}

void LPESliceMarkers::doBeforeEffect(SPLPEItem const *lpeItem)
{
    auto shape = cast<SPShape>(lpeItem);
    if(shape && shape->hasMarkers()) {
        pos = {0,0,0};
        if(auto const mstart = shape->_marker[SP_MARKER_LOC_START]) {
            if (mstart->refY == 0 /* drop unhandled */ ) {
                pos[0] = mstart->refX;
            }
        }
        /* if(auto mmid = shape->_marker[SP_MARKER_LOC_MID]) {
            pos[1] = mmid->refX;
        } */
        if(auto const mend = shape->_marker[SP_MARKER_LOC_END]) {
            pos[1] = mend->refX;
        }
    }
}

Geom::PathVector
LPESliceMarkers::doEffect_path(Geom::PathVector const &path_in)
{
    Geom::PathVector original_pathv = pathv_to_linear_and_cubic_beziers(path_in);
    Geom::PathVector res;
    for (auto & path_it : original_pathv) {
        Geom::PathVector _tmp;
        if (path_it.empty()) {
            continue;
        }
        Geom::Path::iterator curve_it1 = path_it.begin();
        Geom::Path::iterator curve_it2 = ++(path_it.begin());
        Geom::Path::iterator curve_endit = path_it.end_default();
        if (path_it.closed()) {
          const Geom::Curve &closingline = path_it.back_closed(); 
          // the closing line segment is always of type 
          // Geom::LineSegment.
          if (are_near(closingline.initialPoint(), closingline.finalPoint())) {
            // closingline.isDegenerate() did not work, because it only checks for
            // *exact* zero length, which goes wrong for relative coordinates and
            // rounding errors...
            // the closing line segment has zero-length. So stop before that one!
            curve_endit = path_it.end_open();
          }
        }
        Geom::Path tmp;
        if (path_it.size() == 1) {
            tmp.append(path_it.initialCurve().portion(pos[0], 1-pos[1]));
        } else {
            while (curve_it1 != curve_endit) {
                if (*curve_it1 == path_it.initialCurve()){
                    tmp.append(curve_it1->portion(pos[0],1));
                } else {
                    tmp.append(*curve_it1);//->portion(pos[1], 1-pos[1]));
                }
            }
            tmp.append(curve_it1->portion(0, 1-pos[1]));
        }
        res.push_back(tmp);
    }
   
    return res;
}


} // namespace Inkscape::LivePathEffect

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offset:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
