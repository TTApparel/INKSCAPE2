// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Author(s):
 *     Jabiertxo Arraiza Cenoz <jabier.arraiza@marker.es>
 *
 * Copyright (C) 2014 Author(s)
 *
 * Jabiertxof:Thanks to all people help me
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_LPE_SLICE_MARKERS_H
#define INKSCAPE_LPE_SLICE_MARKERS_H

#include "live_effects/effect.h"

namespace Inkscape::LivePathEffect {

class LPESliceMarkers : public Effect {
public:
    LPESliceMarkers(LivePathEffectObject *lpeobject);
    void doBeforeEffect(SPLPEItem const *lpeItem) override;
    void doOnApply(SPLPEItem const *lpeItem) override;
    Geom::PathVector doEffect_path(Geom::PathVector const &path_in) override;
    Gtk::Widget *newWidget() override;
    std::vector<double> pos = {0,0,0};
private:
    LPESliceMarkers(const LPESliceMarkers &);
    LPESliceMarkers &operator=(const LPESliceMarkers &);

};

} // namespace Inkscape::LivePathEffect

#endif // INKSCAPE_SLICE_MARKERS_H

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
