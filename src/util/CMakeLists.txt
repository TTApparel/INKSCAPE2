# SPDX-License-Identifier: GPL-2.0-or-later

set(util_SRC
	action-accel.cpp
    document-fonts.cpp
	drawing-utils.cpp
	expression-evaluator.cpp
    font-collections.cpp
	format_size.cpp
	font-discovery.cpp
	font-tags.cpp
	funclog.cpp
	share.cpp
	object-renderer.cpp
    object-modified-tags.cpp
	paper.cpp
	parse-int-range.cpp
	preview.cpp
	source_date_epoch.cpp
	statics.cpp
	theme-utils.cpp
    recently-used-fonts.cpp
	pool.cpp
	units.cpp
	ziptool.cpp

	# -------
	# Headers
	action-accel.h
	cached_map.h
	cast.h
	const_char_ptr.h
	delete-with.h
    document-fonts.h
	drawing-utils.h
	enums.h
	expression-evaluator.h
    font-collections.h
	fixed_point.h
	font-discovery.h
	font-tags.h
	format.h
	format_size.h
	forward-pointer-iterator.h
	funclog.h
	hybrid-pointer.h
	longest-common-suffix.h
    object-renderer.h
    object-modified-tags.h
	optstr.h
	pages-skeleton.h
	paper.h
	parse-int-range.h
	pool.h
	preview.h
    recently-used-fonts.h
	reference.h
	scope_exit.h
	share.h
	signal-blocker.h
	source_date_epoch.h
	statics.h
	static-doc.h
	theme-utils.h
	trim.h
	smart_ptr_keys.h
	units.h
    variant-visitor.h
    value-utils.h
	ziptool.h
	numeric/converters.h
)

add_inkscape_lib(util_LIB "${util_SRC}")
target_link_libraries(util_LIB PUBLIC 2Geom::2geom)
# add_inkscape_source("${util_SRC}")
