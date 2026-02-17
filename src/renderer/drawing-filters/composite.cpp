// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * feComposite filter effect renderer
 *
 * Authors:
 *   Niko Kiirala <niko@kiirala.com>
 *
 * Copyright (C) 2007 authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "composite.h"
#include "colors/manager.h"
#include "slot.h"

#include "renderer/context.h"
#include "renderer/pixel-filters/composite.h"
#include "renderer/surface.h"

namespace Inkscape::Renderer::DrawingFilter {

void Composite::render(Slot &slot) const
{
    auto input1 = slot.get_copy(_input, _color_space);
    auto input2 = slot.get(_input2, _color_space);

    if (!input1 || !input2) {
        std::cout << "Missing input in Composite::Render\n\n";
        return;
    }

    if (op == CompositeOperator::ARITHMETIC) {
        input1->run_pixel_filter<PixelAccessEdgeMode::WRAP>(PixelFilter::CompositeArithmetic(k1, k2, k3, k4), *input2);
    } else {
        auto ct = Context(*input1);
        ct.setSource(*input2);
        switch(op) {
        case CompositeOperator::IN:
            ct.setOperator(Cairo::Context::Operator::IN);
            break;
        case CompositeOperator::OUT:
            ct.setOperator(Cairo::Context::Operator::OUT);
            break;
        case CompositeOperator::ATOP:
            ct.setOperator(Cairo::Context::Operator::ATOP);
            break;
        case CompositeOperator::XOR:
            ct.setOperator(Cairo::Context::Operator::XOR);
            break;
        case CompositeOperator::LIGHTER:
            ct.setOperator(Cairo::Context::Operator::ADD);
            break;
        case CompositeOperator::OVER:
        case CompositeOperator::DEFAULT:
        default:
            // OVER is the default operator
            break;
        }
        ct.paint();
    }
    slot.set(_output, input1);
}

bool Composite::can_handle_affine(Geom::Affine const &) const
{
    return true;
}

void Composite::set_input(int input)
{
    _input = input;
}

void Composite::set_input(int input, int slot)
{
    if (input == 0) _input = slot;
    if (input == 1) _input2 = slot;
}

void Composite::set_operator(CompositeOperator op_)
{
    if (op_ == CompositeOperator::DEFAULT) {
        op = CompositeOperator::OVER;
    } else if (op_ != CompositeOperator::ENDOPERATOR) {
        op = op_;
    }
}

void Composite::set_arithmetic(double k1_, double k2_, double k3_, double k4_)
{
    if (!std::isfinite(k1) || !std::isfinite(k2) || !std::isfinite(k3) || !std::isfinite(k4)) {
        g_warning("Non-finite parameter for feComposite arithmetic operator");
        return;
    }
    k1 = k1_;
    k2 = k2_;
    k3 = k3_;
    k4 = k4_;
}

double Composite::complexity(Geom::Affine const &) const
{
    return 1.1;
}

} // namespace Inkscape::Renderer::DrawingFilter

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
