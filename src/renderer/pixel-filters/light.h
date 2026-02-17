// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Diffuse Lighting raw filtering
 *//*
 * Authors:
 *   Niko Kiirala
 *   Jean-Rene Reinhard
 *   Martin Owens
 *
 * Copyright (C) 2014-2025 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_RENDERER_PIXEL_FILTER_LIGHT_H
#define INKSCAPE_RENDERER_PIXEL_FILTER_LIGHT_H

#include <algorithm>
#include <vector>
#include <2geom/affine.h>

namespace Inkscape::Renderer::PixelFilter {

inline constexpr int X_3D = 0;
inline constexpr int Y_3D = 1;
inline constexpr int Z_3D = 2;

using vector3d = std::array<double, 3>;

// The eye vector for specular lighting
inline constexpr vector3d EYE_VECTOR = {0.0, 0.0, 1.0};

/**
 * returns the euclidean norm of the vector v
 *
 * \param v a reference to a vector with double components
 * \return the euclidean norm of v
 */
double norm(vector3d const &v)
{
    return sqrt(v[X_3D] * v[X_3D] + v[Y_3D] * v[Y_3D] + v[Z_3D] * v[Z_3D]);
}

/**
 * Normalizes a vector
 *
 * \param v a reference to a vector to normalize
 */
void normalize_vector(vector3d &v)
{
    double nv = norm(v);
    // TODO test nv == 0
    for (int j = 0; j < 3; j++) {
        v[j] /= nv;
    }
}

/**
 * Computes the scalar product between two vector3ds
 *
 * \param a a vector3d reference
 * \param b a vector3d reference
 * \return the scalar product of a and b
 */
double scalar_product(vector3d const &a, vector3d const &b)
{
    return a[X_3D] * b[X_3D] + a[Y_3D] * b[Y_3D] + a[Z_3D] * b[Z_3D];
}

/**
 * Computes the normalized sum of two vector3ds
 *
 * \param r a vector3d reference where we store the result
 * \param a a vector3d reference
 * \param b a vector3d reference
 */
void normalized_sum(vector3d &r, vector3d const &a, vector3d const &b)
{
    r[X_3D] = a[X_3D] + b[X_3D];
    r[Y_3D] = a[Y_3D] + b[Y_3D];
    r[Z_3D] = a[Z_3D] + b[Z_3D];
    normalize_vector(r);
}

/**
 * Applies the transformation matrix to (x, y, z). This function assumes that
 * trans[0] = trans[3]. x and y are transformed according to trans, z is
 * multiplied by trans[0].
 *
 * \param coords a reference to coordinates
 * \param trans a reference to a transformation matrix
 * \param device_scale an optional device scale for HiDPI
 */
void convert_coord(vector3d &coords, Geom::Affine const &trans, double device_scale)
{
    Geom::Point p = Geom::Point(coords[X_3D], coords[Y_3D]) * device_scale * trans;
    coords[X_3D] = p.x();
    coords[Y_3D] = p.y();
    coords[Z_3D] *= device_scale * trans[0];
}

/**
 * Base functionality for diffuse and specular lighting filters
 */
struct Lighting
{
    Lighting(double scale, double light_constant, std::optional<double> specular_exponent)
        : _specular(specular_exponent)
        , _scale(scale)
        , _const(light_constant)
        , _exp(_specular ? *specular_exponent : 1.0)
    {}

protected:
    template <class AccessSrc, typename Output>
    void doLighting(AccessSrc const &src, int x, int y, vector3d light,
                    std::array<double, AccessSrc::channel_total> const &color,
                    Output &output) const
    {
        if (_specular) {
            normalized_sum(light, light, EYE_VECTOR);
        }
        vector3d normal = surfaceNormalAt(src, x, y, _scale);
        double sp = scalar_product(normal, light);
        double k = sp <= 0.0 ? 0.0 : _const * std::pow(sp, _exp);

        for (unsigned i = 0; i < color.size() - 1 && i < output.size(); i++) {
            output[i] = std::clamp(k * color[i], 0.0, 1.0);
            output.back() = std::max(output[i], output.back());
        }
    }

