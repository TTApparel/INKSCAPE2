// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Raw filter functions for gaussian blur
 *
 * IIR filtering method based on:
 * L.J. van Vliet, I.T. Young, and P.W. Verbeek, Recursive Gaussian Derivative Filters,
 * in: A.K. Jain, S. Venkatesh, B.C. Lovell (eds.),
 * ICPR'98, Proc. 14th Int. Conference on Pattern Recognition (Brisbane, Aug. 16-20),
 * IEEE Computer Society Press, Los Alamitos, 1998, 509-514.
 *
 * Using the backwards-pass initialization procedure from:
 * Boundary Conditions for Young - van Vliet Recursive Filtering
 * Bill Triggs, Michael Sdika
 * IEEE Transactions on Signal Processing, Volume 54, Number 5 - may 2006
 *
 *//*
 *
 * Authors:
 *   Martin Owens
 *   Niko Kiirala
 *   bulia byak
 *   Jasper van de Gronde
 *
 * Copyright (C) 2006-2025 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_RENDERER_PIXEL_FILTER_GAUSSIAN_BLUR_H
#define INKSCAPE_RENDERER_PIXEL_FILTER_GAUSSIAN_BLUR_H

#include <complex>
#include <vector>
#include <boost/container/small_vector.hpp>
#include <2geom/point.h>

#include "renderer/threading.h"
#include "renderer/pixel-filters/enums.h"

namespace Inkscape::Renderer::PixelFilter {

/*
 * Number of IIR filter coefficients used. Currently only 3 is supported.
 * "Recursive Gaussian Derivative Filters" says this is enough though (and
 * some testing indeed shows that the quality doesn't improve much if larger
 * filters are used).
 */
constexpr size_t N = 3;

/**
 * A static loop when looping template params
 */
template<int I, int N, class F>
constexpr void static_for(F f) {
  if constexpr (I < N) {
    f.template operator()<I>();
    static_for<I + 1,N>(f);
  }
}

template <typename InIt, typename OutIt, typename Size>
inline void copy_n(InIt beg_in, Size N, OutIt beg_out)
{
    std::copy(beg_in, beg_in + N, beg_out);
}

// Type used for IIR filter coefficients (can be 10.21 signed fixed point, see Anisotropic Gaussian Filtering Using
// Fixed Point Arithmetic, Christoph H. Lampert & Oliver Wirjadi, 2006)
using IIRValue = double;
using FIRValue = double;

// Type used for FIR filter coefficients (can be 16.16 unsigned fixed point, should have 8 or more bits in the
// fractional part, the integer part should be capable of storing approximately 20*255)
// using FIRValue = Util::FixedPoint<unsigned int, 16>;

template <typename T>
static inline T sqr(T const &v)
{
    return v * v;
}

template <typename T>
static inline T clip(T const &v, T const &a, T const &b)
{
    if (v < a)
        return a;
    if (v > b)
        return b;
    return v;
}

template <typename Tt, typename Ts>
static inline Tt round_cast(Ts v)
{
    static Ts const rndoffset(.5);
    return static_cast<Tt>(v + rndoffset);
}

/*
template<>
inline unsigned char round_cast(double v) {
    // This (fast) rounding method is based on:
    // http://stereopsis.com/sree/fpu2006.html
#if G_BYTE_ORDER==G_LITTLE_ENDIAN
    double const dmr = 6755399441055744.0;
    v = v + dmr;
    return ((unsigned char*)&v)[0];
#elif G_BYTE_ORDER==G_BIG_ENDIAN
    double const dmr = 6755399441055744.0;
    v = v + dmr;
    return ((unsigned char*)&v)[7];
#else
    static double const rndoffset(.5);
    return static_cast<unsigned char>(v+rndoffset);
#endif
}*/

template <typename Tt, typename Ts>
static inline Tt clip_round_cast(Ts const v)
{
    Ts const minval = std::numeric_limits<Tt>::min();
    Ts const maxval = std::numeric_limits<Tt>::max();
    Tt const minval_rounded = std::numeric_limits<Tt>::min();
    Tt const maxval_rounded = std::numeric_limits<Tt>::max();
    if (v < minval)
        return minval_rounded;
    if (v > maxval)
        return maxval_rounded;
    return round_cast<Tt>(v);
}

template <typename Tt, typename Ts>
static inline Tt clip_round_cast_varmax(Ts const v, Tt const maxval_rounded)
{
    Ts const minval = std::numeric_limits<Tt>::min();
    Tt const maxval = maxval_rounded;
    Tt const minval_rounded = std::numeric_limits<Tt>::min();
    if (v < minval)
        return minval_rounded;
    if (v > maxval)
        return maxval_rounded;
    return round_cast<Tt>(v);
}

struct GaussianBlur
{
    Geom::Point _deviation;

