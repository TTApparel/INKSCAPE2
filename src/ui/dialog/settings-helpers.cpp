// SPDX-License-Identifier: GPL-2.0-or-later
//
// Created by Michael Kowalski on 2/22/25.
//

#include "settings-helpers.h"

#include <array>
#include <glib/gi18n.h>
#include <glibmm/ustring.h>

#include "inkscape-preferences.h"
#include "inkscape.h"
#include "display/control/ctrl-handle-manager.h"
#include "io/resource.h"
#include "ui/themes.h"
#include "ui/widget/generic/icon-combobox.h"

namespace Inkscape::UI::Dialog::Settings {

std::pair<LanguageArray, LanguageArray> get_ui_languages() {

    static LanguageArray languages = {_("System default"),
        _("Albanian (sq)"), _("Arabic (ar)"), _("Armenian (hy)"), _("Assamese (as)"), _("Azerbaijani (az)"),
        _("Basque (eu)"), _("Belarusian (be)"), _("Bulgarian (bg)"), _("Bengali (bn)"), _("Bengali/Bangladesh (bn_BD)"), _("Bodo (brx)"), _("Breton (br)"),
        _("Catalan (ca)"), _("Valencian Catalan (ca@valencia)"), _("Chinese/China (zh_CN)"),  _("Chinese/Taiwan (zh_TW)"), _("Croatian (hr)"), _("Czech (cs)"),
        _("Danish (da)"), _("Dogri (doi)"), _("Dutch (nl)"), _("Dzongkha (dz)"),
        _("German (de)"), _("Greek (el)"),
        _("English (en)"), _("English/Australia (en_AU)"), _("English/Canada (en_CA)"), _("English/Great Britain (en_GB)"), _("Esperanto (eo)"), _("Estonian (et)"),
        _("Farsi (fa)"), _("Finnish (fi)"), _("French (fr)"),
        _("Galician (gl)"), _("Gujarati (gu)"),
        _("Hebrew (he)"), _("Hindi (hi)"), _("Hungarian (hu)"),
        _("Icelandic (is)"), _("Indonesian (id)"), _("Irish (ga)"), _("Italian (it)"),
        _("Japanese (ja)"),
        _("Kannada (kn)"), _("Kashmiri in Perso-Arabic script (ks@aran)"), _("Kashmiri in Devanagari script (ks@deva)"), _("Khmer (km)"), _("Kinyarwanda (rw)"), _("Konkani (kok)"), _("Konkani in Latin script (kok@latin)"), _("Korean (ko)"),
        _("Latvian (lv)"), _("Lithuanian (lt)"),
        _("Macedonian (mk)"), _("Maithili (mai)"), _("Malayalam (ml)"), _("Manipuri (mni)"), _("Manipuri in Bengali script (mni@beng)"), _("Marathi (mr)"), _("Mongolian (mn)"),
        _("Nepali (ne)"), _("Norwegian Bokmål (nb)"), _("Norwegian Nynorsk (nn)"),
        _("Odia (or)"),
        _("Panjabi (pa)"), _("Polish (pl)"), _("Portuguese (pt)"), _("Portuguese/Brazil (pt_BR)"),
        _("Romanian (ro)"), _("Russian (ru)"),
        _("Sanskrit (sa)"), _("Santali (sat)"), _("Santali in Devanagari script (sat@deva)"), _("Serbian (sr)"), _("Serbian in Latin script (sr@latin)"),
        _("Sindhi (sd)"), _("Sindhi in Devanagari script (sd@deva)"), _("Slovak (sk)"), _("Slovenian (sl)"),  _("Spanish (es)"), _("Spanish/Mexico (es_MX)"), _("Swedish (sv)"),
        _("Tamil (ta)"), _("Telugu (te)"), _("Thai (th)"), _("Turkish (tr)"),
        _("Ukrainian (uk)"), _("Urdu (ur)"),
        _("Vietnamese (vi)")};

    static LanguageArray langValues = {"",
        "sq", "ar", "hy", "as", "az",
        "eu", "be", "bg", "bn", "bn_BD", "brx", "br",
        "ca", "ca@valencia", "zh_CN", "zh_TW", "hr", "cs",
        "da", "doi", "nl", "dz",
        "de", "el",
        "en", "en_AU", "en_CA", "en_GB", "eo", "et",
        "fa", "fi", "fr",
        "gl", "gu",
        "he", "hi", "hu",
        "is", "id", "ga", "it",
        "ja",
        "kn", "ks@aran", "ks@deva", "km", "rw", "kok", "kok@latin", "ko",
        "lv", "lt",
        "mk", "mai", "ml", "mni", "mni@beng", "mr", "mn",
        "ne", "nb", "nn",
        "or",
        "pa", "pl", "pt", "pt_BR",
        "ro", "ru",
        "sa", "sat", "sat@deva", "sr", "sr@latin",
        "sd", "sd@deva", "sk", "sl", "es", "es_MX", "sv",
        "ta", "te", "th", "tr",
        "uk", "ur",
        "vi" };

    {
        // sorting languages according to translated name
        int i = 0;
        int j = 0;
        int n = languages.size();// sizeof( languages ) / sizeof( Glib::ustring );
        Glib::ustring key_language;
        Glib::ustring key_langValue;
        for ( j = 1 ; j < n ; j++ ) {
            key_language = languages[j];
            key_langValue = langValues[j];
            i = j-1;
            while ( i >= 0
                    && ( ( languages[i] > key_language
                         && langValues[i] != "" )
                       || key_langValue == "" ) )
            {
                languages[i+1] = languages[i];
                langValues[i+1] = langValues[i];
                i--;
            }
            languages[i+1] = key_language;
            langValues[i+1] = key_langValue;
        }
    }

    return std::make_pair(languages, langValues);
}

Gtk::DropDown* create_combobox(std::string_view source_name, int scale_factor, bool enable_search) {
    if (source_name == "languages") {
        auto list = Gtk::make_managed<Widget::DropDownList>();
        auto languages = get_ui_languages();
        for (auto& name : languages.first) {
            list->append(name);
        }
        if (enable_search) {
            list->enable_search();
        }
        return list;
    }

    if (source_name == "ui-themes") {
        auto list = Gtk::make_managed<Widget::DropDownList>();
        //
        auto themes = INKSCAPE.themecontext->get_available_themes();
        std::vector<Glib::ustring> labels;
        for (auto const &[theme, dark] : themes) {
            if (theme == "Empty") continue;
            if (theme == "Default") continue;
            // if (theme == default_theme) {
            //     continue;
            // }
            labels.emplace_back(theme);
        }
        std::sort(labels.begin(), labels.end());
        labels.erase(unique(labels.begin(), labels.end()), labels.end());
        auto it = std::find(labels.begin(), labels.end(), "Inkscape");
        if (it != labels.end()) {
            labels.erase(it);
            labels.insert(labels.begin(), "Inkscape");
        }

        for (auto& name : labels) {
            list->append(name);
        }
        // values.emplace_back("");
        // Glib::ustring default_theme_label = _("Use system theme");
        // default_theme_label += " (" + default_theme + ")";
        // labels.emplace_back(default_theme_label);
        return list;
    }

    if (source_name == "icon-themes") {
        auto list = Gtk::make_managed<Widget::DropDownList>();
        std::vector<Glib::ustring> labels;
        // Glib::ustring default_icon_theme = prefs->getString("/theme/defaultIconTheme");
        for (auto &&folder : IO::Resource::get_foldernames(IO::Resource::ICONS, { "application" })) {
            // from https://stackoverflow.com/questions/8520560/get-a-file-name-from-a-path#8520871
            // Maybe we can link boost path utilities
            // Remove directory if present.
            // Do this before extension removal in case the directory has a period character.
            const size_t last_slash_idx = folder.find_last_of("\\/");
            if (std::string::npos != last_slash_idx) {
                folder.erase(0, last_slash_idx + 1);
            }

            // we want use Adwaita instead fallback hicolor theme
            // auto const folder_utf8 = Glib::filename_to_utf8(folder);
            // if (folder_utf8 == default_icon_theme) {
            //     continue;
            // }

            labels.emplace_back(          folder) ;
            // values.emplace_back(std::move(folder));
        }
        std::sort(labels.begin(), labels.end());
        // std::sort(values.begin(), values.end());
        labels.erase(unique(labels.begin(), labels.end()), labels.end());
        // values.erase(unique(values.begin(), values.end()), values.end());
        for (auto& name : labels) {
            list->append(name);
        }
        return list;
    }

    if (source_name == "xml-themes") {
    }

    if (source_name == "handle-colors") {
        auto cb = Gtk::make_managed<Inkscape::UI::Widget::IconComboBox>(false);
        cb->set_valign(Gtk::Align::CENTER);
        auto& mgr = Handles::Manager::get();
        int i = 0;
        for (auto theme : mgr.get_handle_themes()) {
            unsigned int frame = theme.positive ? 0x000000 : 0xffffff; // black or white
            cb->add_row(draw_color_preview(theme.rgb_accent_color, frame, scale_factor), theme.title, i++);
        }
        cb->refilter();
        cb->set_active_by_id(mgr.get_selected_theme());
        return cb;
    }

    //todo

    return Gtk::make_managed<Gtk::DropDown>();
}

} // namespace
