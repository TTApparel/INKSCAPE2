// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_LPE_POWERMASK_H
#define INKSCAPE_LPE_POWERMASK_H

/*
 * Inkscape::LPEPowerMask
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#include "live_effects/effect.h"
#include "live_effects/parameter/bool.h"
#include "live_effects/parameter/colorpicker.h"
#include "live_effects/parameter/hidden.h"
#include "live_effects/parameter/text.h"

namespace Inkscape {
namespace LivePathEffect {

class LPEPowerMask : public Effect
{
public:
    LPEPowerMask(LivePathEffectObject *lpeobject);
    ~LPEPowerMask() override;
    void doOnApply(SPLPEItem const *lpeitem) override;
    void doBeforeEffect(SPLPEItem const *lpeitem) override;
    void doEffect(Geom::PathVector &curve) override;
    void doOnRemove(SPLPEItem const * /*lpeitem*/) override;
    void doOnVisibilityToggled(SPLPEItem const *lpeitem) override;
    void toggleMaskVisibility();
    void tryForkMask();
    Glib::ustring getId();
    void setMask();

private:
    HiddenParam uri;
    BoolParam invert;
    // BoolParam wrap;
    BoolParam hide_mask;
    BoolParam background;
    ColorPickerParam background_color;
    std::optional<Colors::Color> previous_color;
    Geom::Path mask_box;
};

void sp_remove_powermask(Inkscape::Selection *sel);
void sp_inverse_powermask(Inkscape::Selection *sel);

} // namespace LivePathEffect
} // namespace Inkscape

#endif
