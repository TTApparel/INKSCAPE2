// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INKSCAPE_COLORS_LINEAR_GRADIENT_AVERAGE_H
#define INKSCAPE_COLORS_LINEAR_GRADIENT_AVERAGE_H

#include <cassert>

#include "manager.h"

namespace Inkscape::Colors {

/// Accepts a sequence of gradient stops and computes the average colour of a linear gradient.
class LinearGradientAverager
{
public:
    /// @param space The space to perform gradient interpolation in.
    explicit LinearGradientAverager(std::shared_ptr<Space::AnySpace> space = Manager::get().find(Space::Type::RGB))
        : _space{std::move(space)}
    {}

    /// @param pos The position of the gradient stop, which must be in [0, 1] and at least the previous stop.
    void addStop(double pos, Color col);

    /// Complete the gradient, extending the last stop up to the end.
    /// @return The average colour of the gradient.
    /// @throws std::logic_error if there are no stops.
    Color finish();

private:
    std::shared_ptr<Space::AnySpace> _space; ///< The interpolation space
    std::vector<double> _accumulated; ///< Weighted sum of premultiplied values with alpha
    struct Stop
    {
        double pos;
        std::vector<double> values; ///< Premultiplied values with alpha
    };
    std::optional<Stop> _last; ///< The last gradient stop added
};

} // namespace Inkscape::Colors

#endif // INKSCAPE_COLORS_LINEAR_GRADIENT_AVERAGE_H
