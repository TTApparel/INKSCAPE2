// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Access the memory of a surface of pixels in a predictable way.
 *//*
 * Authors:
 *   Martin Owens
 *
 * Copyright (C) 2025 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_RENDERER_PIXEL_ACCESS_H
#define INKSCAPE_RENDERER_PIXEL_ACCESS_H

#include <algorithm>
#include <cmath>
#include <iostream>
#include <type_traits>
#include <cairomm/surface.h>
#include <glib.h>

#include "helper/mathfns.h"
#include "threading.h"

/**
 * Terms:
 *
 *  Color - Is a collection of channels, plus an alpha of an inkscape color space.
 *  Channel - Is one of those color space double values where alpha is always the last
 *            item. For example in CMYKA, C is channel 0, M is 1 and A is 4
 *  Surface - Is a collection of Cairo pixels in a 2d grid with a specific stride.
 *  Pixel - A collection of one OR four Primaries packed into this
 *          surface grid. These may be floats or integers of various scales.
 *  Primary - One of the values packed into a pixel. These get turned into channels
 *            through unpacking of specific memory locations.
 *  Coordinates - Or Coords, are a pair of X,Y values within the surface image.
 *  Position - A single memory address offset which a coordinate can be transformed
 *             into to locate the pixel or primary in the surface memory.
 */
namespace Inkscape::Renderer {

/**
 * What to do when a x/y coordinate is outside the width and height. This happens
 * when filters are asking for small grids of pixels.
 */
enum class PixelAccessEdgeMode
{
    NO_CHECK = 0, // No edge checking needed, crash if out of bounds
    ERROR,  // Raise an error
    EXTEND, // Clamp the x,y to 0,0,w,h
    WRAP,   // Treat surface as a spherical space
    ZERO,   // Return zero for getter, and ignore OOB setter
};

/**
 * Image surface memory access for different types which can span multiple surfaces.
 *
 * @template_arg format           - The cairo type for this pixel access.
 * @template_arg channel_count    - The total number of channels in this format across all surfaces.
 * @template_arg edge_mode        - Set the edge checking and how out of range x,y coordinates treated
 * @template_arg primary_override - Optionally override primary count for accessing contiguous surfaces.
 */
template <cairo_format_t format, int channel_count, PixelAccessEdgeMode edge_mode = PixelAccessEdgeMode::NO_CHECK, int primary_override = 0>
class PixelAccess
{
public:
    // Is the format an integer based format
    constexpr static bool is_integer = format != CAIRO_FORMAT_RGBA128F;

    // How many primaries are there in this format
    constexpr static int primary_count = primary_override ? primary_override : (format == CAIRO_FORMAT_A8 ? 0 : 3);
    constexpr static int primary_total = primary_count + 1; // Plus Alpha

    // The internal type used by each channel in the format
    using PrimaryType = std::conditional_t<is_integer, unsigned char, float>;

    // Provides the size of the primary in memory as number of bytes
    constexpr static int primary_size = sizeof(PrimaryType);

    // Allows for compile time requires that edge mode be set
    constexpr static bool checks_edge = edge_mode != PixelAccessEdgeMode::NO_CHECK;

    // Scale of each primary to convert to a double used in Channels
    constexpr static double primary_scale = is_integer ? 255.0 : 1.0;

    // Position of the alpha primary in this format
    constexpr static int primary_alpha = is_integer ? 0 : primary_count;

    // Does this PixelAccess need two surfaces?
    constexpr static bool has_more_channels = channel_count > primary_count;

    // Actual number of channels when including alpha
    constexpr static int channel_total = channel_count + 1;
    using Color = std::array<double, channel_total>;

