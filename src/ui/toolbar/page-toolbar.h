// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Page toolbar
 */
/* Authors:
 *   Martin Owens <doctormo@geek-2.com>
 *   Vaibhav Malik <vaibhavmalik2018@gmail.com>
 *
 * Copyright (C) 2021 Martin Owens
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_UI_TOOLBAR_PAGE_TOOLBAR_H
#define INKSCAPE_UI_TOOLBAR_PAGE_TOOLBAR_H

#include <string>

#include <sigc++/scoped_connection.h>
#include "toolbar.h"
#include "ui/widget/spinbutton.h"

namespace Gtk {
class ComboBoxText;
class Entry;
class EntryCompletion;
class Label;
class ListStore;
class Popover;
class Button;
class Builder;
class Separator;
} // namespace Gtk

class SPDocument;
class SPPage;

namespace Inkscape {

class PaperSize;

namespace UI {

namespace Tools {
class ToolBase;
} // namespace Tools

namespace Toolbar {

class PageToolbar : public Toolbar
{
public:
    PageToolbar();
    ~PageToolbar() override;

    void setDesktop(SPDesktop *desktop) override;

private:
    PageToolbar(Glib::RefPtr<Gtk::Builder> const &builder);

    void labelEdited();
    void bleedsEdited();
    void marginsEdited();
    void marginTopEdited();
    void marginRightEdited();
    void marginBottomEdited();
    void marginLeftEdited();
    void marginSideEdited(int side, const Glib::ustring &value);
    void sizeChoose(const std::string &preset_key);
    void sizeChanged();
    void setSizeText(SPPage *page = nullptr, bool display_only = true);
    void setMarginText(SPPage *page = nullptr);

    SPDocument *_document = nullptr;

    void toolChanged(SPDesktop *desktop, Inkscape::UI::Tools::ToolBase *tool);
    void pagesChanged();
    void selectionChanged(SPPage *page);
    void populate_sizes();

    sigc::scoped_connection _doc_connection;
    sigc::scoped_connection _pages_changed;
    sigc::scoped_connection _page_selected;
    sigc::scoped_connection _page_modified;
    sigc::scoped_connection _label_edited;
    sigc::scoped_connection _size_edited;

    Gtk::ComboBoxText &_combo_page_sizes;
    Gtk::Entry *_entry_page_sizes;
    Gtk::Entry &_text_page_margins;
    Gtk::Popover &_margin_popover;
    Gtk::Entry &_text_page_bleeds;
    Gtk::Entry &_text_page_label;
    Gtk::Label &_label_page_pos;
    Gtk::Button &_btn_page_backward;
    Gtk::Button &_btn_page_foreward;
    Gtk::Button &_btn_page_delete;
    Gtk::Button &_btn_move_toggle;
    Gtk::Separator &_sep1;

    Glib::RefPtr<Gtk::ListStore> _sizes_list;
    Glib::RefPtr<Gtk::ListStore> _sizes_search;

    Inkscape::UI::Widget::MathSpinButton &_margin_top;
    Inkscape::UI::Widget::MathSpinButton &_margin_right;
    Inkscape::UI::Widget::MathSpinButton &_margin_bottom;
    Inkscape::UI::Widget::MathSpinButton &_margin_left;

    double _unit_to_size(std::string number, std::string unit_str, std::string const &backup);
};

} // namespace Toolbar
} // namespace UI
} // namespace Inkscape

#endif // INKSCAPE_UI_TOOLBAR_PAGE_TOOLBAR_H

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