    GaussianBlur(Geom::Point const &deviation)
        : _deviation(deviation)
    {}

    /**
     * Instead of applying an expensive blur to a massive number of pixels, a lower quality
     * value estimates the blur by bluring a little bit and squashing and scaling in that
     * direction instead. It's not as good as bluring, but it's was faster for worse quality.
     */
    static void downsampleForQuality(BlurQuality quality, Geom::IntPoint &size, Geom::Point &deviation)
    {
        Geom::Point step(1 << _effect_subsample_step_log2(deviation[Geom::X], quality),
                         1 << _effect_subsample_step_log2(deviation[Geom::Y], quality));
        if (step[Geom::X] > 1) {
            size[Geom::X] = ceil((double)size[Geom::X] / step[Geom::X]) + 1;
            deviation[Geom::X] /= step[Geom::X];
        }
        if (step[Geom::Y] > 1) {
            size[Geom::Y] = ceil((double)size[Geom::Y] / step[Geom::Y]) + 1;
            deviation[Geom::Y] /= step[Geom::Y];
        }
    }

    template <class Access>
    void filter(Access &surface) const
        //requires (Access::checks_edge)
    {
        auto const pool = get_global_dispatch_pool();
        auto scr_len = get_effect_area_scr();

        // Decide which filter to use for X and Y
        // This threshold was determined by trial-and-error for one specific machine,
        // so there's a good chance that it's not optimal.
        // Whatever you do, don't go below 1 (and preferably not even below 2), as
        // the IIR filter gets unstable there.
        // I/FIR: In/finite impulse response
        bool use_IIR_x = _deviation[Geom::X] > 3;
        bool use_IIR_y = _deviation[Geom::Y] > 3;

        // Temporary storage for IIR filter
        // NOTE: This can be eliminated, but it reduces the precision a bit
        int threads = pool->size();
        std::vector<std::array<IIRValue, Access::channel_total> *> tmpdata(threads, nullptr);
        if (use_IIR_x || use_IIR_y) {
            for (int i = 0; i < threads; ++i) {
                tmpdata[i] =
                    new std::array<IIRValue, Access::channel_total>[std::max(surface.width(), surface.height())];
            }
        }

        if (scr_len[Geom::X] > 0) {
            if (use_IIR_x) {
                gaussian_pass_IIR<Access, Geom::X>(surface, tmpdata.data(), *pool);
            } else {
                gaussian_pass_FIR<Access, Geom::X>(surface, *pool);
            }
        }

        if (scr_len[Geom::Y] > 0) {
            if (use_IIR_y) {
                gaussian_pass_IIR<Access, Geom::Y>(surface, tmpdata.data(), *pool);
            } else {
                gaussian_pass_FIR<Access, Geom::Y>(surface, *pool);
            }
        }

        // free the temporary data
        if (use_IIR_x || use_IIR_y) {
            for (int i = 0; i < threads; ++i) {
                delete[] tmpdata[i];
            }
        }
    }

private:
    inline Geom::IntPoint get_effect_area_scr() const
    {
        return {(int)std::ceil(std::fabs(_deviation[Geom::X]) * 3.0),
                (int)std::ceil(std::fabs(_deviation[Geom::Y]) * 3.0)};
    }

    /**
     * Request or set a color and allow the axis to be flipped. Is always alpha unpremultiplied.
     */
    template <Geom::Dim2 axis, class Access>
    constexpr inline static std::array<IIRValue, Access::channel_total> colorAt(Access &surface, int x, int y)
    {
        if constexpr (axis == Geom::X) {
            return surface.colorAt(x, y, false);
        }
        return surface.colorAt(y, x, false);
    }
    template <Geom::Dim2 axis, class Access>
    constexpr inline static void colorTo(Access &surface, int x, int y,
                                  std::array<IIRValue, Access::channel_total> const &input)
    {
        if constexpr (axis == Geom::X) {
            surface.colorTo(x, y, input, false);
        } else {
            surface.colorTo(y, x, input, false);
        }
    }

