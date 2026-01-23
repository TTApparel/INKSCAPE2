// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * A container class for filter slots. Allows for simple getting and
 * setting images in filter slots without having to bother with
 * table indexes and such.
 *
 * Author:
 *   Niko Kiirala <niko@kiirala.com>
 *
 * Copyright (C) 2006,2007 Niko Kiirala
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_INKSCAPE_RENDERER_DRAWING_FILTERS_SLOT_H
#define SEEN_INKSCAPE_RENDERER_DRAWING_FILTERS_SLOT_H

#include <memory>
#include <map>
#include <vector>

#include "enums.h"
#include "units.h"

#include "renderer/drawing/drawing-options.h"

namespace Inkscape::Renderer {

class Surface;

namespace DrawingFilter {

class Slot final
{
public:
    /** Creates a new Slot object. */
    Slot(DrawingOptions const &draw_opt, Units const &item_opt);
    Slot();

    /** Destroys the Slot object and all its contents */
    ~Slot() = default;

    /** Returns the Surface in specified slot.
      *
     * @param slot - May be either an positive integer or one of pre-defined types:
     *
     *   SLOT_NOT_SET,
     *   SLOT_SOURCE_IMAGE,     SLOT_SOURCE_ALPHA,
     *   SLOT_BACKGROUND_IMAGE, SLOT_BACKGROUND_ALPHA,
     *   SLOT_FILL_PAINT,       SLOT_STROKE_PAINT.
     *
     */
    std::shared_ptr<Surface> get(int slot = SLOT_NOT_SET) const;

    /**
     * Guarentee that the returned surface is in the given color space. If not provided, data format
     * is assumed to be INT32 RGB instead of FLOAT128 RGB for regular Space::RGB.
     *
     * @param space - The color space to try and guarentee. If provided will make a copy of the
     *                space and return the copied space. If it's the same space as the one used
     *                in slot then the slot is returned without copying.
     */
    std::shared_ptr<Surface> get(int slot, std::shared_ptr<Colors::Space::AnySpace> const &space) const;

    /**
     * Create a copy of the given slot with the exact same format and space, see get()
     */
    std::shared_ptr<Surface> get_copy(int slot = SLOT_NOT_SET) const;

    /**
     * Create a copy in the given color space, this may change the format. see get()
     */
    std::shared_ptr<Surface> get_copy(int slot, std::shared_ptr<Colors::Space::AnySpace> const &space) const;

    /**
     * Returns the same as get() but will undo any transformation applied
     * to the input source graphic when filters were appplied.
     */
    std::shared_ptr<Surface> get_result(int slot = SLOT_NOT_SET) const;

    /**
     * Set the surface for this slot and free any previous surface, then set
     * the last_slot to this slot indicating this is the last in the filter stack.
     */
    void set(int slot, std::shared_ptr<Surface> surface);

    /**
     * Use an existing slot surface to make an alpha version as a new surface and
     * save it in the given destination slot.
     */
    void set_alpha(int slot_from, int slot_to);

    void set_primitive_area(int slot, Geom::Rect &area);
    Geom::Rect get_primitive_area(int slot) const;
    
    /** Returns the number of slots in use. */
    int get_slot_count() const { return _slots.size(); }

    /** Get all the drawing options */
    auto &get_drawing_options() const { return _draw_opt; }
    auto &get_item_options() const { return _item_opt; }

private:
    std::map<int, std::shared_ptr<Surface>> _slots;

    // We need to keep track of the primitive area as this is needed in feTile
    using PrimitiveAreaMap = std::map<int, Geom::Rect>;
    PrimitiveAreaMap _primitiveAreas;

    int _last_out;
    DrawingOptions _draw_opt;
    Units _item_opt;
};

} // namespace Filters
} // namespace Inkscape::Renderer

#endif // SEEN_INKSCAPE_RENDERER_DRAWING_FILTERS_SLOT_H
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