    // compute surface normal at given coordinates using 3x3 Sobel gradient filter
    template <class AccessSrc>
    vector3d surfaceNormalAt(AccessSrc const &src, int x, int y, double scale) const
    {
        // Below there are some multiplies by zero. They will be optimized out.
        // Do not remove them, because they improve readability.
        // NOTE: fetching using src.alphaAt is slightly lazy.
        vector3d normal;
        double fx = -scale, fy = -scale;
        normal[Z_3D] = 1.0;
        if (x == 0) [[unlikely]] {
            // leftmost column
            if (y == 0) [[unlikely]] {
                // upper left corner
                fx *= (2.0 / 3.0);
                fy *= (2.0 / 3.0);
                double p00 = src.alphaAt(x, y), p10 = src.alphaAt(x + 1, y),
                       p01 = src.alphaAt(x, y + 1), p11 = src.alphaAt(x + 1, y + 1);
                normal[X_3D] =
                    -2.0 * p00 +2.0 * p10
                    -1.0 * p01 +1.0 * p11;
                normal[Y_3D] =
                    -2.0 * p00 -1.0 * p10
                    +2.0 * p01 +1.0 * p11;
            } else if (y == (src.height() - 1)) [[unlikely]] {
                // lower left corner
                fx *= (2.0 / 3.0);
                fy *= (2.0 / 3.0);
                double p00 = src.alphaAt(x, y - 1), p10 = src.alphaAt(x + 1, y - 1),
                       p01 = src.alphaAt(x, y    ), p11 = src.alphaAt(x + 1, y);
                normal[X_3D] =
                    -1.0 * p00 +1.0 * p10
                    -2.0 * p01 +2.0 * p11;
                normal[Y_3D] =
                    -2.0 * p00 -1.0 * p10
                    +2.0 * p01 +1.0 * p11;
            } else {
                // leftmost column
                fx *= (1.0 / 2.0);
                fy *= (1.0 / 3.0);
                double p00 = src.alphaAt(x, y - 1), p10 = src.alphaAt(x + 1, y - 1),
                       p01 = src.alphaAt(x, y    ), p11 = src.alphaAt(x + 1, y),
                       p02 = src.alphaAt(x, y + 1), p12 = src.alphaAt(x + 1, y + 1);
                normal[X_3D] =
                    -1.0 * p00 +1.0 * p10
                    -2.0 * p01 +2.0 * p11
                    -1.0 * p02 +1.0 * p12;
                normal[Y_3D] =
                    -2.0 * p00 -1.0 * p10
                    +0.0 * p01 +0.0 * p11 // this will be optimized out
                    +2.0 * p02 +1.0 * p12;
            }
        } else if (x == (src.width() - 1)) [[unlikely]] {
            // rightmost column
            if (y == 0) [[unlikely]] {
                // top right corner
                fx *= (2.0 / 3.0);
                fy *= (2.0 / 3.0);
                double p00 = src.alphaAt(x - 1, y    ), p10 = src.alphaAt(x, y),
                       p01 = src.alphaAt(x - 1, y + 1), p11 = src.alphaAt(x, y + 1);
                normal[X_3D] =
                    -2.0 * p00 +2.0 * p10
                    -1.0 * p01 +1.0 * p11;
                normal[Y_3D] = 
                    -1.0 * p00 -2.0 * p10
                    +1.0 * p01 +2.0 * p11;
            } else if (y == (src.height() - 1)) [[unlikely]] {
                // bottom right corner
                fx *= (2.0 / 3.0);
                fy *= (2.0 / 3.0);
                double p00 = src.alphaAt(x - 1, y - 1), p10 = src.alphaAt(x, y - 1),
                       p01 = src.alphaAt(x - 1, y    ), p11 = src.alphaAt(x, y);
                normal[X_3D] =
                    -1.0 * p00 +1.0 * p10
                    -2.0 * p01 +2.0 * p11;
                normal[Y_3D] = 
                    -1.0 * p00 -2.0 * p10
                    +1.0 * p01 +2.0 * p11;
            } else {
                // rightmost column
                fx *= (1.0 / 2.0);
                fy *= (1.0 / 3.0);
                double p00 = src.alphaAt(x - 1, y - 1), p10 = src.alphaAt(x, y - 1),
                       p01 = src.alphaAt(x - 1, y    ), p11 = src.alphaAt(x, y),
                       p02 = src.alphaAt(x - 1, y + 1), p12 = src.alphaAt(x, y + 1);
                normal[X_3D] =
                    -1.0 * p00 +1.0 * p10
                    -2.0 * p01 +2.0 * p11
                    -1.0 * p02 +1.0 * p12;
                normal[Y_3D] =
                    -1.0 * p00 -2.0 * p10
                    +0.0 * p01 +0.0 * p11
                    +1.0 * p02 +2.0 * p12;
            }
        } else {
            // interior
            if (y == 0) [[unlikely]] {
                // top row
                fx *= (1.0 / 3.0);
                fy *= (1.0 / 2.0);
                double p00 = src.alphaAt(x - 1, y    ), p10 = src.alphaAt(x, y    ), p20 = src.alphaAt(x + 1, y    ),
                       p01 = src.alphaAt(x - 1, y + 1), p11 = src.alphaAt(x, y + 1), p21 = src.alphaAt(x + 1, y + 1);
                normal[X_3D] =
                    -2.0 * p00 +0.0 * p10 +2.0 * p20
                    -1.0 * p01 +0.0 * p11 +1.0 * p21;
                normal[Y_3D] =
                    -1.0 * p00 -2.0 * p10 -1.0 * p20
                    +1.0 * p01 +2.0 * p11 +1.0 * p21;
            } else if (y == (src.height() - 1)) [[unlikely]] {
                // bottom row
                fx *= (1.0 / 3.0);
                fy *= (1.0 / 2.0);
                double p00 = src.alphaAt(x - 1, y - 1), p10 = src.alphaAt(x, y - 1), p20 = src.alphaAt(x + 1, y - 1),
                       p01 = src.alphaAt(x - 1, y    ), p11 = src.alphaAt(x, y    ), p21 = src.alphaAt(x + 1, y    );
                normal[X_3D] =
                    -1.0 * p00 +0.0 * p10 +1.0 * p20
                    -2.0 * p01 +0.0 * p11 +2.0 * p21;
                normal[Y_3D] =
                    -1.0 * p00 -2.0 * p10 -1.0 * p20
                    +1.0 * p01 +2.0 * p11 +1.0 * p21;
            } else {
                // interior pixels
                // note: p11 is actually unused, so we don't fetch its value
                fx *= (1.0 / 4.0);
                fy *= (1.0 / 4.0);
                double p00 = src.alphaAt(x - 1, y - 1), p10 = src.alphaAt(x, y - 1), p20 = src.alphaAt(x + 1, y - 1),
                       p01 = src.alphaAt(x - 1, y    ), p11 = 0.0,                   p21 = src.alphaAt(x + 1, y    ),
                       p02 = src.alphaAt(x - 1, y + 1), p12 = src.alphaAt(x, y + 1), p22 = src.alphaAt(x + 1, y + 1);
                normal[X_3D] =
                    -1.0 * p00 +0.0 * p10 +1.0 * p20
                    -2.0 * p01 +0.0 * p11 +2.0 * p21
                    -1.0 * p02 +0.0 * p12 +1.0 * p22;
                normal[Y_3D] =
                    -1.0 * p00 -2.0 * p10 -1.0 * p20
                    +0.0 * p01 +0.0 * p11 +0.0 * p21
                    +1.0 * p02 +2.0 * p12 +1.0 * p22;
            }
        }
        normal[X_3D] *= fx;
        normal[Y_3D] *= fy;
        normalize_vector(normal);
        return normal;
    }