    template <class Access, Geom::Dim2 axis>
    void gaussian_pass_IIR(Access &surface, std::array<IIRValue, Access::channel_total> **tmpdata, dispatch_pool &pool) const
    {
        // Filter variables
        IIRValue b[N + 1]; // scaling coefficient + filter coefficients (can be 10.21 fixed point)
        double bf[N];      // computed filter coefficients
        double M[N * N];   // matrix used for initialization procedure (has to be double)

        // Compute filter
        calcFilter(_deviation[axis], bf);
        for (double &i : bf)
            i = -i;
        b[0] = 1; // b[0] == alpha (scaling coefficient)
        for (size_t i = 0; i < N; i++) {
            b[i + 1] = bf[i];
            b[0] -= b[i + 1];
        }

        // Compute initialization matrix
        calcTriggsSdikaM(bf, M);

        int w = surface.width();
        int h = surface.height();
        int col_count = axis == Geom::X ? w : h;
        int row_count = axis == Geom::X ? h : w;

        pool.dispatch(row_count, [&](int row, int tid) {
            // Border constants
            auto imin = colorAt<axis>(surface, 0, row);
            auto imax = colorAt<axis>(surface, col_count - 1, row);

            int col_write = col_count;

            // Forward pass
            std::array<IIRValue, Access::channel_total> u[N + 1];
            for (int i = 0; i < N; i++) {
                u[i] = imin;
            }

            for (int col = 0; col < col_count; col++) {
                for (int i = N; i > 0; i--) {
                    u[i] = u[i - 1];
                }
                u[0] = colorAt<axis>(surface, col, row);
                for (int c = 0; c < Access::channel_total; c++) {
                    u[0][c] *= b[0];
                }
                for (int i = 1; i < N + 1; i++) {
                    for (int c = 0; c < Access::channel_total; c++) {
                        u[0][c] += u[i][c] * b[i];
                    }
                }
                *(tmpdata[tid] + col) = u[0];
            }

            // Backward pass
            std::array<IIRValue, Access::channel_total> v[N + 1];
            calcTriggsSdikaInitialization<Access::channel_total>(M, u, imax, imax, b[0], v);

            colorTo<axis>(surface, --col_write, row, v[0]);

            for (int col = col_count - 2; col >= 0; col--) {
                for (int i = N; i > 0; i--) {
                    v[i] = v[i - 1];
                }
                v[0] = *(tmpdata[tid] + col);

                for (int c = 0; c < Access::channel_total; c++) {
                    v[0][c] *= b[0];
                }
                for (int i = 1; i < N + 1; i++) {
                    for (int c = 0; c < Access::channel_total; c++) {
                        v[0][c] += v[i][c] * b[i];
                    }
                }
                colorTo<axis>(surface, --col_write, row, v[0]);
            }
        });
    }

    template <class Access, Geom::Dim2 axis>
    void gaussian_pass_FIR(Access &surface, dispatch_pool &pool) const
    {
        // Filters over 1 dimension
        auto const kernel = make_kernel<axis>();
        // Assumes kernel is symmetric
        const int scr_len = kernel.size() - 1;
        const int col_count = axis == Geom::X ? surface.width() : surface.height();
        const int row_count = axis == Geom::X ? surface.height() : surface.width();

        //pool.dispatch(row_count, [&](int row, int tid) {
        for (int row = 0; row < row_count; row++) {

        static_for<0,Access::channel_total>([&]<int I>() {

            auto line = surface.template getLineAccess<axis == Geom::Y, I>(row);

            // corresponding line in the source and destination buffers (same for now)
            auto src_line = line.pixels;
            auto dst_line = line.pixels;

            // History initialization, past pixels seen (to enable in-place operation)
            boost::container::small_vector<float, 10> history(scr_len + 1, *src_line);
//            std::cout << " Line: " << row << ", history: ["; for (auto x = 0; x < history.size(); x++) std::cout << (x ? "," : "") << history[x]; std::cout << "]\n";

            for ( int c1 = 0 ; c1 < col_count ; c1++ ) {
                auto const src_disp = src_line + c1 * line.next;
                auto const dst_disp = dst_line + c1 * line.next;

                // update history
                for(int i=scr_len; i>0; i--) history[i] = history[i-1];
                history[0] = *src_disp;
//            std::cout << "  Col: " << c1 << ", history: ["; for (auto x = 0; x < history.size(); x++) std::cout << (x ? "," : "") << history[x]; std::cout << "]\n";

                float sum = 0;
                int last_in = -1;
                int different_count = 0;

                // go over our point's neighbours in the history
                for ( int i = 0 ; i <= scr_len ; i++ ) {
                    // value at the pixel
                    float in_byte = history[i];

                    // is it the same as last one we saw?
                    if(in_byte != last_in) different_count++;
                    last_in = in_byte;

                    // sum pixels weighted by the kernel
                    sum += in_byte * kernel[i];
                }

                // go over our point's neighborhood on x axis in the in buffer
                auto nb_src_disp = src_disp;
                for ( int i = 1 ; i <= scr_len ; i++ ) {
                    // the pixel we're looking at
                    int c1_in = c1 + i;
                    if (c1_in >= col_count) {
                        c1_in = col_count - 1;
                    } else {
                        nb_src_disp += line.next;
                    }

                    // value at the pixel
                    float in_byte = *nb_src_disp;

                    // is it the same as last one we saw?
                    if(in_byte != last_in) different_count++;
                    last_in = in_byte;

                    // sum pixels weighted by the kernel
                    sum += in_byte * kernel[i];
                }

                // store the result in bufx
                *dst_disp = sum;

                // optimization: if there was no variation within this point's neighborhood,
                // skip ahead while we keep seeing the same last_in byte:
                // blurring flat color would not change it anyway
                if (different_count <= 1) { // note that different_count is at least 1, because last_in is initialized to -1
                    auto nb_src_disp = src_disp + (1+scr_len)*line.next;
                    auto nb_dst_disp = dst_disp + (1)        *line.next;
                    while(c1 + 1 + scr_len < col_count && *nb_src_disp == last_in) {
                        *nb_dst_disp = last_in;
                        nb_src_disp += line.next;
                        nb_dst_disp += line.next;
                        c1++; // skip the next iter
                    }
                }
            }
        });
        }
    }

