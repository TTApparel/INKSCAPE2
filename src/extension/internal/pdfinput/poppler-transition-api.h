// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO short description
 *//*
 * Authors:
 *   see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_POPPLER_TRANSITION_API_H
#define SEEN_POPPLER_TRANSITION_API_H

#include <glib/poppler-features.h>
#include <poppler/UTF.h>

#if POPPLER_CHECK_VERSION(25,2,0)
#define _POPPLER_GET_CODE_TO_GID_MAP(ff, len) getCodeToGIDMap(ff)
#define _POPPLER_GET_CID_TO_GID_MAP(len) getCIDToGIDMap()
#else
#define _POPPLER_GET_CODE_TO_GID_MAP(ff, len) getCodeToGIDMap(ff, len)
#define _POPPLER_GET_CID_TO_GID_MAP(len) getCIDToGIDMap(len)
#endif

#if POPPLER_CHECK_VERSION(24,12,0)
#define _POPPLER_GET_IMAGE_PARAMS(bits, csMode, hasAlpha) getImageParams(bits, csMode, hasAlpha)
#else
#define _POPPLER_GET_IMAGE_PARAMS(bits, csMode, hasAlpha) getImageParams(bits, csMode)
#endif

#if POPPLER_CHECK_VERSION(24, 10, 0)
#define _POPPLER_CONSUME_UNIQPTR_ARG(value) std::move(value)
#else
#define _POPPLER_CONSUME_UNIQPTR_ARG(value) value.release()
#endif

#if POPPLER_CHECK_VERSION(24, 5, 0)
#define _POPPLER_HAS_UNICODE_BOM(value) (hasUnicodeByteOrderMark(value->toStr()))
#define _POPPLER_HAS_UNICODE_BOMLE(value) (hasUnicodeByteOrderMarkLE(value->toStr()))
#else
#define _POPPLER_HAS_UNICODE_BOM(value) (value->hasUnicodeMarker())
#define _POPPLER_HAS_UNICODE_BOMLE(value) (value->hasUnicodeMarkerLE())
#endif

#if POPPLER_CHECK_VERSION(24, 3, 0)
#define _POPPLER_FUNCTION_TYPE_SAMPLED Function::Type::Sampled
#define _POPPLER_FUNCTION_TYPE_EXPONENTIAL Function::Type::Exponential
#define _POPPLER_FUNCTION_TYPE_STITCHING Function::Type::Stitching
#else
#define _POPPLER_FUNCTION_TYPE_SAMPLED 0
#define _POPPLER_FUNCTION_TYPE_EXPONENTIAL 2
#define _POPPLER_FUNCTION_TYPE_STITCHING 3
#endif

#if POPPLER_CHECK_VERSION(22, 4, 0)
#define _POPPLER_FONTPTR_TO_GFX8(font_ptr) ((Gfx8BitFont *)font_ptr.get())
#else
#define _POPPLER_FONTPTR_TO_GFX8(font_ptr) ((Gfx8BitFont *)font_ptr)
#endif

#if POPPLER_CHECK_VERSION(22, 3, 0)
#define _POPPLER_MAKE_SHARED_PDFDOC(uri) std::make_shared<PDFDoc>(std::make_unique<GooString>(uri))
#else
#define _POPPLER_MAKE_SHARED_PDFDOC(uri) std::make_shared<PDFDoc>(new GooString(uri), nullptr, nullptr, nullptr)
#endif

#if POPPLER_CHECK_VERSION(0, 83, 0)
#define _POPPLER_CONST_83 const
#else
#define _POPPLER_CONST_83
#endif

#if POPPLER_CHECK_VERSION(0, 82, 0)
#define _POPPLER_CONST_82 const
#else
#define _POPPLER_CONST_82
#endif

#if POPPLER_CHECK_VERSION(0, 76, 0)
#define _POPPLER_NEW_PARSER(xref, obj) Parser(xref, obj, gFalse)
#else
#define _POPPLER_NEW_PARSER(xref, obj) Parser(xref, new Lexer(xref, obj), gFalse)
#endif

#if POPPLER_CHECK_VERSION(0, 83, 0)
#define _POPPLER_NEW_GLOBAL_PARAMS(args...) std::unique_ptr<GlobalParams>(new GlobalParams(args))
#else
#define _POPPLER_NEW_GLOBAL_PARAMS(args...) new GlobalParams(args)
#endif


#if POPPLER_CHECK_VERSION(0, 72, 0)
#define getCString c_str
#endif

#if POPPLER_CHECK_VERSION(0,71,0)
typedef bool GBool;
#define gTrue true
#define gFalse false
#endif

#if POPPLER_CHECK_VERSION(0,70,0)
#define _POPPLER_CONST const
#else
#define _POPPLER_CONST
#endif

#if POPPLER_CHECK_VERSION(0,69,0)
#define _POPPLER_DICTADD(dict, key, obj) (dict).dictAdd(key, std::move(obj))
#elif POPPLER_CHECK_VERSION(0,58,0)
#define _POPPLER_DICTADD(dict, key, obj) (dict).dictAdd(copyString(key), std::move(obj))
#else
#define _POPPLER_DICTADD(dict, key, obj) (dict).dictAdd(copyString(key), &obj)
#endif

#define POPPLER_NEW_OBJECT_API
#define _POPPLER_FREE(obj)
#define _POPPLER_CALL(ret, func) (ret = func())
#define _POPPLER_CALL_ARGS(ret, func, ...) (ret = func(__VA_ARGS__))
#define _POPPLER_CALL_ARGS_DEREF _POPPLER_CALL_ARGS

#if !POPPLER_CHECK_VERSION(0, 58, 0)
#error "Requires poppler >= 0.58"
#endif

#endif