    /**
     * Create a pixel access object for the given cairo surface.
     *
     * @arg cairo_surface - The Cairo Surface to gain memory access to.
     * @arg next_surface  - Optionally add another surface to handle color interpolation
     *                      in spaces like CMYKA with more than 3 primaries.
     */
    explicit PixelAccess(Cairo::RefPtr<Cairo::ImageSurface> cairo_surface,
                           Cairo::RefPtr<Cairo::ImageSurface> next_surface = {})
        requires(channel_count <= primary_count * (has_more_channels + 1))
        : _width(cairo_surface->get_width())
        , _height(cairo_surface->get_height())
        , _stride(cairo_surface->get_stride() / primary_size)
        , _size(_height * _stride)
        , _memory(reinterpret_cast<PrimaryType *>(cairo_surface->get_data()))
        , _cairo_surface(cairo_surface)
        , _next_surface(next_surface)
    {
        if (cairo_image_surface_get_format(cairo_surface->cobj()) != format) {
            std::cerr << "format of the cairo surface doesn't match the PixelAccess type.\n";
            throw std::exception();
        }
        _cairo_surface->flush(); // This pairs with mark_dirty in ~PixelAccess

        if constexpr (has_more_channels) {
            if (_width != _next_surface->get_width() || _height != _next_surface->get_height() ||
                _stride != _next_surface->get_stride() / primary_size ||
                cairo_image_surface_get_format(_next_surface->cobj()) != format) {
                std::cerr << "Pixel Access Next Surface must be the same formats.\n";
                throw std::exception();
            }
            _next_memory = reinterpret_cast<PrimaryType *>(_next_surface->get_data());
            _next_surface->flush();
        }
    }

    ~PixelAccess()
    {
        // TODO: Find a way to do this only for changed surfaces
        if (_cairo_surface) {
            _cairo_surface->mark_dirty();
        }
        if constexpr (has_more_channels) {
            _next_surface->mark_dirty();
        }
    }

    /**
     * Create access to a patch of memory which isn't part of a cairo surface. This can be used
     * to do color convertions using lcms2 and run filters on the same memory without needing
     * to convert to cairo formats first.
     */
    PixelAccess(std::vector<PrimaryType> memory, int width, int height)
        : _local_memory(std::move(memory))
        , _width(width)
        , _height(height)
        , _stride(width * primary_total)
        , _size(_height * _stride)
        , _memory(_local_memory.data())
    {}

    /**
     * Get a color from the surface at the given coordinates.
     *
     * @arg x - The pixel x coordinate to get
     * @arg y - The pixel y coordinate to get
     * @arg unmultiply_alpha - Remove premultiplied alpha if true
     *
     * @return - The pre-sized memory for the returned color space including alpha.
     *           We use the same memory so we don't have to re-allocate for every pixel in a filter.
     */
    inline Color colorAt(int x, int y, bool unmultiply_alpha = false) const
    {
        Color ret;
        int pos = _pixel_pos(x, y);
        double alpha = _get_alpha(pos);
        double alpha_mult = unmultiply_alpha ? _mult(alpha) : 1.0;
        for (int c = 0; c < channel_count; c++) {
            ret[c] = _get_channel(pos, c, alpha_mult);
        }
        ret[channel_count] = alpha;
        return ret;
    }

    /**
     * Using bilinear interpolation get the effective pixel at the given coordinates.
     * Note: Bilinear interpolation is two linear interpolations across 4 pixels
     *
     * @arg x - The fractional position in the x coordinate to get.
     * @arg y - The fractional position in the y coordinate to get.
     * @arg unmultiply_alpha - Remove premultiplied alpha if true
     *
     * @return_arg - The pre-sized memory for the returned color space including alpha.
     *               We use the same memory so we don't have to re-allocate for every pixel in a filter.
     */
    Color colorAt(double x, double y, bool unmultiply_alpha = false) const
    {
        int fx = floor(x), fy = floor(y);
        int cx = ceil(x), cy = ceil(y);
        double weight_x = x - fx, weight_y = y - fy;

        Color ret;
        for (int c = 0; c < channel_total; c++) {
            ret[c] = _bilinear_interpolate(
                _get_channel(_pixel_pos(fx, fy), c, 1.0), _get_channel(_pixel_pos(cx, fy), c, 1.0),
                _get_channel(_pixel_pos(fx, cy), c, 1.0), _get_channel(_pixel_pos(cx, cy), c, 1.0), weight_x, weight_y);
        }
        if (unmultiply_alpha) {
            auto alpha_mult = _mult(ret[channel_count]);
            for (int c = 0; c < channel_count; c++) {
                ret[c] *= alpha_mult;
            }
        }
        return ret;
    }