    template <Geom::Dim2 axis>
    std::vector<FIRValue> make_kernel() const
    {
        int const scr_len = get_effect_area_scr()[axis];
        if (scr_len < 0) {
            throw std::exception();
        }
        std::vector<FIRValue> kernel(scr_len + 1);
        _make_kernel(&kernel[0], scr_len, _deviation[axis]);
        return kernel;
    }

    void _make_kernel(FIRValue *const kernel, int const scr_len, double const deviation) const
    {
        double const d_sq = sqr(deviation) * 2;
        boost::container::small_vector<double, 10> k(scr_len + 1); // This is only called for small kernel sizes (above
                                                                   // approximately 10 coefficients the IIR filter is
                                                                   // used)

        // Compute kernel and sum of coefficients
        // Note that actually only half the kernel is computed, as it is symmetric
        double sum = 0;
        for (int i = scr_len; i >= 0; i--) {
            k[i] = std::exp(-sqr(i) / d_sq);
            if (i > 0)
                sum += k[i];
        }
        // the sum of the complete kernel is twice as large (plus the center element which we skipped above to prevent
        // counting it twice)
        sum = 2 * sum + k[0];

        // Normalize kernel (making sure the sum is exactly 1)
        double ksum = 0;
        FIRValue kernelsum = 0;
        for (int i = scr_len; i >= 1; i--) {
            ksum += k[i] / sum;
            kernel[i] = ksum - static_cast<double>(kernelsum);
            kernelsum += kernel[i];
        }
        kernel[0] = FIRValue(1) - 2 * kernelsum;
    }

    // Return value (v) should satisfy:
    //  2^(2*v)*255<2^32
    //  255<2^(32-2*v)
    //  2^8<=2^(32-2*v)
    //  8<=32-2*v
    //  2*v<=24
    //  v<=12
    static int _effect_subsample_step_log2(double const deviation, BlurQuality const quality)
    {
        // To make sure FIR will always be used (unless the kernel is VERY big):
        //  deviation/step <= 3
        //  deviation/3 <= step
        //  log(deviation/3) <= log(step)
        // So when x below is >= 1/3 FIR will almost always be used.
        // This means IIR is almost only used with the modes BETTER or BEST.
        int stepsize_l2;
        switch (quality) {
            case BlurQuality::WORST:
                // 2 == log(x*8/3))
                // 2^2 == x*2^3/3
                // x == 3/2
                stepsize_l2 = clip(static_cast<int>(log(deviation * (3. / 2.)) / log(2.)), 0, 12);
                break;
            case BlurQuality::WORSE:
                // 2 == log(x*16/3))
                // 2^2 == x*2^4/3
                // x == 3/2^2
                stepsize_l2 = clip(static_cast<int>(log(deviation * (3. / 4.)) / log(2.)), 0, 12);
                break;
            case BlurQuality::BETTER:
                // 2 == log(x*32/3))
                // 2 == x*2^5/3
                // x == 3/2^4
                stepsize_l2 = clip(static_cast<int>(log(deviation * (3. / 16.)) / log(2.)), 0, 12);
                break;
            case BlurQuality::BEST:
                stepsize_l2 = 0; // no subsampling at all
                break;
            case BlurQuality::NORMAL:
            default:
                // 2 == log(x*16/3))
                // 2 == x*2^4/3
                // x == 3/2^3
                stepsize_l2 = clip(static_cast<int>(log(deviation * (3. / 8.)) / log(2.)), 0, 12);
                break;
        }
        return stepsize_l2;
    }

