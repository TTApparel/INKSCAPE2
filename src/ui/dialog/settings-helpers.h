// SPDX-License-Identifier: GPL-2.0-or-later
//
// Created by Michael Kowalski on 2/22/25.
//

#ifndef SETTINGS_HELPERS_H
#define SETTINGS_HELPERS_H
#include <utility>
#include <glibmm/ustring.h>

#include "ui/widget/drop-down-list.h"

namespace Inkscape::UI::Dialog::Settings {

using LanguageArray = std::array<Glib::ustring, 90>;

// list of translated languages and their symbols
std::pair<LanguageArray, LanguageArray> get_ui_languages();

//
Gtk::DropDown* create_combobox(std::string_view source_name, int scale_factor, bool enable_search = false);

} // namespace

#endif //SETTINGS_HELPERS_H