    /**
     * Set the given pixel to the color values, apply premultiplication of alpha if neccessary to
     * keep the surface in a premultiplied state for further drawing operations.
     *
     * @arg x - The x coordinate to set
     * @arg y - The y coordinate to set
     * @arg values - The color values to set
     * @arg not_premultiplied - If true, values are premultiplied before saving
     *
     * @arg values - A set of doubles to apply to the pixel data
     */
    void colorTo(int x, int y, Color const &values, bool unmultiply_alpha = false)
    {
        _set_primaries_recursively(_pixel_pos(x, y), values[channel_count], values, unmultiply_alpha);
    }

    /**
     * Return the alpha compnent only.
     *
     * @arg x - The x coordinate to get
     * @arg y - The y coordinate to get
     *
     * @returns The alpha channel at the given coordinates
     */
    double alphaAt(int x, int y) const { return _get_alpha(_pixel_pos(x, y)); }

    /**
     * Use bilinear interpolation to get an alpha channel value inbetween pixels.
     */
    double alphaAt(double x, double y) const
    {
        int fx = floor(x), fy = floor(y);
        int cx = ceil(x), cy = ceil(y);
        double weight_x = x - fx, weight_y = y - fy;

        return _bilinear_interpolate(_get_alpha(_pixel_pos(fx, fy)), _get_alpha(_pixel_pos(cx, fy)),
                                     _get_alpha(_pixel_pos(fx, cy)), _get_alpha(_pixel_pos(cx, cy)), weight_x,
                                     weight_y);
    }

    /**
     * Set the alpha channel
     *
     * @arg x - The x coordinate to set
     * @arg y - The y coordinate to set
     */
    void alphaTo(int x, int y, double value) { return _set_alpha(_pixel_pos(x, y), value); }

    /**
     * Get the width of the surface image
     */
    int width() const { return _width; }

    /**
     * Get the height of the surface image
     */
    int height() const { return _height; }

    /**
     * Get the calculated stride for the surface
     */
    int stride() const { return _stride; }

    /**
     * Get the number of output channels minus alpha
     */
    static int getOutputChannels() { return channel_count; }

    /**
     * Get access to the memory directly
     *
     * @template_arg - ForChannel provides access to the memory of the second buffer.
     */
    template<int ForChannel = -1>
    PrimaryType *memory()
        requires(!has_more_channels || ForChannel >= 0)
    {
        if constexpr (has_more_channels && ForChannel >= primary_count && ForChannel != channel_count) {
            return _next_memory;
        }
        return _memory;
    }
    template<int ForChannel = -1>
    PrimaryType const *memory() const
        requires(!has_more_channels || ForChannel >= 0)
    {
        if constexpr (has_more_channels && ForChannel >= primary_count && ForChannel != channel_count) {
            return _next_memory;
        }
        return _memory;
    }

    /**
     * Get access to the contiguous memory, if any
     */
    auto const &local_memory() const { return _local_memory; }

    /**
     * Create a block of contiguous memory suitable for this pixel surface.
     *
     * @arg copy - If false, doesn't copy and returns an empty surface
     * @arg unpremultiply_alpha - If true, unmultiplies the alpha on copy.
     *
     * @returns A vector or the requested memory types
     */
    template <typename T0>
    auto contiguousMemory(bool copy = true, bool unpremultiply_alpha = false) const
    {
        std::vector<T0> memory;
        memory.reserve(_width * _height * channel_total);
        if (copy) {
            for (int pos = 0; pos < _size; pos += primary_total) {
                double alpha_mult = unpremultiply_alpha ? _mult(_get_alpha(pos)) : 1.0;
                for (int c = 0; c < channel_count; c++) {
                    memory.emplace_back(_get_channel<T0>(pos, c, alpha_mult));
                }
                memory.emplace_back(_get_channel<T0>(pos, channel_count, 1.0));
            }
        }
        return memory;
    }