    static void calcFilter(double const sigma, double b[N])
    {
        assert(N == 3);
        std::complex<double> const d1_org(1.40098, 1.00236);
        double const d3_org = 1.85132;
        double qbeg = 1; // Don't go lower than sigma==2 (we'd probably want a normal convolution in that case anyway)
        double qend = 2 * sigma;
        double const sigmasqr = sqr(sigma);
        do { // Binary search for right q (a linear interpolation scheme is suggested, but this should work fine as
             // well)
            double const q = (qbeg + qend) / 2;
            // Compute scaled filter coefficients
            std::complex<double> const d1 = pow(d1_org, 1.0 / q);
            double const d3 = pow(d3_org, 1.0 / q);
            // Compute actual sigma^2
            double const ssqr = 2 * (2 * (d1 / sqr(d1 - 1.)).real() + d3 / sqr(d3 - 1.));
            if (ssqr < sigmasqr) {
                qbeg = q;
            } else {
                qend = q;
            }
        } while (qend - qbeg > (sigma / (1 << 30)));
        // Compute filter coefficients
        double const q = (qbeg + qend) / 2;
        std::complex<double> const d1 = pow(d1_org, 1.0 / q);
        double const d3 = pow(d3_org, 1.0 / q);
        double const absd1sqr = std::norm(d1); // d1*d2 = d1*conj(d1) = |d1|^2 = std::norm(d1)
        double const re2d1 = 2 * d1.real();    // d1+d2 = d1+conj(d1) = 2*real(d1)
        double const bscale = 1.0 / (absd1sqr * d3);
        b[2] = -bscale;
        b[1] = bscale * (d3 + re2d1);
        b[0] = -bscale * (absd1sqr + d3 * re2d1);
    }

    static void calcTriggsSdikaM(double const b[N], double M[N * N])
    {
        assert(N == 3);
        double a1 = b[0], a2 = b[1], a3 = b[2];
        double const Mscale = 1.0 / ((1 + a1 - a2 + a3) * (1 - a1 - a2 - a3) * (1 + a2 + (a1 - a3) * a3));
        M[0] = 1 - a2 - a1 * a3 - sqr(a3);
        M[1] = (a1 + a3) * (a2 + a1 * a3);
        M[2] = a3 * (a1 + a2 * a3);
        M[3] = a1 + a2 * a3;
        M[4] = (1 - a2) * (a2 + a1 * a3);
        M[5] = a3 * (1 - a2 - a1 * a3 - sqr(a3));
        M[6] = a1 * (a1 + a3) + a2 * (1 - a2);
        M[7] = a1 * (a2 - sqr(a3)) + a3 * (1 + a2 * (a2 - 1) - sqr(a3));
        M[8] = a3 * (a1 + a2 * a3);
        for (unsigned int i = 0; i < 9; i++)
            M[i] *= Mscale;
    }

    template <int channel_total>
    static void calcTriggsSdikaInitialization(double const M[N * N], std::array<IIRValue, channel_total> const uold[N],
                                              std::array<IIRValue, channel_total> const &uplus,
                                              std::array<IIRValue, channel_total> const &vplus, IIRValue alpha,
                                              std::array<IIRValue, channel_total> vold[N])
    {
        for (int c = 0; c < channel_total; c++) {
            double uminp[N];
            for (unsigned int i = 0; i < N; i++)
                uminp[i] = uold[i][c] - uplus[c];
            for (unsigned int i = 0; i < N; i++) {
                double voldf = 0;
                for (unsigned int j = 0; j < N; j++) {
                    voldf += uminp[j] * M[i * N + j];
                }
                // Properly takes care of the scaling coefficient alpha and vplus (which is already appropriately
                // scaled) This was arrived at by starting from a version of the blur filter that ignored the scaling
                // coefficient (and scaled the final output by alpha^2) and then gradually reintroducing the scaling
                // coefficient.
                vold[i][c] = voldf * alpha;
                vold[i][c] += vplus[c];
            }
        }
    }
};

} // namespace Inkscape::Renderer::PixelFilter

#endif // INKSCAPE_RENDERER_PIXEL_FILTER_GAUSSIAN_BLUR_H

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
