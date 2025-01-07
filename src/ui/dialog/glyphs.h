// SPDX-License-Identifier: GPL-2.0-or-later
/* Authors:
 *   Jon A. Cruz
 *
 * Copyright (C) 2010 Jon A. Cruz
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_DIALOGS_GLYPHS_H
#define SEEN_DIALOGS_GLYPHS_H

#include <vector>
#include <glibmm/refptr.h>
#include <gtkmm/treemodel.h>

#include <sigc++/scoped_connection.h>
#include "ui/dialog/dialog-base.h"

namespace Gtk {
class Button;
class ComboBoxText;
class Entry;
class IconView;
class Label;
class ListStore;
} // namespace Gtk

namespace Inkscape::UI {

namespace Widget {
class FontSelector;
} // namespace Widget

namespace Dialog {

class GlyphColumns;

/**
 * A panel that displays character glyphs.
 */
class GlyphsPanel final : public DialogBase
{
public:
    GlyphsPanel();
    ~GlyphsPanel() final;

    void selectionChanged (Selection *selection                ) final;
    void selectionModified(void* sender, Selection* selection, unsigned flags) final;

private:
    static GlyphColumns *getColumns();

    void rebuild();

    void glyphActivated(Gtk::TreeModel::Path const &path);
    void glyphSelectionChanged();
    void readSelection( bool updateStyle, bool updateContent );
    void calcCanInsert();
    void insertText();

    Glib::RefPtr<Gtk::ListStore> store;
    Gtk::IconView                      *iconView     = nullptr;
    Gtk::Entry                         *entry        = nullptr;
    Gtk::Label                         *label        = nullptr;
    Gtk::Button                        *insertBtn    = nullptr;
    Gtk::ComboBoxText                  *scriptCombo  = nullptr;
    Gtk::ComboBoxText                  *rangeCombo   = nullptr;
    Inkscape::UI::Widget::FontSelector *fontSelector = nullptr;

    std::vector<sigc::scoped_connection> instanceConns;
};

} // namespace Dialog

} // namespace Inkscape::UI

#endif // SEEN_DIALOGS_GLYPHS_H

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
