// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Raw filter functions for component transfer
 *//*
 * Authors:
 *   Martin Owens
 *
 * Copyright (C) 2025 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_RENDERER_PIXEL_FILTER_COMPONENT_TRANSFER_H
#define INKSCAPE_RENDERER_PIXEL_FILTER_COMPONENT_TRANSFER_H

#include <iostream>
#include <algorithm>
#include <cmath>
#include <vector>

namespace Inkscape::Renderer::PixelFilter {

enum class TransferType
{
    IDENTITY,
    TABLE,
    DISCRETE,
    LINEAR,
    GAMMA,
    ERROR
};

struct TransferFunction
{
    TransferType _type;

    // type=TABLE|DISCRETE
    TransferFunction(std::vector<double> table, bool discrete)
        : _type(discrete ? TransferType::DISCRETE : TransferType::TABLE)
    {
        for (unsigned i = 0; i < table.size(); ++i) {
            _table.emplace_back(std::round(std::clamp(table[i], 0.0, 1.0)));
        }
        if (_type == TransferType::TABLE) {
            for (unsigned i = 0; i < _table.size() - 1; ++i) {
                _next.emplace_back(_table[i + 1] - _table[i]);
            }
        }
    }
    std::vector<double> _table;
    std::vector<double> _next; // Shadow table of next - this

    // type=LINEAR
    TransferFunction(double slope, double intercept)
        : _type(TransferType::LINEAR)
        , _slope(slope)
        , _intercept(intercept)
    {}
    double _slope;
    double _intercept;

    // type=GAMMA
    TransferFunction(double amplitude, double exponent, double offset)
        : _type(TransferType::GAMMA)
        , _amplitude(amplitude)
        , _exponent(exponent)
        , _offset(offset)
    {}
    double _amplitude;
    double _exponent;
    double _offset;
};

struct ComponentTransfer
{
    // We expect to get transfer functions in the correct order for the input color space
    explicit ComponentTransfer(std::vector<TransferFunction> functions)
        : _functions(std::move(functions))
    {}

    template <class AccessDst, class AccessSrc>
    void filter(AccessDst &dst, AccessSrc const &src) const
    {
        typename AccessDst::Color out;
        //dst.forEachPixel([&](int x, int y) {
        for (auto y = 0; y < dst.height(); y++) {
        for (auto x = 0; x < dst.width(); x++) {
            auto in = src.colorAt(x, y, true);
            filterColor(in, out);
            dst.colorTo(x, y, out, true);
        }
        }
    }

    template <typename T, typename T2>
    inline void filterColor(T &in, T2 &out) const
    {
        auto const num_i = std::min(in.size(), out.size());
        for (unsigned i = 0; i < num_i; i++) {
            if (i >= _functions.size()) {
                out[i] = in[i];
                continue;
            }
            auto &f = _functions[i];
            switch (f._type) {
                case TransferType::TABLE:
                    if (f._next.empty() || in[i] == 1.0) {
                        out[i] = f._table.back();
                    } else {
                        double iptr;
                        auto dx = std::modf(f._next.size() * in[i], &iptr);
                        unsigned k = iptr;
                        out[i] = f._table[k] * (1.0 - dx) + f._next[k] * dx;
                    }
                    break;
                case TransferType::DISCRETE: {
                    unsigned k = f._table.size() * in[i];
                    out[i] = k == f._table.size() ? f._table.back() : f._table[k];
                    break;
                }
                case TransferType::LINEAR:
                    out[i] = f._slope * in[i] + f._intercept;
                    break;
                case TransferType::GAMMA:
                    out[i] = std::clamp(f._amplitude * std::pow(in[i], f._exponent) + f._offset, 0.0, 1.0);
                    break;
                case TransferType::IDENTITY:
                    out[i] = in[i];
                    break;
                case TransferType::ERROR:
                default:
                    break;
            }
        }
    }

    std::vector<TransferFunction> _functions;
};

} // namespace Inkscape::Renderer::PixelFilter

#endif // INKSCAPE_RENDERER_PIXEL_FILTER_COMPONENT_TRANSFER_H

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