    /*
     * Same as contiguouMemory but wraps the output in a correctly configured PixelAccess object.
     *
     * @returns A pixel access object which unlike regular objects, owns it's
     *          memory and will unallocate the temporary surface on destruction.
     */
    auto createContiguous(bool copy = true, bool unpremultiply_alpha = false) const
        requires (has_more_channels)
    {
        return PixelAccess<format, channel_count, edge_mode, channel_count>(contiguousMemory<PrimaryType>(copy, unpremultiply_alpha), _width, _height);
    }

    /**
     * Simple multi-thread enabled loop for all the pixels in this raster.
     */
    void forEachPixel(std::function<void(int, int)> &&function)
    {
        auto const pool = get_global_dispatch_pool();
        int const limit = width() * height();

        pool->dispatch_threshold(height(), limit > POOL_THRESHOLD, [&](int y, int) {
            for (int x = 0; x < width(); x++) {
                function(x, y);
            }
        });
    }

#ifdef UNIT_TEST
    /**
     * Write the connected surface to a png file for debugging
     */
    void write_to_png(std::string const &filename) const
    {
        if constexpr (has_more_channels) {
            _cairo_surface->write_to_png(filename + "-0.png");
            _next_surface->write_to_png(filename + "-1.png");
        } else if (!primary_override) {
            _cairo_surface->write_to_png(filename + ".png");
        } else {
            std::cerr << "Can't debug contigious surface. '" << filename << "' skipped\n";
        }
    }
#endif

private:
    /*
     * Sets the Primaries from this Color.
     *
     * If the next access is set it will recursively set the next unused channels to the next
     * surface primaries until all are exhausted.
     *
     * @param pos - The typed memory Position in the surface to get a value from
     * @param alpha - The value from the alpha Channel in the Color.
     * @param values - The Color we're setting to this surface
     * @param offset - Internal recursive value off where in the Color we have gotten to.
     *
     */
    inline void _set_primaries_recursively(int pos, double alpha, Color const &values, bool unmultiply_alpha,
                                           int offset = 0)
    {
        if (_edge_check(pos)) {
            return;
        }
        // Set alpha in the surface
        _memory[pos + _primary_pos(primary_alpha)] = alpha * primary_scale;
        auto mult = unmultiply_alpha ? alpha : 1.0;

        // Set the primaries in the surface
        for (int p = 0; p < primary_total && offset < values.size(); p++) {
            if (p != primary_alpha) {
                _memory[pos + _primary_pos(p)] = values[offset] * mult * primary_scale;
                offset++;
            }
        }

        // If we have more channels, keep setting them
        if constexpr (has_more_channels) {
            // Alpha is always set in every surface
            _next_memory[pos + _primary_pos(primary_alpha)] = alpha * primary_scale;

            for (int p = 0; p < primary_total && offset < values.size(); p++) {
                if (p != primary_alpha) {
                    _next_memory[pos + _primary_pos(p)] = values[offset] * mult * primary_scale;
                    offset++;
                }
            }
        }
    }

    /**
     * Get the channel value from a specific memory position
     *
     * @arg pos        - The memory position in the surface (see _pixel_pos)
     * @arg channel    - Which channel to get, NOT the primary number.
     * @arg alpha_mult - If set will unpremultiply the channel, we do it here to preserve
     *                   as much precision before possible conversion to int.
     */
    template <typename T0 = double>
    inline T0 _get_channel(int pos, int channel, double alpha_mult) const
    {
        // Allow this function to output integer types of various sizes as well as floating point types
        constexpr static double scale =
            std::is_integral_v<T0>
                ? (std::is_integral_v<PrimaryType>
                       ? std::numeric_limits<T0>::max() / std::numeric_limits<PrimaryType>::max() // T=char T0=int|char
                       : std::numeric_limits<T0>::max()) // T=float      T0=int|char
                : (T0)(1.0 / primary_scale);              // T=float|char T0=float|double

        if (_edge_check(pos)) {
            return 0.0;
        }
        if constexpr (has_more_channels) {
            if (channel >= primary_count && channel != channel_count) {
                return _next_memory[pos + _channel_to_primary(channel)] * scale * alpha_mult;
            }
        }
        return _memory[pos + _channel_to_primary(channel)] * scale * alpha_mult;
    }