    bool _specular;
    double _scale;
    double _const;
    double _exp;
};

struct DistantLight : public Lighting
{
    DistantLight(double azimuth, double elevation, std::vector<double> color, double scale, double light_constant,
                 std::optional<double> specular_exponent = {})
        : Lighting(scale, light_constant, specular_exponent)
        , _azimuth(M_PI / 180 * azimuth)
        , _elevation(M_PI / 180 * elevation)
        , _color(color)
        // Computes the light vector of the distant light
        , _lightv{std::cos(_azimuth) * std::cos(_elevation), std::sin(_azimuth) * std::cos(_elevation),
                  std::sin(_elevation)}
    {}

    template <class AccessDst, class AccessSrc>
    void filter(AccessDst &dst, AccessSrc const &src) const
        requires (AccessSrc::checks_edge)
    {
        typename AccessSrc::Color lit_color;
        typename AccessDst::Color output;
        // Conversion of color here
        for (auto i = 0; i < _color.size(); i++) {
            lit_color[i] = _color[i];
        }
        if (!_specular) { // Diffuse is alpha 1.0
            output.back() = 1.0;
        }
        for (int y = 0; y < dst.height(); y++) {
            for (int x = 0; x < dst.width(); x++) {
                doLighting(src, x, y, _lightv, lit_color, output);
                dst.colorTo(x, y, output, true);
            }
        }
    }

private:
    double _azimuth;
    double _elevation;
    std::vector<double> _color;
    vector3d _lightv;
};

/**
 * @arg device_scale - high DPI monitors
 * @arg trans - The transformation between absolute coordinate use in the svg
 *              and current coordinate used in the rendering
 */
struct PointLight : public Lighting
{
    PointLight(vector3d coords, double x0, double y0, Geom::Affine const &trans, int device_scale,
               std::vector<double> color, double scale, double light_constant,
               std::optional<double> specular_exponent = {})
        : Lighting(scale, light_constant, specular_exponent)
        , _coords(coords)
        , _x0(x0)
        , _y0(y0)
        , _color(color)
    {
        // Computes the light vector of the distant light at point (x,y,z)
        convert_coord(_coords, trans, device_scale);
    }

