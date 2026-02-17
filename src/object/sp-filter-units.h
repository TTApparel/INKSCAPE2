// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef SEEN_SP_FILTER_UNITS_H
#define SEEN_SP_FILTER_UNITS_H

enum SPFilterUnits {
    SP_FILTER_UNITS_OBJECTBOUNDINGBOX,
    SP_FILTER_UNITS_USERSPACEONUSE
};

enum class CompositeOperator
{
    // Default value is 'over', but let's distinguish specifying the
    // default and implicitly using the default
    DEFAULT,
    OVER,              /* Source Over */
    IN,                /* Source In   */
    OUT,               /* Source Out  */
    ATOP,              /* Source Atop */
    XOR,
    ARITHMETIC,        /* Not a fundamental PorterDuff operator, nor Cairo */
    LIGHTER,           /* Plus, Add (Not a fundamental PorterDuff operator  */
    ENDOPERATOR        /* Cairo Saturate is not included in CSS */
};

#endif /* !SEEN_SP_FILTER_UNITS_H */

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