    /**
     * Set the primary position, like _get_channel
     */
    inline void _set_channel(int pos, int channel, double value) const
    {
        if constexpr (has_more_channels) {
            if (channel >= primary_count && channel != channel_count) {
                _next_memory[pos + _channel_to_primary(channel)] = value * primary_scale;
            }
        } else {
            _memory[pos + _channel_to_primary(channel)] = value * primary_scale;
        }
    }

    /**
     * Return the primary position given the channel index.
     */
    inline int _channel_to_primary(int channel) const
    {
        if constexpr (has_more_channels) {
            if (channel >= primary_count && channel != channel_count) {
                return _primary_pos(channel - primary_count + is_integer);
            }
        }
        return _primary_pos(channel < channel_count ? channel + is_integer : primary_alpha);
    }

    /**
     * Get the alpha primary only
     */
    inline double _get_alpha(int pos) const
    {
        if constexpr (checks_edge) {
            if (_edge_check(pos)) {
                return 0.0;
            }
        }
        return _memory[pos + _primary_pos(primary_alpha)] / primary_scale;
    }

    /**
     * Set the alpha primary only.
     */
    inline void _set_alpha(int pos, double alpha)
    {
        if constexpr(checks_edge) {
            if (_edge_check(pos)) {
                return;
            }
        }
        _memory[pos + _primary_pos(primary_alpha)] = alpha * primary_scale;
    }

    /**
     * Return true if pos is off the edge of the surface. Compiled out when not needed.
     */
    inline bool _edge_check(int pos) const {
        if constexpr(edge_mode != PixelAccessEdgeMode::NO_CHECK) {
            return pos < 0 || pos >= _size;
        }
        return false;
    }

    /**
     * Get the multiplication alpha for use in premultiplications
     */
    static inline double _mult(double alpha) { return alpha > 0 ? 1.0 / alpha : 0.0; }

    /**
     * Get the position in the memory of this pixel
     */
    inline int _pixel_pos(int x, int y) const
    {
        if constexpr (edge_mode != PixelAccessEdgeMode::NO_CHECK) {
            if (x < 0 || y < 0 || x >= _width || y >= _height) {
                switch (edge_mode) {
                    case PixelAccessEdgeMode::EXTEND:
                        x = std::clamp(x, (int)0, _width - 1);
                        y = std::clamp(y, (int)0, _height - 1);
                        break;
                    case PixelAccessEdgeMode::WRAP:
                        x = Util::safemod(x, _width);
                        y = Util::safemod(y, _height);
                        break;
                    case PixelAccessEdgeMode::ZERO:
                        // This means OOB to _get_channel, which will return zero
                        return -1;
                    case PixelAccessEdgeMode::ERROR:
                    default:
                        throw std::exception();
                        break;
                }
            }
        }
        return y * _stride + x * primary_total;
    }

    /**
     * Convert the primary position into a memory location based on the endianness
     * of the uint32 Cairo stores things in. This might need adjusting for platforms.
     */
    static inline int _primary_pos(int p)
    {
        if constexpr (G_BYTE_ORDER == G_LITTLE_ENDIAN) {
            return is_integer ? primary_count - p : p;
        } else {
            return p;
        }
    }

    /**
     * Standard bilinear interpolation
     */
    static inline double _bilinear_interpolate(double a, double b, double c, double d, double wx, double wy)
    {
        // This should only be useful for linearRGB color space, gamut curved colors such as sRGB and
        // periodic channels like HSL/HSV would give bad results. This equation is for premultiplied values.
        return (a * wx + b * (1 - wx)) * wy + (c * wx + d * (1 - wx)) * (1 - wy);
    }

    // This is used for temporary contiguous surfaces in color transformations.
    std::vector<PrimaryType> _local_memory;

    // Basic metrics for the surface
    int const _width;
    int const _height;
    int const _stride;
    int const _size;
    PrimaryType *_memory{};

    // Keep a copy of the cairo surface RefPtr to keep it alive while we exist (we don't use it directly)
    Cairo::RefPtr<Cairo::ImageSurface> _cairo_surface;

    // When the color space involves more channels than primaries available in one cairo surface
    PrimaryType *_next_memory = nullptr;
    Cairo::RefPtr<Cairo::ImageSurface> _next_surface;

public:

    template <bool is_column, int LA_channel>
    auto getLineAccess(int line = -1) {
        return LineAccess<is_column, LA_channel, false>(*this, line);
    }
    template <bool is_column, int LA_channel>
    auto getLineAccess(int line = -1) const {
        return LineAccess<is_column, LA_channel, true>(*this, line);
    }

    /**
     * Provides access to a line of bytes as floats. If the memory is already float, the
     * memory used is a pointer into the PixelAccess memory. If it's integer, then a copy
     * of the line is made and reformatted into float.
     *
     * @template_arg is_column  - If set, changes the direction of the line from row to column.
     * @template_arg LA_channel - Which channel this line should focus on.
     * @teamplte_arg is_const   - Read only access. TODO: Can this be done in a better way?

     * @arg access    - The PixelAccess we are trying to read or modify.
     * @arg index     - Which line to get access to.
     */
    template <bool is_column, int LA_channel, bool is_const = false>
    struct LineAccess
    {
        // Inputs
        using PixelAccessType = std::conditional_t<is_const, const PixelAccess<format, channel_count, edge_mode, primary_override>,
                                                                   PixelAccess<format, channel_count, edge_mode, primary_override>>;
        using DataAccessType = std::conditional_t<is_const, const float, float>;
        PixelAccessType *_access;

        // Calculated and stored
        int _line_num = -1;
        int _line_pos = 0;
        const int _next_col;
        const int _next_line;
        std::vector<float> _memory;
public:
        LineAccess(PixelAccessType &access, int index = -1)
            : _access(&access)
            , _next_col(access._pixel_pos(!is_column, is_column))
            , _next_line(access._pixel_pos(is_column, !is_column))
            , size(is_column ? _access->height() : _access->width())
        {
            if constexpr (is_integer) {
                _memory.reserve(size);
                for (auto i = 0; i < size; i++) {
                    _memory.emplace_back(0);
                }
                pixels = _memory.data();
                next = 1;
            } else {
                next = _next_col;
            }
            if (index >= 0) {
                gotoLine(index);
            }
        }

        void nextLine() { gotoLine(_line_num + 1); }
        void gotoLine(int line)
        {
            if constexpr (!is_const && is_integer) {
                commitLine();
            }
            _line_num = line;
            _line_pos = line * _next_line;

            if constexpr (is_integer) {
                // Make a copy of just this one channel of this line
                for (auto i = 0; i < size; i++) {
                    // Narrows from double to float
                    _memory[i] = _access->_get_channel(_line_pos + i * _next_col, LA_channel, 1.0);
                }
            } else {
                // Actually let's not copy, access directly
                pixels = _access->template memory<LA_channel>();
                pixels += _line_pos + _access->_channel_to_primary(LA_channel);
            }
        }

        void commitLine() requires(!is_const && is_integer)
        {
            if (_line_num >= 0) {
                // Copy the data back into the PixelAccess
                for (auto i = 0; i < size; i++) {
                    _access->_set_channel(_line_pos + i * _next_col, LA_channel, _memory[i]);
                }
            }
        }

        ~LineAccess() = default;
        ~LineAccess() requires(!is_const && is_integer) { commitLine(); }

        int next;
        const int size;
        DataAccessType *pixels = nullptr;
    };
};

} // namespace Inkscape::Renderer

#endif // INKSCAPE_RENDERER_PIXEL_ACCESS_H

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