    template <class AccessDst, class AccessSrc>
    void filter(AccessDst &dst, AccessSrc const &src) const
        requires (AccessSrc::checks_edge)
    {
        typename AccessSrc::Color lit_color;
        typename AccessDst::Color output;
        // Conversion of color here
        for (auto i = 0; i < _color.size(); i++) {
            lit_color[i] = _color[i];
        }
        if (!_specular) { // Diffuse is alpha 1.0
            output.back() = 1.0;
        }
        for (int y = 0; y < dst.height(); y++) {
            for (int x = 0; x < dst.width(); x++) {
                vector3d light{_coords[X_3D] - (_x0 + x), _coords[Y_3D] - (_y0 + y),
                               _coords[Z_3D] - _scale * src.alphaAt(x, y)};
                normalize_vector(light);
                doLighting(src, x, y, light, lit_color, output);
                dst.colorTo(x, y, output, true);
            }
        }
    }

private:
    vector3d _coords;
    double _x0, _y0;
    std::vector<double> _color;
};

struct SpotLight : public Lighting
{
    SpotLight(vector3d coords, vector3d pointAt, double limitingConeAngle, double specularExponent, double x0,
              double y0, Geom::Affine const &trans, int device_scale, std::vector<double> color, double scale,
              double light_constant, std::optional<double> specular_exponent = {})
        : Lighting(scale, light_constant, specular_exponent)
        , _coords(coords)
        , _pointAt(pointAt)
        , _cos_lca(std::cos(M_PI / 180 * limitingConeAngle))
        , _spe_exp(specularExponent)
        , _color(color)
        , _x0(x0)
        , _y0(y0)
    {
        convert_coord(_coords, trans, device_scale);
        convert_coord(_pointAt, trans, device_scale);
        S = {_pointAt[X_3D] - _coords[X_3D], _pointAt[Y_3D] - _coords[Y_3D], _pointAt[Z_3D] - _coords[Z_3D]};
        normalize_vector(S);
    }

    template <class AccessDst, class AccessSrc>
    void filter(AccessDst &dst, AccessSrc const &src) const
        requires (AccessSrc::checks_edge)
    {
        typename AccessSrc::Color lit_color;
        typename AccessDst::Color output;
        if (!_specular) { // Diffuse is alpha 1.0
            output.back() = 1.0;
        }

        for (int y = 0; y < dst.height(); y++) {
            for (int x = 0; x < dst.width(); x++) {
                vector3d light{_coords[X_3D] - (_x0 + x), _coords[Y_3D] - (_y0 + y),
                               _coords[Z_3D] - _scale * src.alphaAt(x, y)};
                normalize_vector(light);

                double spmod = (-1) * scalar_product(light, S);
                if (spmod <= _cos_lca) {
                    spmod = 0;
                } else {
                    spmod = std::pow(spmod, _spe_exp);
                }
                for (unsigned i = 0; i < _color.size() - 1; i++) {
                    lit_color[i] = _color[i] * spmod;
                }
                doLighting(src, x, y, light, lit_color, output);
                dst.colorTo(x, y, output, true);
            }
        }
    }

private:
    // light position coordinates in render setting
    vector3d _coords;
    vector3d _pointAt;
    double _cos_lca; // cos of the limiting cone angle
    double _spe_exp; // specular exponent;

    std::vector<double> _color;
    double _x0, _y0;

    vector3d S; // unit vector from light position in the direction
                // the spot point at
};

} // namespace Inkscape::Renderer::PixelFilter

#endif // INKSCAPE_RENDERER_PIXEL_FILTER_LIGHT_H

/*
  ;Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :
