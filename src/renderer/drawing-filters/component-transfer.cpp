// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * feComponentTransfer filter primitive renderer
 *
 * Authors:
 *   Felipe Corrêa da Silva Sanches <juca@members.fsf.org>
 *   Jasper van de Gronde <th.v.d.gronde@hccnet.nl>
 *
 * Copyright (C) 2007 authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "colors/manager.h"
#include "component-transfer.h"
#include "slot.h"

#include "renderer/context.h"
#include "renderer/pixel-filters/component-transfer.h"
#include "renderer/surface.h"

namespace Inkscape::Renderer::DrawingFilter {

void ComponentTransfer::render(Slot &slot) const
{
    auto input = slot.get_copy(_input, _color_space);
    if (!input) {
        return;
    }
    std::vector<PixelFilter::TransferFunction> tfs;

    // To support more channels (CMYK etc) this needs to be changed.
    for (unsigned i = 0; i < 4; ++i) {
        switch (type[i]) {
        case ComponentTransferType::TABLE:
        case ComponentTransferType::DISCRETE:
            if (!tableValues[i].empty()) {
                tfs.emplace_back(PixelFilter::TransferFunction(tableValues[i], type[i] == ComponentTransferType::DISCRETE));
            }
            break;
        case ComponentTransferType::LINEAR:
            tfs.emplace_back(PixelFilter::TransferFunction(intercept[i], slope[i]));
            break;
        case ComponentTransferType::GAMMA:
            tfs.emplace_back(PixelFilter::TransferFunction(amplitude[i], exponent[i], offset[i]));
            break;
        case ComponentTransferType::ERROR:
        case ComponentTransferType::IDENTITY:
        default:
            break;
        }
    }
    input->run_pixel_filter(PixelFilter::ComponentTransfer(tfs), *input);
    slot.set(_output, input);
}

bool ComponentTransfer::can_handle_affine(Geom::Affine const &) const
{
    return true;
}

double ComponentTransfer::complexity(Geom::Affine const &) const
{
    return 2.0;
}

} // namespace Inkscape::Renderer::Drawing

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
