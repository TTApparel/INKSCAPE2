// SPDX-License-Identifier: GPL-2.0-or-later
#include "linear-gradient-average.h"

#include "color.h"

namespace Inkscape::Colors {
namespace {

std::vector<double> premultiply(std::vector<double> vec)
{
    for (int i = 0; i < vec.size() - 1; i++) {
        vec[i] *= vec.back();
    }
    return vec;
}

std::vector<double> mult(std::vector<double> vec, double x)
{
    for (auto &v : vec) {
        v *= x;
    }
    return vec;
}

std::vector<double> add(std::vector<double> a, std::vector<double> const &b)
{
    assert(a.size() == b.size());
    for (int i = 0; i < a.size(); i++) {
        a[i] += b[i];
    }
    return a;
}

} // namespace

void LinearGradientAverager::addStop(double pos, Color col)
{
    if (!col.convert(_space)) {
        return;
    }
    col.enableOpacity(true);

    auto next = Stop{
        .pos = std::clamp(pos, _last ? _last->pos : 0.0, 1.0),
        .values = premultiply(col.getValues())
    };

    if (!_last) {
        // First stop.
        _accumulated = mult(next.values, next.pos);
    } else {
        // Subsequent stops.
        auto const diff = next.pos - _last->pos;
        _accumulated = add(mult(add(std::move(_last->values), next.values), diff / 2), _accumulated);
    }

    _last = std::move(next);
}

Color LinearGradientAverager::finish()
{
    if (!_last) {
        throw std::logic_error{"Averaging gradient with no stops"};
    }

    if (_last->pos < 1) {
        auto const diff = 1 - _last->pos;
        _accumulated = add(mult(std::move(_last->values), diff), _accumulated);
    }

    // Unpremultiply
    if (_accumulated.back() != 0) {
        for (int i = 0; i < _accumulated.size() - 1; i++) {
            _accumulated[i] /= _accumulated.back();
        }
    }

    return Color{_space, std::move(_accumulated)};
}

} // namespace Inkscape::Colors
