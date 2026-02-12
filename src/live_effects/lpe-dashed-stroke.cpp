// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#include "live_effects/lpe-dashed-stroke.h"
#include "object/sp-rect.h"
#include <2geom/path.h>
#include <2geom/pathvector.h>
#include "helper/geom.h"

// TODO due to internal breakage in glibmm headers, this must be last:
#include <glibmm/i18n.h>

namespace Inkscape {
namespace LivePathEffect {

LPEDashedStroke::LPEDashedStroke(LivePathEffectObject *lpeobject)
    : Effect(lpeobject)
    , numberdashes(_("Number of dashes"), _("Number of dashes"), "numberdashes", &wr, this, 3)
    , holefactor(_("Hole factor"), _("Hole factor, allow negative value"), "holefactor", &wr, this, 0.0)
    , offset(_("Rect Offset"), _("Rect Offset"), "offset", &wr, this, 0.0)
    , splitsegments(_("Use segments"), _("Use segments"), "splitsegments", &wr, this, true)
    , halfextreme(_("Half start/end"), _("Start and end of each segment has half size"), "halfextreme", &wr, this, true)
    , unifysegment(_("Equalize dashes"), _("Global dash length is approximately the length of the dashes in the shortest path segment"),
                   "unifysegment", &wr, this, true)
    , message(_("Note"), _("Important messages"), "message", &wr, this,
              _("Add <b>\"Fill Between Many LPE\"</b> to add fill."))
{
    registerParameter(&numberdashes);
    registerParameter(&holefactor);
    registerParameter(&offset);
    registerParameter(&splitsegments);
    registerParameter(&halfextreme);
    registerParameter(&unifysegment);
    registerParameter(&message);
    
    message.write_to_SVG(); // resert old legacy uneeded data
    numberdashes.param_set_range(2, 9999);
    numberdashes.param_set_increments(1, 1);
    numberdashes.param_set_digits(0);
    holefactor.param_set_range(-0.99999, 0.99999);
    holefactor.param_set_increments(0.01, 0.01);
    holefactor.param_set_digits(5);
    offset.param_set_range(-1, 1);
    offset.param_set_increments(0.01, 0.01);
    offset.param_set_digits(2);
    message.param_set_min_height(30);
}

LPEDashedStroke::~LPEDashedStroke() = default;

void LPEDashedStroke::doBeforeEffect(SPLPEItem const *lpeitem) {}

///Calculate the time in curve_in with a real time of A
//TODO: find a better place to it
double LPEDashedStroke::timeAtLength(double const A, Geom::Path const &segment)
{
    if ( A == 0 || segment[0].isDegenerate()) {
        return 0;
    }
    double t = 1;
    t = timeAtLength(A, segment.toPwSb());
    return t;
}

///Calculate the time in curve_in with a real time of A
//TODO: find a better place to it
double LPEDashedStroke::timeAtLength(double const A, Geom::Piecewise<Geom::D2<Geom::SBasis>> pwd2)
{
    if ( A == 0 || pwd2.size() == 0) {
        return 0;
    }

    double t = pwd2.size();
    std::vector<double> t_roots = roots(Geom::arcLengthSb(pwd2) - A);
    if (!t_roots.empty()) {
        t = t_roots[0];
    }
    return t;
}

void 
LPEDashedStroke::doOnApply(SPLPEItem const *lpeitem)
{
    lpeversion.param_setValue("1.5", true);
}

Geom::PathVector LPEDashedStroke::doEffect_path(Geom::PathVector const &path_in)
{
    Geom::PathVector const pv = pathv_to_linear_and_cubic_beziers(path_in);
    Geom::PathVector result;
    for (const auto & path_it : pv) {
        if (path_it.empty()) {
            continue;
        }
        Geom::Path::const_iterator curve_it1 = path_it.begin();
        Geom::Path::const_iterator curve_it2 = ++(path_it.begin());
        Geom::Path::const_iterator curve_endit = path_it.end_default();
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
        auto rect = cast<SPRect>(sp_lpe_item);
        bool userectround = (lpeversion.param_getSVGValue() >= "1.5" && 
            rect && 
            (rect->rx.value != 0 || rect->ry.value != 0)
        );
        // get the total dashes per segment or path
        size_t numberdashes_fixed = numberdashes; // allow use number of dashes in path or segment mode without altering paramenter
        if (!splitsegments && !userectround) {
            if (lpeversion.param_getSVGValue() >= "1.5") {
                if(path_it.closed()) {
                    numberdashes_fixed++;
                }
            } else {
                numberdashes_fixed++;
            }
        }
        // and his holes
        size_t numberholes = numberdashes_fixed - 1; // always one hole less
        //get number of slots
        size_t ammount = numberdashes_fixed + numberholes; //total of regions
        if (halfextreme) { //if half exteme one region less
            ammount--;
        }
        //slot average
        double base = 1/(double)ammount; // average proportion
        // Size of dash -- (1 + holefactor) is a number from 1 to 2
        // base * numberdashes_fixed is always (1 + holefactor) < 1 and is balanced by hole factor
        // get total percent of solid dash
        double globaldash =  base * numberdashes_fixed * (1 + holefactor); 
        if (halfextreme) { //remove a dash if half
            globaldash =  base * (numberdashes_fixed - 1) * (1 + holefactor);
        }
        // get total percent of solid hole
        double globalhole =  1-globaldash;
        // get total percent of one solid dash
        double dashpercent = globaldash/numberdashes_fixed;
        if (halfextreme) { 
           dashpercent = globaldash/(numberdashes_fixed -1);
        }
        // get total percent of one solid hole
        double holepercent = globalhole/numberholes;
        double dashsize_fixed = 0;
        double holesize_fixed = 0;
        Geom::Piecewise<Geom::D2<Geom::SBasis> > pwd2 = path_it.toPwSb();
        double length_pwd2 = length (pwd2);
        //store segment length
        double minlength = length_pwd2;
        size_t p_index = 0;
        // store size
        size_t start_index = result.size();
        //store segments to later process
        std::vector<Geom::Path> segments;
        if (splitsegments || userectround ) {
            int subs = 1; //we have a subs segment of 1 full segment
            double gap = offset / 2.0; // offset of dashes
            if (userectround) { //we are using rect mode with rouded corners
                // here we can have paths of 8, 4 or 6 segments
                // here we force segment mode but instead use segments
                // we use one side segment and half curved exterem segments and take it as unique segment
                // we do for all kind of rects and store portions as segments
                // we store the subs variable that represent the size of the special segment
                if (path_it.size() == 8) { // a rect with 4 non rounding segments
                    auto start = path_it.portion(7.5+gap, 8); 
                    start.append(path_it.portion(0, 1.5+gap));
                    segments.push_back(path_it.portion(1.5+gap, 3.5+gap));
                    segments.push_back(path_it.portion(3.5+gap, 5.5+gap));
                    segments.push_back(path_it.portion(5.5+gap, 7.5+gap));
                    segments.push_back(start);
                    subs = 3; //we have a subs segment of 3 - half corner + side + half corner
                } else if (path_it.size() == 4) { //small rect only rounded (circle)
                    auto start = path_it.portion(3.5+gap, 4);
                    start.append(path_it.portion(0, 0.5+gap));
                    segments.push_back(path_it.portion(0.5+gap, 1.5+gap));
                    segments.push_back(path_it.portion(1.5+gap, 2.5+gap));
                    segments.push_back(path_it.portion(2.5+gap, 3.5+gap));
                    segments.push_back(start);
                    subs = 2; //we have a subs segment of 2 - half corner + half corner
                } else if (path_it.size() == 6) { // rectangle only one side with segments
                    if (rect->width.value > rect->height.value) { // check if is rectagle vertical or horizontal
                        auto start = path_it.portion(5.5+gap, 6);
                        start.append(path_it.portion(0, 1.5+gap));
                        segments.push_back(path_it.portion(1.5+gap, 2.5+gap));
                        segments.push_back(path_it.portion(2.5+gap, 4.5+gap));
                        segments.push_back(path_it.portion(4.5+gap, 5.5+gap));
                        segments.push_back(start);
                    } else {
                        auto start = path_it.portion(5.5+gap, 6);
                        start.append(path_it.portion(0, 0.5+gap));
                        segments.push_back(path_it.portion(0.5+gap, 2.5+gap));
                        segments.push_back(path_it.portion(2.5+gap, 3.5+gap));
                        segments.push_back(path_it.portion(3.5+gap, 5.5+gap));
                        segments.push_back(start);
                    }
                    subs = -1 ;//we have a subs segment of -1 == segment size (later)
                }
                // we calculate optionaly the small segment in the path
                // to use his size to all segments (the darsh size looks same sice in diferent length segments)
                if(unifysegment) {
                    for (auto segment:segments) {
                        //loop path to get the min length to apply
                        double length_segment = segment.length();
                        if (length_segment < minlength) {
                            minlength = length_segment;
                            dashsize_fixed = segment.length() * dashpercent;
                            holesize_fixed = segment.length() * holepercent;
                        } 
                    } 
                }
            } else { //we use normal segments mode
                rect = nullptr;
                if(unifysegment) {
                    while (curve_it1 != curve_endit) {
                        //loop path to get the min length to apply
                        double length_segment = (*curve_it1).length();
                        if (length_segment < minlength) {
                            minlength = length_segment;
                            dashsize_fixed = (*curve_it1).length() * dashpercent;
                            holesize_fixed = (*curve_it1).length() * holepercent;
                        } 
                        ++curve_it1;
                        ++curve_it2;
                    } 
                    curve_it1 = path_it.begin();
                    curve_it2 = ++(path_it.begin());
                    curve_endit = path_it.end_default();
                }
                // generate segments to join
                while (curve_it1 != curve_endit) {
                    Geom::Path segment = path_it.portion(p_index, p_index + 1);
                    segments.push_back(segment);
                    p_index ++;
                    ++curve_it1;
                    ++curve_it2;
                } 
            }
            // join results
            for (auto segment:segments) {
                if (unifysegment) {
                    double integral;
                    //Calculate the number od dashes
                    modf(segment.length()/(dashsize_fixed + holesize_fixed), &integral);
                    numberdashes_fixed = (size_t)integral + 1;
                    //Calculate the number od holes
                    numberholes = numberdashes_fixed - 1;
                    //Calculate the number od slots
                    ammount = numberdashes_fixed + numberholes;
                    if (halfextreme) {
                        ammount--;
                    }
                    //slot average
                    base = 1/(double)ammount;
                    // get total percent of solid dash
                    globaldash =  base * numberdashes_fixed * (1 + holefactor);
                    if (halfextreme) { 
                        globaldash =  base * (numberdashes_fixed - 1) * (1 + holefactor);
                    }
                    // get total percent of solid hole
                    globalhole =  1-globaldash;
                    // get total percent of one solid dash
                    dashpercent = globaldash/numberdashes_fixed;
                    if (halfextreme) { 
                        dashpercent = globaldash/(numberdashes_fixed -1);
                    }
                    // get total percent of one solid hole
                    holepercent = globalhole/numberholes;
                }
                // calculate sizes (all segments have same lenghts
                double dashsize = segment.length() * dashpercent;
                double holesize = segment.length() * holepercent;
                double start = 0.0;
                double end = 0.0;
                if (halfextreme) {
                    end = timeAtLength(dashsize/2.0,segment);
                } else {
                    end = timeAtLength(dashsize,segment);
                }
                // add start dash
                // if closed
                if (result.size() && Geom::are_near(segment.initialPoint(),result[result.size()-1].finalPoint(),0.01)) {
                    result[result.size()-1].setFinal(segment.initialPoint());
                    result[result.size()-1].append(segment.portion(start, end));
                } else {
                    result.push_back(segment.portion(start, end));
                }
                double startsize = dashsize + holesize;
                if (halfextreme) {
                    startsize = (dashsize/2.0) + holesize;
                }
                double endsize = startsize + dashsize;
                size_t subs_fixed = subs; //dont overwrite subs
                if (subs == -1) {
                    subs_fixed = segment.size();
                }
                start = timeAtLength(startsize,segment);
                end   = timeAtLength(endsize,segment);
                //add 
                while (start  < subs_fixed && start  > 0) {
                    result.push_back(segment.portion(start, end));
                    startsize = endsize + holesize;
                    endsize = startsize + dashsize;
                    start = timeAtLength(startsize,segment);
                    end   = timeAtLength(endsize,segment);
                }
            }
            if (path_it.closed()) {
                Geom::Path end = result[result.size()-1];
                end.setFinal(result[start_index].initialPoint());
                end.append(result[start_index]);
                if (lpeversion.param_getSVGValue() >= "1.5") { // keep wrong backward compat -in the end of closed itms to 2 subpaths intead one, study change test and fix always
                    result.pop_back();
                }
                result[start_index] = end;
            }
        } else {
            double start = 0.0;
            double end = 0.0;
            double dashsize = length_pwd2 * dashpercent;
            double holesize = length_pwd2 * holepercent;
            if (halfextreme) {
                end = timeAtLength(dashsize/2.0,pwd2);
            } else {
                end = timeAtLength(dashsize,pwd2);
            }
            result.push_back(path_it.portion(start, end));
            double startsize = dashsize + holesize;
            if (halfextreme) {
                startsize = (dashsize/2.0) + holesize;
            }
            double endsize = startsize + dashsize;
            start = timeAtLength(startsize,pwd2);
            end   = timeAtLength(endsize,pwd2);
            while (start  < path_it.size() && start  > 0) {
                result.push_back(path_it.portion(start, end));
                startsize = endsize + holesize;
                endsize = startsize + dashsize;
                start = timeAtLength(startsize,pwd2);
                end   = timeAtLength(endsize,pwd2);
            }
            if (path_it.closed()) {
                Geom::Path end = result[result.size()-1];
                end.setFinal(result[start_index].initialPoint());
                end.append(result[start_index]);
                if (lpeversion.param_getSVGValue() >= "1.5") { // keep wrong backward compat -in the end of closed itms to 2 subpaths intead one, study change test and fix always
                    result.pop_back();
                }
                result[start_index] = end;
            }
        }
    }
    return result;
}

}; //namespace LivePathEffect
}; /* namespace Inkscape */

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
