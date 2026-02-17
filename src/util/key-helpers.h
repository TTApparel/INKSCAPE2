// SPDX-License-Identifier: GPL-2.0-or-later
//
// Created by Michael Kowalski on 2/13/25.
//

#ifndef KEY_FORMAT_H
#define KEY_FORMAT_H

#include <gdkmm/display.h>
#include <glibmm/ustring.h>
#include <gtkmm/accelkey.h>

namespace Inkscape::Util {

// Get a label for key shortcut based on its 'keyval' and modifiers
Gtk::AccelKey transform_key_value(const Glib::RefPtr<Gdk::Display>& display, int keyval, Gdk::ModifierType mod);

// Return true if keyval represents modifier key
bool is_key_modifier(int keyval);

// Format all accelerator keys
Glib::ustring format_accel_keys(const Glib::RefPtr<Gdk::Display>& display, const std::vector<Gtk::AccelKey>& accels);

// Parse accelerator string; handles U+xxxx unicode sequences in addition to the other standard forms
Gtk::AccelKey parse_accelerator_string(const Glib::ustring& accelerator);

// Get accelerator's abbreviation, handle Unicode values
Glib::ustring get_accel_key_abbrev(const Gtk::AccelKey& accel);

} // namespace

#endif //KEY_FORMAT_H
